#pragma once

#include <cstdint>
#include <stdexcept>
#include <thread>

namespace sadx {

class X87Context;
class X87Transcendental;

enum class X87Comparison { less, equal, greater, unordered };

// FCOM condition-code bits in FNSTSW's architectural status word.
constexpr std::uint16_t x87_comparison_status(X87Comparison comparison) noexcept {
    switch (comparison) {
    case X87Comparison::less: return 0x0100;
    case X87Comparison::equal: return 0x4000;
    case X87Comparison::greater: return 0;
    case X87Comparison::unordered: return 0x4500;
    }
    return 0x4500;
}

// Canonical ext80 value with explicit integer significand. No host long-double
// assumptions and no arbitrary/noncanonical guest representation constructor.
class X87Value {
public:
    X87Value negated() const noexcept;
    bool negative() const noexcept { return (sign_exponent_ & 0x8000U) != 0; }
    std::uint16_t sign_exponent() const noexcept { return sign_exponent_; }
    std::uint64_t significand() const noexcept { return significand_; }

private:
    friend class X87Context;
    friend class X87Transcendental;
    X87Value(std::uint16_t sign_exponent, std::uint64_t significand)
        : sign_exponent_(sign_exponent), significand_(significand) {}
    std::uint16_t sign_exponent_;
    std::uint64_t significand_;
};

enum class X87Failure {
    reserved_precision_control,
    cross_thread_context,
    missing_thread_context,
    unmasked_exception,
    noncanonical_ext80_source,
};

class X87Error : public std::runtime_error {
public:
    X87Error(X87Failure failure, std::uint16_t exceptions, const char* message);
    X87Failure failure() const noexcept { return failure_; }
    std::uint16_t exceptions() const noexcept { return exceptions_; }
private:
    X87Failure failure_;
    std::uint16_t exceptions_;
};

// An explicitly selected/captured control word is required; there is no hidden
// game FPU default. Context starts with clear software exception flags, which
// is not a claim about initial flags in the original process.
// Each instance belongs to the thread which constructed it. SoftFloat's
// thread-local environment is scoped per operation, so contexts do not leak
// precision/rounding/exception state into one another.
class X87Context {
public:
    explicit X87Context(std::uint16_t control_word);
    X87Context(const X87Context&) = delete;
    X87Context& operator=(const X87Context&) = delete;
    X87Context(X87Context&&) = delete;
    X87Context& operator=(X87Context&&) = delete;

    void set_control_word(std::uint16_t control_word);
    std::uint16_t control_word() const { check_thread(); return control_word_; }
    // Architectural low-six exception bits, including denormal-input detection.
    // Full x87 status/tag/stack/condition flags are outside this arithmetic API.
    std::uint16_t exception_flags() const { check_thread(); return exceptions_; }
    void clear_exceptions();

    X87Value from_float(float value);
    X87Value from_double(double value);
    X87Value from_int64(std::int64_t value);
    // Source-constant import, not arbitrary CPU-state decoding or FLD80
    // execution. Canonical encodings are validated; valid NaN payload/sign
    // bits remain unchanged until a subsequent arithmetic/load operation.
    X87Value from_canonical_ext80(std::uint16_t sign_exponent, std::uint64_t significand);
    std::int64_t to_int64_current_rounding(X87Value value);
    X87Value round_to_integer(X87Value value); // FRNDINT: RC applies, PC does not
    X87Value add(X87Value first, X87Value second);
    X87Value subtract(X87Value first, X87Value second);
    X87Value multiply(X87Value first, X87Value second);
    X87Value divide(X87Value numerator, X87Value denominator);
    X87Value sqrt(X87Value value);
    float to_float(X87Value value);
    double to_double(X87Value value);
    // Signaling FCOM semantics: every NaN, even quiet, raises invalid. This
    // is not FUCOM. Masked invalid returns unordered/C0=C2=C3=1.
    X87Comparison compare(X87Value first, X87Value second);
    // FUCOM/FUCOMPP register operands: QNaN is unordered without invalid;
    // canonical SNaN raises invalid. This does not perform implicit FLD32/64.
    X87Comparison compare_quiet(X87Value first, X87Value second);

    // Actual m32/m64 arithmetic operands are decoded WITHOUT a separate FLD:
    // signaling NaNs remain signaling until this combined instruction chooses
    // its NaN/exception result. from_float/from_double retain FLD semantics.
    // Reverse operations mean memory minus/divided by the register value.
    X87Value add_float_memory(X87Value reg, float memory);
    X87Value subtract_float_memory(X87Value reg, float memory);
    X87Value reverse_subtract_float_memory(X87Value reg, float memory);
    X87Value multiply_float_memory(X87Value reg, float memory);
    X87Value divide_float_memory(X87Value reg, float memory);
    X87Value reverse_divide_float_memory(X87Value reg, float memory);
    X87Comparison compare_float_memory(X87Value reg, float memory);
    X87Value add_double_memory(X87Value reg, double memory);
    X87Value subtract_double_memory(X87Value reg, double memory);
    X87Value reverse_subtract_double_memory(X87Value reg, double memory);
    X87Value multiply_double_memory(X87Value reg, double memory);
    X87Value divide_double_memory(X87Value reg, double memory);
    X87Value reverse_divide_double_memory(X87Value reg, double memory);
    X87Comparison compare_double_memory(X87Value reg, double memory);

    // Requires X87Scope on the current thread; absence is an explicit failure.
    static X87Context& current();

private:
    friend class X87Scope;
    friend class X87Transcendental;
    class Operation;
    enum class BinaryOperation { add, subtract, multiply, divide };
    struct MemoryOperand { X87Value value; bool denormal; };
    std::thread::id owner_;
    std::uint16_t control_word_;
    std::uint16_t exceptions_ = 0;
    void check_thread() const;
    void check_pending() const;
    void record_exceptions(std::uint16_t exceptions);
    void check_denormal(X87Value value);
    MemoryOperand decode_float_memory(float) const;
    MemoryOperand decode_double_memory(double) const;
    X87Value binary(X87Value first, X87Value second, BinaryOperation,
                    bool first_memory_denormal = false, bool second_memory_denormal = false);
    X87Comparison comparison(X87Value first, X87Value second,
                             bool first_memory_denormal = false, bool second_memory_denormal = false,
                             bool signaling = true);
};

// Thread-local active-context ownership. Nested scopes restore their prior
// native context pointer; this models arithmetic ownership, not game task
// globals. Masked flags accrue in the selected context. Unmasked exceptions
// stop native evaluation with X87Error, not silently masked arithmetic.
// Hardware's deferred exception delivery/timing is not implemented or claimed.
class X87Scope {
public:
    explicit X87Scope(X87Context& context);
    ~X87Scope();
    X87Scope(const X87Scope&) = delete;
    X87Scope& operator=(const X87Scope&) = delete;
private:
    X87Context* previous_;
};

} // namespace sadx
