#include "engine/ninja_fog_internal.hpp"
#include <cmath>
#include <cstring>
#include <utility>

namespace sadx::fog {
namespace {
// The unchanged oracle covers 12 precision/rounding controls, retained exponents and NaNs.
// Explicit rounded() stores preserve the cases where a plain float expression diverges.
using Number = fidelity::ExactFloat;
// NOTSA: device parameters and exact bit-tested table entries use binary32 words.
std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
// NOTSA: ordered comparisons must preserve the source unordered branch.
bool less(X87Comparison comparison) { return comparison == X87Comparison::less; }
// NOTSA: source greater-than excludes unordered results.
bool greater(X87Comparison comparison) { return comparison == X87Comparison::greater; }
// NOTSA: source below-or-equal excludes unordered results.
bool less_equal(X87Comparison comparison) {
    return comparison == X87Comparison::less || comparison == X87Comparison::equal;
}
// NOTSA: one linear-fog submission preserves the three separate device calls.
void linear(Context &context, float start, float end, Phase phase) {
    context.phase(phase);
    context.render(RenderState::Mode, 3);
    context.render(RenderState::Start, bits(start));
    context.render(RenderState::End, bits(end));
}
// NOTSA: the two hardware fallback paths use the same recovered threshold policy.
std::pair<float, float> thresholds(Context &context, float end, Phase phase) {
    context.phase(phase);
    if (less(context.number(end).compare(0.3F)))
        return {65534.0F, 65535.0F};
    if (!less(context.number(0.3F).compare(end)))
        return {0.0F, 100.0F};
    return {0.0F, less(context.number(end).compare(0.6F)) ? 32767.0F : 100.0F};
}
// NOTSA: the software fallback publishes density bits rather than issuing device commands.
void software_thresholds(Context &context, float end) {
    context.phase(Phase::SoftwareThresholds);
    if (less(context.number(end).compare(0.3F))) {
        context.software_density(bits(1.0F));
        return;
    }
    if (!less(context.number(0.3F).compare(end))) {
        context.software_density(bits(0.3F));
        return;
    }
    context.software_density(bits(0.6F));
    if (!less(context.number(end).compare(0.6F)))
        context.software_density(bits(0.3F));
}
// NOTSA: the logarithmic branch retains m64 constants and the source's intermediate widths.
void exponential(Context &context, float start_factor, float end_factor, float start_depth,
                 float end_depth) {
    context.require_device();
    context.phase(Phase::ExponentialDensity);
    const auto sum = context.number(start_factor) + end_factor;
    const auto mean = sum * 0.5F;
    const auto complement = 1.0F - mean;
    const auto logarithm = complement.natural_log();
    const auto positive_logarithm = logarithm.multiply_double(-1.0);
    const auto root = positive_logarithm.square_root();
    const auto depth_sum = context.number(start_depth) + end_depth;
    const auto mean_depth = depth_sum * 0.5F;
    const auto reciprocal_depth = mean_depth.reciprocal_double(1.0);
    const auto density = (root * reciprocal_depth).rounded();
    context.render(RenderState::Mode, 2);
    context.render(RenderState::Density, bits(density));
}
} // namespace

namespace {
// NOTSA: evaluated range values shared by the hardware and software policies.
struct FogRange {
    float start_factor, end_factor, start_depth, end_depth, difference, depth_difference;
};
// NOTSA: hardware policy uses the stored binary32 difference.
void apply_hardware(Context &context, const FogRange &range) {
    context.phase(Phase::NearDecision);
    if (less(context.number(range.start_factor).compare(0.9F))) {
        context.phase(Phase::NearSpan);
        if (greater(context.number(range.difference).compare(1.0F))) {
            context.phase(Phase::ExponentialDecision);
            if (less_equal(context.number(0.5F).compare(range.end_factor)))
                linear(context, 0.0F, 100.0F, Phase::LinearNearSaturated);
            else
                exponential(context, range.start_factor, range.end_factor, range.start_depth,
                            range.end_depth);
        } else {
            const auto limits = thresholds(context, range.end_factor, Phase::NearThresholds);
            linear(context, limits.first, limits.second, Phase::LinearNearThresholds);
        }
        return;
    }
    context.phase(Phase::FarSpan);
    if (greater(context.number(range.difference).compare(0.0F))) {
        context.phase(Phase::LinearRange);
        const auto slope = context.number(range.depth_difference) / range.difference;
        const auto start_offset = -context.number(range.end_factor) * slope;
        float start = (start_offset + range.end_depth).rounded();
        const auto end_fraction = context.number(1.0F) - range.start_factor;
        const auto end_offset = end_fraction * slope;
        float end = (end_offset + range.start_depth).rounded();
        context.phase(Phase::ClampStart);
        if (less(context.number(start).compare(0.0F)))
            start = 0.0F;
        context.phase(Phase::ClampEnd);
        if (less(context.number(65535.0F).compare(end)))
            end = 65535.0F;
        linear(context, start, end, Phase::LinearFar);
    } else {
        const auto limits = thresholds(context, range.end_factor, Phase::FarThresholds);
        linear(context, limits.first, limits.second, Phase::LinearFar);
    }
}
// NOTSA: software interpolation retains the unspilled factor difference.
void apply_software(Context &context, const FogRange &range, const Number &retained_difference) {
    context.phase(Phase::SoftwareNear);
    if (less(context.number(range.start_factor).compare(0.9F))) {
        context.phase(Phase::SoftwareNearSpan);
        if (greater(context.number(range.difference).compare(1.0F))) {
            context.phase(Phase::SoftwareMean);
            const auto comparison = context.number(0.5F).compare(range.end_factor);
            context.software_density(bits(less_equal(comparison) ? 0.3F : 1.0F));
        } else {
            software_thresholds(context, range.end_factor);
        }
        return;
    }
    context.phase(Phase::SoftwareFarSpan);
    if (greater(retained_difference.compare(0.0F))) {
        context.phase(Phase::SoftwareDensity);
        const auto slope = context.number(range.depth_difference) / retained_difference;
        const auto fraction = context.number(1.0F) - range.start_factor;
        const auto offset = slope * fraction;
        const auto end = offset + range.start_depth;
        context.software_density(bits(less(end.compare(100.0F)) ? 0.3F : 1.0F));
    } else {
        software_thresholds(context, range.end_factor);
    }
}
} // namespace
// FUNCTION: SONIC 0x7949f0 Direct3D_ApplyFogTable
void apply_table(Context &context) {
    context.phase(Phase::ReadRange);
    const auto end_index = context.end_index();
    const auto end_depth_value = context.number(context.reciprocal(end_index));
    const auto start_index = context.start_index();
    const float start_factor = context.start_factor();
    const float end_depth = end_depth_value.rounded();
    const auto start_depth_value = context.number(context.reciprocal(start_index));
    const float end_factor = context.end_factor();
    const float start_depth = start_depth_value.rounded();
    context.phase(Phase::FactorDifference);
    const auto retained_difference = context.number(start_factor) - end_factor;
    const float difference = retained_difference.rounded();
    context.phase(Phase::DepthDifference);
    const float depth_difference = (context.number(start_depth) - end_depth).rounded();
    const FogRange range{start_factor, end_factor, start_depth,
                         end_depth,    difference, depth_difference};
    if (context.supported())
        apply_hardware(context, range);
    else
        apply_software(context, range, retained_difference);
}
// FUNCTION: SONIC 0x794db0 SetFogDensityMaybe
void update_reciprocals(Context &context, std::uint32_t parameter) {
    const auto shift = parameter & 31U;
    // Original signed FILD: shifting into bit 31 supplies a negative power.
    const auto power = static_cast<std::int32_t>(1U << shift);
    const auto fraction = static_cast<std::int32_t>(parameter & 65280U);
    context.phase(Phase::ReciprocalScale);
    const auto power_value = context.integer(power);
    const auto fraction_value = context.integer(fraction);
    const auto scaled_fraction = fraction_value * (1.0F / 65536.0F);
    const auto scale = 1.0F / (power_value * scaled_fraction);
    for (unsigned index = 0; index < 128; ++index) {
        context.phase(Phase::ReciprocalEntry, index);
        const float depth = context.depth(index);
        const auto product = scale * depth;
        const auto minimum = context.number(1.0F / 65536.0F);
        if (less(minimum.compare(product)))
            context.reciprocal(index, (context.number(1.0F) / product).rounded());
        else
            context.reciprocal(index, 65536.0F);
    }
    apply_table(context);
}

void initialize(Context &context) {
    context.phase(Phase::Configure);
    context.device_load();
    context.enabled(0);
    context.render(RenderState::Enabled, 0);
    context.supported((context.raster_caps() >> 20) & 1U);
    const auto flags = context.device_flags();
    bool software = true;
    if (flags & 32U)
        context.supported(0);
    else if (context.supported()) {
        software = false;
        context.phase(Phase::Defaults);
        context.render(RenderState::Color, 16777215);
        context.render(RenderState::Mode, 3);
        context.render(RenderState::Start, bits(0.0F));
        context.render(RenderState::End, bits(65535.0F));
    }
    if (software)
        context.software_density(bits(1.0F));
    context.color(16777215);
    context.end_index(126);
    context.start_index(127);
    // Exact powers and binary fractions reproduce the original pow table under every CW.
    for (unsigned index = 0; index < 128; ++index)
        context.depth(index, static_cast<float>(std::ldexp(1.0, int((index >> 4) & 15U)) *
                                                (double(index & 15U) + 16.0) * 0.0625));
    update_reciprocals(context, 32784);
}
void set_color(Context &context, std::uint32_t color) {
    context.phase(Phase::Color);
    color &= 16777215U;
    if (context.color() == color)
        return;
    const auto supported = context.supported();
    context.color(color);
    if (supported)
        context.render(RenderState::Color, color);
}

void generate_table(Context &context, float start, float end) {
    context.phase(Phase::GenerateParameters);
    std::uint32_t parameter = 32784;
    if (!less(context.number(start).compare(-65535.0F))) {
        const double negative_end = (-context.number(end)).rounded_double();
        if (!std::isfinite(negative_end))
            context.fail(ErrorKind::NonfiniteRange);
        int exponent = 0;
        const double mantissa = std::frexp(negative_end, &exponent);
        const auto scaled = context.wide(mantissa) * 65536.0F;
        const auto truncated = scaled.truncated_integer();
        const auto signed_exponent = static_cast<std::int32_t>(
            static_cast<std::int8_t>(static_cast<std::uint8_t>(exponent - 1)));
        parameter = static_cast<std::uint32_t>(signed_exponent) |
                    (static_cast<std::uint32_t>(truncated) & 65280U);
    }
    update_reciprocals(context, parameter);
    context.phase(Phase::GenerateScale);
    const auto scaled_end = context.number(end) * 0.00390625F;
    const bool beyond = greater(scaled_end.compare(start));
    const auto scale = beyond ? -context.number(end) : context.number(1.0F);
    const auto negative_start = -context.number(start);
    unsigned count = 0;
    for (unsigned index = 0; index < 128; ++index) {
        context.phase(Phase::GenerateSamples, index);
        const auto power = context.integer(std::int64_t{1} << (index >> 4));
        const auto low = context.integer(index & 15U);
        const auto sum = low + 16.0F;
        const auto product = power * sum;
        const auto depth = product * 0.0625F;
        const auto value = scale / depth;
        context.sample(index, value.rounded());
        if (less(negative_start.compare(value)))
            ++count;
    }
    context.phase(Phase::NormalizeDecision);
    const auto negative_end = -context.number(end);
    if (scale.quiet_compare(negative_end) != X87Comparison::equal)
        return;
    if (count >= 128)
        context.fail(ErrorKind::TableIndex);
    context.phase(Phase::NormalizeReference);
    const auto reference = context.number(context.sample(count));
    unsigned done = 0;
    // The original four-entry group and remainder have different operand loading order.
    if (count >= 4) {
        context.phase(Phase::NormalizeGroupDenominator);
        const auto denominator = context.number(end) + reference;
        done = (((count - 4) >> 2) + 1) * 4;
        for (unsigned index = 0; index < done; ++index) {
            context.phase(Phase::NormalizeGroup, index & 3U);
            const auto sum = context.number(end) + context.sample(index);
            const auto ratio = sum / denominator;
            context.sample(index, (1.0F - ratio).rounded());
        }
    }
    if (done < count) {
        context.phase(Phase::NormalizeTailDenominator);
        const auto denominator = reference + end;
        for (unsigned index = done; index < count; ++index) {
            context.phase(Phase::NormalizeTail, index);
            const auto sum = context.number(end) + context.sample(index);
            const auto ratio = sum / denominator;
            context.sample(index, (1.0F - ratio).rounded());
        }
    }
    for (unsigned index = count; index < 128; ++index)
        context.sample(index, 0.0F);
}
void set_table(Context &context) {
    context.phase(Phase::ScanLeading);
    unsigned start = 126;
    for (unsigned index = 0; index < 126; ++index) {
        if (bits(context.sample(index)) != bits(1.0F)) {
            start = index;
            break;
        }
    }
    unsigned end = start + 1;
    if (start != 126) {
        for (unsigned index = 126; index != start; --index) {
            context.phase(Phase::ScanTrailing, index);
            const auto value = context.number(context.sample(index));
            if (context.number(0.0F).quiet_compare(value) != X87Comparison::equal) {
                end = index + 1;
                break;
            }
        }
    }
    context.end_index(end);
    context.start_index(start);
    context.phase(Phase::PublishEdges, 0);
    context.end_factor(context.number(context.sample(end)).rounded());
    context.phase(Phase::PublishEdges, 1);
    context.start_factor(context.number(context.sample(start)).rounded());
    apply_table(context);
}
namespace {
// NOTSA: shared device switch; the two source paths use the same transition policy.
void device_switch(Context &context, bool enabled) {
    context.phase(enabled ? Phase::Enable : Phase::Disable);
    const auto prior = context.enabled();
    if ((enabled && prior == 0) || (!enabled && prior != 0)) {
        const auto supported = context.supported();
        context.enabled(enabled ? 1 : 0);
        if (supported)
            context.render(RenderState::Enabled, enabled ? 1 : 0);
    }
    context.mode(enabled ? 1 : 0);
}
// FUNCTION: SONIC 0x4119b0 ItDisablesFog
void disable_profile(Context &context) {
    context.phase(Phase::DisabledStart);
    const auto sentinel = context.number(context.sentinel());
    const auto cached_start = context.cached_start();
    bool changed = sentinel.compare(cached_start) != X87Comparison::equal;
    if (!changed) {
        context.phase(Phase::DisabledEnd);
        const auto end_sentinel = context.number(context.sentinel());
        const auto cached_end = context.cached_end();
        changed = end_sentinel.compare(cached_end) != X87Comparison::equal;
    }
    if (changed) {
        context.phase(Phase::DisabledUpdate);
        context.require_table();
        set_color(context, 0);
        generate_table(context, -65535.0F, -65535.0F);
        set_table(context);
        context.cached_start(-65535.0F);
        context.cached_end(-65535.0F);
        device_switch(context, false);
    }
    device_switch(context, false);
}
// FUNCTION: SONIC 0x411a20 UpdateFogTable
void update_level(Context &context) {
    context.phase(Phase::LevelParameters);
    float start = context.level_start(), end = context.level_end();
    const auto color = context.level_color();
    context.phase(Phase::LevelStart);
    if (greater(context.number(start).compare(0.0F)))
        start = (-context.number(start)).rounded();
    context.phase(Phase::LevelEnd);
    if (greater(context.number(end).compare(0.0F)))
        end = (-context.number(end)).rounded();
    context.phase(Phase::LevelColor);
    context.require_table();
    set_color(context, color);
    context.phase(Phase::LevelRangeStart);
    const auto cached_start = context.cached_start();
    bool changed = context.number(start).compare(cached_start) != X87Comparison::equal;
    if (!changed) {
        context.phase(Phase::LevelRangeEnd);
        const auto cached_end = context.cached_end();
        changed = context.number(end).compare(cached_end) != X87Comparison::equal;
    }
    if (changed) {
        context.phase(Phase::LevelUpdate);
        generate_table(context, start, end);
        set_table(context);
    }
    context.cached_start(start);
    context.cached_end(end);
    device_switch(context, true);
    context.active(1);
}
// FUNCTION: SONIC 0x411af0 ___njFogEnable
void enable_game(Context &context) {
    context.phase(Phase::GameEnable);
    if (context.display_switch())
        return;
    if (context.profile() == 0) {
        disable_profile(context);
        return;
    }
    context.phase(Phase::LevelParameters);
    if (!(context.special_flags() & 1U))
        update_level(context);
    else {
        device_switch(context, false);
        context.active(1);
    }
}
// FUNCTION: SONIC 0x411b40 ___njFogDisable
void disable_game(Context &context) {
    context.phase(Phase::GameDisable);
    if (context.display_switch())
        return;
    context.active(0);
    device_switch(context, false);
}
} // namespace
void set_enabled(Context &context, bool enabled) {
    if (enabled)
        enable_game(context);
    else
        disable_game(context);
}
} // namespace sadx::fog
