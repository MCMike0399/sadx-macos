#pragma once

#include "cpu/x87_math.hpp"
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

// NOTSA: retained arithmetic value with the caller's precision, rounding and exception context.
// A float operand is a source memory operand; an ExactFloat operand is already loaded.
// Conversion back to binary32 is always explicit, so a C++ temporary cannot add a spill.
// The context must outlive its values; operations keep its existing thread/exception rules.
// This models m32 arithmetic only. A double operand must use its own fidelity helper.
// Sequence independent operand calculations in named temporaries when their exception order
// matters; C++ does not specify which side of a binary expression is evaluated first.
class ExactFloat {
  public:
    ExactFloat(X87Context &context, float value)
        : context_(&context), value_(context.from_float(value)) {}
    ExactFloat(X87Context &, double) = delete;

    float rounded() const { return context_->to_float(value_); }
    ExactFloat operator-() const { return retain(value_.negated()); }

    ExactFloat operator+(float memory) const {
        return retain(context_->add_float_memory(value_, memory));
    }
    ExactFloat operator-(float memory) const {
        return retain(context_->subtract_float_memory(value_, memory));
    }
    ExactFloat operator*(float memory) const {
        return retain(context_->multiply_float_memory(value_, memory));
    }
    ExactFloat operator/(float memory) const {
        return retain(context_->divide_float_memory(value_, memory));
    }
    ExactFloat operator+(const ExactFloat &other) const {
        require_same_context(other);
        return retain(context_->add(value_, other.value_));
    }
    ExactFloat operator-(const ExactFloat &other) const {
        require_same_context(other);
        return retain(context_->subtract(value_, other.value_));
    }
    ExactFloat operator*(const ExactFloat &other) const {
        require_same_context(other);
        return retain(context_->multiply(value_, other.value_));
    }
    ExactFloat operator/(const ExactFloat &other) const {
        require_same_context(other);
        return retain(context_->divide(value_, other.value_));
    }
    X87Comparison compare(float memory) const {
        return context_->compare_float_memory(value_, memory);
    }
    X87Comparison compare(const ExactFloat &other) const {
        require_same_context(other);
        return context_->compare(value_, other.value_);
    }

    ExactFloat operator+(double) const = delete;
    ExactFloat operator-(double) const = delete;
    ExactFloat operator*(double) const = delete;
    ExactFloat operator/(double) const = delete;
    X87Comparison compare(double) const = delete;

    friend ExactFloat operator-(float memory, const ExactFloat &reg) {
        return reg.retain(reg.context_->reverse_subtract_float_memory(reg.value_, memory));
    }
    friend ExactFloat operator/(float memory, const ExactFloat &reg) {
        return reg.retain(reg.context_->reverse_divide_float_memory(reg.value_, memory));
    }
    friend ExactFloat operator-(double, const ExactFloat &) = delete;
    friend ExactFloat operator/(double, const ExactFloat &) = delete;

  private:
    X87Context *context_;
    X87Value value_;
    ExactFloat(X87Context &context, X87Value value) : context_(&context), value_(value) {}
    ExactFloat retain(X87Value value) const { return ExactFloat(*context_, value); }
    void require_same_context(const ExactFloat &other) const {
        if (context_ != other.context_)
            throw std::logic_error("exact arithmetic values belong to different contexts");
    }
};

} // namespace sadx::fidelity
