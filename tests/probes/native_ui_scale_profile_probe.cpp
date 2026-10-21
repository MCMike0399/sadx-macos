#include "engine/ui_scale_profile.hpp"
#include <cstring>
#include <stdexcept>
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
} // namespace
extern "C" int sadx_ui_scale_scope(const std::uint32_t *v, std::uint32_t *out) {
    float h = value(v[3]), vertical = value(v[4]);
    sadx::ui_scale::State state(h, vertical);
    state.background_fill = static_cast<sadx::ui_scale::FillMode>(v[0]);
    state.enabled = v[1] != 0;
    state.movie_fill = static_cast<sadx::ui_scale::FillMode>(v[9]);
    for (unsigned i = 0; i < v[2]; ++i)
        state.entries.push_back({54, 1, 1, true});
    unsigned calls = 0;
    bool completed = false;
    struct Stop {
        int unused = 0;
    };
    std::function<void(unsigned)> invoke;
    invoke = [&](unsigned depth) {
        const auto body = [&] {
            const unsigned row = 16 + calls++ * 6;
            out[row] = state.enabled;
            out[row + 1] = static_cast<unsigned>(state.entries.size());
            out[row + 2] = bits(h);
            out[row + 3] = bits(vertical);
            out[row + 4] = state.entries.back().alignment;
            out[row + 5] = state.entries.back().background;
            if (calls == v[6])
                throw Stop{};
            if (depth > 1)
                invoke(depth - 1);
            else if (v[7])
                state.entries.clear();
        };
        if (v[8])
            sadx::ui_scale::run_movie_scope(state, body);
        else
            sadx::ui_scale::run_background_scope(state, body);
    };
    try {
        invoke(v[5]);
        completed = true;
    } catch (Stop) {
    }
    out[0] = completed;
    out[1] = calls;
    out[2] = state.enabled;
    out[3] = static_cast<unsigned>(state.entries.size());
    out[4] = bits(h);
    out[5] = bits(vertical);
    out[6] = sadx::ui_scale::scaling_enabled(state);
    out[7] = static_cast<unsigned>(state.background_fill);
    return 1;
}
extern "C" int sadx_ui_scale_forwarding(const std::uint32_t *v, std::uint32_t *out) {
    float h = 1, vertical = 1;
    sadx::ui_scale::State state(h, vertical);
    state.enabled = v[0] != 0;
    state.background_fill = static_cast<sadx::ui_scale::FillMode>(v[1]);
    for (unsigned i = 0; i < v[2]; ++i)
        state.entries.push_back({54, 1, 1, v[3] != 0});
    try {
        sadx::ui_scale::require_message_forwarding(state);
        out[0] = 1;
    } catch (const std::runtime_error &) {
        out[0] = 0;
    }
    for (unsigned i = 0; i < 5; ++i)
        out[i + 1] = v[i + 4];
    return 1;
}
