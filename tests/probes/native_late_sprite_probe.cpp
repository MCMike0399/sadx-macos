#include "engine/message_sprite_queue.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
float value(std::uint32_t b) {
    float v;
    std::memcpy(&v, &b, 4);
    return v;
}
std::uint32_t bits(float v) {
    std::uint32_t b;
    std::memcpy(&b, &v, 4);
    return b;
}
struct Stop {};
} // namespace
extern "C" int sadx_native_late_sprite(const std::uint32_t *v, std::uint32_t *out) {
    using namespace sadx;
    queue::Storage storage;
    queue::State state;
    state.values.execution_mode = static_cast<std::int32_t>(v[2]);
    state.values.depth_comparison = static_cast<std::int32_t>(v[10]);
    std::array<float, 4> reset, secondary, device{};
    for (unsigned i = 0; i < 4; ++i) {
        reset[i] = value(v[12 + i]);
        state.current_material[i] = value(v[16 + i]);
        secondary[i] = value(v[20 + i]);
    }
    std::int32_t disabled = static_cast<std::int32_t>(v[1]);
    queue::Owners owners{storage, state, reset, disabled};
    texture_lifetime::Memory memory;
    texture_lifetime::List list;
    if (v[7] >= 2)
        list.entries.push_back(v[7] == 2 ? nullptr : &memory);
    list.first_filename_invalid = v[7] == 3;
    sprite::ScreenSprite sprite;
    sprite.textures = v[7] == 0 ? nullptr : &list;
    X87Context fp(static_cast<std::uint16_t>(v[0]));
    std::vector<std::array<std::uint32_t, 8>> trace;
    const auto event = [&](std::array<std::uint32_t, 8> row) {
        trace.push_back(row);
        if (trace.size() == v[9])
            throw Stop{};
        if (trace.size() == 1 && v[24])
            for (unsigned i = 0; i < 4; ++i)
                state.current_material[i] = value(v[25 + i]);
    };
    queue::Services base{[&](std::uint32_t mode) {
                             event({1, mode, 0, 0, 0, 0, 0, 0});
                             return true;
                         },
                         [&](bool write) {
                             event({2, write, 0, 0, 0, 0, 0, 0});
                             return true;
                         },
                         [](bool) { return true; },
                         [&](const std::array<float, 4> &color) {
                             std::array<std::uint32_t, 8> row{3, 0, 0, 0, 0, 0, 0, 0};
                             for (unsigned i = 0; i < 4; ++i)
                                 row[4 + i] = bits(color[i]);
                             event(row);
                             device = color;
                         },
                         [](std::uint32_t, std::uint32_t) { return true; }};
    queue::LateSpriteServices calls{
        base, [&]() -> const std::array<float, 4> & { return secondary; },
        base.set_constant_material,
        [&](const sprite::ScreenSprite &, std::int32_t frame, float priority,
            std::uint32_t attributes) {
            event({4, static_cast<std::uint32_t>(frame), bits(priority), attributes, 0, 0, 0, 0});
        },
        [&] {
            base.set_depth_comparison(static_cast<std::uint32_t>(state.values.depth_comparison));
        }};
    queue::LateResult result;
    bool failed = false;
    try {
        result = queue::draw_late_sprite(v[8] ? &sprite : nullptr, static_cast<std::int32_t>(v[5]),
                                         value(v[6]), v[4], v[3], {owners}, calls, fp);
    } catch (const Stop &) {
        failed = true;
    }
    std::fill_n(out, 128, 0);
    out[0] = failed ? 99 : static_cast<std::uint32_t>(result.route);
    out[1] = result.completed;
    out[2] = static_cast<std::uint32_t>(trace.size());
    out[3] = fp.exception_flags() & 63;
    for (unsigned i = 0; i < 4; ++i) {
        out[4 + i] = bits(state.current_material[i]);
        out[8 + i] = bits(device[i]);
    }
    for (std::size_t i = 0; i < trace.size(); ++i)
        std::copy(trace[i].begin(), trace[i].end(), out + 16 + i * 8);
    return 1;
}
