#include "cpu/x87_transcendental.hpp"

#include "../../third_party/bochs_fpatan/math.hpp"

namespace sadx {

X87Value X87Transcendental::fpatan_bochs_p5_p6(X87Context& context, X87Value y, X87Value x) {
    context.check_thread();
    context.check_pending();

    bochs_fpatan::softfloat_status_t status{};
    status.softfloat_roundingMode = static_cast<std::uint8_t>((context.control_word_ >> 10U) & 3U);
    status.softfloat_exceptionMasks = static_cast<std::uint8_t>(context.control_word_ & 0x3fU);
    status.extF80_roundingPrecision = 80; // FPATAN's architectural PC-independent result
    const bochs_fpatan::extFloat80_t input_x{x.significand(), x.sign_exponent()};
    const bochs_fpatan::extFloat80_t input_y{y.significand(), y.sign_exponent()};
    const auto result = bochs_fpatan::fpatan(input_x, input_y, status);
    context.record_exceptions(static_cast<std::uint16_t>(status.softfloat_exceptionFlags & 0x3fU));
    return X87Value(result.signExp, result.signif);
}

namespace {
bochs_fpatan::softfloat_status_t status_for(const std::uint16_t control_word) {
    bochs_fpatan::softfloat_status_t status{};
    status.softfloat_roundingMode = static_cast<std::uint8_t>((control_word >> 10U) & 3U);
    status.softfloat_exceptionMasks = static_cast<std::uint8_t>(control_word & 0x3fU);
    status.extF80_roundingPrecision = 80;
    return status;
}
}

std::optional<X87Transcendental::SineCosine> X87Transcendental::fsincos_bochs(X87Context& context, X87Value x) {
    context.check_thread();
    context.check_pending();
    auto status = status_for(context.control_word_);
    const bochs_fpatan::extFloat80_t input{x.significand(), x.sign_exponent()};
    bochs_fpatan::extFloat80_t sine{}, cosine{};
    const auto code = bochs_fpatan::fsincos(input, &sine, &cosine, status);
    context.record_exceptions(static_cast<std::uint16_t>(status.softfloat_exceptionFlags & 0x3fU));
    if (code < 0) return std::nullopt;
    return SineCosine{X87Value(sine.signExp, sine.signif), X87Value(cosine.signExp, cosine.signif)};
}

std::optional<X87Value> X87Transcendental::fptan_bochs(X87Context& context, X87Value x) {
    context.check_thread();
    context.check_pending();
    auto status = status_for(context.control_word_);
    bochs_fpatan::extFloat80_t value{x.significand(), x.sign_exponent()};
    const auto code = bochs_fpatan::fptan(value, status);
    context.record_exceptions(static_cast<std::uint16_t>(status.softfloat_exceptionFlags & 0x3fU));
    if (code < 0) return std::nullopt;
    return X87Value(value.signExp, value.signif);
}

} // namespace sadx
