#include "loader/guest_heap.hpp"
#include <limits>

namespace sadx {
namespace {
// NOTSA: deterministic reference identities, not Windows allocator layout facts.
constexpr std::uint64_t kAlignment = 8;
constexpr std::uint64_t kBlockGap = 8;
constexpr std::uint64_t kLargestAddress = std::numeric_limits<std::uint32_t>::max();
}

GuestHeap::GuestHeap(std::uint32_t handle_token, std::uint32_t first_address)
    : handle_token_(handle_token), next_address_(first_address) {}

std::uint32_t GuestHeap::create() {
    trace.push_back({'C', handle_token_, 0});
    return handle_token_;
}

std::optional<std::uint32_t> GuestHeap::alloc(std::uint32_t handle, std::uint32_t size) {
    if (handle != handle_token_) {
        trace.push_back({'A', size, 0});
        return std::nullopt;
    }
    const auto backed_size = size == 0 ? 1U : size;
    const auto span = (static_cast<std::uint64_t>(backed_size) + kAlignment - 1) & ~(kAlignment - 1);
    if (static_cast<std::uint64_t>(next_address_) + span + kBlockGap > kLargestAddress) {
        trace.push_back({'A', size, 0});
        return std::nullopt;
    }
    const auto address = next_address_;
    blocks_[address] = Block{size, std::vector<std::optional<std::uint8_t>>(backed_size)};
    next_address_ = static_cast<std::uint32_t>(next_address_ + span + kBlockGap);
    trace.push_back({'A', size, address});
    return address;
}

bool GuestHeap::free(std::uint32_t handle, std::uint32_t address) {
    trace.push_back({'F', address, 0});
    if (handle != handle_token_)
        return false;
    return blocks_.erase(address) == 1;
}

const GuestHeap::Block *GuestHeap::find(std::uint32_t address, std::uint32_t &offset) const {
    auto block = blocks_.upper_bound(address);
    if (block == blocks_.begin())
        return nullptr;
    --block;
    offset = address - block->first;
    if (offset >= block->second.bytes.size())
        return nullptr;
    return &block->second;
}

std::optional<std::uint8_t> GuestHeap::read(std::uint32_t address) const {
    std::uint32_t offset = 0;
    const auto *block = find(address, offset);
    if (!block)
        return std::nullopt;
    return block->bytes[offset];
}

bool GuestHeap::write(std::uint32_t address, std::uint8_t value) {
    auto block = blocks_.upper_bound(address);
    if (block == blocks_.begin())
        return false;
    --block;
    const auto offset = address - block->first;
    if (offset >= block->second.bytes.size())
        return false;
    block->second.bytes[offset] = value;
    return true;
}
} // namespace sadx
