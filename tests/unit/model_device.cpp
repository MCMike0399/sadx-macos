#include "runtime/model_device.hpp"
#include <cassert>
#include <cstring>

namespace {
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
template <std::size_t Count>
sadx::polybuff::Vectors vectors(std::array<std::uint32_t, Count> &words,
                                sadx::polybuff::DataKind kind) {
    return {{words.data(), words.size(), 0, kind}};
}
} // namespace

int main(int argc, char **argv) {
    assert(argc == 2);
    using namespace sadx;
    namespace pb = polybuff;
    frame::ModeState mode;
    native::Display display(argv[1], mode);
    pb::Context context{display};
    pb::BufferState buffer{};
    constexpr std::uint32_t kVertexCapacity = 6;
    constexpr std::uint32_t kNormalUvFormat = 0x152;
    pb::initialize(context, &buffer, kVertexCapacity, pb::kNormalUvVertexBytes, kNormalUvFormat,
                   "native device unit");
    auto *resource = buffer.resource;
    assert(resource && resource->references == 1);
    assert(resource->memory.bytes.size() == kVertexCapacity * pb::kNormalUvVertexBytes);
    assert(resource->usage == pb::kDynamicWriteOnly && resource->format == kNormalUvFormat);
    assert(resource->pool == 0);
    pb::set_current(context, buffer);
    assert(resource->references == 2);
    pb::set_current(context, buffer);
    assert(resource->references == 2);

    // Failed locks must not replace a usable earlier output, or the source's stale output.
    pb::VertexCursor cursor{&resource->memory, 4};
    pb::lock_vertices(context, resource, buffer.capacity_bytes, 1, cursor, pb::kDiscard);
    assert(cursor.memory == &resource->memory && cursor.byte_offset == 4);
    pb::VertexCursor stale{nullptr, pb::kNormalUvVertexBytes};
    pb::lock_vertices(context, resource, buffer.capacity_bytes, 1, stale, pb::kDiscard);
    bool stopped = false;
    try {
        pb::store_vertex_word(context, stale, 0, 1);
    } catch (const pb::DataFault &fault) {
        stopped =
            fault.kind == pb::DataKind::VertexBytes && fault.index == pb::kNormalUvVertexBytes;
    }
    assert(stopped);

    std::array<std::int16_t, 4> indices{3, 0, 1, 2};
    std::array<std::int16_t, 6> uv{0, 0, 255, 0, 0, 255};
    std::array<std::uint32_t, 9> points{bits(0), bits(0), bits(0), bits(1), bits(0),
                                        bits(0), bits(0), bits(1), bits(0)};
    std::array<std::uint32_t, 9> normals{bits(0), bits(0), bits(1), bits(0), bits(0),
                                         bits(1), bits(0), bits(0), bits(1)};
    pb::Mesh mesh{1,
                  {indices.data(), indices.size(), 0, pb::DataKind::Mesh},
                  {uv.data(), uv.size(), 0, pb::DataKind::TextureCoordinates}};
    auto &device = context.device();
    device.world = device.view = device.projection = graphics::kIdentity;
    device.material.diffuse = {0.25f, 0.5f, 0.75f, 1};
    device.render_states[137] = 0;
    constexpr std::uint32_t kBlue = 0xff0080ff;
    pb::normal_uv_strip(context, buffer, mesh, vectors(points, pb::DataKind::Points),
                        vectors(normals, pb::DataKind::Normals), kBlue, 1);
    assert(buffer.lock_count == 0 && context.frame().commands.size() == 1);
    const auto &first = std::get<graphics::Draw>(context.frame().commands[0]);
    assert(first.primitive_type == pb::kTriangleStripPrimitive && first.primitive_count == 3);
    assert(first.vertices.size() == 5 && first.has_normals && first.texture_coordinates == 1);
    assert(first.vertices[0].position[0] == 0 && first.vertices[1].position[0] == 0);
    assert(first.vertices[2].position[0] == 1 && first.vertices[3].position[1] == 1);
    assert(first.vertices[4].position == first.vertices[3].position);
    assert(first.vertices[0].diffuse[0] == 0 && first.vertices[0].diffuse[2] == 1);
    assert(first.render_states.at(pb::kCullModeState) == 1);
    assert(first.material.diffuse == device.material.diffuse);

    // Resource reuse and later state changes cannot rewrite an earlier submitted draw.
    pb::store_vertex_word(context, {&resource->memory, 0}, 0, bits(20));
    device.material.diffuse[0] = 0.875f;
    pb::draw_primitive(context, pb::kTriangleStripPrimitive, 0, 3);
    const auto &old_draw = std::get<graphics::Draw>(context.frame().commands[0]);
    const auto &new_draw = std::get<graphics::Draw>(context.frame().commands[1]);
    assert(old_draw.vertices[0].position[0] == 0 && new_draw.vertices[0].position[0] == 20);
    assert(old_draw.material.diffuse[0] == 0.25f && new_draw.material.diffuse[0] == 0.875f);

    // The device still owns its stream after the source game releases its handle.
    pb::release(context, &buffer);
    assert(!buffer.resource && resource->references == 1);
    pb::draw_primitive(context, pb::kTriangleStripPrimitive, 0, 3);
    pb::set_stream_source(context, nullptr, 0, 0);
    assert(context.stream().resource == nullptr);
    assert(std::get<graphics::Draw>(context.frame().commands[0]).vertices[0].position[0] == 0);
    stopped = false;
    try {
        pb::draw_primitive(context, pb::kTriangleStripPrimitive, 0, 3);
    } catch (const pb::DataFault &fault) {
        stopped = fault.kind == pb::DataKind::DeviceResource;
    }
    assert(stopped && context.frame().commands.size() == 3);
}
