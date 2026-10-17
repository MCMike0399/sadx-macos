#include "guest/ninja_fog_reference.hpp"
#include "engine/ninja_fog_internal.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sadx {
namespace {
using fog::Phase;
struct FogStop {};
// Diagnostics describe the old source markers; engine code observes logical calculations only.
std::vector<std::uint32_t> arithmetic_sites(Phase phase, unsigned lane) {
    switch (phase) {
    case Phase::ReadRange:
        return {0x7949f8, 0x794a0a, 0x794a0e, 0x794a1b};
    case Phase::FactorDifference:
        return {0x794a31, 0x794a31, 0x794a34};
    case Phase::DepthDifference:
        return {0x794a3c, 0x794a3c, 0x794a3c};
    case Phase::NearDecision:
        return {0x794a58, 0x794a58};
    case Phase::NearSpan:
        return {0x794a6d, 0x794a6d};
    case Phase::ExponentialDecision:
        return {0x794a84, 0x794a84};
    case Phase::ExponentialDensity:
        return {0x794ade, 0x794ade, 0x794aec, 0x794af2, 0x794afc, 0x794afe, 0x794b04,
                0x794b06, 0x794b06, 0x794b0e, 0x794b14, 0x794b1a, 0x794b1c};
    case Phase::NearThresholds:
        return {0x794b42, 0x794b42, 0x794b67, 0x794b67, 0x794b7c, 0x794b7c};
    case Phase::FarSpan:
        return {0x794bd9, 0x794bd9};
    case Phase::LinearRange:
        return {0x794bee, 0x794bee, 0x794bf7, 0x794bf7, 0x794bf7, 0x794bf9,
                0x794bf9, 0x794c06, 0x794c06, 0x794c0a, 0x794c0c, 0x794c0c};
    case Phase::ClampStart:
        return {0x794c19, 0x794c19};
    case Phase::ClampEnd:
        return {0x794c33, 0x794c33};
    case Phase::FarThresholds:
        return {0x794c88, 0x794c88, 0x794ca4, 0x794ca4, 0x794cb1, 0x794cb1};
    case Phase::SoftwareNear:
        return {0x794cea, 0x794cea};
    case Phase::SoftwareNearSpan:
        return {0x794cfd, 0x794cfd};
    case Phase::SoftwareMean:
        return {0x794d10, 0x794d10};
    case Phase::SoftwareFarSpan:
        return {0x794d28};
    case Phase::SoftwareDensity:
        return {0x794d39, 0x794d39, 0x794d41, 0x794d41, 0x794d45, 0x794d47, 0x794d4b};
    case Phase::SoftwareThresholds:
        return {0x794d5f, 0x794d5f, 0x794d72, 0x794d72, 0x794d89, 0x794d89};
    case Phase::ReciprocalScale:
        return {0x794dc8, 0x794dd0, 0x794ddc, 0x794de9, 0x794deb};
    case Phase::ReciprocalEntry:
        return {0x794df3, 0x794dff, 0x794dff, 0x794e0e, 0x794e0e, 0x794e0e};
    case Phase::GenerateParameters:
        return {0x787266, 0x787266, 0x78728b, 0x78728b, 0x78728b, 0x787293, 0x787293, 0x78729c};
    case Phase::GenerateScale:
        return {0x7872c1, 0x7872c1, 0x7872ca, 0x7872ca, 0x7872ca, 0x7872ca, 0x7872ca};
    case Phase::GenerateSamples:
        return {0x78730d, 0x78730d, 0x787319, 0x78731f, 0x787321, 0x787327, 0x787329, 0x78732e};
    case Phase::NormalizeDecision:
        return {0x78752d, 0x78752d, 0x78752d};
    case Phase::NormalizeReference:
        return {0x78753a};
    case Phase::NormalizeGroupDenominator:
        return {0x78754e, 0x78754e};
    case Phase::NormalizeGroup: {
        const auto base = 0x787560 + 0x12 * lane;
        return {base, base, base + 3, base + 5, base + 0xb};
    }
    case Phase::NormalizeTailDenominator:
        return {0x7875ac};
    case Phase::NormalizeTail:
        return {0x7875bc, 0x7875bc, 0x7875bf, 0x7875c1, 0x7875c7};
    case Phase::ScanTrailing:
        return {0x794e70, 0x794e79, 0x794e79};
    case Phase::PublishEdges:
        return lane ? std::vector<std::uint32_t>{0x794ea1, 0x794ea1}
                    : std::vector<std::uint32_t>{0x794e98, 0x794e98};
    case Phase::DisabledStart:
        return {0x4119b0, 0x4119b6};
    case Phase::DisabledEnd:
        return {0x4119c3, 0x4119c9};
    case Phase::LevelStart:
        return {0x411a4d, 0x411a4d, 0x411a60, 0x411a60, 0x411a60};
    case Phase::LevelEnd:
        return {0x411a68, 0x411a68, 0x411a7b, 0x411a7b, 0x411a7b};
    case Phase::LevelRangeStart:
        return {0x411a89, 0x411a89};
    case Phase::LevelRangeEnd:
        return {0x411a9d, 0x411a9d};
    default:
        return {};
    }
}
// Event signatures guard diagnostic mappings against a reordered arithmetic expression.
std::string_view arithmetic_signature(Phase phase) {
    switch (phase) {
    case Phase::ReadRange:
        return "lsls";
    case Phase::FactorDifference:
    case Phase::DepthDifference:
        return "l-s";
    case Phase::NearDecision:
    case Phase::NearSpan:
    case Phase::ExponentialDecision:
    case Phase::FarSpan:
    case Phase::ClampStart:
    case Phase::ClampEnd:
    case Phase::SoftwareNear:
    case Phase::SoftwareNearSpan:
    case Phase::SoftwareMean:
    case Phase::DisabledStart:
    case Phase::DisabledEnd:
    case Phase::LevelRangeStart:
    case Phase::LevelRangeEnd:
        return "lc";
    case Phase::NearThresholds:
    case Phase::FarThresholds:
    case Phase::SoftwareThresholds:
        return "lclclc";
    case Phase::ExponentialDensity:
        return "l+*-g*rl+*/*s";
    case Phase::LinearRange:
        return "l/ln*+sl-*+s";
    case Phase::SoftwareFarSpan:
        return "c";
    case Phase::SoftwareDensity:
        return "l/l-*+c";
    case Phase::ReciprocalScale:
        return "ll**/";
    case Phase::ReciprocalEntry:
        return "*lcl/s";
    case Phase::GenerateParameters:
        return "lclnsl*i";
    // The conditional negation has no intervening source marker; either branch uses this site.
    case Phase::GenerateScale:
        return "l*cl???";
    case Phase::GenerateSamples:
        return "ll+**/sc";
    case Phase::NormalizeDecision:
        return "lnc";
    case Phase::NormalizeReference:
        return "l";
    case Phase::NormalizeGroupDenominator:
        return "l+";
    case Phase::NormalizeGroup:
    case Phase::NormalizeTail:
        return "l+/-s";
    case Phase::NormalizeTailDenominator:
        return "+";
    case Phase::ScanTrailing:
        return "llc";
    case Phase::PublishEdges:
        return "ls";
    case Phase::LevelStart:
    case Phase::LevelEnd:
        return "lclns";
    default:
        return "";
    }
}
class ReferenceContext final : public fog::Context {
  public:
    NinjaFogResult result;
    ReferenceContext(NinjaFogDeviceOwners &owners, X87Context &fp,
                     const NinjaFogDeviceFacts *facts = nullptr,
                     const NinjaFogTable *table = nullptr, NinjaFogTable *writable = nullptr)
        : Context(fp), state_(owners.state), device_(&owners.device), render_(owners.render),
          facts_(facts), table_(table), writable_(writable) {}
    ReferenceContext(NinjaFogOwners &owners, X87Context &fp)
        : Context(fp), state_(owners.state), device_(owners.device), render_(owners.render),
          game_(&owners), table_(owners.table_at_3b10f78), writable_(owners.table_at_3b10f78) {}
    template <class Function> NinjaFogResult run(Function function) {
        try {
            function();
        } catch (FogStop) {
        } catch (const X87Error &error) {
            result.failure = NinjaFogFailure::fp_failed;
            result.fp_failure = error.failure();
            result.fp_exceptions = error.exceptions();
            result.source_boundary = result.last_fp_site.value_or(0);
        }
        return std::move(result);
    }
    void phase(Phase phase, unsigned lane = 0) override {
        phase_ = phase;
        lane_ = lane;
        arithmetic_ = arithmetic_sites(phase, lane);
        signature_ = arithmetic_signature(phase);
        if (signature_.size() != arithmetic_.size())
            throw std::logic_error("invalid fog diagnostic signature");
        arithmetic_index_ = 0;
        render_index_ = 0;
        reciprocal_reads_ = 0;
        // These source FLD markers precede an optional fact load, even if that fact is missing.
        if (phase == Phase::NormalizeReference)
            site(0x78753a);
        if (phase == Phase::ScanTrailing)
            site(0x794e70);
        if (phase == Phase::PublishEdges)
            site(lane ? 0x794ea1 : 0x794e98);
    }
    void before(fidelity::ExactEvent event) override {
        if (arithmetic_index_ >= arithmetic_.size())
            throw std::logic_error("unexpected fog arithmetic in phase " +
                                   std::to_string(unsigned(phase_)));
        constexpr char codes[] = {'l', 's', 'n', '+', '-', '*', '/', 'c', 'i', 'g', 'r'};
        const auto expected = signature_[arithmetic_index_];
        if (expected != '?' && expected != codes[unsigned(event)])
            throw std::logic_error("reordered fog arithmetic in phase " +
                                   std::to_string(unsigned(phase_)));
        site(arithmetic_[arithmetic_index_++]);
    }
    void device_load() override {
        if (!render_)
            stop(NinjaFogFailure::render_failed, 0x794ec1);
        if (!render_->device_at_3d128b0) {
            result.render.failure = NinjaRenderFailure::unknown_device;
            result.render.source_boundary = 0x794ec1;
            stop(NinjaFogFailure::render_failed, 0x794ec1);
        }
    }
    void require_device() override {
        if (!render_)
            stop(NinjaFogFailure::render_failed, 0x794a8c);
        if (!render_->device_at_3d128b0) {
            result.render.failure = NinjaRenderFailure::unknown_device;
            result.render.source_boundary = 0x794a8c;
            stop(NinjaFogFailure::render_failed, 0x794a8c);
        }
        if (!*render_->device_at_3d128b0) {
            result.render.failure = NinjaRenderFailure::null_device;
            result.render.source_boundary = 0x794ae2;
            stop(NinjaFogFailure::render_failed, 0x794ae2);
        }
    }
    void require_table() override {
        if (!device_ || !table_ || !writable_)
            stop(NinjaFogFailure::unlifted_callee,
                 phase_ == Phase::DisabledUpdate ? 0x4119d8 : 0x411a80, 0x787240);
    }
    void render(fog::RenderState state, std::uint32_t value) override {
        const auto sites = render_sites();
        if (!render_)
            stop(NinjaFogFailure::render_failed, sites.device_load);
        if (!ninja_issue_render_state(result.render, *render_, std::uint32_t(state), value, sites))
            stop(NinjaFogFailure::render_failed, result.render.source_boundary);
    }
    void fail(fog::ErrorKind kind) override {
        if (kind == fog::ErrorKind::NonfiniteRange)
            stop(NinjaFogFailure::unmodeled_operation, 0x78728e, 0x7b5eef);
        if (kind == fog::ErrorKind::TableIndex)
            stop(NinjaFogFailure::index_out_of_range, 0x78753a);
        throw std::logic_error("unexpected native fog failure");
    }
    std::uint32_t raster_caps() override {
        return need(facts_->raster_caps_from_3d127e4, 0x794edb, 0x3d127e4);
    }
    std::uint32_t device_flags() override {
        return need(facts_->flags_from_3d08ae0, 0x795930, 0x3d08ae0);
    }
    std::uint32_t color() override { return need(device_->color_at_3d12898, 0x7949b4, 0x3d12898); }
    void color(std::uint32_t value) override { device_->color_at_3d12898 = value; }
    std::uint32_t start_index() override {
        return need(device_->start_index_at_3d12950, 0x7949ff, 0x3d12950);
    }
    void start_index(std::uint32_t value) override { device_->start_index_at_3d12950 = value; }
    std::uint32_t end_index() override {
        return need(device_->end_index_at_3d0fce0, 0x7949f3, 0x3d0fce0);
    }
    void end_index(std::uint32_t value) override { device_->end_index_at_3d0fce0 = value; }
    float start_factor() override {
        return need(device_->start_value_at_3d0fac0, 0x794a05, 0x3d0fac0);
    }
    void start_factor(float value) override { device_->start_value_at_3d0fac0 = value; }
    float end_factor() override { return need(device_->end_value_at_3d12894, 0x794a15, 0x3d12894); }
    void end_factor(float value) override { device_->end_value_at_3d12894 = value; }
    void software_density(std::uint32_t value) override {
        device_->software_density_at_3d128a0 = value;
    }
    float depth(unsigned index) override {
        return need(device_->w_at_3d12a00[index], 0x794df3, 0x3d12a00 + 4 * index);
    }
    void depth(unsigned index, float value) override { device_->w_at_3d12a00[index] = value; }
    float reciprocal(unsigned index) override {
        const auto source = reciprocal_reads_++ ? 0x794a0e : 0x7949f8;
        site(source);
        if (index >= device_->reciprocal_at_3d12c00.size())
            stop(NinjaFogFailure::index_out_of_range, source);
        return need(device_->reciprocal_at_3d12c00[index], source, 0x3d12c00 + 4 * index);
    }
    void reciprocal(unsigned index, float value) override {
        device_->reciprocal_at_3d12c00[index] = value;
    }
    float sample(unsigned index) override {
        std::uint32_t source = 0;
        switch (phase_) {
        case Phase::ScanLeading:
            source = 0x794e50;
            break;
        case Phase::ScanTrailing:
            source = 0x794e70;
            break;
        case Phase::PublishEdges:
            source = lane_ ? 0x794ea1 : 0x794e98;
            break;
        case Phase::NormalizeReference:
            source = 0x78753a;
            break;
        case Phase::NormalizeGroup:
            source = 0x787560 + 0x12 * lane_;
            break;
        case Phase::NormalizeTail:
            source = 0x7875bc;
            break;
        default:
            throw std::logic_error("unexpected fog sample read");
        }
        return need((*table_)[index], source, 0x3b10f78 + 4 * index);
    }
    void sample(unsigned index, float value) override { (*writable_)[index] = value; }
    std::uint32_t enabled() override {
        return need(state_.fog_enabled_at_3d12798, phase_ == Phase::Enable ? 0x794950 : 0x794980,
                    0x3d12798);
    }
    void enabled(std::uint32_t value) override { state_.fog_enabled_at_3d12798 = value; }
    std::uint32_t supported() override {
        const auto source = phase_ == Phase::Enable    ? 0x794959
                            : phase_ == Phase::Disable ? 0x794989
                            : phase_ == Phase::Color   ? 0x7949c3
                                                       : 0x794a27;
        return need(state_.fog_supported_at_3d128a4, source, 0x3d128a4);
    }
    void supported(std::uint32_t value) override { state_.fog_supported_at_3d128a4 = value; }
    void mode(std::uint32_t value) override { state_.fog_mode_at_3d0849c = value; }
    std::int32_t display_switch() override {
        const auto source = phase_ == Phase::GameEnable ? 0x411af0 : 0x411b40;
        if (!game_->display_switch_at_3b1117c) {
            result.required_fact = 0x3b1117c;
            stop(NinjaFogFailure::unknown_fact, source);
        }
        return need(*game_->display_switch_at_3b1117c, source, 0x3b1117c);
    }
    std::uint8_t profile() override {
        return need(state_.fog_profile_at_3abdc6c, 0x411af9, 0x3abdc6c);
    }
    std::uint8_t special_flags() override {
        return need(state_.special_flags_at_909eb4, 0x411a20, 0x909eb4);
    }
    float cached_start() override {
        return need(state_.cached_fog_start_at_90a098,
                    phase_ == Phase::DisabledStart ? 0x4119b6 : 0x411a89, 0x90a098);
    }
    void cached_start(float value) override { state_.cached_fog_start_at_90a098 = value; }
    float cached_end() override {
        return need(state_.cached_fog_end_at_90a09c,
                    phase_ == Phase::DisabledEnd ? 0x4119c9 : 0x411a9d, 0x90a09c);
    }
    void cached_end(float value) override { state_.cached_fog_end_at_90a09c = value; }
    void active(std::uint8_t value) override { game_->flag_at_3abdcfe = value; }
    float sentinel() override { return game_->fog_sentinel_at_7dce50; }
    float level_start() override {
        if (!game_->level_start_at_3abdc60 || !game_->level_end_at_3abdc64 ||
            !game_->level_color_at_3abdc68)
            stop(NinjaFogFailure::unlifted_callee, 0x411b02, 0x411a20);
        return need(*game_->level_start_at_3abdc60, 0x411a30, 0x3abdc60);
    }
    float level_end() override { return need(*game_->level_end_at_3abdc64, 0x411a35, 0x3abdc64); }
    std::uint32_t level_color() override {
        return need(*game_->level_color_at_3abdc68, 0x411a3b, 0x3abdc68);
    }

  private:
    NinjaQueueConsumerState &state_;
    NinjaFogDeviceState *device_;
    NinjaRenderState *render_;
    const NinjaFogDeviceFacts *facts_ = nullptr;
    NinjaFogOwners *game_ = nullptr;
    const NinjaFogTable *table_ = nullptr;
    NinjaFogTable *writable_ = nullptr;
    Phase phase_ = Phase::Configure;
    unsigned lane_ = 0, arithmetic_index_ = 0, render_index_ = 0, reciprocal_reads_ = 0;
    std::vector<std::uint32_t> arithmetic_;
    std::string_view signature_;
    void site(std::uint32_t source) { result.last_fp_site = source; }
    [[noreturn]] void stop(NinjaFogFailure failure, std::uint32_t source,
                           std::uint32_t callee = 0) {
        result.failure = failure;
        result.source_boundary = source;
        result.callee_boundary = callee;
        throw FogStop{};
    }
    template <class T>
    T need(const std::optional<T> &value, std::uint32_t source, std::uint32_t fact) {
        if (!value) {
            result.required_fact = fact;
            stop(NinjaFogFailure::unknown_fact, source);
        }
        return *value;
    }
    NinjaRenderSourceSites render_sites() {
        const auto index = render_index_++;
        switch (phase_) {
        case Phase::Configure:
            return {0x794ec1, 0x794ed2, 0x794ed5};
        case Phase::Defaults: {
            static constexpr NinjaRenderSourceSites sites[] = {{0x794f92, 0x794f97, 0x794fa1},
                                                               {0x794fa7, 0x794fac, 0x794fb3},
                                                               {0x794fb9, 0x794fbe, 0x794fd0},
                                                               {0x794fd6, 0x794fdb, 0x794fed}};
            return sites[index];
        }
        case Phase::Color:
            return {0x7949d2, 0x7949d8, 0x7949de};
        case Phase::ExponentialDensity:
            return index ? NinjaRenderSourceSites{0x794b2a, 0x794b2f, 0x794b35}
                         : NinjaRenderSourceSites{0x794a8c, 0x794ae2, 0x794b20};
        case Phase::LinearNearSaturated:
            if (!index)
                return {0x794a8c, 0x794a93, 0x794aaa};
            return linear_site(index == 1 ? 0x794ab4 : 0x794ac9);
        case Phase::LinearNearThresholds:
            if (!index)
                return {0x794b99, 0x794b9e, 0x794ba5};
            return linear_site(index == 1 ? 0x794baf : 0x794bc4);
        case Phase::LinearFar:
            if (!index)
                return {0x794c46, 0x794c4b, 0x794c52};
            return linear_site(index == 1 ? 0x794c5b : 0x794c70);
        case Phase::Enable:
            return {0x79496c, 0x794971, 0x794978};
        case Phase::Disable:
            return {0x79499c, 0x7949a1, 0x7949a8};
        default:
            throw std::logic_error("unexpected fog render submission");
        }
    }
    static NinjaRenderSourceSites linear_site(std::uint32_t load) {
        return {load, load + 5, load + 0xb};
    }
};
} // namespace
NinjaFogResult ninja_fog_initialize_794ec0(NinjaFogDeviceOwners &owners,
                                           const NinjaFogDeviceFacts &facts, X87Context &fp) {
    ReferenceContext context(owners, fp, &facts);
    return context.run([&] { fog::initialize(context); });
}
NinjaFogResult ninja_fog_color_7949b0(std::uint32_t color, NinjaFogDeviceOwners &owners) {
    X87Context fp(0x7f);
    ReferenceContext context(owners, fp);
    return context.run([&] { fog::set_color(context, color); });
}
NinjaFogResult ninja_fog_generate_table_787260(NinjaFogTable &table, float start, float end,
                                               NinjaFogDeviceOwners &owners, X87Context &fp) {
    ReferenceContext context(owners, fp, nullptr, &table, &table);
    return context.run([&] { fog::generate_table(context, start, end); });
}
NinjaFogResult ninja_fog_set_table_794e40(const NinjaFogTable &table, NinjaFogDeviceOwners &owners,
                                          X87Context &fp) {
    ReferenceContext context(owners, fp, nullptr, &table);
    return context.run([&] { fog::set_table(context); });
}
NinjaFogResult ninja_fog_411af0_411b40(bool enabled, NinjaFogOwners &owners, X87Context &fp) {
    ReferenceContext context(owners, fp);
    return context.run([&] { fog::set_enabled(context, enabled); });
}
// Only composition and diagnostics leave the queue consumer; its game decisions stay in engine.
bool update_queue_fog(bool enabled, NinjaQueueConsumerRuntime &runtime, X87Context &fp,
                      NinjaQueueConsumerResult &result, std::uint32_t &last_fp_site) {
    NinjaFogOwners owners{runtime.state,
                          runtime.queue_runtime.flag_at_3abdcfe,
                          runtime.queue_runtime.display_switch_at_3b1117c,
                          runtime.queue_runtime.render_state,
                          runtime.constants.fog_sentinel_at_7dce50,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr};
    auto fog = ninja_fog_411af0_411b40(enabled, owners, fp);
    if (fog.last_fp_site)
        last_fp_site = *fog.last_fp_site;
    auto &all = result.render;
    all.commands.insert(all.commands.end(), fog.render.commands.begin(), fog.render.commands.end());
    all.ignored_hresults.insert(all.ignored_hresults.end(), fog.render.ignored_hresults.begin(),
                                fog.render.ignored_hresults.end());
    result.source_boundary = fog.source_boundary;
    result.callee_boundary = fog.callee_boundary;
    switch (fog.failure) {
    case NinjaFogFailure::none:
        return true;
    case NinjaFogFailure::unknown_fact:
        result.required_fact = fog.required_fact;
        result.failure = NinjaQueueConsumerFailure::unknown_fact;
        break;
    case NinjaFogFailure::unlifted_callee:
        result.failure = NinjaQueueConsumerFailure::unlifted_callee;
        break;
    case NinjaFogFailure::render_failed:
        result.failure = NinjaQueueConsumerFailure::render_failed;
        if (!runtime.queue_runtime.render_state) {
            result.source_boundary = 0x77ed04;
            break;
        }
        all.failure = fog.render.failure;
        all.source_boundary = fog.render.source_boundary;
        all.pending_command = fog.render.pending_command;
        break;
    case NinjaFogFailure::fp_failed:
        throw X87Error(*fog.fp_failure, fog.fp_exceptions, "fog");
    case NinjaFogFailure::unmodeled_operation:
    case NinjaFogFailure::index_out_of_range:
        result.failure = NinjaQueueConsumerFailure::unlifted_callee;
        break;
    }
    return false;
}
} // namespace sadx
