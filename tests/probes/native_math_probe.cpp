// Native half of the original-instruction comparison. This library is a test
// artifact; neither Unicorn nor x86 execution is part of the game's runtime.
#include "ninja/ninja_math.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int sadx_native_trig(const unsigned char* bytes, std::size_t size,
                                 std::uint32_t* results, std::size_t count,
                                 std::uint16_t control_word) {
    if (!bytes || size != 4100 || !results || count != 65536 * 2) return 1;
    sadx::NinjaMath::SineTable table{};
    for (std::size_t i=0; i<table.size(); ++i) {
        const std::uint32_t word = static_cast<std::uint32_t>(bytes[i*4]) |
            (static_cast<std::uint32_t>(bytes[i*4+1]) << 8) |
            (static_cast<std::uint32_t>(bytes[i*4+2]) << 16) |
            (static_cast<std::uint32_t>(bytes[i*4+3]) << 24);
        std::memcpy(&table[i], &word, 4);
    }
    sadx::X87Context context(control_word);
    const sadx::NinjaMath math(table, context);
    for (std::int32_t i=0; i<65536; ++i) {
        const float sine=math.sin(i), cosine=math.cos(i);
        std::memcpy(&results[i*2], &sine, 4);
        std::memcpy(&results[i*2+1], &cosine, 4);
    }
    return 0;
}

static std::uint32_t float_bits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits,&value,4);
    return bits;
}
extern "C" std::uint32_t sadx_native_dot(const sadx::NinjaVector* first,
                                  const sadx::NinjaVector* second,
                                  std::uint16_t control_word) {
    sadx::X87Context context(control_word);
    return float_bits(sadx::inner_product(*first,*second,context));
}
extern "C" std::uint32_t sadx_native_scalar_squared(const sadx::NinjaVector* vector,
                                                    std::uint16_t control_word) {
    sadx::X87Context context(control_word);
    return float_bits(sadx::scalar_squared(*vector,context));
}
extern "C" std::uint32_t sadx_native_signed_sqrt(float value, std::uint16_t control_word) {
    sadx::X87Context context(control_word);
    return float_bits(sadx::signed_sqrt(value,context));
}
