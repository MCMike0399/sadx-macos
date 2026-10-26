#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>
#include <functional>
#include <utility>

namespace sadx {

namespace collision_component { struct Work; }

class TaskWorkArena;
class TaskWork;
using TaskWorkHandle = std::shared_ptr<TaskWork>;

enum class TaskWorkFailure {
    none, exhausted, size_wrap, foreign_work, released_work, out_of_bounds,
    unobserved_pointer_bytes, unbound_reference,
    invalid_buffer,
    unobserved_live_field,
    interior_reference_requires_view,
    conflicting_live_view,
};
struct TaskWorkResult {
    TaskWorkFailure failure = TaskWorkFailure::none;
    TaskWorkHandle work;
    explicit operator bool() const noexcept { return failure == TaskWorkFailure::none; }
};

class TaskWork {
public:
    // NOTSA: typed collision payload; arena bytes retain only scheduler cleanup metadata.
    std::shared_ptr<collision_component::Work> collision_state;
    std::uint32_t data_offset() const noexcept { return header_ + 32; }
    std::uint32_t requested_size() const noexcept { return requested_; }
    bool released() const noexcept { return released_; }
private:
    friend class TaskWorkArena;
    TaskWorkArena* owner_ = nullptr;
    std::uint32_t header_ = 0;
    std::uint32_t requested_ = 0;
    bool released_ = false;
};

struct TaskWorkBlock {
    std::uint32_t offset = 0;
    std::optional<std::uint32_t> free_next, free_previous;
    std::optional<std::uint32_t> physical_next, physical_previous;
};

struct TaskWorkReferenceResult {
    TaskWorkFailure failure = TaskWorkFailure::none;
    TaskWorkHandle work;
    std::uint32_t byte_offset = 0;
    std::optional<std::uint32_t> allocation_data_offset;
    explicit operator bool() const noexcept { return failure==TaskWorkFailure::none; }
};

// Source 0x40b1a0/0x40b220/0x40b310. Offsets identify native arena storage;
// they are never converted into host or guest pointers. Optional reference_base
// is provenance for reproducing pointer bytes, not a functional address space.
class TaskWorkArena {
public:
    static constexpr std::uint32_t usable_bytes = 0x1000000;
    static constexpr std::uint32_t header_bytes = 32;
    explicit TaskWorkArena(std::optional<std::uint32_t> reference_base = {});
    TaskWorkArena(const TaskWorkArena&) = delete;
    TaskWorkArena& operator=(const TaskWorkArena&) = delete;

    TaskWorkResult allocate(std::uint32_t bytes);
    TaskWorkResult release(const TaskWorkHandle& work); // null is source no-op
    TaskWorkResult read(const TaskWorkHandle&, std::uint32_t at,
                        void* output, std::uint32_t bytes) const;
    TaskWorkResult write(const TaskWorkHandle&, std::uint32_t at,
                         const void* input, std::uint32_t bytes);
    TaskWorkResult fill(const TaskWorkHandle&, std::uint32_t at,
                        std::uint8_t value, std::uint32_t bytes);
    // Native ownership for actual source pointer fields. Null writes known zero.
    TaskWorkResult set_reference(const TaskWorkHandle&, std::uint32_t at,
                                 const TaskWorkHandle& value);
    TaskWorkResult set_reference_view(const TaskWorkHandle&,std::uint32_t at,
                                     const TaskWorkHandle& value,std::uint32_t byte_offset);
    TaskWorkResult reference(const TaskWorkHandle&, std::uint32_t at) const;
    TaskWorkReferenceResult reference_view(const TaskWorkHandle&,std::uint32_t at) const;
    std::optional<std::uint32_t> free_head() const noexcept { return free_head_; }
    std::vector<TaskWorkBlock> blocks() const;
    const std::vector<std::uint8_t>& source_bytes() const noexcept { return bytes_; }
    const std::vector<std::uint8_t>& known_bytes() const noexcept { return known_; }
    // Transport serialized metadata stores to an external owner. Payload remains owned
    // separately by that runtime. nullopt reports pointer bytes without provenance.
    using MetadataWriter = std::function<void(std::uint32_t,std::optional<std::uint32_t>)>;
    void set_metadata_writer(MetadataWriter writer) { metadata_writer_=std::move(writer); }
    using PayloadReader = std::function<bool(std::uint32_t,void*,std::uint32_t)>;
    using PayloadWriter = std::function<bool(std::uint32_t,const void*,std::uint32_t)>;
    void set_payload_transport(PayloadReader read,PayloadWriter write) { payload_reader_=std::move(read);payload_writer_=std::move(write); }
    using ReferenceResolver = std::function<TaskWorkReferenceResult(std::uint32_t)>;
    void set_reference_resolver(ReferenceResolver resolve) { reference_resolver_=std::move(resolve); }

private:
    std::optional<std::uint32_t> reference_base_;
    std::vector<std::uint8_t> bytes_, known_;
    std::map<std::uint32_t, TaskWorkBlock> blocks_;
    std::map<std::uint32_t, TaskWorkHandle> live_;
    struct Reference {
        std::weak_ptr<TaskWork> work;
        std::uint32_t byte_offset,allocation_data_offset;
    };
    std::map<std::uint32_t, Reference> references_;
    std::optional<std::uint32_t> free_head_;
    MetadataWriter metadata_writer_;
    PayloadReader payload_reader_;
    PayloadWriter payload_writer_;
    ReferenceResolver reference_resolver_;
    TaskWorkResult validate(const TaskWorkHandle&, std::uint32_t at,
                            std::uint32_t bytes) const;
    void store_pointer(std::uint32_t at, std::optional<std::uint32_t> offset);
    void sync(const TaskWorkBlock&);
    void remove_free(std::uint32_t);
    void replace_free(std::uint32_t old_offset, std::uint32_t new_offset);
    void forget_references(std::uint32_t at, std::uint32_t bytes);
};

} // namespace sadx
