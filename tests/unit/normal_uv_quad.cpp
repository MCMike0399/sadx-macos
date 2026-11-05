#include "normal_uv_strip_probe.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

extern "C" unsigned sadx_native_normal_uv_quad_probe(const std::uint32_t *, std::uint32_t *);
namespace {
std::uint32_t bits(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
void verify(bool triangles) {
    std::vector<std::uint32_t> input(kStripProbeInputWords);
    input[1] = 1;
    input[2] = 36;
    input[3] = input[4] = 8192;
    input[6] = 338;
    input[7] = 1;
    input[8] = 0xff556677;
    input[9] = 127;
    input[11] = 4096;
    input[12] = triangles ? 3 : 4;
    input[13] = triangles ? 6 : 8;
    input[15] = input[16] = 12;
    input[17] = 512;
    input[26] = triangles;
    for (unsigned corner = 0; corner < 4; ++corner) {
        input[kMeshInput + corner] = corner;
        input[kUvInput + corner * 2] = corner * 20;
        input[kUvInput + corner * 2 + 1] = corner * 30;
        input[kPointsInput + corner * 3] = bits(float(corner + 1));
        input[kNormalsInput + corner * 3 + 1] = bits(1);
    }
    std::vector<std::uint32_t> output(kStripProbeOutputWords);
    const auto size = sadx_native_normal_uv_quad_probe(input.data(), output.data());
    assert(size < output.size());
    std::vector<std::uint32_t> selected;
    bool drew_list = false;
    for (unsigned index = 0; index < output[11]; ++index) {
        const auto *event = output.data() + 12 + index * 6;
        if (event[0] == 3 && event[1] % 36 == 0)
            selected.push_back(event[2]);
        if (event[0] == 4 && event[1] == 6) {
            assert(event[2] == 4 && event[3] == 0 && event[4] == (triangles ? 1 : 2));
            drew_list = true;
        }
    }
    const std::vector<std::uint32_t> expected =
        triangles
            ? std::vector<std::uint32_t>{bits(1), bits(2), bits(3)}
            : std::vector<std::uint32_t>{bits(1), bits(2), bits(3), bits(2), bits(3), bits(4)};
    assert(selected == expected && drew_list);
    assert(output[2] == 8192 - expected.size() * 36 && output[6] == 0);
}
} // namespace
int main() {
    verify(false);
    verify(true);
    std::cout
        << "Normal/UV source quad winding, triangle list and submitted vertex ranges checked\n";
}
