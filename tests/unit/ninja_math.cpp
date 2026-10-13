#include "ninja/ninja_math.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

sadx::NinjaMath::SineTable read_table(const char* path) {
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "cannot open extracted table");
    std::array<unsigned char, 4100> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    require(input.gcount() == static_cast<std::streamsize>(bytes.size()),
            "table must contain exactly 4100 bytes");
    require(input.peek() == std::char_traits<char>::eof(), "table has trailing bytes");
    std::uint64_t digest = 0xcbf29ce484222325ULL;
    for (const auto byte : bytes) {
        digest = (digest ^ byte) * 0x100000001b3ULL;
    }
    // Binary table provenance is additionally bound by SHA-256 in report.md.
    require(digest == 0xb047ecf880b993c2ULL, "supplied sonic.exe table digest differs");
    sadx::NinjaMath::SineTable table{};
    for (std::size_t i = 0; i < table.size(); ++i) {
        const std::uint32_t word = static_cast<std::uint32_t>(bytes[i * 4]) |
            (static_cast<std::uint32_t>(bytes[i * 4 + 1]) << 8U) |
            (static_cast<std::uint32_t>(bytes[i * 4 + 2]) << 16U) |
            (static_cast<std::uint32_t>(bytes[i * 4 + 3]) << 24U);
        std::memcpy(&table[i], &word, sizeof(word));
    }
    return table;
}

void test_trig(const sadx::NinjaMath::SineTable& table) {
    sadx::X87Context context(0x027f);
    const sadx::NinjaMath math(table, context);
    require(bits(math.sin(0)) == 0, "sine zero");
    require(bits(math.sin(16384)) == bits(1.0F), "sine positive peak");
    require(bits(math.sin(32768)) == 0x80000000U, "sine half-turn negative zero");
    require(bits(math.sin(49152)) == bits(-1.0F), "sine negative peak");
    require(bits(math.cos(0)) == bits(1.0F), "cosine positive peak");
    require(bits(math.cos(16384)) == 0x80000000U, "cosine quarter-turn negative zero");
    require(bits(math.cos(32768)) == bits(-1.0F), "cosine negative peak");
    require(bits(math.cos(49152)) == 0, "cosine three-quarter-turn positive zero");
    for (std::size_t i = 0; i < table.size(); ++i) {
        require(bits(math.sin(static_cast<std::int32_t>(i * 16))) == bits(table[i]),
                "exact first-quadrant table knot");
    }
    constexpr double pi = 3.141592653589793238462643383279502884;
    for (std::int32_t angle = 0; angle < 65536; ++angle) {
        const float sine = math.sin(angle);
        const float cosine = math.cos(angle);
        require(bits(sine) == bits(math.sin(angle + 65536)), "positive sine wrap");
        require(bits(sine) == bits(math.sin(angle - 65536)), "negative sine wrap");
        require(bits(cosine) == bits(math.cos(angle + 65536)), "positive cosine wrap");
        require(bits(cosine) == bits(math.cos(angle - 65536)), "negative cosine wrap");
        require(bits(cosine) == bits(math.sin(angle + 16384)), "quarter-turn phase");
        require(sine == -math.sin(-angle), "sine odd symmetry");
        require(cosine == math.cos(-angle), "cosine even symmetry");
        require(sine == -math.sin(angle + 32768), "sine half-turn symmetry");
        require(sine == math.sin(32768 - angle), "sine reflection symmetry");
        // Independent analytic sanity check. Host trig is only a test oracle,
        // never a replacement for the recovered table/interpolation behavior.
        const double radians = static_cast<double>(angle) * (2.0 * pi / 65536.0);
        // This executable's table itself differs from analytic sine by up to
        // 5.31e-7 at its knots; interpolation raises the measured bound to
        // 7.54e-7. A regenerated host-sine table would change game behavior.
        require(std::abs(static_cast<double>(sine) - std::sin(radians)) < 1e-6,
                "sine table/interpolation analytic error");
        require(std::abs(static_cast<double>(cosine) - std::cos(radians)) < 1e-6,
                "cosine table/interpolation analytic error");
        const auto unsigned_angle = static_cast<std::uint32_t>(angle);
        const auto knot_angle = static_cast<std::int32_t>(unsigned_angle & ~15U);
        const double lower = math.sin(knot_angle);
        const double upper = math.sin(knot_angle + 16);
        const double fraction = static_cast<double>(unsigned_angle & 15U) / 16.0;
        const float chord = static_cast<float>((1.0 - fraction) * lower + fraction * upper);
        require(sine == chord, "all integer angles lie on original table chords");
    }
    for (const auto angle : {std::numeric_limits<std::int32_t>::min(),
                             std::numeric_limits<std::int32_t>::max()}) {
        const auto wrapped = static_cast<std::int32_t>(static_cast<std::uint32_t>(angle) & 65535U);
        require(bits(math.sin(angle)) == bits(math.sin(wrapped)), "extreme sine wrap");
        require(bits(math.cos(angle)) == bits(math.cos(wrapped)), "extreme cosine wrap");
    }
}

void test_vectors_and_root() {
    sadx::X87Context context(0x027f);
    require(sadx::inner_product({1, 2, 3}, {4, 5, 6}, context) == 32, "inner product");
    require(sadx::scalar_squared({1, 2, 3}, context) == 14, "scalar squared");
    require(sadx::inner_product({1, 2, 3}, {2, -1, 0}, context) == 0, "orthogonal vectors");
    // z+y cancels before x is added. Reversing the summation produces 0.
    const float large = 0x1p60F;
    require(sadx::inner_product({1, -large, large}, {1, 1, 1}, context) == 1,
            "recovered inner product summation order");
    require(sadx::scalar_squared({0, 0, 0}, context) == 0, "zero vector");
    require(sadx::signed_sqrt(9, context) == 3, "positive root");
    require(sadx::signed_sqrt(-9, context) == -3, "negative signed root");
    require(bits(sadx::signed_sqrt(-0.0F, context)) == 0x80000000U, "negative zero root");
    require(bits(sadx::signed_sqrt(0.0F, context)) == 0, "positive zero root");
    require(std::isnan(sadx::signed_sqrt(std::numeric_limits<float>::quiet_NaN(), context)),
            "unordered root");
    require(sadx::signed_sqrt(std::numeric_limits<float>::infinity(), context) ==
            std::numeric_limits<float>::infinity(), "positive infinite root");
    require(sadx::signed_sqrt(-std::numeric_limits<float>::infinity(), context) ==
            -std::numeric_limits<float>::infinity(), "negative infinite root");
    const float tiny = std::numeric_limits<float>::denorm_min();
    require(sadx::signed_sqrt(tiny, context) > 0, "subnormal input root");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "usage: ninja_math_test EXTRACTED_4100_BYTE_TABLE");
        require(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                "requires IEEE-754 binary32 float");
        test_trig(read_table(argv[1]));
        test_vectors_and_root();
        std::cout << "Ninja math: 65536 integer angles, exact table knots, wraps, "
                     "symmetries, vector order and signed roots passed.\n"
                     "Original full-game x87 control state/runtime equivalence remains unverified.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
