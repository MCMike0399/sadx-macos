#include "ninja/ninja_math.hpp"

#include <utility>

#if defined(__FAST_MATH__)
#error Recovered Ninja math must be compiled without fast-math.
#endif

namespace sadx {

NinjaMath::NinjaMath(SineTable quarter_sine_table, X87Context& context)
    : quarter_sine_table_(std::move(quarter_sine_table)), context_(context) {}

X87Value NinjaMath::sin_extended(std::int32_t angle) const {
    // sonic.exe VA 0x788290: (ECX >> 4) & 0xfff, EDX & 0xf.
    const auto wrapped = static_cast<std::uint32_t>(angle) & 0xffffU;
    const auto index = wrapped >> 4U;
    const auto fraction = wrapped & 15U;
    const auto quadrant = index >> 10U;
    const auto local = index & 1023U;
    const bool reflected = (quadrant & 1U) != 0;
    const bool negative = quadrant >= 2;
    const auto first_index = reflected ? 1024U - local : local;
    const float first = negative ? -quarter_sine_table_[first_index]
                                 : quarter_sine_table_[first_index];
    // The no-interpolation branch also preserves the original signed zeros.
    if (fraction == 0) {
        return context_.from_float(first);
    }
    const auto second_index = reflected ? first_index - 1U : first_index + 1U;
    const float second = negative ? -quarter_sine_table_[second_index]
                                  : quarter_sine_table_[second_index];
    // 0x7e0120 contains float 0.0625. Respect fsub, fmulp, faddp order;
    // Extended intermediates retain the selected x87 precision and rounding.
    const auto loaded_first = context_.from_float(first);
    const auto loaded_second = context_.from_float(second);
    const auto difference = context_.subtract(loaded_second,loaded_first);
    const auto weight = context_.multiply_float_memory(context_.from_int64(fraction),0.0625F);
    const auto interpolated = context_.multiply(difference, weight);
    return context_.add(loaded_first,interpolated);
}

X87Value NinjaMath::cos_extended(std::int32_t angle) const {
    // 0x7883c0 selects the same quarter table with a quadrant phase shift.
    // Unsigned addition avoids signed overflow for arbitrary caller angles.
    const auto shifted = (static_cast<std::uint32_t>(angle) + 16384U) & 0xffffU;
    return sin_extended(static_cast<std::int32_t>(shifted));
}

float NinjaMath::sin(std::int32_t angle) const { return context_.to_float(sin_extended(angle)); }
float NinjaMath::cos(std::int32_t angle) const { return context_.to_float(cos_extended(angle)); }

X87Value inner_product_extended(const NinjaVector& first, const NinjaVector& second, X87Context& context) {
    // 0x787cf0: (z*z + y*y) + x*x, with distinct input vectors.
    const auto z = context.multiply_float_memory(context.from_float(first.z),second.z);
    const auto y = context.multiply_float_memory(context.from_float(first.y),second.y);
    const auto zy = context.add(z,y);
    const auto x = context.multiply_float_memory(context.from_float(first.x),second.x);
    return context.add(zy,x);
}

X87Value scalar_squared_extended(const NinjaVector& vector, X87Context& context) {
    // 0x787e10: (x*x + y*y) + z*z; unlike inner_product's summation order.
    const auto loaded_z=context.from_float(vector.z);
    const auto loaded_y=context.from_float(vector.y);
    const auto loaded_x=context.from_float(vector.x);
    const auto x=context.multiply(loaded_x,loaded_x);
    const auto y=context.multiply(loaded_y,loaded_y);
    const auto xy=context.add(x,y);
    const auto z=context.multiply(loaded_z,loaded_z);
    return context.add(xy,z);
}

X87Value signed_sqrt_extended(float value, X87Context& context) {
    // 0x7871f0 compares against double 0.0 and multiplies negatives by -1.0f
    // before fsqrt/fchs. Nonnegative and unordered values take plain fsqrt.
    const auto loaded = context.from_float(value);
    // FCOMP is signaling for quiet as well as signaling NaNs. Classifying
    // the source bits alone would lose its observable invalid exception.
    const auto comparison = context.compare_double_memory(loaded,0);
    const auto reloaded=context.from_float(value); // source pops compare operand, reloads m32
    if (comparison == X87Comparison::less) {
        const auto positive = context.multiply_float_memory(reloaded,-1.0F);
        return context.sqrt(positive).negated();
    }
    return context.sqrt(reloaded);
}

float inner_product(const NinjaVector& first, const NinjaVector& second, X87Context& context) {
    return context.to_float(inner_product_extended(first,second,context));
}
float scalar_squared(const NinjaVector& vector, X87Context& context) {
    return context.to_float(scalar_squared_extended(vector,context));
}
float signed_sqrt(float value, X87Context& context) {
    return context.to_float(signed_sqrt_extended(value,context));
}
X87Value cross_product_length_extended(const NinjaVector& a,const NinjaVector& b,
                                        NinjaVector& output,X87Context& fp) {
    const auto x1=fp.multiply_float_memory(fp.from_float(b.z),a.y);
    const auto x2=fp.multiply_float_memory(fp.from_float(b.y),a.z);
    const auto x=fp.subtract(x1,x2);
    const auto y1=fp.multiply_float_memory(fp.from_float(b.x),a.z);
    const auto y2=fp.multiply_float_memory(fp.from_float(b.z),a.x);
    const auto y=fp.subtract(y1,y2);
    const auto z1=fp.multiply_float_memory(fp.from_float(b.y),a.x);
    const auto z2=fp.multiply_float_memory(fp.from_float(b.x),a.y);
    const auto z=fp.subtract(z1,z2);
    output.x=fp.to_float(x);output.y=fp.to_float(y);output.z=fp.to_float(z);
    const auto zz=fp.multiply(z,z);const auto yy=fp.multiply(y,y);
    const auto zy=fp.add(zz,yy);const auto xx=fp.multiply(x,x);
    return signed_sqrt_extended(fp.to_float(fp.add(zy,xx)),fp);
}

X87Value normalize_vector_extended(NinjaVector& v,CoordinateAxisConstants c,X87Context& fp) {
    const auto length=fp.sqrt(scalar_squared_extended(v,fp));
    const auto comparison=fp.compare_float_memory(length,c.zero);
    if(comparison==X87Comparison::equal) {
        v.z=0;v.y=0;v.x=0;return length;
    }
    const auto inverse=fp.divide(fp.from_float(c.one),length);
    v.x=fp.to_float(fp.multiply_float_memory(inverse,v.x));
    v.y=fp.to_float(fp.multiply_float_memory(inverse,v.y));
    v.z=fp.to_float(fp.multiply_float_memory(inverse,v.z));return length;
}

} // namespace sadx
