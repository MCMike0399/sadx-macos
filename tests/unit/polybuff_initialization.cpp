#include "engine/polybuff_initialization.hpp"
#include "game/library_initial_state.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>
extern "C" unsigned sadx_native_polybuff_initialization(const std::uint32_t *, std::uint32_t *);
namespace {
unsigned checks = 0;
void require(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
    ++checks;
}
}
int main() {
    namespace p = sadx::polybuff;
    std::array<std::uint32_t, 46> input{};
    input[4] = 0x6000;
    input[5] = 0x12345678;
    input[6] = 32;
    input[8] = 0x44444444;
    const sadx::LibraryConfigurationDefaults configuration{};
    for (unsigned i = 0; i < 9; ++i) input[10+i] = configuration.buffer_counts[i];
    std::vector<std::uint32_t> out(4000000);
    sadx_native_polybuff_initialization(input.data(), out.data());
    require(out[0] == 1 && out[1] == input[5] && out[2] == 1, "complete native initialization");
    for (unsigned i = 0; i < 9; ++i) {
        const auto base = 3 + i * 9;
        const auto &description = p::buffer_descriptions()[i];
        require(out[base] == 1 && out[base+1] == input[10+i] * description.stride &&
                out[base+2] == out[base+1] && out[base+3] == description.stride &&
                out[base+4] == description.vertex_format && out[base+5] == 1 &&
                out[base+6] == 0 && out[base+8] == 0, "all nine persistent records initialized");
    }
    require(out[3+8*9+1] == 32000*36, "configured library count reaches normal/UV resource");
    for (unsigned i = 0; i < 128; ++i) require(out[84+i] == 0, "scratch cleared after helpers");
    input[0] = 1; // scratch malloc receipt fails before its result/store.
    input[7] = 17;
    sadx_native_polybuff_initialization(input.data(), out.data());
    require(out[0] == 0 && out[1] == input[5] && out[2] == 0 && out[84] == input[8],
            "allocation failure preserves base store and uncleared scratch prefix");
    input[0] = 0;
    input[3] = 5; // fifth argument allocation returns null; first write fails.
    sadx_native_polybuff_initialization(input.data(), out.data());
    require(out[0] == 0 && out[3+4*9] == 1 && out[3+4*9+5] == 0 && out[3+5*9] == 0 &&
            out[84] == input[8], "partial fifth helper stops later records and final clear");
    std::printf("%u native checks pass\n", checks);
}
