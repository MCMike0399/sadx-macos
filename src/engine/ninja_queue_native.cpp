#include "engine/ninja_queue_native.hpp"
#include "engine/graphics_frame.hpp"
#include "engine/ninja_sprite_queue.hpp"
#include <utility>

namespace sadx::queue {
Buffer allocate_buffer() {
    auto arena = std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(kBufferBytes), 0,
                                                         kBufferBytes, 0);
    const auto address = reinterpret_cast<std::uintptr_t>(arena->bytes().data());
    return {std::move(arena), static_cast<std::uint32_t>(address % 32)};
}
struct Storage::Impl {
    Buffer buffer;
    NinjaSpriteQueue queue{nullptr};
};
Storage::Storage() : impl_(std::make_unique<Impl>()) {}
Storage::~Storage() = default;
NinjaSpriteQueue &Storage::records() {
    return impl_->queue;
}
const NinjaSpriteQueue &Storage::records() const {
    return impl_->queue;
}
const Buffer &Storage::buffer() const {
    return impl_->buffer;
}
void Storage::release_buffer() {
    impl_->queue.rebind_arena(nullptr);
    impl_->buffer = {};
}
void Storage::bind_buffer(Buffer buffer) {
    if (!buffer.arena || buffer.arena->bytes().size() != kBufferBytes ||
        buffer.alignment_remainder >= 32)
        throw Unavailable(Boundary::Buffer);
    impl_->buffer = std::move(buffer);
    impl_->queue.rebind_arena(impl_->buffer.arena);
}
void Storage::reset_buffer() {
    if (!impl_->buffer.arena)
        throw Unavailable(Boundary::Buffer);
    const auto result = impl_->buffer.arena->reset_407ab0(impl_->buffer.alignment_remainder);
    if (result != NinjaQueueFailure::none)
        throw Unavailable(Boundary::Buffer);
    impl_->queue.forget_records_after_arena_reset();
}
Unavailable::Unavailable(Boundary boundary)
    : std::runtime_error("native queue resource/service unavailable"), boundary_(boundary) {}
void set_material(const std::array<float, 4> &input, State &state, const Services &services,
                  X87Context &fp) {
    auto &material = state.current_material;
    material[0] = input[0];
    auto loaded = fp.from_float(input[0]);
    const auto first = fp.compare_float_memory(loaded, 1.0f);
    material[1] = input[1];
    material[2] = input[2];
    material[3] = input[3];
    for (std::size_t component = 0; component != material.size(); ++component) {
        auto compared = first;
        if (component != 0) {
            loaded = fp.from_float(material[component]);
            compared = fp.compare_float_memory(loaded, 1.0f);
        }
        if (compared == X87Comparison::greater) {
            material[component] = 1.0f;
        } else {
            loaded = fp.from_float(material[component]);
            if (fp.compare_float_memory(loaded, -1.0f) == X87Comparison::less)
                material[component] = -1.0f;
        }
    }
    if (!services.set_constant_material)
        throw Unavailable(Boundary::Material);
    services.set_constant_material(material);
}
void reset_material(Owners &owners, const Services &services, X87Context &fp) {
    set_material(owners.reset_material, owners.state, services, fp);
}
void reset_draw_queue(Storage &storage, State &state, const Services &services) {
    const auto comparison = static_cast<std::uint32_t>(state.values.depth_comparison) & 7U;
    if (!services.set_depth_comparison || !services.set_depth_comparison(comparison))
        throw Unavailable(Boundary::DepthComparison);
    if (!services.set_depth_write || !services.set_depth_write(true))
        throw Unavailable(Boundary::DepthWrite);
    if (!services.set_alpha || !services.set_alpha(true))
        throw Unavailable(Boundary::Alpha);
    state.values.lighting_palette = 0;
    state.values.lighting_copy = 0;
    storage.records().initialize_sort_buckets();
    state.values.reset_capture = state.values.reset_source;
    if (!services.set_constant_material)
        throw Unavailable(Boundary::Material);
    services.set_constant_material(state.current_material);
    state.values.execution_mode = 0;
}
void reset(Owners &owners, const Services &services, X87Context &fp) {
    owners.storage.reset_buffer();
    reset_material(owners, services, fp);
    reset_draw_queue(owners.storage, owners.state, services);
    owners.loop_count = 0;
    owners.state.values.primary_count = 0;
    owners.state.values.secondary_count = 0;
}
void initialize(Owners &owners, const Services &services, X87Context &fp,
                const std::function<Buffer()> &allocator) {
    owners.storage.release_buffer();
    if (!allocator)
        throw Unavailable(Boundary::Buffer);
    owners.storage.bind_buffer(allocator());
    owners.state.values.allocation_capture = owners.state.values.allocation_source;
    reset(owners, services, fp);
}
void set_blend(std::uint32_t selector, std::uint32_t value, State &state,
               const Services &services) {
    auto &packed = state.values.packed_blend;
    if (selector == 0)
        packed = static_cast<std::uint8_t>((packed & 240) | (value & 15));
    else if (selector == 1)
        packed = static_cast<std::uint8_t>((packed & 15) | ((value & 15) << 4));
    if (!services.set_blend || !services.set_blend(selector, value))
        throw Unavailable(Boundary::Blend);
}
namespace {
// NOTSA: require the current native device service without manufacturing one.
RenderStateWriter acquire_device(const DeviceAccess &access) {
    if (!access)
        throw Unavailable(Boundary::Device);
    auto writer = access();
    if (!writer)
        throw Unavailable(Boundary::Device);
    return writer;
}
void issue(const RenderStateWriter &writer, std::uint32_t setting, std::uint32_t value) {
    if (!writer(setting, value))
        throw Unavailable(Boundary::Blend);
}
} // namespace
void set_device_blend(std::uint32_t selector, std::uint32_t mode, const BlendTables &tables,
                      const DeviceAccess &access) {
    if (mode < 2) {
        auto writer = acquire_device(access);
        issue(writer, 19, mode == 0 ? 6 : 5);
        writer = acquire_device(access);
        issue(writer, 20, mode == 0 ? 5 : 6);
        return;
    }
    if (mode >= tables.modes.size() + tables.following_depth_modes.size())
        throw Unavailable(Boundary::BlendValue);
    const auto mapped = mode < tables.modes.size()
                            ? tables.modes[mode]
                            : tables.following_depth_modes[mode - tables.modes.size()];
    auto writer = acquire_device(access);
    if (selector >= tables.targets.size())
        throw Unavailable(Boundary::BlendSelector);
    issue(writer, tables.targets[selector], mapped);
}
void set_device_blend(std::uint32_t selector, std::uint32_t mode, const BlendTables &tables,
                      graphics::Draw &draw) {
    const DeviceAccess access = [&draw] {
        return [&draw](std::uint32_t setting, std::uint32_t value) {
            draw.render_states[setting] = value;
            return true;
        };
    };
    set_device_blend(selector, mode, tables, access);
}
} // namespace sadx::queue
