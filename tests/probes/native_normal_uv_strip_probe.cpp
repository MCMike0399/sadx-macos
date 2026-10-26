#include "engine/normal_uv_strip.hpp"
#include "normal_uv_strip_probe.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <vector>

namespace sadx::polybuff {
struct DeviceBuffer {};
struct BufferMemory {
    std::vector<std::uint8_t> bytes;
};
struct Stop {};
struct Context {
    X87Context fp;
    const std::uint32_t *input;
    std::vector<std::array<std::uint32_t, 6>> trace;
    std::vector<std::uint32_t> arguments;
    DeviceBuffer resource;
    BufferMemory memory;
    unsigned api_count = 0;
    Context(const std::uint32_t *v) : fp(static_cast<std::uint16_t>(v[9])), input(v),
        arguments(v + kArgumentsInput, v + kArgumentsInput + 512) {
        memory.bytes.resize(v[11] * 4, 0xcd);
    }
    void record(std::uint32_t type, std::uint32_t a = 0, std::uint32_t b = 0,
                std::uint32_t c = 0, std::uint32_t d = 0, std::uint32_t e = 0) {
        if (trace.size() >= 40000) throw Stop{};
        trace.push_back({type, a, b, c, d, e});
    }
    void api(std::uint32_t id, std::uint32_t a = 0, std::uint32_t b = 0,
             std::uint32_t c = 0, std::uint32_t d = 0) {
        record(4, id, a, b, c, d);
        if (++api_count == input[20]) throw Stop{};
    }
};
void set_vertex_shader(Context &c, std::uint32_t f) { c.api(1, f); }
void set_stream_source(Context &c, DeviceBuffer *r, std::uint32_t s, std::uint32_t stride) {
    c.api(2, r ? 1 : 0, s, stride);
}
void lock_vertices(Context &c, DeviceBuffer *r, std::uint32_t offset, std::uint32_t size,
                   VertexCursor &out, std::uint32_t flags) {
    if (!r) throw DataFault(DataKind::DeviceResource, 0);
    c.api(3, offset, size, flags, 1);
    if (!(c.input[10] & 1)) out = {&c.memory, c.input[21]};
}
std::int32_t unlock_vertices(Context &c, DeviceBuffer *r) {
    if (!r) throw DataFault(DataKind::DeviceResource, 0);
    c.api(4, 1);
    return c.input[10] & 2 ? static_cast<std::int32_t>(0x80004005U) : 0;
}
void set_render_state(Context &c, std::uint32_t s, std::uint32_t v) { c.api(5, s, v); }
void draw_primitive(Context &c, std::uint32_t t, std::uint32_t s, std::uint32_t n) { c.api(6,t,s,n); }
void create_vertex_buffer(Context &c, BufferState &b, std::uint32_t bytes, std::uint32_t usage,
                          std::uint32_t format, std::uint32_t pool) {
    c.api(7, bytes, usage, format, pool);
    if (!(c.input[10] & 4)) {
        c.record(1, static_cast<unsigned>(BufferField::Resource), 1);
        b.resource = &c.resource;
    }
}
RenderArguments allocate_arguments(Context &c, std::uint32_t bytes) {
    c.api(8, bytes);
    return {{c.input[10] & 8 ? nullptr : c.arguments.data(), c.input[10] & 8 ? 0U : c.input[17],
            0, DataKind::RenderArguments}};
}
void release_vertex_buffer(Context &c, DeviceBuffer *) { c.api(9, 1); }
void free_arguments(Context &c, RenderArguments) { c.api(10, 1); }
void store_vertex_word(Context &c, VertexCursor out, std::uint32_t word, std::uint32_t value) {
    const auto offset = out.byte_offset + word * 4;
    if (!out.memory || offset / 4 >= c.input[11]) {
        c.record(5, 5, offset);
        throw Stop{};
    }
    std::memcpy(out.memory->bytes.data() + offset, &value, sizeof(value));
    c.record(3, offset, value);
}
void observe_buffer_store(Context &c, const BufferState &, BufferField field, std::uint32_t value) {
    if (field == BufferField::DebugName) value = c.input[19];
    c.record(1, static_cast<unsigned>(field), value);
}
void observe_argument_store(Context &c, std::uint32_t row, ArgumentField field, std::uint32_t value) {
    const auto word = (row * kRenderArgumentBytes) / kWordBytes + static_cast<std::uint32_t>(field);
    c.record(2, word, value);
}
fidelity::ExactFloat exact(Context &c, float v) { return fidelity::ExactFloat(c.fp, v); }
} // namespace sadx::polybuff

extern "C" unsigned sadx_native_normal_uv_strip_probe(const std::uint32_t *v, std::uint32_t *out) {
    namespace p = sadx::polybuff;
    p::Context c(v);
    p::BufferState b{v[10] & 16 ? nullptr : &c.resource, v[3], v[4], v[2], v[6],
        {{v[10] & 64 ? nullptr : c.arguments.data(), v[10] & 64 ? 0U : v[17], 0,
         p::DataKind::RenderArguments}}, v[5], reinterpret_cast<const char *>(std::uintptr_t(v[19])), v[22]};
    std::vector<std::int16_t> mesh(2048), uv(4096);
    for (unsigned n = 0; n < mesh.size(); ++n) mesh[n] = static_cast<std::int16_t>(v[kMeshInput+n]);
    for (unsigned n = 0; n < uv.size(); ++n) uv[n] = static_cast<std::int16_t>(v[kUvInput+n]);
    const auto signed_word = [](std::uint32_t n) { return static_cast<std::int32_t>(n); };
    p::Mesh m{static_cast<std::uint16_t>(v[1]),
        {mesh.data(), v[12], signed_word(v[23]), p::DataKind::Mesh},
        {uv.data(), v[13], signed_word(v[24]), p::DataKind::TextureCoordinates}};
    p::Vectors points{{v+kPointsInput, v[15], signed_word(v[14]), p::DataKind::Points}};
    p::Vectors normals{{v+kNormalsInput, v[16], signed_word(v[25]), p::DataKind::Normals}};
    try {
        switch (v[0]) {
        case 0: p::normal_uv_strip(c,b,m,points,normals,v[8],v[7]); break;
        case 1: {
            const auto cursor = p::lock_triangle_strip(c,b,v[18],v[7]);
            c.record(6, cursor.memory ? 1 : 0, cursor.byte_offset);
            break;
        }
        case 2: p::draw_triangle_strip(c,b); break;
        case 3: p::initialize(c,v[10]&32 ? nullptr : &b,v[18],v[2],v[6],b.debug_name); break;
        case 4: p::release(c,v[10]&32 ? nullptr : &b); break;
        case 5: p::discard(c,b); break;
        case 6: p::set_current(c,b); break;
        case 7: c.record(6, static_cast<std::uint32_t>(p::unlock(c,b))); break;
        default: throw p::Stop{};
        }
    } catch (const p::DataFault &e) {
        c.record(5, static_cast<unsigned>(e.kind), static_cast<std::uint32_t>(e.index));
    } catch (const std::domain_error &) { c.record(5, 7); }
      catch (const p::Stop &) {}
    unsigned n = 0;
    out[n++] = b.resource ? 1 : 0;
    out[n++] = b.capacity_bytes; out[n++] = b.remaining_bytes; out[n++] = b.stride;
    out[n++] = b.vertex_format; out[n++] = b.render_arguments.words.data ? 1 : 0;
    out[n++] = b.lock_count; out[n++] = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(b.debug_name));
    out[n++] = b.unused;
    out[n++] = c.fp.exception_flags(); out[n++] = c.fp.control_word();
    out[n++] = static_cast<unsigned>(c.trace.size());
    std::set<std::uint32_t> offsets;
    for (const auto &r : c.trace) {
        for (auto word : r) out[n++] = word;
        if (r[0] == 3) offsets.insert(r[1]);
    }
    out[n++] = static_cast<unsigned>(offsets.size());
    for (auto offset : offsets) {
        out[n++] = offset;
        std::uint32_t word;
        std::memcpy(&word, c.memory.bytes.data() + offset, sizeof(word));
        out[n++] = word;
    }
    const auto argument_count = b.render_arguments.words.data ? b.render_arguments.words.size : 0;
    out[n++] = static_cast<unsigned>(argument_count);
    for (std::size_t word = 0; word != argument_count; ++word)
        out[n++] = c.arguments.at(word);
    return n;
}
