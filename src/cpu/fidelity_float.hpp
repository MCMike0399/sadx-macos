#pragma once

#include "cpu/crt_integer.hpp"
#include "cpu/x87_math.hpp"
#include "cpu/x87_transcendental.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx::fidelity {

// NOTSA: FABS clears the sign without quieting a signaling binary32 NaN.
inline float absolute_float(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    bits &= ~(std::uint32_t{1} << 31);
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

// NOTSA: optional observation of arithmetic, before exceptions can stop evaluation.
// Reference adapters translate these events to diagnostics; native arithmetic needs no observer.
enum class ExactEvent {
    Load,
    Store,
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    Compare,
    IntegerConversion,
    Logarithm,
    SquareRoot
};
struct ExactObserver {
    virtual ~ExactObserver() = default;
    virtual void before(ExactEvent) = 0;
};

// NOTSA: retained arithmetic value with the caller's precision, rounding and exception context.
// A float operand is a source memory operand; an ExactFloat operand is already loaded.
// Conversion back to binary32 is always explicit, so a C++ temporary cannot add a spill.
// The context must outlive its values; operations keep its existing thread/exception rules.
// This models m32 arithmetic only. A double operand must use its own fidelity helper.
// Sequence independent operand calculations in named temporaries when their exception order
// matters; C++ does not specify which side of a binary expression is evaluated first.
class ExactFloat {
  public:
    ExactFloat(X87Context &context, float value, ExactObserver *observer = nullptr)
        : context_(&context), observer_(observer), value_(load(context, value, observer)) {}
    ExactFloat(X87Context &, double, ExactObserver * = nullptr) = delete;

    float rounded() const {
        observe(ExactEvent::Store);
        return context_->to_float(value_);
    }
    ExactFloat operator-() const {
        observe(ExactEvent::Negate);
        return retain(value_.negated());
    }

    ExactFloat operator+(float memory) const {
        observe(ExactEvent::Add);
        return retain(context_->add_float_memory(value_, memory));
    }
    ExactFloat operator-(float memory) const {
        observe(ExactEvent::Subtract);
        return retain(context_->subtract_float_memory(value_, memory));
    }
    ExactFloat operator*(float memory) const {
        observe(ExactEvent::Multiply);
        return retain(context_->multiply_float_memory(value_, memory));
    }
    ExactFloat operator/(float memory) const {
        observe(ExactEvent::Divide);
        return retain(context_->divide_float_memory(value_, memory));
    }
    ExactFloat operator+(const ExactFloat &other) const {
        require_same_context(other);
        observe(ExactEvent::Add);
        return retain(context_->add(value_, other.value_));
    }
    ExactFloat operator-(const ExactFloat &other) const {
        require_same_context(other);
        observe(ExactEvent::Subtract);
        return retain(context_->subtract(value_, other.value_));
    }
    ExactFloat operator*(const ExactFloat &other) const {
        require_same_context(other);
        observe(ExactEvent::Multiply);
        return retain(context_->multiply(value_, other.value_));
    }
    ExactFloat operator/(const ExactFloat &other) const {
        require_same_context(other);
        observe(ExactEvent::Divide);
        return retain(context_->divide(value_, other.value_));
    }
    X87Comparison compare(float memory) const {
        observe(ExactEvent::Compare);
        return context_->compare_float_memory(value_, memory);
    }
    X87Comparison compare(const ExactFloat &other) const {
        require_same_context(other);
        observe(ExactEvent::Compare);
        return context_->compare(value_, other.value_);
    }

    // NOTSA: explicit wide loads/stores and m64 operations retain their source width.
    static ExactFloat integer(X87Context &context, std::int64_t value,
                              ExactObserver *observer = nullptr) {
        notify(observer, ExactEvent::Load);
        return ExactFloat(context, context.from_int64(value), observer);
    }
    static ExactFloat wide(X87Context &context, double value, ExactObserver *observer = nullptr) {
        notify(observer, ExactEvent::Load);
        return ExactFloat(context, context.from_double(value), observer);
    }
    double rounded_double() const {
        observe(ExactEvent::Store);
        return context_->to_double(value_);
    }
    ExactFloat multiply_double(double memory) const {
        observe(ExactEvent::Multiply);
        return retain(context_->multiply_double_memory(value_, memory));
    }
    ExactFloat reciprocal_double(double numerator) const {
        observe(ExactEvent::Divide);
        return retain(context_->reverse_divide_double_memory(value_, numerator));
    }
    ExactFloat square_root() const {
        observe(ExactEvent::SquareRoot);
        return retain(context_->sqrt(value_));
    }
    X87Comparison quiet_compare(const ExactFloat &other) const {
        require_same_context(other);
        observe(ExactEvent::Compare);
        return context_->compare_quiet(value_, other.value_);
    }
    // NOTSA: FLDLN2's canonical constant and the selected Bochs FYL2X model.
    ExactFloat natural_log() const {
        const auto ln2 = context_->from_canonical_ext80(16382, 12786308645202655660ULL);
        observe(ExactEvent::Logarithm);
        return retain(X87Transcendental::fyl2x_bochs(*context_, value_, ln2));
    }
    // NOTSA: MSVC _ftol2 corrects a current-RC FISTP result toward zero.
    // Its sign tests use binary32 stores, including their NaN and exception behavior.
    std::int64_t truncated_integer() const {
        observe(ExactEvent::IntegerConversion);
        const auto word = crt_ftol(value_, *context_);
        std::int64_t result;
        std::memcpy(&result, &word, sizeof(result));
        return result;
    }

    ExactFloat operator+(double) const = delete;
    ExactFloat operator-(double) const = delete;
    ExactFloat operator*(double) const = delete;
    ExactFloat operator/(double) const = delete;
    X87Comparison compare(double) const = delete;

    friend ExactFloat operator-(float memory, const ExactFloat &reg) {
        reg.observe(ExactEvent::Subtract);
        return reg.retain(reg.context_->reverse_subtract_float_memory(reg.value_, memory));
    }
    friend ExactFloat operator/(float memory, const ExactFloat &reg) {
        reg.observe(ExactEvent::Divide);
        return reg.retain(reg.context_->reverse_divide_float_memory(reg.value_, memory));
    }
    friend ExactFloat operator-(double, const ExactFloat &) = delete;
    friend ExactFloat operator/(double, const ExactFloat &) = delete;

  private:
    X87Context *context_;
    ExactObserver *observer_;
    X87Value value_;
    ExactFloat(X87Context &context, X87Value value, ExactObserver *observer)
        : context_(&context), observer_(observer), value_(value) {}
    ExactFloat retain(X87Value value) const { return ExactFloat(*context_, value, observer_); }
    static void notify(ExactObserver *observer, ExactEvent event) {
        if (observer)
            observer->before(event);
    }
    void observe(ExactEvent event) const { notify(observer_, event); }
    static X87Value load(X87Context &context, float value, ExactObserver *observer) {
        notify(observer, ExactEvent::Load);
        return context.from_float(value);
    }
    void require_same_context(const ExactFloat &other) const {
        if (context_ != other.context_)
            throw std::logic_error("exact arithmetic values belong to different contexts");
    }
};

} // namespace sadx::fidelity
