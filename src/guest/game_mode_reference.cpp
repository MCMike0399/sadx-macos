#include "guest/game_mode_reference.hpp"
#include "game/game_mode_internal.hpp"
#include <map>
#include <stdexcept>
#include <string>

namespace sadx {
namespace {
using flow::FlowContext;
using flow::FlowPhase;
struct ReferenceStop {};
struct CallSite {
    FlowPhase phase;
    std::uint32_t callee, site;
    bool needs_return;
};
constexpr CallSite call_sites[] = {
    {FlowPhase::Start, 0x415000, 0x40c0b4, false},
    {FlowPhase::Start, 0x42cea0, 0x40c0b9, false},
    {FlowPhase::Start, 0x421f70, 0x40c0be, true},
    {FlowPhase::Start, 0x421f80, 0x40c0c7, true},
    {FlowPhase::NoSave, 0x4221e0, 0x40c0e6, false},
    {FlowPhase::NoSave, 0x4f88a0, 0x40c0ee, false},
    {FlowPhase::LogoInit, 0x42c7b0, 0x40c115, false},
    {FlowPhase::Logo, 0x42cc70, 0x40c124, true},
    {FlowPhase::LogoTitle, 0x411e30, 0x40c130, false},
    {FlowPhase::LogoMovie, 0x411e30, 0x40c14a, false},
    {FlowPhase::Title, 0x42e890, 0x40c16d, true},
    {FlowPhase::Title, 0x42f590, 0x40c1bd, false},
    {FlowPhase::Title, 0x40fdc0, 0x40c1c2, false},
    {FlowPhase::Title, 0x40ef40, 0x40c1c7, false},
    {FlowPhase::TitleAccepted, 0x4212e0, 0x40c17d, false},
    {FlowPhase::TitleAccepted, 0x413c00, 0x40c185, true},
    {FlowPhase::TitleAccepted, 0x40fdc0, 0x40c19b, false},
    {FlowPhase::TitleAccepted, 0x40ef40, 0x40c1a0, false},
    {FlowPhase::SelectPlayer, 0x42f240, 0x40c1d1, true},
    {FlowPhase::Action, 0x416610, 0x40c21a, true},
    {FlowPhase::ActionRestart, 0x40bf30, 0x40c1fb, false},
    {FlowPhase::ActionRestart, 0x40bf40, 0x40c200, false},
    {FlowPhase::ActionReturn, 0x40bf30, 0x40c244, false},
    {FlowPhase::ActionReturn, 0x40bf40, 0x40c249, false},
    {FlowPhase::ActionReturn, 0x42f080, 0x40c252, false},
    {FlowPhase::ActionReturn, 0x42caa0, 0x40c257, true},
    {FlowPhase::Adventure, 0x4173b0, 0x40c279, true},
    {FlowPhase::Adventure, 0x40bf30, 0x40c2d0, false},
    {FlowPhase::Adventure, 0x40bf40, 0x40c2d5, false},
    {FlowPhase::AdventureReturn, 0x40bf30, 0x40c2a3, false},
    {FlowPhase::AdventureReturn, 0x40bf40, 0x40c2a8, false},
    {FlowPhase::AdventureReturn, 0x42caa0, 0x40c2b1, true},
    {FlowPhase::Movie, 0x42f020, 0x40c2ef, true},
    {FlowPhase::Movie, 0x40bf30, 0x40c2ff, false},
    {FlowPhase::Movie, 0x40bf40, 0x40c304, false},
    {FlowPhase::Trial, 0x416610, 0x40c318, true},
    {FlowPhase::Trial, 0x40ef70, 0x40c350, false},
    {FlowPhase::TrialCancel, 0x40bf30, 0x40c32b, false},
    {FlowPhase::TrialCancel, 0x40bf40, 0x40c330, false},
    {FlowPhase::TrialCancel, 0x40ef70, 0x40c33b, false},
    {FlowPhase::TrialFinish, 0x40ef70, 0x40c38e, false},
    {FlowPhase::Mission, 0x415af0, 0x40c39b, true},
    {FlowPhase::MissionReturn, 0x40bf30, 0x40c3ef, false},
    {FlowPhase::MissionReturn, 0x40bf40, 0x40c3f4, false},
    {FlowPhase::MissionReturn, 0x40ef70, 0x40c410, false},
    {FlowPhase::TitleInit, 0x4228f0, 0x40c448, false},
    {FlowPhase::TitleInit, 0x4228f0, 0x40c44e, false},
    {FlowPhase::TitleInit, 0x4228f0, 0x40c455, false},
    {FlowPhase::TitleInit, 0x4228f0, 0x40c45c, false},
    {FlowPhase::TitleInit, 0x4228f0, 0x40c463, false},
    {FlowPhase::TitleInit, 0x4228f0, 0x40c46a, false},
    {FlowPhase::TitleInit, 0x4228f0, 0x40c471, false},
    {FlowPhase::TitleInit, 0x780fb0, 0x40c483, false},
    {FlowPhase::TitleInit, 0x420f40, 0x40c491, false},
    {FlowPhase::TitleInit, 0x420ef0, 0x40c496, false},
    {FlowPhase::TitleInit, 0x406f00, 0x40c49b, false},
    {FlowPhase::TitleInit, 0x5034a0, 0x40c4a0, false},
    {FlowPhase::TitleInit, 0x4221e0, 0x40c4ab, false},
    {FlowPhase::TitleResources, 0x5033f0, 0x40c4bc, false},
    {FlowPhase::TitleLoading, 0x5032e0, 0x40c4d2, false},
    {FlowPhase::TitleLoading, 0x77ed40, 0x40c4e1, false},
    {FlowPhase::TitleLoading, 0x424c80, 0x40c4fc, false},
    {FlowPhase::TitleMenu, 0x5033b0, 0x40c51f, true},
    {FlowPhase::TitleBack, 0x503380, 0x40c542, false},
    {FlowPhase::TitleBack, 0x42e780, 0x40c554, false},
    {FlowPhase::TitleTrial, 0x503380, 0x40c5a2, false},
    {FlowPhase::TitleTrial, 0x4212e0, 0x40c5ae, false},
    {FlowPhase::TitleTrial, 0x413c00, 0x40c5b6, false},
    {FlowPhase::TitlePlayer, 0x503380, 0x40c55f, false},
    {FlowPhase::TitlePlayer, 0x42c630, 0x40c564, false},
    {FlowPhase::TitlePlayer, 0x4212e0, 0x40c570, false},
    {FlowPhase::TitlePlayer, 0x4144e0, 0x40c575, true},
    {FlowPhase::TitlePlayer, 0x414470, 0x40c57b, false},
    {FlowPhase::TitlePlayer, 0x413c00, 0x40c583, false},
    {FlowPhase::TitlePlayer, 0x427ee0, 0x40c588, false},
    {FlowPhase::TitlePlayer, 0x427ec0, 0x40c58d, false},
    {FlowPhase::TitleSummary, 0x503380, 0x40c5cb, false},
    {FlowPhase::TitleMission, 0x503380, 0x40c613, false},
    {FlowPhase::TitleMission, 0x42c630, 0x40c618, false},
    {FlowPhase::TitleMission, 0x4144e0, 0x40c623, true},
    {FlowPhase::TitleMission, 0x42d770, 0x40c62a, false},
    {FlowPhase::TitleMission, 0x412ac0, 0x40c62f, false},
    {FlowPhase::TitleMission, 0x414470, 0x40c635, false},
    {FlowPhase::TitleMission, 0x413380, 0x40c63b, false},
    {FlowPhase::TitleMission, 0x4133e0, 0x40c64b, false},
    {FlowPhase::TitleStaff, 0x503380, 0x40c5fb, false},
    {FlowPhase::TitleTutorial, 0x503380, 0x40c5e3, false},
    {FlowPhase::TitleDemo, 0x42c820, 0x40c678, false},
    {FlowPhase::SummaryInit, 0x4221e0, 0x40c6a3, false},
    {FlowPhase::SummaryInit, 0x642640, 0x40c6a9, true},
    {FlowPhase::SummaryFallback, 0x427ee0, 0x40c6c4, false},
    {FlowPhase::SummaryFallback, 0x4144e0, 0x40c6c9, true},
    {FlowPhase::SummaryFallback, 0x42d770, 0x40c6d0, false},
    {FlowPhase::SummaryFallback, 0x412d20, 0x40c6dc, true},
    {FlowPhase::SummaryFallback, 0x42f020, 0x40c6ea, true},
    {FlowPhase::SummaryFallback, 0x412ac0, 0x40c6fa, false},
    {FlowPhase::SummaryFallback, 0x414470, 0x40c700, false},
    {FlowPhase::SummaryFallback, 0x413380, 0x40c706, false},
    {FlowPhase::SummaryFallback, 0x4133e0, 0x40c716, false},
    {FlowPhase::SummaryFallback, 0x4212e0, 0x40c726, false},
    {FlowPhase::SummaryUpdate, 0x6428d0, 0x40c73a, true},
    {FlowPhase::SummaryComplete, 0x4144e0, 0x40c74b, true},
    {FlowPhase::SummaryComplete, 0x427ee0, 0x40c752, false},
    {FlowPhase::SummaryComplete, 0x42d770, 0x40c757, false},
    {FlowPhase::SummaryComplete, 0x412ac0, 0x40c75c, false},
    {FlowPhase::SummaryComplete, 0x414470, 0x40c762, false},
    {FlowPhase::SummaryComplete, 0x413380, 0x40c768, false},
    {FlowPhase::SummaryComplete, 0x4133e0, 0x40c777, false},
    {FlowPhase::SummaryComplete, 0x4212e0, 0x40c787, false},
    {FlowPhase::SummaryComplete, 0x413bc0, 0x40c78f, true},
    {FlowPhase::TutorialInit, 0x4221e0, 0x40c7c5, false},
    {FlowPhase::TutorialInit, 0x642170, 0x40c7cb, false},
    {FlowPhase::TutorialUpdate, 0x641c60, 0x40c7df, true},
    {FlowPhase::TutorialMission, 0x42c630, 0x40c808, false},
    {FlowPhase::TutorialMission, 0x4144e0, 0x40c817, true},
    {FlowPhase::TutorialMission, 0x42d770, 0x40c81e, false},
    {FlowPhase::TutorialMission, 0x412ac0, 0x40c823, false},
    {FlowPhase::TutorialMission, 0x414470, 0x40c829, false},
    {FlowPhase::TutorialMission, 0x413380, 0x40c82f, false},
    {FlowPhase::TutorialMission, 0x4133e0, 0x40c83e, false},
    {FlowPhase::StaffInit, 0x4221e0, 0x40c8b7, false},
    {FlowPhase::StaffInit, 0x641a80, 0x40c8bd, false},
    {FlowPhase::StaffInit, 0x411e30, 0x40c8d9, false},
    {FlowPhase::StaffInit, 0x77ed40, 0x40c8e0, false},
    {FlowPhase::StaffUpdate, 0x640f20, 0x40c8e9, true},
    {FlowPhase::StaffUpdate, 0x42c730, 0x40c900, false},
    {FlowPhase::StaffReturn, 0x411e30, 0x40c915, false},
    {FlowPhase::StaffCancel, 0x411e30, 0x40c934, false},
    {FlowPhase::Backdrop, 0x402f10, 0x40c42c, false},
    {FlowPhase::Backdrop, 0x40bf40, 0x40c3b3, false},
    {FlowPhase::ClearToTitle, 0x40bf30, 0x40c3ae, false},
    {FlowPhase::ClearToTitle, 0x40bf40, 0x40c3b3, false},
    {FlowPhase::EnterTitle, 0x42ca10, 0x40c3bc, false},
    {FlowPhase::CancelTitle, 0x40bf30, 0x40c93c, false},
    {FlowPhase::CancelTitle, 0x40bf40, 0x40c941, false},
    {FlowPhase::CancelTitle, 0x42ca10, 0x40c946, false},
    {FlowPhase::BeginGameplay, 0x4212e0, 0x40c67e, false},
    {FlowPhase::BeginGameplay, 0x413c00, 0x40c686, true},
    {FlowPhase::ConfigureMission, 0x4144e0, 0x40c653, true},
    {FlowPhase::ConfigureMission, 0x414570, 0x40c84e, false},
    {FlowPhase::ConfigureMission, 0x4147f0, 0x40c857, false},
    {FlowPhase::ConfigureMission, 0x412c00, 0x40c85d, false},
    {FlowPhase::ConfigureMission, 0x4212e0, 0x40c863, false},
    {FlowPhase::ConfigureMission, 0x4144e0, 0x40c868, true},
    {FlowPhase::ConfigureMission, 0x414470, 0x40c86e, false},
    {FlowPhase::ConfigureMission, 0x413c00, 0x40c876, false},
    {FlowPhase::ConfigureMission, 0x427ee0, 0x40c87b, false},
    {FlowPhase::FrameTail, 0x785330, 0x40c980, false},
    {FlowPhase::FrameTail, 0x424320, 0x40c988, false},
    {FlowPhase::FrameTail, 0x590650, 0x40c995, true},
    {FlowPhase::FrameTail, 0x590660, 0x40c99e, false},
    {FlowPhase::FrameTail, 0x40d490, 0x40c9a4, false},
    {FlowPhase::FrameTail, 0x42c4f0, 0x40c9ac, false},
    {FlowPhase::FrameTail, 0x426050, 0x40c9b1, false},
};
enum class Field {
    mode,
    pause_state,
    save_phase,
    save_ready,
    message_resource,
    action_restart_requested,
    title_mode,
    title_load_phase,
    playing_super_sonic,
    tutorial_variant,
    alert_flag,
    alert_blocked,
    movie_number,
    menu_message_enabled,
    sequence_level
};
struct ReadSite {
    FlowPhase phase;
    Field field;
    std::uint32_t site;
};
constexpr ReadSite read_sites[] = {
    {FlowPhase::Dispatch, Field::mode, 0x40c091},
    {FlowPhase::Action, Field::action_restart_requested, 0x40c1e7},
    {FlowPhase::Action, Field::save_phase, 0x40c1ef},
    {FlowPhase::Movie, Field::movie_number, 0x40c2e9},
    {FlowPhase::TitleInit, Field::title_load_phase, 0x40c439},
    {FlowPhase::TitleInit, Field::message_resource, 0x40c476},
    {FlowPhase::TitleResources, Field::title_load_phase, 0x40c4b0},
    {FlowPhase::TitleLoading, Field::mode, 0x40c4e6},
    {FlowPhase::TitleLoading, Field::title_load_phase, 0x40c501},
    {FlowPhase::SummaryInit, Field::mode, 0x40c733},
    {FlowPhase::SummaryFallback, Field::sequence_level, 0x40c711},
    {FlowPhase::SummaryComplete, Field::sequence_level, 0x40c772},
    {FlowPhase::TitleMission, Field::sequence_level, 0x40c646},
    {FlowPhase::TutorialInit, Field::mode, 0x40c7d0},
    {FlowPhase::TutorialUpdate, Field::tutorial_variant, 0x40c7f0},
    {FlowPhase::TutorialMission, Field::sequence_level, 0x40c839},
    {FlowPhase::StaffInit, Field::mode, 0x40c8c2},
    {FlowPhase::FrameTail, Field::menu_message_enabled, 0x40c95f},
    {FlowPhase::FrameTail, Field::alert_flag, 0x40c967},
    {FlowPhase::FrameTail, Field::alert_blocked, 0x40c96f},
    {FlowPhase::FrameTail, Field::save_phase, 0x40c977},
    {FlowPhase::FrameTail, Field::mode, 0x40c98d},
};
struct Binding {
    GameModeRuntime &runtime;
    GameModeResult result{};
    std::map<std::uint32_t, std::size_t> call_counts;
};
thread_local Binding *active = nullptr;
Binding &binding() {
    if (!active)
        throw std::logic_error("game-flow reference binding is absent");
    return *active;
}
[[noreturn]] void fail(GameModeFailure failure, std::uint32_t site) {
    binding().result.failure = failure;
    binding().result.source_boundary = site;
    throw ReferenceStop{};
}
std::uint32_t read_site(FlowPhase phase, Field field) {
    for (const auto &row : read_sites) {
        if (row.phase == phase && row.field == field)
            return row.site;
    }
    throw std::logic_error("missing game-flow state provenance");
}
template <class T>
T require_fact(FlowContext &context, Field field, const std::optional<T> &value) {
    if (!value)
        fail(GameModeFailure::unknown_state, read_site(context.phase, field));
    return *value;
}
std::uint32_t invoke(FlowContext &context, std::uint32_t callee,
                     std::array<std::optional<std::uint32_t>, 3> args = {}) {
    auto &current = binding();
    const auto occurrence = current.call_counts[callee]++;
    std::size_t count = 0;
    const CallSite *match = nullptr;
    for (const auto &row : call_sites) {
        if (row.phase == context.phase && row.callee == callee && count++ == occurrence) {
            match = &row;
            break;
        }
    }
    if (!match)
        throw std::logic_error("missing game-flow call provenance: " + std::to_string(callee));
    auto &result = current.result;
    result.pending_call = GameModeCall{match->site, callee, args};
    if (!current.runtime.invoke)
        fail(GameModeFailure::missing_owner, match->site);
    const auto receipt = current.runtime.invoke(*result.pending_call);
    result.nested_source_boundary = receipt.owner.source_boundary;
    if (receipt.owner.state != GameLogicOwnerState::completed) {
        const auto failure =
            receipt.owner.state == GameLogicOwnerState::cancelled ? GameModeFailure::cancelled
            : receipt.owner.state == GameLogicOwnerState::failed  ? GameModeFailure::owner_failed
                                                                  : GameModeFailure::missing_owner;
        fail(failure, match->site);
    }
    if (match->needs_return && !receipt.return_value)
        fail(GameModeFailure::missing_return, match->site);
    result.pending_call.reset();
    result.nested_source_boundary = 0;
    // Ignored source returns may be absent; this value never drives a core decision at those sites.
    return receipt.return_value.value_or(0);
}
struct BindScope {
    Binding *previous;
    explicit BindScope(Binding &value) : previous(active) { active = &value; }
    ~BindScope() { active = previous; }
};
} // namespace
GameModeResult run_game_mode_40c090(GameModeRuntime &runtime) {
    Binding current{runtime, {}, {}};
    BindScope scope(current);
    flow::GameFlowState state{};
    try {
        flow::advance_game_mode(state);
        current.result.completed = true;
    } catch (const ReferenceStop &) {
    }
    return current.result;
}
namespace flow {
void enter_phase(FlowContext &context, FlowPhase phase) {
    context.phase = phase;
    binding().call_counts.clear();
}
GameMode read_mode(FlowContext &context) {
    const auto value = require_fact(context, Field::mode, binding().runtime.mode_at_3abdc7c);
    context.state.mode = static_cast<GameMode>(value);
    return context.state.mode;
}
void write_mode(FlowContext &context, GameMode value) {
    binding().runtime.mode_at_3abdc7c = static_cast<std::int32_t>(value);
    context.state.mode = value;
}
void write_pause_state(FlowContext &context, std::uint32_t value) {
    binding().runtime.loop.mode_at_3b28114 = value;
    context.state.pause_state = value;
}
std::int32_t read_save_phase(FlowContext &context) {
    const auto value =
        require_fact(context, Field::save_phase, binding().runtime.loop.save_phase_at_3b291a4);
    context.state.save_phase = value;
    return context.state.save_phase;
}
void write_save_ready(FlowContext &context, std::uint8_t value) {
    binding().runtime.loop.request_at_3b291ad = value;
    context.state.save_ready = value;
}
MenuResource read_message_resource(FlowContext &context) {
    const auto value =
        require_fact(context, Field::message_resource, binding().runtime.loop.word_at_3abdcf8);
    context.state.message_resource = MenuResource{value};
    return context.state.message_resource;
}
void write_message_resource(FlowContext &context, MenuResource value) {
    binding().runtime.loop.word_at_3abdcf8 = value.value;
    context.state.message_resource = value;
}
std::uint8_t read_action_restart_requested(FlowContext &context) {
    const auto value = require_fact(context, Field::action_restart_requested,
                                    binding().runtime.state.byte_at_3abdf74);
    context.state.action_restart_requested = value;
    return context.state.action_restart_requested;
}
void write_action_restart_requested(FlowContext &context, std::uint8_t value) {
    binding().runtime.state.byte_at_3abdf74 = value;
    context.state.action_restart_requested = value;
}
void write_title_mode(FlowContext &context, std::uint8_t value) {
    binding().runtime.state.byte_at_3abdcfc = value;
    context.state.title_mode = value;
}
std::uint8_t read_title_load_phase(FlowContext &context) {
    const auto value =
        require_fact(context, Field::title_load_phase, binding().runtime.state.byte_at_3abdcfd);
    context.state.title_load_phase = value;
    return context.state.title_load_phase;
}
void write_title_load_phase(FlowContext &context, std::uint8_t value) {
    binding().runtime.state.byte_at_3abdcfd = value;
    context.state.title_load_phase = value;
}
void write_playing_super_sonic(FlowContext &context, std::uint8_t value) {
    binding().runtime.state.byte_at_3b18db4 = value;
    context.state.playing_super_sonic = value;
}
std::uint8_t read_tutorial_variant(FlowContext &context) {
    const auto value =
        require_fact(context, Field::tutorial_variant, binding().runtime.state.byte_at_3b2a2fa);
    context.state.tutorial_variant = value;
    return context.state.tutorial_variant;
}
std::uint8_t read_alert_flag(FlowContext &context) {
    const auto value =
        require_fact(context, Field::alert_flag, binding().runtime.state.byte_at_3b22e1e);
    context.state.alert_flag = value;
    return context.state.alert_flag;
}
std::uint8_t read_alert_blocked(FlowContext &context) {
    const auto value =
        require_fact(context, Field::alert_blocked, binding().runtime.state.byte_at_3abdf78);
    context.state.alert_blocked = value;
    return context.state.alert_blocked;
}
std::uint32_t read_movie_number(FlowContext &context) {
    const auto value =
        require_fact(context, Field::movie_number, binding().runtime.state.word_at_3b2c45c);
    context.state.movie_number = value;
    return context.state.movie_number;
}
void write_movie_number(FlowContext &context, std::uint32_t value) {
    binding().runtime.state.word_at_3b2c45c = value;
    context.state.movie_number = value;
}
std::uint32_t read_menu_message_enabled(FlowContext &context) {
    const auto value =
        require_fact(context, Field::menu_message_enabled, binding().runtime.state.word_at_3b290e4);
    context.state.menu_message_enabled = value;
    return context.state.menu_message_enabled;
}
std::int16_t current_sequence_level(FlowContext &context) {
    return require_fact(context, Field::sequence_level,
                        binding().runtime.state.level_word_via_3b0ef4c);
}
void initialize_game(FlowContext &context) { invoke(context, 0x415000); }
void initialize_demo_state(FlowContext &context) { invoke(context, 0x42cea0); }
std::uint32_t startup_status(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x421f70));
}
std::int32_t startup_readiness(FlowContext &context) {
    return static_cast<std::int32_t>(invoke(context, 0x421f80));
}
void request_file_module(FlowContext &context, FlowModule module) {
    const std::uint32_t pointer = module == FlowModule::AlertWindow ? 0x7dcc20
                                  : module == FlowModule::Advertise ? 0x7dcc10
                                                                    : 0x7dcc04;
    invoke(context, 0x4221e0, {{pointer, 0U, {}}});
}
void on_edit(FlowContext &context) { invoke(context, 0x4f88a0); }
void initialize_sega_logo(FlowContext &context) { invoke(context, 0x42c7b0); }
std::uint32_t run_logos(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x42cc70));
}
void set_frame_rate_mode(FlowContext &context, std::int32_t mode, std::int32_t multiplier) {
    invoke(context, 0x411e30,
           {{static_cast<std::uint32_t>(mode), static_cast<std::uint32_t>(multiplier), {}}});
}
std::uint32_t run_title(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x42e890));
}
void load_regular_object_textures(FlowContext &context, std::int32_t kind) {
    invoke(context, 0x4212e0, {{static_cast<std::uint32_t>(kind), {}, {}}});
}
std::uint32_t initialize_level_game(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x413c00));
}
void initialize_player_selection(FlowContext &context) { invoke(context, 0x42f590); }
void get_switch_data(FlowContext &context) { invoke(context, 0x40fdc0); }
void enable_pad_reading(FlowContext &context) { invoke(context, 0x40ef40); }
std::int16_t run_player_selection(FlowContext &context) {
    return static_cast<std::int16_t>(invoke(context, 0x42f240));
}
void clear_game(FlowContext &context) { invoke(context, 0x40bf30); }
void reset_game(FlowContext &context) { invoke(context, 0x40bf40); }
std::int16_t run_action(FlowContext &context) {
    return static_cast<std::int16_t>(invoke(context, 0x416610));
}
void prepare_action_return(FlowContext &context) { invoke(context, 0x42f080); }
std::uint32_t check_return_mode(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x42caa0));
}
std::int16_t run_adventure(FlowContext &context) {
    return static_cast<std::int16_t>(invoke(context, 0x4173b0));
}
std::int32_t run_movie(FlowContext &context, std::uint32_t movie) {
    return static_cast<std::int32_t>(
        invoke(context, 0x42f020, {{static_cast<std::uint32_t>(movie), {}, {}}}));
}
void enable_player_pad_reading(FlowContext &context, std::uint32_t player) {
    invoke(context, 0x40ef70, {{static_cast<std::uint32_t>(player), {}, {}}});
}
std::int16_t run_mission(FlowContext &context) {
    return static_cast<std::int16_t>(invoke(context, 0x415af0));
}
void run_level_destructor(FlowContext &context, std::uint32_t kind) {
    invoke(context, 0x4228f0, {{static_cast<std::uint32_t>(kind), {}, {}}});
}
void free_message_resource(FlowContext &context, MenuResource resource) {
    invoke(context, 0x780fb0, {{resource.value, {}, {}}});
}
void free_regular_object_texlists(FlowContext &context) { invoke(context, 0x420f40); }
void free_character_texlists(FlowContext &context) { invoke(context, 0x420ef0); }
void release_all_textures(FlowContext &context) { invoke(context, 0x406f00); }
void prepare_title_assets(FlowContext &context) { invoke(context, 0x5034a0); }
void prepare_title_dialogs(FlowContext &context) { invoke(context, 0x5033f0); }
void initialize_title_menu(FlowContext &context) {
    invoke(context, 0x5032e0, {{0x3abdcfcU, {}, {}}});
}
void set_wait_vsync_count(FlowContext &context, std::int32_t count) {
    invoke(context, 0x77ed40, {{static_cast<std::uint32_t>(count), {}, {}}});
}
void load_level_sounds(FlowContext &context) { invoke(context, 0x424c80); }
std::int32_t run_title_menu(FlowContext &context) {
    return static_cast<std::int32_t>(invoke(context, 0x5033b0, {{0x3abdcfcU, {}, {}}}));
}
void close_title_menu(FlowContext &context, std::int32_t value) {
    invoke(context, 0x503380, {{static_cast<std::uint32_t>(value), {}, {}}});
}
void prepare_title(FlowContext &context) { invoke(context, 0x42e780); }
void start_screen_fade(FlowContext &context) { invoke(context, 0x42c630); }
std::uint32_t current_player_number(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x4144e0));
}
void setup_character(FlowContext &context, std::uint32_t character) {
    invoke(context, 0x414470, {{static_cast<std::uint32_t>(character), {}, {}}});
}
void reset_player_work(FlowContext &context) { invoke(context, 0x427ee0); }
void reset_player_parameters(FlowContext &context) { invoke(context, 0x427ec0); }
void load_save(FlowContext &context) { invoke(context, 0x42d770); }
void prepare_sequence(FlowContext &context) { invoke(context, 0x412ac0); }
void set_sequence_player(FlowContext &context, std::uint32_t character) {
    invoke(context, 0x413380, {{static_cast<std::uint32_t>(character), {}, {}}});
}
void apply_sequence_level(FlowContext &context, std::int16_t level) {
    invoke(context, 0x4133e0,
           {{static_cast<std::uint32_t>(static_cast<std::int32_t>(level)), {}, {}}});
}
void set_stage_number(FlowContext &context, std::int32_t level, std::int32_t act) {
    invoke(context, 0x414570,
           {{static_cast<std::uint32_t>(level), static_cast<std::uint32_t>(act), {}}});
}
void set_entrance_number(FlowContext &context, std::int32_t entrance) {
    invoke(context, 0x4147f0, {{static_cast<std::uint32_t>(entrance), {}, {}}});
}
void set_sequence_time(FlowContext &context, std::int32_t time) {
    invoke(context, 0x412c00, {{static_cast<std::uint32_t>(time), {}, {}}});
}
void go_demo_loop(FlowContext &context) { invoke(context, 0x42c820); }
std::uint32_t initialize_summary(FlowContext &context, std::int32_t value) {
    return static_cast<std::uint32_t>(
        invoke(context, 0x642640, {{static_cast<std::uint32_t>(value), {}, {}}}));
}
std::uint32_t check_sequence_flag(FlowContext &context, std::int32_t flag) {
    return static_cast<std::uint32_t>(
        invoke(context, 0x412d20, {{static_cast<std::uint32_t>(flag), {}, {}}}));
}
std::uint32_t run_summary(FlowContext &context, std::int32_t value) {
    return static_cast<std::uint32_t>(
        invoke(context, 0x6428d0, {{static_cast<std::uint32_t>(value), {}, {}}}));
}
std::uint32_t current_level_type(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x413bc0));
}
void initialize_tutorial(FlowContext &context, std::int32_t value) {
    invoke(context, 0x642170, {{static_cast<std::uint32_t>(value), {}, {}}});
}
std::uint32_t run_tutorial(FlowContext &context, std::int32_t value) {
    return static_cast<std::uint32_t>(
        invoke(context, 0x641c60, {{static_cast<std::uint32_t>(value), {}, {}}}));
}
void initialize_staff_roll(FlowContext &context, std::int32_t value) {
    invoke(context, 0x641a80, {{static_cast<std::uint32_t>(value), {}, {}}});
}
std::int32_t run_staff_roll(FlowContext &context, std::int32_t value) {
    return static_cast<std::int32_t>(
        invoke(context, 0x640f20, {{static_cast<std::uint32_t>(value), {}, {}}}));
}
void load_sega_logo(FlowContext &context) { invoke(context, 0x42c730); }
void set_background_color(FlowContext &context, Rgba top, Rgba middle, Rgba bottom) {
    const auto pack = [](Rgba color) {
        return (std::uint32_t(color.alpha) << 24U) | (std::uint32_t(color.red) << 16U) |
               (std::uint32_t(color.green) << 8U) | color.blue;
    };
    invoke(context, 0x402f10, {{pack(top), pack(middle), pack(bottom)}});
}
void prepare_title_initialization(FlowContext &context) { invoke(context, 0x42ca10); }
void stop_vibration(FlowContext &context, std::uint32_t port) {
    invoke(context, 0x785330, {{static_cast<std::uint32_t>(port), {}, {}}});
}
void pause_all_sound(FlowContext &context) { invoke(context, 0x424320); }
std::uint32_t mission_clear_status(FlowContext &context) {
    return static_cast<std::uint32_t>(invoke(context, 0x590650));
}
void release_mission_clear_texlist(FlowContext &context) { invoke(context, 0x590660); }
void display_message(FlowContext &context) { invoke(context, 0x40d490, {{0U, {}, {}}}); }
void draw_screen_fade(FlowContext &context) { invoke(context, 0x42c4f0); }
void increment_frame_counter(FlowContext &context) { invoke(context, 0x426050); }
} // namespace flow
} // namespace sadx
