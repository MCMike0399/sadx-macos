#include "engine/polybuff_initialization.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace sadx::polybuff {
// Standalone receipt tokens are never dereferenced as runtime resources.
struct ReceiptToken { std::uint32_t unused = 0; };
struct Stop {};
struct Context {
    const std::uint32_t *input;
    const std::array<std::uint32_t, 9> *counts = nullptr;
    Buffers buffers{};
    RetainedBase base{};
    Scratch scratch{};
    std::array<std::vector<std::uint32_t>, 9> arguments;
    std::array<ReceiptToken, 9> resources;
    ReceiptToken vertices;
    std::vector<std::array<std::uint32_t, 8>> trace;
    std::size_t current = 0;
    unsigned api_count = 0;
    void record(std::uint32_t type, std::uint32_t a = 0, std::uint32_t b = 0,
                std::uint32_t c = 0, std::uint32_t d = 0, std::uint32_t e = 0,
                std::uint32_t f = 0, std::uint32_t g = 0) {
        trace.push_back({type, a, b, c, d, e, f, g});
    }
    void api(std::uint32_t kind, std::uint32_t bytes, std::uint32_t usage = 0,
             std::uint32_t format = 0, std::uint32_t pool = 0) {
        record(3, kind, static_cast<std::uint32_t>(current), bytes, usage, format, pool);
        if (++api_count == input[0]) throw Stop{};
    }
};
BufferMemory *allocate_vertex_scratch(Context &c, std::uint32_t bytes) {
    c.api(0, bytes);
    return c.input[1] ? nullptr : reinterpret_cast<BufferMemory *>(&c.vertices);
}
void observe_initialization_store(Context &c, InitializationField field, std::size_t index) {
    const auto value = field == InitializationField::RetainedBase ? c.input[5] :
                       field == InitializationField::Vertices ? !c.input[1] : 0;
    c.record(4, static_cast<unsigned>(field), static_cast<unsigned>(index), value);
}
void create_vertex_buffer(Context &c, BufferState &b, std::uint32_t bytes, std::uint32_t usage,
                          std::uint32_t format, std::uint32_t pool) {
    c.api(1, bytes, usage, format, pool);
    if (c.input[2] != c.current + 1) {
        c.record(1, static_cast<unsigned>(c.current), 0, 1);
        b.resource = reinterpret_cast<DeviceBuffer *>(&c.resources[c.current]);
    }
}
RenderArguments allocate_arguments(Context &c, std::uint32_t bytes) {
    c.api(2, bytes);
    if (c.input[3] == c.current + 1)
        return {{nullptr, 0, 0, DataKind::RenderArguments}};
    auto &arguments = c.arguments[c.current];
    arguments.resize(std::max(1U, bytes / 4), 0xa5a5a5a5U);
    return {{arguments.data(), bytes / 4, 0, DataKind::RenderArguments}};
}
void observe_buffer_store(Context &c, const BufferState &b, BufferField field, std::uint32_t value) {
    const auto index = static_cast<unsigned>(&b - c.buffers.records.data());
    c.current = index;
    if (field == BufferField::DebugName) value = c.input[37 + index];
    c.record(1, index, static_cast<unsigned>(field), value);
}
void observe_argument_store(Context &c, std::uint32_t row, ArgumentField field, std::uint32_t value) {
    c.record(2, static_cast<unsigned>(c.current), row * 4 + static_cast<unsigned>(field), value);
}
ConfiguredCounts configured_counts(Context &context) { return {*context.counts}; }
#ifndef POLYBUFF_INITIALIZATION_NATIVE_BINDING
std::uint32_t configured_count(Context &context, std::size_t index) {
    return configured_counts(context).values.at(index);
}
BufferDescription buffer_description(Context &c, std::size_t index) {
    return {c.input[19 + index], c.input[28 + index],
            reinterpret_cast<const char *>(std::uintptr_t(c.input[37 + index]))};
}
void initialize_record(Context &c, BufferState &b, std::size_t index, std::uint32_t count,
                       BufferDescription d) {
    c.current = index;
    c.record(6, static_cast<unsigned>(index), count, d.stride, d.vertex_format,
             static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(d.debug_name)));
    initialize(c, &b, count, d.stride, d.vertex_format, d.debug_name);
}
#endif
// Unreached functions are explicit stops; this closure exercises initialization only.
void set_vertex_shader(Context &, std::uint32_t) { throw Stop{}; }
void set_stream_source(Context &, DeviceBuffer *, std::uint32_t, std::uint32_t) { throw Stop{}; }
void lock_vertices(Context &, DeviceBuffer *, std::uint32_t, std::uint32_t, VertexCursor &,
                   std::uint32_t) { throw Stop{}; }
std::int32_t unlock_vertices(Context &, DeviceBuffer *) { throw Stop{}; }
void set_render_state(Context &, std::uint32_t, std::uint32_t) { throw Stop{}; }
void draw_primitive(Context &, std::uint32_t, std::uint32_t, std::uint32_t) { throw Stop{}; }
void release_vertex_buffer(Context &, DeviceBuffer *) { throw Stop{}; }
void free_arguments(Context &, RenderArguments) { throw Stop{}; }
void store_vertex_word(Context &, VertexCursor, std::uint32_t, std::uint32_t) { throw Stop{}; }
fidelity::ExactFloat exact(Context &, float) { throw Stop{}; }
} // namespace sadx::polybuff

extern "C" unsigned sadx_native_polybuff_initialization(const std::uint32_t *input,
                                                       std::uint32_t *output) {
    namespace p = sadx::polybuff;
    p::Context c{input, nullptr, {}, {}, {}, {}, {}, {}, {}, 0, 0};
    std::array<std::uint32_t, 9> counts{};
    std::copy(input + 10, input + 19, counts.begin());
    c.counts = &counts;
    c.base.value = reinterpret_cast<void *>(std::uintptr_t(input[7]));
    c.scratch.words.fill(input[8]);
    for (auto &b : c.buffers.records) {
        b.capacity_bytes = input[9];
        b.remaining_bytes = input[9];
        b.stride = input[9];
        b.vertex_format = input[9];
        b.lock_count = input[9];
        b.debug_name = reinterpret_cast<const char *>(std::uintptr_t(input[9]));
        b.unused = input[9];
    }
    bool completed = false;
    try {
        p::initialize_buffers(c, c.buffers, c.base, c.scratch, input[6], input[4],
                              reinterpret_cast<void *>(std::uintptr_t(input[5])));
        completed = true;
    } catch (const p::DataFault &e) {
        c.record(5, static_cast<unsigned>(e.kind), static_cast<unsigned>(c.current),
                 static_cast<std::uint32_t>(e.index));
    } catch (const p::Stop &) {}
    unsigned n = 0;
    output[n++] = completed;
    output[n++] = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(c.base.value));
    output[n++] = c.buffers.vertices != nullptr;
    for (const auto &b : c.buffers.records) {
        const std::uint32_t fields[]{b.resource != nullptr, b.capacity_bytes, b.remaining_bytes,
            b.stride, b.vertex_format, b.render_arguments.words.data != nullptr, b.lock_count,
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(b.debug_name)), b.unused};
        for (auto value : fields) output[n++] = value;
    }
    for (auto value : c.scratch.words) output[n++] = value;
    output[n++] = static_cast<unsigned>(c.trace.size());
    for (const auto &row : c.trace)
        for (auto value : row) output[n++] = value;
    for (std::size_t index = 0; index < 9; ++index) {
        const auto &arguments = c.arguments[index];
        const auto size = arguments.empty() ? 0U : counts[index] * 4;
        output[n++] = size;
        for (unsigned word = 0; word < size; ++word) output[n++] = arguments[word];
    }
    return n;
}
