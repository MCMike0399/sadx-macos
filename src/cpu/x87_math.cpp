#include "cpu/x87_math.hpp"

#include <cstring>

extern "C" {
#include "../../third_party/softfloat/platform.h"
#include "../../third_party/softfloat/source/include/softfloat.h"
}

namespace sadx {

namespace {

thread_local X87Context* active_context = nullptr;

extFloat80_t raw(X87Value value) {
    extFloat80_t result;
    result.signExp = value.sign_exponent();
    result.signif = value.significand();
    return result;
}

bool is_nan(X87Value value) {
    return (value.sign_exponent()&0x7fffU)==0x7fffU &&
        (value.significand()&0x7fffffffffffffffULL)!=0;
}

bool is_zero(X87Value value) {
    return (value.sign_exponent()&0x7fffU)==0 && value.significand()==0;
}

bool is_infinite(X87Value value) {
    return (value.sign_exponent()&0x7fffU)==0x7fffU && value.significand()==0x8000000000000000ULL;
}

std::uint16_t architectural_exceptions(std::uint_fast8_t flags) {
    std::uint16_t result = 0;
    if ((flags & softfloat_flag_invalid) != 0) result |= 0x01;
    if ((flags & softfloat_flag_infinite) != 0) result |= 0x04;
    if ((flags & softfloat_flag_overflow) != 0) result |= 0x08;
    if ((flags & softfloat_flag_underflow) != 0) result |= 0x10;
    if ((flags & softfloat_flag_inexact) != 0) result |= 0x20;
    return result;
}

} // namespace

class X87Context::Operation {
public:
    explicit Operation(X87Context& context)
        : context_(context), rounding_(softfloat_roundingMode),
          precision_(extF80_roundingPrecision), tininess_(softfloat_detectTininess),
          flags_(softfloat_exceptionFlags) {
        context_.check_thread();
        context_.check_pending();
        switch ((context_.control_word_ >> 8U) & 3U) {
        case 0: extF80_roundingPrecision = 32; break; // 24 significant bits
        case 2: extF80_roundingPrecision = 64; break; // 53 significant bits
        case 3: extF80_roundingPrecision = 80; break; // 64 significant bits
        default: break; // rejected before a context is constructed/updated
        }
        switch ((context_.control_word_ >> 10U) & 3U) {
        case 0: softfloat_roundingMode = softfloat_round_near_even; break;
        case 1: softfloat_roundingMode = softfloat_round_min; break;
        case 2: softfloat_roundingMode = softfloat_round_max; break;
        case 3: softfloat_roundingMode = softfloat_round_minMag; break;
        }
        softfloat_detectTininess = softfloat_tininess_afterRounding;
        softfloat_exceptionFlags = 0;
    }
    ~Operation() {
        softfloat_roundingMode = rounding_;
        extF80_roundingPrecision = precision_;
        softfloat_detectTininess = tininess_;
        softfloat_exceptionFlags = flags_;
    }
    void finish() {
        context_.record_exceptions(architectural_exceptions(softfloat_exceptionFlags));
    }
private:
    X87Context& context_;
    std::uint_fast8_t rounding_, precision_, tininess_, flags_;
};

X87Value X87Value::negated() const noexcept {
    return X87Value(sign_exponent_ ^ 0x8000U, significand_);
}

X87Error::X87Error(X87Failure failure, std::uint16_t exceptions, const char* message)
    : std::runtime_error(message), failure_(failure), exceptions_(exceptions) {}

X87Context::X87Context(std::uint16_t control_word)
    : owner_(std::this_thread::get_id()), control_word_(control_word) {
    set_control_word(control_word);
}

void X87Context::check_thread() const {
    if (owner_ != std::this_thread::get_id()) {
        throw X87Error(X87Failure::cross_thread_context, 0,
                       "x87 context belongs to another native thread");
    }
}

void X87Context::check_pending() const {
    const auto unmasked = static_cast<std::uint16_t>(exceptions_ & ~control_word_ & 0x3fU);
    if (unmasked != 0) {
        throw X87Error(X87Failure::unmasked_exception, unmasked,
                       "unmasked x87 arithmetic exception; hardware delivery timing unverified");
    }
}

void X87Context::record_exceptions(std::uint16_t exceptions) {
    exceptions_ |= exceptions;
    check_pending();
}

void X87Context::check_denormal(X87Value value) {
    if ((value.sign_exponent() & 0x7fffU) == 0 && value.significand() != 0) {
        record_exceptions(0x02);
    }
}

void X87Context::set_control_word(std::uint16_t control_word) {
    check_thread();
    if (((control_word >> 8U) & 3U) == 1U) {
        throw X87Error(X87Failure::reserved_precision_control, 0,
                       "reserved x87 precision-control encoding 01 is unsupported");
    }
    control_word_ = control_word; // preserve every caller-supplied control bit
}

void X87Context::clear_exceptions() {
    check_thread();
    exceptions_ = 0;
}

X87Value X87Context::from_float(float value) {
    Operation operation(*this);
    float32_t input;
    std::memcpy(&input.v, &value, sizeof(value));
    if ((input.v & 0x7f800000U) == 0 && (input.v & 0x007fffffU) != 0) {
        record_exceptions(0x02); // FLD binary32 denormal operand, no FTZ
    }
    const auto result = f32_to_extF80(input);
    operation.finish();
    return X87Value(result.signExp, result.signif);
}

X87Value X87Context::add(X87Value first, X87Value second) {
    return binary(first,second,BinaryOperation::add);
}

X87Value X87Context::round_to_integer(X87Value value) {
    Operation operation(*this);
    check_denormal(value);
    const auto result=extF80_roundToInt(raw(value),softfloat_roundingMode,true);
    operation.finish();
    return X87Value(result.signExp,result.signif);
}

X87Value X87Context::from_double(double value) {
    Operation operation(*this);
    float64_t input;
    std::memcpy(&input.v, &value, sizeof(value));
    if ((input.v & 0x7ff0000000000000ULL) == 0 && (input.v & 0x000fffffffffffffULL) != 0) {
        record_exceptions(0x02);
    }
    const auto result = f64_to_extF80(input);
    operation.finish();
    return X87Value(result.signExp, result.signif);
}

X87Value X87Context::from_int64(std::int64_t value) {
    Operation operation(*this);
    const auto result = i64_to_extF80(value); // FILD is exact, independent of PC
    operation.finish();
    return X87Value(result.signExp, result.signif);
}

std::int64_t X87Context::to_int64_current_rounding(X87Value value) {
    Operation operation(*this);
    check_denormal(value);
    const auto result = extF80_to_i64(raw(value), softfloat_roundingMode, true);
    operation.finish();
    return result; // 8086 invalid/overflow masked result is integer indefinite
}

X87Value X87Context::from_canonical_ext80(std::uint16_t sign_exponent,std::uint64_t significand) {
    check_thread();
    const bool zero_exponent=(sign_exponent&0x7fffU)==0;
    const bool explicit_integer_bit=(significand&0x8000000000000000ULL)!=0;
    if (zero_exponent==explicit_integer_bit) {
        throw X87Error(X87Failure::noncanonical_ext80_source,0,
                       "unsupported/noncanonical ext80 source-constant encoding");
    }
    return X87Value(sign_exponent,significand);
}

X87Value X87Context::subtract(X87Value first, X87Value second) {
    return binary(first,second,BinaryOperation::subtract);
}

X87Value X87Context::multiply(X87Value first, X87Value second) {
    return binary(first,second,BinaryOperation::multiply);
}

X87Value X87Context::sqrt(X87Value value) {
    Operation operation(*this);
    // NaN and invalid negative operands precede denormal handling (Intel4.9.2).
    if (!is_nan(value) && !(value.negative() && !is_zero(value))) check_denormal(value);
    const auto result = extF80_sqrt(raw(value));
    operation.finish();
    return X87Value(result.signExp, result.signif);
}

X87Value X87Context::divide(X87Value numerator, X87Value denominator) {
    return binary(numerator,denominator,BinaryOperation::divide);
}

X87Value X87Context::binary(X87Value first,X87Value second,BinaryOperation kind,
                            bool first_memory_denormal,bool second_memory_denormal) {
    Operation operation(*this);
    // IEEE/Intel NaN priority is decided BEFORE denormal handling. In particular,
    // QNaN+sNaN memory is NOT QNaN+an already-quieted FLD32 value.
    const bool nan=is_nan(first)||is_nan(second);
    const bool zero_divide=kind==BinaryOperation::divide && is_zero(second) &&
        !is_zero(first) && !is_infinite(first) && !nan;
    if (!nan && !zero_divide) {
        check_denormal(first);check_denormal(second);
        if (first_memory_denormal||second_memory_denormal) record_exceptions(0x02);
    }
    extFloat80_t result;
    switch(kind) {
    case BinaryOperation::add: result=extF80_add(raw(first),raw(second));break;
    case BinaryOperation::subtract: result=extF80_sub(raw(first),raw(second));break;
    case BinaryOperation::multiply: result=extF80_mul(raw(first),raw(second));break;
    case BinaryOperation::divide: result=extF80_div(raw(first),raw(second));break;
    }
    operation.finish();return X87Value(result.signExp,result.signif);
}

X87Context::MemoryOperand X87Context::decode_float_memory(float value) const {
    check_thread();
    float32_t input;std::memcpy(&input.v,&value,4);
    const auto fraction=input.v&0x007fffffU;
    if ((input.v&0x7f800000U)==0x7f800000U && fraction!=0) {
        // Format widening only: preserve sign/payload AND signaling bit. An
        // actual arithmetic operation, not this decoder, owns quieting/invalid.
        return {X87Value(static_cast<std::uint16_t>((input.v>>16U)&0x8000U)|0x7fffU,
                         0x8000000000000000ULL|(static_cast<std::uint64_t>(fraction)<<40U)),false};
    }
    // The unchanged upstream finite/infinity conversion is exact and does not
    // read rounding mode or raise flags. Its NaN path is deliberately bypassed.
    const auto result=f32_to_extF80(input);
    return {X87Value(result.signExp,result.signif),(input.v&0x7f800000U)==0 && fraction!=0};
}

X87Context::MemoryOperand X87Context::decode_double_memory(double value) const {
    check_thread();
    float64_t input;std::memcpy(&input.v,&value,8);
    const auto fraction=input.v&0x000fffffffffffffULL;
    if ((input.v&0x7ff0000000000000ULL)==0x7ff0000000000000ULL && fraction!=0) {
        return {X87Value(static_cast<std::uint16_t>((input.v>>48U)&0x8000U)|0x7fffU,
                         0x8000000000000000ULL|(fraction<<11U)),false};
    }
    const auto result=f64_to_extF80(input);
    return {X87Value(result.signExp,result.signif),(input.v&0x7ff0000000000000ULL)==0 && fraction!=0};
}

X87Value X87Context::add_float_memory(X87Value reg,float memory) {
    const auto m=decode_float_memory(memory);return binary(reg,m.value,BinaryOperation::add,false,m.denormal);
}
X87Value X87Context::subtract_float_memory(X87Value reg,float memory) {
    const auto m=decode_float_memory(memory);return binary(reg,m.value,BinaryOperation::subtract,false,m.denormal);
}
X87Value X87Context::reverse_subtract_float_memory(X87Value reg,float memory) {
    const auto m=decode_float_memory(memory);return binary(m.value,reg,BinaryOperation::subtract,m.denormal,false);
}
X87Value X87Context::multiply_float_memory(X87Value reg,float memory) {
    const auto m=decode_float_memory(memory);return binary(reg,m.value,BinaryOperation::multiply,false,m.denormal);
}
X87Value X87Context::divide_float_memory(X87Value reg,float memory) {
    const auto m=decode_float_memory(memory);return binary(reg,m.value,BinaryOperation::divide,false,m.denormal);
}
X87Value X87Context::reverse_divide_float_memory(X87Value reg,float memory) {
    const auto m=decode_float_memory(memory);return binary(m.value,reg,BinaryOperation::divide,m.denormal,false);
}
X87Value X87Context::add_double_memory(X87Value reg,double memory) {
    const auto m=decode_double_memory(memory);return binary(reg,m.value,BinaryOperation::add,false,m.denormal);
}
X87Value X87Context::subtract_double_memory(X87Value reg,double memory) {
    const auto m=decode_double_memory(memory);return binary(reg,m.value,BinaryOperation::subtract,false,m.denormal);
}
X87Value X87Context::reverse_subtract_double_memory(X87Value reg,double memory) {
    const auto m=decode_double_memory(memory);return binary(m.value,reg,BinaryOperation::subtract,m.denormal,false);
}
X87Value X87Context::multiply_double_memory(X87Value reg,double memory) {
    const auto m=decode_double_memory(memory);return binary(reg,m.value,BinaryOperation::multiply,false,m.denormal);
}
X87Value X87Context::divide_double_memory(X87Value reg,double memory) {
    const auto m=decode_double_memory(memory);return binary(reg,m.value,BinaryOperation::divide,false,m.denormal);
}
X87Value X87Context::reverse_divide_double_memory(X87Value reg,double memory) {
    const auto m=decode_double_memory(memory);return binary(m.value,reg,BinaryOperation::divide,m.denormal,false);
}

float X87Context::to_float(X87Value value) {
    Operation operation(*this);
    const auto result = extF80_to_f32(raw(value));
    operation.finish();
    float output;
    std::memcpy(&output, &result.v, sizeof(output));
    return output;
}

double X87Context::to_double(X87Value value) {
    Operation operation(*this);
    const auto result=extF80_to_f64(raw(value));
    operation.finish();
    double output;
    std::memcpy(&output,&result.v,sizeof(output));
    return output;
}

X87Comparison X87Context::compare(X87Value first, X87Value second) {
    return comparison(first,second);
}

X87Comparison X87Context::compare_quiet(X87Value first,X87Value second) {
    return comparison(first,second,false,false,false);
}

X87Comparison X87Context::compare_float_memory(X87Value reg,float memory) {
    const auto m=decode_float_memory(memory);return comparison(reg,m.value,false,m.denormal);
}

X87Comparison X87Context::compare_double_memory(X87Value reg,double memory) {
    const auto m=decode_double_memory(memory);return comparison(reg,m.value,false,m.denormal);
}

X87Comparison X87Context::comparison(X87Value first,X87Value second,
                                    bool first_memory_denormal,bool second_memory_denormal,bool signaling) {
    Operation operation(*this);
    if (!is_nan(first)&&!is_nan(second)) {
        check_denormal(first);check_denormal(second);
        if (first_memory_denormal||second_memory_denormal) record_exceptions(0x02);
    }
    // FCOM raises invalid for ALL NaNs; FUCOM only for signaling NaNs. Classify only
    // after invoking it; a raw-bit unordered fast path would lose exceptions.
    const bool equal = signaling?extF80_eq_signaling(raw(first),raw(second)):extF80_eq(raw(first),raw(second));
    X87Comparison result;
    if (is_nan(first) || is_nan(second)) {
        result = X87Comparison::unordered;
    } else if (equal) {
        result = X87Comparison::equal;
    } else {
        result = extF80_lt(raw(first), raw(second)) ? X87Comparison::less : X87Comparison::greater;
    }
    operation.finish();
    return result;
}

X87Context& X87Context::current() {
    if (!active_context) {
        throw X87Error(X87Failure::missing_thread_context, 0,
                       "native x87 arithmetic requires an explicitly owned context scope");
    }
    active_context->check_thread();
    return *active_context;
}

X87Scope::X87Scope(X87Context& context) : previous_(active_context) {
    context.check_thread();
    active_context = &context;
}

X87Scope::~X87Scope() {
    active_context = previous_;
}

} // namespace sadx
