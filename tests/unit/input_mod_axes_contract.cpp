// Before-source native contracts: unmasked faults, retained destinations and aliases.
// These characterize the maintained SSE model, not unobserved Intel hardware.
#include "input/input_mod_axes.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {
using namespace sadx;
std::uint64_t digest = 14695981039346656037ULL;
std::uint64_t bytes = 0;
bool capture = false;
void word(std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        const auto byte = static_cast<unsigned char>(value >> shift);
        digest = (digest ^ byte) * 1099511628211ULL;
        ++bytes;
        if (capture) std::cout.put(static_cast<char>(byte));
    }
}
void output(const InputModAxesOutput &value) {
    word(value.x.has_value()); word(value.x ? static_cast<std::uint32_t>(*value.x) : 0);
    word(value.y.has_value()); word(value.y ? static_cast<std::uint32_t>(*value.y) : 0);
    word(value.magnitude.has_value()); word(value.magnitude ? value.magnitude->bits : 0);
}
std::uint32_t next(std::uint32_t &state) {
    state = state * 1664525U + 1013904223U;
    return state;
}
std::int16_t signed_word(std::uint32_t value) {
    const auto low = value & 65535U;
    return static_cast<std::int16_t>(low < 32768U ? static_cast<std::int32_t>(low)
                                                : static_cast<std::int32_t>(low) - 65536);
}
InputModAxesOutput seeded(bool populated) {
    if (!populated) return {};
    return {std::int16_t{123}, std::int16_t{-234}, Sse32{0x12345678U}};
}
void run_case(unsigned id, unsigned kind, InputModRawAxes left, InputModRawAxes right,
              std::int16_t deadzone, const InputModAxesConstants &constants,
              std::uint32_t control, bool populated) {
    InputModAxesOutput a = seeded(populated), b = seeded(populated);
    SseContext arithmetic(control);
    std::uint32_t failure = 0, exceptions = 0;
    try {
        if (kind < 2)
            convert_input_mod_261_axes(left, deadzone, kind == 1, constants, arithmetic, a);
        else
            normalize_input_mod_261_keyboard(left, right, constants, arithmetic, a, kind == 3 ? a : b);
    } catch (const SseError &error) {
        failure = 1U + static_cast<unsigned>(error.failure());
        exceptions = error.exceptions();
    }
    word(id); word(kind); word(control); word(populated);
    word(failure); word(exceptions); word(arithmetic.mxcsr());
    output(a); output(b);
}
void cross_thread() {
    const InputModAxesConstants constants{{0x34000000}, {0x46fffe00}, {0x42fe0000},
                                         {0x43000000}, {0x3f800000}};
    for (bool active : {false, true}) {
        SseContext arithmetic(0x1f80);
        auto destination = seeded(true);
        unsigned failure = 0;
        std::thread other([&] {
            try {
                convert_input_mod_261_axes({static_cast<std::int16_t>(active ? 30000 : 0), 0},
                    7849, true, constants, arithmetic, destination);
            } catch (const SseError &error) {
                failure = 1U + static_cast<unsigned>(error.failure());
            }
        });
        other.join();
        word(active); word(failure); word(arithmetic.mxcsr()); output(destination);
    }
}
} // namespace
int main(int argc, char **argv) {
    capture = argc == 2 && std::string(argv[1]) == "--capture";
    const std::array<InputModAxesConstants, 5> coefficients{{
        {{0x34000000}, {0x46fffe00}, {0x42fe0000}, {0x43000000}, {0x3f800000}},
        {{0}, {0x46fffe00}, {0x42fe0000}, {0x43000000}, {0x3f800000}},
        {{0x34000000}, {0x7fc12345}, {0x42fe0000}, {0x43000000}, {0x3f800000}},
        {{0x34000000}, {0x46fffe00}, {0x7f7fffff}, {0x7f800001}, {0x3f800000}},
        {{0x34000000}, {0x46fffe00}, {0xff800000}, {0x7f800000}, {0x80000000}}
    }};
    const std::array<std::int16_t, 8> edge{{-32768, -32767, -7849, -1, 0, 1, 7849, 32767}};
    const std::array<std::int16_t, 6> deadzones{{-32768, -1, 0, 7849, 8689, 32767}};
    std::uint32_t random = 314159265;
    unsigned id = 0;
    for (unsigned sample = 0; sample < 128; ++sample) {
        const InputModRawAxes left{
            sample < 64 ? edge[sample / 8] : signed_word(next(random)),
            sample < 64 ? edge[sample % 8] : signed_word(next(random))};
        const InputModRawAxes right{signed_word(next(random)), signed_word(next(random))};
        for (unsigned rounding = 0; rounding < 4; ++rounding)
            for (unsigned denormal = 0; denormal < 2; ++denormal)
                for (unsigned flush = 0; flush < 2; ++flush)
                    for (unsigned unmasked = 0; unmasked < 8; ++unmasked) {
                        auto control = 0x1f80U | (rounding << 13) | (denormal << 6) | (flush << 15);
                        if (unmasked < 6) control &= ~(1U << (7 + unmasked));
                        if (unmasked == 6) control &= ~0x1f80U;
                        if (sample & 1) control |= 63U; // Existing sticky flags do not retroactively trap.
                        for (const auto &constants : coefficients)
                            for (unsigned kind = 0; kind < 4; ++kind)
                                run_case(id++, kind, left, right, deadzones[sample % deadzones.size()],
                                         constants, control, (sample & 2) != 0);
                    }
    }
    cross_thread();
    if (bytes != 24903752 || digest != 17167205880272457936ULL) {
        std::cerr << "Native axis partial-store/alias contract changed\n";
        return 1;
    }
    if (!capture) std::cout << id << ' ' << bytes << ' ' << digest << '\n';
}
