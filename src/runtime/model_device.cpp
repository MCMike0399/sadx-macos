#include "runtime/model_device.hpp"
#include <cstring>
#include <new>

namespace sadx::polybuff {
namespace {
// NOTSA: the native resource follows the reference device's COM retention rules.
void retain_resource(DeviceBuffer *resource) {
    if (resource)
        ++resource->references;
}
void release_resource(DeviceBuffer *resource) {
    if (resource && --resource->references == 0)
        delete resource;
}
// NOTSA: one checked range for buffer reads and writes at the platform boundary.
bool available(const BufferMemory &memory, std::uint64_t offset, std::uint64_t bytes) {
    return offset <= memory.bytes.size() && bytes <= memory.bytes.size() - offset;
}
} // namespace

// [platform:d3d8] SetVertexShader's fixed-function FVF is device state.
void set_vertex_shader(Context &context, std::uint32_t format) { context.stream().format = format; }
void set_stream_source(Context &context, DeviceBuffer *resource, std::uint32_t stream,
                       std::uint32_t stride) {
    if (stream != 0)
        throw DataFault(DataKind::DeviceResource, stream);
    auto &current = context.stream();
    if (current.resource != resource) {
        retain_resource(resource);
        release_resource(current.resource);
        current.resource = resource;
    }
    current.stride = stride;
}

// [platform:d3d8] Invalid Lock leaves the caller's output pointer untouched.
// Previous submitted draws already own their vertex bytes across DISCARD/reuse.
void lock_vertices(Context &, DeviceBuffer *resource, std::uint32_t offset, std::uint32_t bytes,
                   VertexCursor &output, std::uint32_t) {
    if (!resource)
        throw DataFault(DataKind::DeviceResource, 0);
    if (available(resource->memory, offset, bytes))
        output = {&resource->memory, offset};
}
std::int32_t unlock_vertices(Context &, DeviceBuffer *resource) {
    if (!resource)
        throw DataFault(DataKind::DeviceResource, 0);
    return 0;
}
void set_render_state(Context &context, std::uint32_t state, std::uint32_t value) {
    context.device().render_states[state] = value;
}

// [platform:d3d8] DrawPrimitive snapshots all state and only the selected vertex range.
void draw_primitive(Context &context, std::uint32_t type, std::uint32_t start,
                    std::uint32_t count) {
    if (type != kTriangleStripPrimitive)
        throw DataFault(DataKind::DeviceResource, type);
    const auto &stream = context.stream();
    if (!stream.resource || stream.stride == 0)
        throw DataFault(DataKind::DeviceResource, 0);
    const auto offset = std::uint64_t(start) * stream.stride;
    const auto bytes = (std::uint64_t(count) + 2) * stream.stride;
    const auto &memory = stream.resource->memory;
    if (!available(memory, offset, bytes))
        throw DataFault(DataKind::VertexBytes, static_cast<std::int64_t>(offset));
    context.display.output().snapshot_bound_texture();
    auto draw = context.device();
    draw.primitive_type = type;
    draw.primitive_count = count;
    draw.indices.clear();
    const std::vector<std::uint8_t> vertices(memory.bytes.begin() + offset,
                                             memory.bytes.begin() + offset + bytes);
    if (const auto problem =
            graphics::decode_vertices(draw, stream.format, vertices, stream.stride);
        !problem.empty())
        throw std::runtime_error("native vertex buffer: " + problem);
    context.frame().commands.emplace_back(std::move(draw));
}

// [platform:d3d8] Creation does not release a prior output handle; failure leaves it intact.
void create_vertex_buffer(Context &, BufferState &buffer, std::uint32_t bytes, std::uint32_t usage,
                          std::uint32_t format, std::uint32_t pool) {
    try {
        auto resource = std::make_unique<DeviceBuffer>(
            DeviceBuffer{BufferMemory{std::vector<std::uint8_t>(bytes)}, 1, usage, format, pool});
        buffer.resource = resource.release();
    } catch (const std::bad_alloc &) {
        // Source ignores the failed HRESULT and consumes the unchanged handle later.
    }
}
RenderArguments allocate_arguments(Context &, std::uint32_t bytes) {
    const auto words = bytes / kWordBytes;
    auto *storage = new (std::nothrow) std::uint32_t[words];
    return {{storage, storage ? words : 0, 0, DataKind::RenderArguments}};
}
void release_vertex_buffer(Context &, DeviceBuffer *resource) { release_resource(resource); }
void free_arguments(Context &, RenderArguments arguments) { delete[] arguments.words.data; }

void store_vertex_word(Context &, VertexCursor output, std::uint32_t word, std::uint32_t value) {
    const auto offset = output.byte_offset + word * kWordBytes;
    if (!output.memory || !available(*output.memory, offset, kWordBytes))
        throw DataFault(DataKind::VertexBytes, offset);
    std::memcpy(output.memory->bytes.data() + offset, &value, sizeof(value));
}
// NOTSA: the source stores already happened in the engine. Native has no reference observer.
void observe_buffer_store(Context &, const BufferState &, BufferField, std::uint32_t) {}
void observe_argument_store(Context &, std::uint32_t, ArgumentField, std::uint32_t) {}
fidelity::ExactFloat exact(Context &context, float value) {
    return {context.display.arithmetic(), value};
}
ConfiguredCounts configured_counts(Context &context) { return {context.counts()}; }
BufferMemory *allocate_vertex_scratch(Context &, std::uint32_t bytes) {
    try {
        return new BufferMemory{std::vector<std::uint8_t>(bytes)};
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}
void observe_initialization_store(Context &, InitializationField, std::size_t) {}
} // namespace sadx::polybuff

namespace sadx::native {
ModelVertexStream::~ModelVertexStream() { polybuff::release_resource(resource); }
} // namespace sadx::native
