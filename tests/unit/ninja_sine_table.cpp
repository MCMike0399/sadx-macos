#include "math/ninja_sine_table.hpp"
#include "math/ninja_math.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
std::uint32_t bits(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    auto table = sadx::ninja_sine_table();
    sadx::X87Context fp(0x023f);
    const sadx::NinjaMath math(table, fp);
    require(bits(math.sin(0)) == 0 && bits(math.sin(16384)) == bits(1.0F), "positive endpoints");
    require(bits(math.sin(32768)) == 0x80000000U && bits(math.cos(16384)) == 0x80000000U,
            "source signed zeros");
    require(bits(math.sin(49152)) == bits(-1.0F), "negative endpoint");
    for (std::int32_t angle = 0; angle < 65536; ++angle) {
        require(bits(math.sin(angle)) == bits(math.sin(angle - 65536)), "sine signed wrap");
        require(bits(math.cos(angle)) == bits(math.sin(angle + 16384)), "cosine phase");
    }
    require(bits(math.sin(std::numeric_limits<std::int32_t>::min())) == bits(math.sin(0)),
            "minimum signed input wraps");
    require(bits(math.cos(std::numeric_limits<std::int32_t>::max())) == bits(math.cos(65535)),
            "maximum signed input wraps");
    table[0] = 7.0F;
    require(bits(math.sin(0)) == 0 && sadx::ninja_sine_table()[0] == 0,
            "math owners and factory results retain independent values");
    std::cout
        << "Native sine table: full-angle wrap/phase, signed zeros and copy ownership passed.\n";
}
