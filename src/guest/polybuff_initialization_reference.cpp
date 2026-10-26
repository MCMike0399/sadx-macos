#include "guest/polybuff_owners.hpp"
#include "engine/ninja_system_init.hpp"

namespace sadx::polybuff_reference {
using namespace sadx::polybuff;
struct Context {
    NinjaSystemInitRuntime &runtime;
    NinjaSystemInitResult result{};

};
namespace {
struct Stop {};
[[noreturn]] void fail(Context &c, NinjaSystemInitFailure failure, std::uint32_t at,
                       std::optional<std::uint32_t> fact = {}) {
    c.result.failure = failure;
    c.result.source_boundary = at;
    c.result.required_fact = fact;
    throw Stop{};
}
std::uint32_t need(Context &c, const std::optional<std::uint32_t> &value,
                   std::uint32_t at, std::uint32_t fact) {
    if (!value) fail(c, NinjaSystemInitFailure::unknown_state, at, fact);
    return *value;
}
}
std::uint32_t configured_count(Context &c, std::size_t index) {
    return need(c, c.runtime.borrowed.polygon_counts()[index], 0x78e763,
                0x389d864 + static_cast<std::uint32_t>(4 * index));
}
BufferDescription buffer_description(Context &c, std::size_t index) {
    auto &table = c.runtime.state.polygon_descriptions();
    const auto offset = static_cast<std::uint32_t>(4 * index);
    const auto name = need(c, table[18 + index], 0x78e750, 0x389d8d0 + offset);
    const auto format = need(c, table[9 + index], 0x78e756, 0x389d8ac + offset);
    const auto stride = need(c, table[index], 0x78e75c, 0x389d888 + offset);
    return {stride, format, reinterpret_cast<const char *>(std::uintptr_t(name))};
}
BufferMemory *allocate_vertex_scratch(Context &c, std::uint32_t bytes) {
    auto &heap = c.runtime.borrowed.crt_heap;
    const auto identity = heap.alloc(heap.create(), bytes).value_or(0);
    return reinterpret_cast<BufferMemory *>(std::uintptr_t(identity));
}
void observe_initialization_store(Context &c, InitializationField field, std::size_t index) {
    // The core performs the value stores immediately after these observations. Adapters
    // publish availability only, never a duplicate value or an alternate clear algorithm.
    if (field == InitializationField::RetainedBase)
        c.runtime.state.polygon_base_fact->available = true;
    else if (field == InitializationField::Vertices)
        c.runtime.borrowed.frame.polygon_facts->vertices.available = true;
    else {
        auto &bytes = c.runtime.borrowed.polygon_scratch().bytes;
        for (std::size_t byte = 0; byte != 4; ++byte) bytes[4 * index + byte].available = true;
    }
}
void initialize_record(Context &c, BufferState &, std::size_t index, std::uint32_t count,
                       BufferDescription description) {
    // This existing reference boundary intentionally remains the frozen judge's receipt.
    // Native linkage instead selects initialize_record in polybuff_initialization_binding.cpp.
    auto &pending = c.result.pending_call;
    pending = NinjaSystemCall{0x78e76d, 0x794540,
        0x3d0fe00 + static_cast<std::uint32_t>(36 * index), count,
        {description.stride, description.vertex_format,
         static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(description.debug_name)), {}}};
    if (!c.runtime.invoke) fail(c, NinjaSystemInitFailure::missing_owner, 0x78e76d);
    const auto receipt = c.runtime.invoke(*pending);
    if (receipt.owner.state != GameLogicOwnerState::completed) {
        const auto failure = receipt.owner.state == GameLogicOwnerState::cancelled ?
            NinjaSystemInitFailure::cancelled : receipt.owner.state == GameLogicOwnerState::failed ?
            NinjaSystemInitFailure::owner_failed : NinjaSystemInitFailure::missing_owner;
        fail(c, failure, 0x78e76d);
    }
    pending.reset();
}
} // namespace sadx::polybuff_reference

namespace sadx {
NinjaSystemInitResult initialize_system_polygon_buffers(NinjaSystemInitRuntime &runtime,
                                                       std::uint32_t size, std::uint32_t base) {
    polybuff_reference::Context context{runtime, {}};
    try {
        polybuff::initialize_buffers(context, runtime.borrowed.frame.polygon_buffers,
            runtime.state.polygon_base, runtime.borrowed.polygon_scratch().owner,
            32, size, reinterpret_cast<void *>(std::uintptr_t(base)));
    } catch (const polybuff_reference::Stop &) {}
    return context.result;
}
} // namespace sadx
