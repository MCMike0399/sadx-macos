#include "cpu/x87_transcendental.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
float from_bits(std::uint32_t bits) {
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
using sadx::X87Transcendental;

void axes_and_rounding() {
    for (const std::uint16_t precision : {0x007f, 0x027f, 0x037f}) {
        for (unsigned rounding = 0; rounding < 4; ++rounding) {
            const auto cw = static_cast<std::uint16_t>(precision | (rounding << 10U));
            sadx::X87Context fp(cw);
            auto output = X87Transcendental::fpatan_bochs_p5_p6(fp, fp.from_float(1), fp.from_float(0));
            require(output.sign_exponent() == 0x3fff && output.significand() ==
                (rounding == 0 || rounding == 2 ? 0xc90fdaa22168c235ULL : 0xc90fdaa22168c234ULL),
                "positive axis constant must use RC and full FPATAN precision independently of PC");
            require(fp.control_word() == cw && fp.exception_flags() == 0x20,
                "positive axis must preserve CW and record precision flag");
            fp.clear_exceptions();
            output = X87Transcendental::fpatan_bochs_p5_p6(fp, fp.from_float(-127), fp.from_float(0));
            require(output.sign_exponent() == 0xbfff && output.significand() ==
                (rounding == 0 || rounding == 1 ? 0xc90fdaa22168c235ULL : 0xc90fdaa22168c234ULL),
                "GetAnalog forced negative axis must round correctly within selected model");
            require(fp.exception_flags() == 0x20 && fp.control_word() == cw,
                "negative axis must preserve controls and record precision");
        }
    }
}

void zero_infinity_nan() {
    sadx::X87Context fp(0x037f);
    const auto infinity = std::numeric_limits<float>::infinity();
    for (float y : {0.0F, -0.0F}) {
        for (float x : {0.0F, -0.0F, infinity, -infinity}) {
            const auto input_y = fp.from_float(y);
            const auto input_x = fp.from_float(x);
            fp.clear_exceptions();
            const auto output = X87Transcendental::fpatan_bochs_p5_p6(fp, input_y, input_x);
            require(output.negative() == input_y.negative(), "signed-zero atan preserves numerator sign");
            require(output.significand() == (input_x.negative() ? 0xc90fdaa22168c235ULL : 0),
                    "signed-zero denominator selects pi or zero");
            require((output.sign_exponent() & 0x7fffU) == (input_x.negative() ? 0x4000 : 0),
                    "signed-zero denominator exponent");
            require(fp.exception_flags() == (input_x.negative() ? 0x20 : 0), "axis exception flags");
        }
    }
    const auto one = fp.from_float(1);
    for (const float y : {1.0F, -1.0F, infinity, -infinity}) {
        for (const float x : {1.0F, -1.0F, infinity, -infinity}) {
            fp.clear_exceptions();
            const auto output = X87Transcendental::fpatan_bochs_p5_p6(fp, fp.from_float(y), fp.from_float(x));
            require(output.negative() == (y < 0), "quadrant must retain numerator sign");
            if ((y == infinity || y == -infinity) == (x == infinity || x == -infinity)) {
                require((output.sign_exponent() & 0x7fffU) == (x < 0 ? 0x4000 : 0x3ffe) &&
                        output.significand() == (x < 0 ? 0x96cbe3f9990e91a8ULL : 0xc90fdaa22168c235ULL),
                        "equal finite/infinite quadrants select pi/4 or 3pi/4");
            } else if (y == infinity || y == -infinity) {
                require((output.sign_exponent() & 0x7fffU) == 0x3fff &&
                        output.significand() == 0xc90fdaa22168c235ULL, "infinite numerator selects pi/2");
            } else {
                require((output.sign_exponent() & 0x7fffU) == (x < 0 ? 0x4000 : 0) &&
                        output.significand() == (x < 0 ? 0xc90fdaa22168c235ULL : 0),
                        "infinite denominator selects pi or signed zero");
            }
        }
    }
    const auto nan = fp.from_float(from_bits(0x7fc12345));
    for (bool nan_y : {false, true}) {
        fp.clear_exceptions();
        const auto output = X87Transcendental::fpatan_bochs_p5_p6(fp, nan_y ? nan : one, nan_y ? one : nan);
        require(output.sign_exponent() == nan.sign_exponent() && output.significand() == nan.significand(),
                "quiet NaN payload must be preserved by selected math model");
        require(fp.exception_flags() == 0, "quiet FPATAN NaN must not be signaling FCOM");
    }
    const auto tiny = fp.from_float(std::numeric_limits<float>::denorm_min());
    fp.clear_exceptions(); // FLD f32 already raised denormal; ext80 value is normal.
    const auto angle = X87Transcendental::fpatan_bochs_p5_p6(fp, tiny, one);
    require(angle.significand() == tiny.significand() && angle.sign_exponent() == tiny.sign_exponent(),
            "tiny ratio must retain ext80 range and significand");
    require(fp.exception_flags() == 0x20, "finite nonzero FPATAN records inexact");

    auto ext80_tiny = fp.from_double(0x1p-1022);
    const auto step = ext80_tiny;
    for (int i = 1; i < 16; ++i) ext80_tiny = fp.multiply(ext80_tiny, step);
    ext80_tiny = fp.multiply(ext80_tiny, fp.from_double(0x1p-31)); // Exact ext80 subnormal 2^-16383.
    require(ext80_tiny.sign_exponent() == 0 && ext80_tiny.significand() != 0,
            "test must exercise actual ext80 subnormal, not an f32 denormal widened to normal");
    fp.clear_exceptions();
    const auto subnormal_angle = X87Transcendental::fpatan_bochs_p5_p6(fp, ext80_tiny, one);
    require(subnormal_angle.sign_exponent() == 0 && subnormal_angle.significand() == ext80_tiny.significand(),
            "ext80 tiny-ratio path preserves subnormal significand");
    require((fp.exception_flags() & 0x22U) == 0x22U,
            "FPATAN ext80 denormal input and finite inexact flags retained");
}

void ownership_and_unmasked() {
    sadx::X87Context fp(0x035f); // Precision exception unmasked.
    const auto y = fp.from_float(1), x = fp.from_float(1);
    bool stopped = false;
    try { (void)X87Transcendental::fpatan_bochs_p5_p6(fp, y, x); }
    catch (const sadx::X87Error& e) {
        stopped = e.failure() == sadx::X87Failure::unmasked_exception && e.exceptions() == 0x20;
    }
    require(stopped && fp.exception_flags() == 0x20 && fp.control_word() == 0x035f,
            "unmasked FPATAN precision must retain flags and controls");
    stopped = false;
    try { (void)X87Transcendental::fpatan_bochs_p5_p6(fp, y, x); }
    catch (const sadx::X87Error& e) { stopped = e.failure() == sadx::X87Failure::unmasked_exception; }
    require(stopped, "pending unmasked exception must stop subsequent FPATAN");
    fp.clear_exceptions();
    std::exception_ptr failure;
    std::thread worker([&] {
        try {
            bool rejected = false;
            try { (void)X87Transcendental::fpatan_bochs_p5_p6(fp, y, x); }
            catch (const sadx::X87Error& e) { rejected = e.failure() == sadx::X87Failure::cross_thread_context; }
            require(rejected, "FPATAN must reject a foreign-thread context");
        } catch (...) { failure = std::current_exception(); }
    });
    worker.join();
    if (failure) std::rethrow_exception(failure);
}

void general_finite_coverage() {
    sadx::X87Context fp(0x037f);
    const auto one = fp.from_float(1);
    auto previous = X87Transcendental::fpatan_bochs_p5_p6(fp, fp.from_float(0), one);
    // Exercise every table interval including the k<=1 high-order branch.
    for (int index = 1; index <= 8192; ++index) {
        const auto y = fp.from_float(static_cast<float>(index) / 8192.0F);
        const auto value = X87Transcendental::fpatan_bochs_p5_p6(fp, y, one);
        require(fp.compare(previous, value) == sadx::X87Comparison::less,
                "FPATAN table boundaries must preserve increasing angles");
        const auto negative = X87Transcendental::fpatan_bochs_p5_p6(fp, y.negated(), one);
        require(value.negated().sign_exponent() == negative.sign_exponent() &&
                value.significand() == negative.significand(), "nearest finite odd symmetry");
        previous = value;
    }
    require(previous.sign_exponent() == 0x3ffe && previous.significand() == 0xc90fdaa22168c235ULL,
            "pi/4 reconstruction of equal positive operands");
}
} // namespace

int main() {
    try {
        axes_and_rounding();
        zero_infinity_nan();
        ownership_and_unmasked();
        general_finite_coverage();
        std::cout << "Native Bochs P5/P6 FPATAN candidate: RC/PC independence, axes, signed zero, "
                     "NaN payload, exception ownership and table intervals passed.\n"
                     "Intel Ultra hardware equivalence remains unverified.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
