#pragma once
#include "cpu/fidelity_float.hpp"
#include "engine/ninja_fog.hpp"

namespace sadx::fog {
// NOTSA: calculations and device submissions, shared with reference diagnostics.
// Arithmetic observations within a phase carry no source addresses into game code.
enum class Phase {
    Configure,
    Defaults,
    ReciprocalScale,
    ReciprocalEntry,
    ReadRange,
    FactorDifference,
    DepthDifference,
    NearDecision,
    NearSpan,
    ExponentialDecision,
    ExponentialDensity,
    NearThresholds,
    FarSpan,
    LinearRange,
    ClampStart,
    ClampEnd,
    FarThresholds,
    LinearNearSaturated,
    LinearNearThresholds,
    LinearFar,
    SoftwareNear,
    SoftwareNearSpan,
    SoftwareMean,
    SoftwareFarSpan,
    SoftwareDensity,
    SoftwareThresholds,
    Color,
    GenerateParameters,
    GenerateScale,
    GenerateSamples,
    NormalizeDecision,
    NormalizeReference,
    NormalizeGroupDenominator,
    NormalizeGroup,
    NormalizeTailDenominator,
    NormalizeTail,
    ScanLeading,
    ScanTrailing,
    PublishEdges,
    GameEnable,
    GameDisable,
    DisabledStart,
    DisabledEnd,
    DisabledUpdate,
    LevelParameters,
    LevelStart,
    LevelEnd,
    LevelColor,
    LevelRangeStart,
    LevelRangeEnd,
    LevelUpdate,
    Enable,
    Disable
};
// NOTSA: typed access seam. Reads occur when the game uses them; stores publish immediately.
// Native composition owns plain values. Only the reference adapter models missing fixture facts.
class Context : public fidelity::ExactObserver {
  public:
    explicit Context(X87Context &arithmetic) : arithmetic(arithmetic) {}
    X87Context &arithmetic;
    fidelity::ExactFloat number(float value) { return {arithmetic, value, this}; }
    fidelity::ExactFloat integer(std::int64_t value) {
        return fidelity::ExactFloat::integer(arithmetic, value, this);
    }
    fidelity::ExactFloat wide(double value) {
        return fidelity::ExactFloat::wide(arithmetic, value, this);
    }
    void before(fidelity::ExactEvent) override {}
    virtual void phase(Phase, unsigned = 0) {}
    virtual void device_load() = 0;
    virtual void require_device() = 0;
    virtual void require_table() = 0;
    virtual void render(RenderState, std::uint32_t) = 0;
    virtual void fail(ErrorKind kind) { throw Error(kind); }
    virtual std::uint32_t raster_caps() = 0;
    virtual std::uint32_t device_flags() = 0;
    virtual std::uint32_t color() = 0;
    virtual void color(std::uint32_t) = 0;
    virtual std::uint32_t start_index() = 0;
    virtual std::uint32_t end_index() = 0;
    virtual void start_index(std::uint32_t) = 0;
    virtual void end_index(std::uint32_t) = 0;
    virtual float start_factor() = 0;
    virtual float end_factor() = 0;
    virtual void start_factor(float) = 0;
    virtual void end_factor(float) = 0;
    virtual void software_density(std::uint32_t) = 0;
    virtual float depth(unsigned) = 0;
    virtual void depth(unsigned, float) = 0;
    virtual float reciprocal(unsigned) = 0;
    virtual void reciprocal(unsigned, float) = 0;
    virtual float sample(unsigned) = 0;
    virtual void sample(unsigned, float) = 0;
    virtual std::uint32_t enabled() = 0;
    virtual void enabled(std::uint32_t) = 0;
    virtual std::uint32_t supported() = 0;
    virtual void supported(std::uint32_t) = 0;
    virtual void mode(std::uint32_t) = 0;
    virtual std::int32_t display_switch() = 0;
    virtual std::uint8_t profile() = 0;
    virtual std::uint8_t special_flags() = 0;
    virtual float cached_start() = 0;
    virtual float cached_end() = 0;
    virtual void cached_start(float) = 0;
    virtual void cached_end(float) = 0;
    virtual void active(std::uint8_t) = 0;
    virtual float sentinel() = 0;
    virtual float level_start() = 0;
    virtual float level_end() = 0;
    virtual std::uint32_t level_color() = 0;
};
// FUNCTION: SONIC 0x794ec0 Direct3D_ConfigureFog
void initialize(Context &);
// FUNCTION: SONIC 0x7949b0 njSetFogColor_
void set_color(Context &, std::uint32_t);
// FUNCTION: SONIC 0x787260 njGenerateFogTable3
void generate_table(Context &, float start, float end);
// FUNCTION: SONIC 0x794e40 njSetFogTable_
void set_table(Context &);
// NOTSA: common game enable/disable entry; source routines retain their separate annotations below.
void set_enabled(Context &, bool);
} // namespace sadx::fog
