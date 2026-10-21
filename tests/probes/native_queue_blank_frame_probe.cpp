#include "engine/queue_blank_frame.hpp"
#include "engine/queue_consumer_native.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
float value(std::uint32_t b) {
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}
std::uint32_t bits(float f) {
    std::uint32_t b;
    std::memcpy(&b, &f, 4);
    return b;
}
struct Stop {};
struct Calls final : sadx::pvr::Calls {
    std::function<void(std::array<std::uint32_t, 16>)> emit;
    sadx::pvr::TextureInfo texture(std::uint32_t) override { throw Stop{}; }
    void draw_texture(const std::array<sadx::pvr::Vertex, 4> &, std::uint32_t,
                      std::uint32_t) override {
        throw Stop{};
    }
    void draw_polygon(const std::array<sadx::pvr::Point, 4> &points, std::uint32_t color,
                      float depth, std::uint32_t flags) override {
        std::array<std::uint32_t, 16> row{};
        row[0] = 3;
        for (unsigned n = 0; n < 4; ++n) {
            row[1 + n * 2] = bits(points[n].x);
            row[2 + n * 2] = bits(points[n].y);
        }
        row[9] = color;
        row[10] = bits(depth);
        row[11] = flags;
        emit(row);
    }
    void queue_box(float l, float t, float r, float b, float z, std::uint32_t c) override {
        emit({2, bits(l), bits(t), bits(r), bits(b), bits(z), c, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    }
};
} // namespace
extern "C" int sadx_native_queue_blank_frame(const std::uint32_t *v, std::uint32_t *out) {
    using namespace sadx;
    queue::Storage storage;
    queue::State state;
    state.values.execution_mode = static_cast<std::int32_t>(v[16]);
    std::int32_t disabled = static_cast<std::int32_t>(v[1]);
    std::uint8_t special = static_cast<std::uint8_t>(v[2]), fog = static_cast<std::uint8_t>(v[3]);
    float h = value(v[4]), vertical = value(v[5]), offset = value(v[6]);
    std::uint32_t restore = v[7];
    std::array<float, 4> white{1, 1, 1, 1};
    queue::Owners owners{storage, state, white, disabled};
    std::vector<std::array<std::uint32_t, 16>> trace;
    const auto emit = [&](std::array<std::uint32_t, 16> row) {
        row[12] = special;
        row[13] = bits(offset);
        trace.push_back(row);
        if (trace.size() == v[8])
            throw Stop{};
        if (row[0] == 1 && row[1] == 7 && (v[9] & 1)) {
            h = value(v[10]);
            vertical = value(v[11]);
        }
        if ((row[0] == 2 || row[0] == 3) && (v[9] & 2))
            restore = v[12];
        if (row[0] == 2 && (v[9] & 4)) {
            disabled = static_cast<std::int32_t>(v[13]);
            special = static_cast<std::uint8_t>(v[14]);
        }
    };
    Calls calls;
    calls.emit = emit;
    queue::Services services{[&](std::uint32_t mode) {
                                 emit({1, mode, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
                                 return true;
                             },
                             [](bool) {
                                 throw Stop{};
                                 return false;
                             },
                             [](bool) {
                                 throw Stop{};
                                 return false;
                             },
                             [&](const std::array<float, 4> &color) {
                                 emit({4, bits(color[0]), bits(color[1]), bits(color[2]),
                                       bits(color[3]), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
                             },
                             {}};
    std::uint32_t control = 0, mask = 0, attributes = 0;
    const texture_lifetime::List *list = nullptr;
    queue::ConsumerView view{owners, control, mask, attributes, special, fog, list};
    X87Context fp(static_cast<std::uint16_t>(v[0]));
    bool completed = false;
    try {
        queue::ConsumerServices consumer{
            services, {}, {}, {}, [&] {
                queue::draw_blank_frame(
                    {h, vertical, disabled, special, fog, offset}, calls,
                    [&] { emit({1, restore, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}); }, fp);
            }};
        completed = queue::consume(view, consumer, fp).completed;
    } catch (Stop) {
    }
    out[0] = completed;
    out[1] = static_cast<std::uint32_t>(state.values.execution_mode);
    out[2] = special;
    out[3] = bits(offset);
    out[4] = static_cast<std::uint32_t>(disabled);
    out[5] = restore;
    out[6] = static_cast<std::uint32_t>(trace.size());
    out[7] = fp.exception_flags() & 63;
    out[8] = bits(h);
    out[9] = bits(vertical);
    for (unsigned n = 0; n < trace.size(); ++n)
        std::copy(trace[n].begin(), trace[n].end(), out + 16 + n * 16);
    return 1;
}
