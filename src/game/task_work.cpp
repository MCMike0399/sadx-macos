#include "game/task_work.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

namespace sadx {

TaskWorkArena::TaskWorkArena(std::optional<std::uint32_t> reference_base)
    : reference_base_(reference_base), bytes_(usable_bytes + header_bytes, 0),
      known_(usable_bytes + header_bytes, 1), free_head_(0) {
    // HeapAlloc zeroes the last 32 bytes; REP STOSD overwrites the first 16 MiB.
    std::fill(bytes_.begin(), bytes_.begin() + usable_bytes, 0xff);
    blocks_.emplace(0, TaskWorkBlock{0, usable_bytes, {}, usable_bytes, {}});
    blocks_.emplace(usable_bytes, TaskWorkBlock{usable_bytes, {}, 0, {}, 0});
    sync(blocks_.at(0));
    sync(blocks_.at(usable_bytes));
}

void TaskWorkArena::store_pointer(std::uint32_t at,
                                  std::optional<std::uint32_t> offset) {
    std::uint32_t value = 0;
    const bool known = !offset || reference_base_.has_value();
    if (offset && reference_base_) value = *reference_base_ + *offset;
    for (unsigned i = 0; i != 4; ++i) {
        bytes_[at + i] = static_cast<std::uint8_t>(value >> (i * 8));
        known_[at + i] = known ? 1 : 0;
    }
}

void TaskWorkArena::sync(const TaskWorkBlock& block) {
    store_pointer(block.offset, block.free_next);
    store_pointer(block.offset + 4, block.free_previous);
    store_pointer(block.offset + 8, block.physical_next);
    store_pointer(block.offset + 12, block.physical_previous);
}

void TaskWorkArena::remove_free(std::uint32_t offset) {
    const auto block = blocks_.at(offset);
    if (block.free_previous) {
        auto& previous = blocks_.at(*block.free_previous);
        previous.free_next = block.free_next;
        sync(previous);
    } else free_head_ = block.free_next;
    if (block.free_next) {
        auto& next = blocks_.at(*block.free_next);
        next.free_previous = block.free_previous;
        sync(next);
    }
}

void TaskWorkArena::replace_free(std::uint32_t old_offset, std::uint32_t new_offset) {
    const auto old = blocks_.at(old_offset);
    auto& replacement = blocks_.at(new_offset);
    replacement.free_next = old.free_next;
    replacement.free_previous = old.free_previous;
    if (old.free_previous) {
        auto& previous = blocks_.at(*old.free_previous);
        previous.free_next = new_offset;
        sync(previous);
    } else free_head_ = new_offset;
    if (old.free_next) {
        auto& next = blocks_.at(*old.free_next);
        next.free_previous = new_offset;
        sync(next);
    }
    sync(replacement);
}

TaskWorkResult TaskWorkArena::allocate(std::uint32_t bytes) {
    if (!bytes) return {TaskWorkFailure::exhausted, {}};
    if (bytes > std::numeric_limits<std::uint32_t>::max() - 63)
        return {TaskWorkFailure::size_wrap, {}};
    const std::uint32_t total = (bytes + 63) & ~std::uint32_t{31};
    auto candidate = free_head_;
    while (candidate) {
        auto& block = blocks_.at(*candidate);
        if (!block.physical_next) break; // source sentinel terminates search
        const auto available = *block.physical_next - block.offset;
        if (available >= total) {
            if (available == total) remove_free(block.offset);
            else {
                const auto offset = block.offset + total;
                blocks_.emplace(offset, TaskWorkBlock{offset, {}, {},
                                                      block.physical_next, block.offset});
                replace_free(block.offset, offset);
                auto& physical_next = blocks_.at(*block.physical_next);
                physical_next.physical_previous = offset;
                sync(physical_next);
                block.physical_next = offset;
            }
            block.free_next.reset(); block.free_previous.reset(); sync(block);
            TaskWorkHandle work(new TaskWork);
            work->owner_ = this; work->header_ = block.offset; work->requested_ = bytes;
            live_.emplace(block.offset, work);
            return {TaskWorkFailure::none, work};
        }
        candidate = block.free_next;
    }
    return {TaskWorkFailure::exhausted, {}};
}

TaskWorkResult TaskWorkArena::validate(const TaskWorkHandle& work,
                                      std::uint32_t at, std::uint32_t bytes) const {
    if (!work || work->owner_ != this) return {TaskWorkFailure::foreign_work, work};
    if (work->released_) return {TaskWorkFailure::released_work, work};
    const auto found = live_.find(work->header_);
    if (found == live_.end() || found->second != work)
        return {TaskWorkFailure::foreign_work, work};
    // Rounded payload padding is observable in original storage; callers may
    // inspect it, but never cross the next physical header.
    const auto capacity = *blocks_.at(work->header_).physical_next - work->data_offset();
    if (at > capacity || bytes > capacity - at)
        return {TaskWorkFailure::out_of_bounds, work};
    return {TaskWorkFailure::none, work};
}

void TaskWorkArena::forget_references(std::uint32_t at, std::uint32_t bytes) {
    for (auto i = references_.begin(); i != references_.end();) {
        if (i->first < at + bytes && at < i->first + 4) i = references_.erase(i);
        else ++i;
    }
}

TaskWorkResult TaskWorkArena::read(const TaskWorkHandle& work, std::uint32_t at,
                                  void* output, std::uint32_t bytes) const {
    const auto accepted = validate(work, at, bytes);
    if (!accepted) return accepted;
    if (bytes && !output) return {TaskWorkFailure::invalid_buffer, work};
    const auto start = work->data_offset() + at;
    for (std::uint32_t i = 0; i != bytes; ++i)
        if (!known_[start + i]) return {TaskWorkFailure::unobserved_pointer_bytes, work};
    if (bytes) std::memcpy(output, bytes_.data() + start, bytes);
    return accepted;
}

TaskWorkResult TaskWorkArena::write(const TaskWorkHandle& work, std::uint32_t at,
                                   const void* input, std::uint32_t bytes) {
    const auto accepted = validate(work, at, bytes);
    if (!accepted) return accepted;
    if (bytes && !input) return {TaskWorkFailure::invalid_buffer, work};
    const auto start = work->data_offset() + at;
    forget_references(start, bytes);
    if (bytes) std::memcpy(bytes_.data() + start, input, bytes);
    std::fill(known_.begin() + start, known_.begin() + start + bytes, 1);
    return accepted;
}

TaskWorkResult TaskWorkArena::fill(const TaskWorkHandle& work, std::uint32_t at,
                                  std::uint8_t value, std::uint32_t bytes) {
    const auto accepted = validate(work, at, bytes);
    if (!accepted) return accepted;
    const auto start = work->data_offset() + at;
    forget_references(start, bytes);
    std::fill(bytes_.begin() + start, bytes_.begin() + start + bytes, value);
    std::fill(known_.begin() + start, known_.begin() + start + bytes, 1);
    return accepted;
}

TaskWorkResult TaskWorkArena::set_reference(const TaskWorkHandle& work, std::uint32_t at,
                                           const TaskWorkHandle& value) {
    return set_reference_view(work,at,value,0);
}

TaskWorkResult TaskWorkArena::set_reference_view(const TaskWorkHandle& work,std::uint32_t at,
                                               const TaskWorkHandle& value,std::uint32_t byte_offset) {
    const auto accepted = validate(work, at, 4);
    if (!accepted) return accepted;
    if (value) { const auto checked = validate(value,byte_offset,1); if (!checked) return checked; }
    else if(byte_offset)return {TaskWorkFailure::out_of_bounds,work};
    const auto offset = work->data_offset() + at;
    forget_references(offset, 4);
    if (value) references_[offset] = Reference{value,byte_offset,value->data_offset()};
    store_pointer(offset, value ? std::optional<std::uint32_t>(value->data_offset()+byte_offset)
                                : std::nullopt);
    return accepted;
}

TaskWorkResult TaskWorkArena::reference(const TaskWorkHandle& work, std::uint32_t at) const {
    const auto result=reference_view(work,at);
    if(!result)return {result.failure,result.work};
    if(result.byte_offset)return {TaskWorkFailure::interior_reference_requires_view,result.work};
    return {TaskWorkFailure::none,result.work};
}

TaskWorkReferenceResult TaskWorkArena::reference_view(const TaskWorkHandle& work,std::uint32_t at) const {
    const auto accepted = validate(work, at, 4);
    if (!accepted) return {accepted.failure,work,0,{}};
    const auto found = references_.find(work->data_offset() + at);
    if (found != references_.end()) {
        const auto target=found->second.work.lock();
        return {(!target||target->released_)?TaskWorkFailure::released_work:TaskWorkFailure::none,
                target,found->second.byte_offset,found->second.allocation_data_offset};
    }
    std::uint32_t raw = 0;
    const auto read_result = read(work, at, &raw, 4);
    if (!read_result) return {read_result.failure,work,0,{}};
    return {raw ? TaskWorkFailure::unbound_reference : TaskWorkFailure::none, {},0,{}};
}

TaskWorkResult TaskWorkArena::release(const TaskWorkHandle& work) {
    if (!work) return {};
    const auto accepted = validate(work, 0, 0);
    if (!accepted) return accepted; // source double-free can hang/corrupt metadata
    auto& block = blocks_.at(work->header_);
    const auto original_capacity = *block.physical_next - work->data_offset();
    const auto previous = block.physical_previous;
    const auto next = *block.physical_next;
    const bool previous_free = previous &&
        (blocks_.at(*previous).free_next || blocks_.at(*previous).free_previous);
    const auto next_block = blocks_.at(next);
    const bool next_free = next_block.physical_next &&
        (next_block.free_next || next_block.free_previous);
    if (previous_free) {
        auto& previous_block = blocks_.at(*previous);
        if (next_free) {
            // 0x40b35e writes the removed next header too. Those stale bytes
            // later become allocated payload and are observable after reuse.
            blocks_.at(next).physical_previous = *previous;
            sync(blocks_.at(next));
            remove_free(next);
            previous_block.physical_next = next_block.physical_next;
            auto& following = blocks_.at(*next_block.physical_next);
            following.physical_previous = *previous; sync(following);
            sync(previous_block);
            blocks_.erase(next);
        } else {
            previous_block.physical_next = next;
            blocks_.at(next).physical_previous = *previous;
            sync(blocks_.at(next)); sync(previous_block);
        }
        blocks_.erase(work->header_);
    } else if (next_free) {
        replace_free(next, block.offset);
        block.physical_next = next_block.physical_next;
        auto& following = blocks_.at(*next_block.physical_next);
        following.physical_previous = block.offset; sync(following); sync(block);
        blocks_.erase(next);
    } else {
        block.free_next = free_head_; block.free_previous.reset();
        if (free_head_) {
            blocks_.at(*free_head_).free_previous = block.offset;
            sync(blocks_.at(*free_head_));
        }
        free_head_ = block.offset; sync(block);
    }
    forget_references(work->data_offset(), original_capacity);
    work->released_ = true;
    live_.erase(work->header_);
    return accepted;
}

std::vector<TaskWorkBlock> TaskWorkArena::blocks() const {
    std::vector<TaskWorkBlock> result;
    for (const auto& pair : blocks_) result.push_back(pair.second);
    return result;
}

} // namespace sadx
