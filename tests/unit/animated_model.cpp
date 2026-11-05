#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

extern "C" std::uint32_t animated_model_probe(const std::uint32_t *, std::uint32_t *);
namespace {
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
}
int main() {
    constexpr std::uint32_t object = 0x11000400, model = 0x11000600;
    std::array<std::uint32_t, 512> input{};
    input[2] = input[3] = 1;
    input[4] = bits(2);
    input[5] = 3;
    input[6] = 2;
    input[9] = bits(3.5f);
    input[10] = 37;
    for (unsigned node = 0; node < 4; ++node) {
        auto *row = input.data() + 16 + node * 13;
        row[0] = node == 0 ? 16 : 0; // animated traversal ignores static Break
        row[1] = model + node * 64;
        row[8] = row[9] = row[10] = bits(1);
        row[11] = node == 0 ? object + 64 : 0;
        row[12] = node == 0 ? object + 3 * 64 : node == 1 ? object + 2 * 64 : 0;
        for (unsigned channel = 0; channel < 2; ++channel) {
            auto *keys = input.data() + 68 + (node * 2 + channel) * 17;
            keys[0] = 2;
            keys[1] = 0;
            keys[5] = 4;
            keys[6] = channel == 0 ? bits(8) : 8192;
        }
    }
    for (unsigned wrapper = 0; wrapper < 2; ++wrapper) {
        input[11] = wrapper;
        std::array<std::uint32_t, 2048> output{};
        const auto count = animated_model_probe(input.data(), output.data());
        assert(count > 62 && count < output.size());
        assert(output[0] == 0x403470);
        assert(output[1] == (wrapper ? 0 : 37));
        assert(output[2] == 0 && output[3] == bits(2));
        assert(output[4] == 4 && output[5] == 2 && output[8] == 0);
        std::vector<std::uint32_t> consumed;
        unsigned depth = 0, pushes = 0, pops = 0;
        bool sampled_translation = false;
        for (unsigned at = 62; at < count; at += 6) {
            switch (output[at]) {
            case 2: ++depth; ++pushes; break;
            case 8: assert(depth != 0); --depth; ++pops; break;
            case 3: sampled_translation |= output[at + 1] == bits(4); break;
            case 7:
                consumed.push_back(output[at + 1]);
                assert(output[at + 2] == (wrapper ? 0 : 37));
                assert(output[at + 3] == 0 && output[at + 4] == 0x403470);
                break;
            }
        }
        assert((consumed == std::vector<std::uint32_t>{model, model + 64, model + 128, model + 192}));
        assert(pushes == 4 && pops == 4 && depth == 0 && sampled_translation);
    }
    std::cout << "Animated action samples consumed; four body models submitted in child/sibling order\n";
}
