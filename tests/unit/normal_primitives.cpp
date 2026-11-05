#include "normal_uv_strip_probe.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

extern "C" unsigned sadx_native_normal_primitives_probe(const std::uint32_t *, std::uint32_t *);
namespace {
std::uint32_t bits(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
void verify(unsigned primitive) {
    std::vector<std::uint32_t> input(kStripProbeInputWords);
    input[1] = 1;
    input[2] = 28;
    input[3] = input[4] = 8192;
    input[6] = 82;
    input[7] = 1;
    input[8] = 0xff556677;
    input[9] = 127;
    input[11] = 4096;
    input[12] = 4;
    input[15] = input[16] = 12;
    input[17] = 512;
    input[26] = primitive;
    for (unsigned corner = 0; corner < 4; ++corner) {
        input[kMeshInput + corner] = corner;
        input[kPointsInput + corner * 3] = bits(float(corner + 1));
        input[kNormalsInput + corner * 3 + 1] = bits(1);
    }
    if (primitive == 0) {
        input[kMeshInput] = 3;
        input[kMeshInput + 1] = 0;
        input[kMeshInput + 2] = 1;
        input[kMeshInput + 3] = 2;
    }
    std::vector<std::uint32_t> output(kStripProbeOutputWords);
    const auto size = sadx_native_normal_primitives_probe(input.data(), output.data());
    assert(size < output.size());
    std::vector<std::uint32_t> selected;
    bool drew = false;
    for (unsigned index = 0; index < output[11]; ++index) {
        const auto *event = output.data() + 12 + index * 6;
        if (event[0] == 3 && event[1] % 28 == 0)
            selected.push_back(event[2]);
        if (event[0] == 4 && event[1] == 6) {
            assert(event[2] == (primitive == 0 ? 5 : 4));
            assert(event[3] == 0 && event[4] == (primitive == 0 ? 3 : primitive == 1 ? 1 : 2));
            drew = true;
        }
    }
    const std::vector<std::uint32_t> expected =
        primitive == 0 ? std::vector<std::uint32_t>{bits(1), bits(1), bits(2), bits(3), bits(3)}
        : primitive == 1
            ? std::vector<std::uint32_t>{bits(1), bits(2), bits(3)}
            : std::vector<std::uint32_t>{bits(1), bits(2), bits(3), bits(2), bits(3), bits(4)};
    assert(selected == expected && drew);
    assert(output[2] == 8192 - expected.size() * 28 && output[6] == 0);
}
} // namespace
int main() {
    for (unsigned primitive = 0; primitive < 3; ++primitive)
        verify(primitive);
    std::cout << "Normal-only strip degenerates, quad winding and triangle-list ranges checked\n";
}
