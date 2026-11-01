#include "cpu/sse_math.hpp"

extern "C" {
#include "../../third_party/softfloat/platform.h"
#include "../../third_party/softfloat/source/include/softfloat.h"
}

namespace sadx {
namespace {

constexpr std::uint8_t kInvalid = 1;
constexpr std::uint8_t kDenormalOperand = 2;
constexpr std::uint8_t kDivideByZero = 4;
constexpr std::uint8_t kOverflow = 8;
constexpr std::uint8_t kUnderflow = 16;
constexpr std::uint8_t kPrecision = 32;
constexpr std::uint32_t kExceptionFlags = 63;
constexpr std::uint32_t kDenormalsAreZero = 0x40;
constexpr std::uint32_t kDenormalMask = 0x100;
constexpr std::uint32_t kOverflowMask = 0x400;
constexpr std::uint32_t kUnderflowMask = 0x800;
constexpr std::uint32_t kFlushToZero = 0x8000;

enum class BinaryOperation : unsigned { Add, Subtract, Multiply, Divide };

// NOTSA: classify raw encodings without host floating-point state or NaN changes.
bool is_nan32(std::uint32_t bits) {
    return (bits & 0x7fffffffU) > 0x7f800000U;
}

// NOTSA: binary64 counterpart of the raw binary32 classification.
bool is_nan64(std::uint64_t bits) {
    return (bits & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL;
}

// NOTSA: UCOMISD distinguishes signaling NaNs from quiet NaNs.
bool is_signaling_nan64(std::uint64_t bits) {
    return is_nan64(bits) && !(bits & 0x8000000000000ULL);
}

// NOTSA: a zero exponent with nonzero fraction is a denormal operand/result.
bool is_denormal32(std::uint32_t bits) {
    return !(bits & 0x7f800000U) && (bits & 0x7fffffU);
}

// NOTSA: binary64 counterpart of the raw binary32 classification.
bool is_denormal64(std::uint64_t bits) {
    return !(bits & 0x7ff0000000000000ULL) && (bits & 0xfffffffffffffULL);
}

// NOTSA: DAZ replaces each input with its signed zero before any classification.
std::uint32_t apply_daz32(std::uint32_t bits, std::uint32_t mxcsr) {
    if ((mxcsr & kDenormalsAreZero) && is_denormal32(bits)) {
        return bits & 0x80000000U;
    }
    return bits;
}

// NOTSA: preserve the binary64 input sign when DAZ replaces a denormal.
std::uint64_t apply_daz64(std::uint64_t bits, std::uint32_t mxcsr) {
    if ((mxcsr & kDenormalsAreZero) && is_denormal64(bits)) {
        return bits & 0x8000000000000000ULL;
    }
    return bits;
}

// NOTSA: scope the shared SoftFloat TLS for one scalar SSE operation, including
// unwind after an immediate trap. x87 precision and host fenv are never changed.
struct SoftFloatOperation {
    std::uint_fast8_t saved_rounding = softfloat_roundingMode;
    std::uint_fast8_t saved_tininess = softfloat_detectTininess;
    std::uint_fast8_t saved_flags = softfloat_exceptionFlags;

    explicit SoftFloatOperation(std::uint32_t mxcsr) {
        constexpr std::uint_fast8_t modes[] = {
            softfloat_round_near_even, softfloat_round_min,
            softfloat_round_max, softfloat_round_minMag};
        softfloat_roundingMode = modes[(mxcsr >> 13) & 3];
        softfloat_detectTininess = softfloat_tininess_afterRounding;
        softfloat_exceptionFlags = 0;
    }

    ~SoftFloatOperation() {
        softfloat_roundingMode = saved_rounding;
        softfloat_detectTininess = saved_tininess;
        softfloat_exceptionFlags = saved_flags;
    }

    std::uint8_t exceptions() const {
        const auto flags = softfloat_exceptionFlags;
        std::uint8_t result = 0;
        if (flags & softfloat_flag_invalid) {
            result |= kInvalid;
        }
        if (flags & softfloat_flag_infinite) {
            result |= kDivideByZero;
        }
        if (flags & softfloat_flag_overflow) {
            result |= kOverflow;
        }
        if (flags & softfloat_flag_underflow) {
            result |= kUnderflow;
        }
        if (flags & softfloat_flag_inexact) {
            result |= kPrecision;
        }
        return result;
    }
};

// NOTSA: order finite/infinite encodings after callers handle NaNs and DAZ.
// Both signed zeros compare equal; negative encodings reverse magnitude order.
template<typename Word>
SseComparison ordered_comparison(Word destination, Word source, Word sign_bit) {
    if (destination == source || !((destination | source) & ~sign_bit)) {
        return SseComparison::equal;
    }
    bool less;
    if ((destination & sign_bit) != (source & sign_bit)) {
        less = bool(destination & sign_bit);
    } else if (destination & sign_bit) {
        less = destination > source;
    } else {
        less = destination < source;
    }
    return less ? SseComparison::less : SseComparison::greater;
}

// NOTSA: retain the existing scalar finishing policy after SoftFloat rounding.
// Unmasked underflow detects even exact tiny working results and suppresses PE.
// Masked underflow allows FTZ, including exact tiny results, with signed zero.
std::uint32_t finish_single(std::uint32_t result, std::uint8_t &flags, std::uint32_t mxcsr) {
    const bool tiny = is_denormal32(result) || (flags & kUnderflow);
    if (tiny && !(mxcsr & kUnderflowMask)) {
        flags = static_cast<std::uint8_t>((flags | kUnderflow) & ~std::uint32_t{kPrecision});
    } else if (tiny && (mxcsr & kFlushToZero)) {
        result &= 0x80000000U;
        flags |= kUnderflow | kPrecision;
    }
    if ((flags & kOverflow) && !(mxcsr & kOverflowMask)) {
        flags &= static_cast<std::uint8_t>(~std::uint32_t{kPrecision});
    }
    return result;
}

// NOTSA: the existing SUBSD tail uses the same policy with binary64 result bits.
std::uint64_t finish_double_subtraction(std::uint64_t result, std::uint8_t &flags,
                                      std::uint32_t mxcsr) {
    const bool tiny = is_denormal64(result) || (flags & kUnderflow);
    if (tiny && !(mxcsr & kUnderflowMask)) {
        flags = static_cast<std::uint8_t>((flags | kUnderflow) & ~std::uint32_t{kPrecision});
    } else if (tiny && (mxcsr & kFlushToZero)) {
        result &= 0x8000000000000000ULL;
        flags |= kUnderflow | kPrecision;
    }
    if ((flags & kOverflow) && !(mxcsr & kOverflowMask)) {
        flags &= static_cast<std::uint8_t>(~std::uint32_t{kPrecision});
    }
    return result;
}

} // namespace

SseContext::SseContext(std::uint32_t mxcsr)
    : mxcsr_(0), owner_(std::this_thread::get_id()) {
    set_mxcsr(mxcsr);
}

void SseContext::check_thread() const {
    if (owner_ != std::this_thread::get_id()) {
        throw SseError(SseFailure::cross_thread_context, 0, "SSE context belongs to another thread");
    }
}

std::uint32_t SseContext::mxcsr() const {
    check_thread();
    return mxcsr_;
}

void SseContext::set_mxcsr(std::uint32_t mxcsr) {
    check_thread();
    if (mxcsr & 0xffff0000U) {
        throw SseError(SseFailure::reserved_mxcsr, 0, "reserved MXCSR bits");
    }
    mxcsr_ = mxcsr;
}

void SseContext::clear_exceptions() {
    check_thread();
    mxcsr_ &= ~kExceptionFlags;
}

void SseContext::commit(std::uint8_t flags) {
    mxcsr_ |= flags;
    // Only new flags can trap. Incoming unmasked sticky flags are retained.
    const auto unmasked = static_cast<std::uint8_t>(flags & ~(mxcsr_ >> 7) & kExceptionFlags);
    if (unmasked) {
        throw SseError(SseFailure::unmasked_exception, unmasked, "unmasked scalar SSE exception");
    }
}

Sse32 SseContext::from_int32(std::int32_t value) {
    check_thread();
    SoftFloatOperation operation(mxcsr_);
    const auto result = i32_to_f32(value);
    commit(operation.exceptions());
    return {result.v};
}

Sse32 SseContext::binary(Sse32 destination, Sse32 source, unsigned kind) {
    check_thread();
    destination.bits = apply_daz32(destination.bits, mxcsr_);
    source.bits = apply_daz32(source.bits, mxcsr_);
    SoftFloatOperation operation(mxcsr_);
    const float32_t left{destination.bits};
    const float32_t right{source.bits};
    float32_t result{};
    switch (static_cast<BinaryOperation>(kind)) {
    case BinaryOperation::Add:
        result = f32_add(left, right);
        break;
    case BinaryOperation::Subtract:
        result = f32_sub(left, right);
        break;
    case BinaryOperation::Multiply:
        result = f32_mul(left, right);
        break;
    default:
        result = f32_div(left, right);
        break;
    }
    auto flags = operation.exceptions();
    // NaNs, other invalid operations and zero division precede denormal faults.
    const bool prior_exception = is_nan32(destination.bits) || is_nan32(source.bits) ||
                                 (flags & (kInvalid | kDivideByZero));
    if (!prior_exception && (is_denormal32(destination.bits) || is_denormal32(source.bits))) {
        if (!(mxcsr_ & kDenormalMask)) {
            commit(kDenormalOperand);
        }
        flags |= kDenormalOperand;
    }
    result.v = finish_single(result.v, flags, mxcsr_);
    commit(flags);
    return {result.v};
}

Sse32 SseContext::add(Sse32 destination, Sse32 source) {
    return binary(destination, source, static_cast<unsigned>(BinaryOperation::Add));
}

Sse32 SseContext::subtract(Sse32 destination, Sse32 source) {
    return binary(destination, source, static_cast<unsigned>(BinaryOperation::Subtract));
}

Sse32 SseContext::multiply(Sse32 destination, Sse32 source) {
    return binary(destination, source, static_cast<unsigned>(BinaryOperation::Multiply));
}

Sse32 SseContext::divide(Sse32 destination, Sse32 source) {
    return binary(destination, source, static_cast<unsigned>(BinaryOperation::Divide));
}

Sse64 SseContext::to_double(Sse32 source) {
    check_thread();
    source.bits = apply_daz32(source.bits, mxcsr_);
    if (is_denormal32(source.bits)) {
        commit(kDenormalOperand);
    }
    SoftFloatOperation operation(mxcsr_);
    const auto result = f32_to_f64(float32_t{source.bits});
    commit(operation.exceptions());
    return {result.v};
}

Sse32 SseContext::to_single(Sse64 source) {
    check_thread();
    source.bits = apply_daz64(source.bits, mxcsr_);
    if (is_denormal64(source.bits)) {
        commit(kDenormalOperand);
    }
    SoftFloatOperation operation(mxcsr_);
    auto result = f64_to_f32(float64_t{source.bits});
    auto flags = operation.exceptions();
    result.v = finish_single(result.v, flags, mxcsr_);
    commit(flags);
    return {result.v};
}

Sse64 SseContext::sqrt(Sse64 source) {
    check_thread();
    source.bits = apply_daz64(source.bits, mxcsr_);
    SoftFloatOperation operation(mxcsr_);
    const auto result = f64_sqrt(float64_t{source.bits});
    auto flags = operation.exceptions();
    if (!(flags & kInvalid) && !is_nan64(source.bits) && is_denormal64(source.bits)) {
        if (!(mxcsr_ & kDenormalMask)) {
            commit(kDenormalOperand);
        }
        flags |= kDenormalOperand;
    }
    commit(flags);
    return {result.v};
}

Sse32 SseContext::sqrt_single(Sse32 source) {
    check_thread();
    source.bits = apply_daz32(source.bits, mxcsr_);
    SoftFloatOperation operation(mxcsr_);
    const auto result = f32_sqrt(float32_t{source.bits});
    auto flags = operation.exceptions();
    if (!(flags & kInvalid) && !is_nan32(source.bits) && is_denormal32(source.bits)) {
        if (!(mxcsr_ & kDenormalMask)) {
            commit(kDenormalOperand);
        }
        flags |= kDenormalOperand;
    }
    commit(flags);
    return {result.v};
}

Sse64 SseContext::subtract_double(Sse64 destination, Sse64 source) {
    check_thread();
    destination.bits = apply_daz64(destination.bits, mxcsr_);
    source.bits = apply_daz64(source.bits, mxcsr_);
    SoftFloatOperation operation(mxcsr_);
    auto result = f64_sub(float64_t{destination.bits}, float64_t{source.bits});
    auto flags = operation.exceptions();
    const bool prior_exception = is_nan64(destination.bits) || is_nan64(source.bits) ||
                                 (flags & (kInvalid | kDivideByZero));
    if (!prior_exception && (is_denormal64(destination.bits) || is_denormal64(source.bits))) {
        if (!(mxcsr_ & kDenormalMask)) {
            commit(kDenormalOperand);
        }
        flags |= kDenormalOperand;
    }
    result.v = finish_double_subtraction(result.v, flags, mxcsr_);
    commit(flags);
    return {result.v};
}

bool SseContext::less_than(Sse64 destination, Sse64 source) {
    check_thread();
    destination.bits = apply_daz64(destination.bits, mxcsr_);
    source.bits = apply_daz64(source.bits, mxcsr_);
    if (is_nan64(destination.bits) || is_nan64(source.bits)) {
        commit(kInvalid);
        return false;
    }
    if (is_denormal64(destination.bits) || is_denormal64(source.bits)) {
        commit(kDenormalOperand);
    }
    return ordered_comparison(destination.bits, source.bits, std::uint64_t{0x8000000000000000ULL}) ==
           SseComparison::less;
}

std::int32_t SseContext::truncate_int32(Sse32 source) {
    check_thread();
    source.bits = apply_daz32(source.bits, mxcsr_);
    SoftFloatOperation operation(mxcsr_);
    // CVTT ignores RC and does not raise a denormal operand exception.
    const auto result = f32_to_i32_r_minMag(float32_t{source.bits}, true);
    commit(operation.exceptions());
    return static_cast<std::int32_t>(result);
}

SseComparison SseContext::comiss(Sse32 destination, Sse32 source) {
    check_thread();
    destination.bits = apply_daz32(destination.bits, mxcsr_);
    source.bits = apply_daz32(source.bits, mxcsr_);
    if (is_nan32(destination.bits) || is_nan32(source.bits)) {
        commit(kInvalid);
        return SseComparison::unordered;
    }
    if (is_denormal32(destination.bits) || is_denormal32(source.bits)) {
        commit(kDenormalOperand);
    }
    return ordered_comparison(destination.bits, source.bits, 0x80000000U);
}

SseComparison SseContext::ucomisd(Sse64 destination, Sse64 source) {
    check_thread();
    destination.bits = apply_daz64(destination.bits, mxcsr_);
    source.bits = apply_daz64(source.bits, mxcsr_);
    if (is_nan64(destination.bits) || is_nan64(source.bits)) {
        if (is_signaling_nan64(destination.bits) || is_signaling_nan64(source.bits)) {
            commit(kInvalid);
        }
        return SseComparison::unordered;
    }
    if (is_denormal64(destination.bits) || is_denormal64(source.bits)) {
        commit(kDenormalOperand);
    }
    return ordered_comparison(destination.bits, source.bits, std::uint64_t{0x8000000000000000ULL});
}

Sse32 SseContext::minimum(Sse32 destination, Sse32 source) {
    check_thread();
    destination.bits = apply_daz32(destination.bits, mxcsr_);
    source.bits = apply_daz32(source.bits, mxcsr_);
    if (is_nan32(destination.bits) || is_nan32(source.bits)) {
        commit(kInvalid);
        return source; // Legacy MINSS returns source bytes, including an SNaN.
    }
    if (is_denormal32(destination.bits) || is_denormal32(source.bits)) {
        commit(kDenormalOperand);
    }
    const auto comparison = ordered_comparison(destination.bits, source.bits, 0x80000000U);
    return comparison == SseComparison::less ? destination : source;
}

} // namespace sadx
