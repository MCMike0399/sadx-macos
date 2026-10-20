#pragma once
#include "game/game_mode_services.hpp"

namespace sadx::flow {
// NOTSA: logical game-flow steps also identify reference diagnostics and future observers.
enum class FlowPhase {
    Dispatch,
    Start,
    NoSave,
    LogoInit,
    Logo,
    LogoTitle,
    LogoMovie,
    Title,
    TitleAccepted,
    SelectPlayer,
    Action,
    ActionRestart,
    ActionReturn,
    Adventure,
    AdventureReturn,
    Movie,
    Trial,
    TrialCancel,
    TrialFinish,
    Mission,
    MissionReturn,
    TitleInit,
    TitleResources,
    TitleLoading,
    TitleMenu,
    TitleBack,
    TitleTrial,
    TitleSummary,
    TitleStaff,
    TitleTutorial,
    TitlePlayer,
    TitleDemo,
    TitleMission,
    SummaryInit,
    SummaryFallback,
    SummaryUpdate,
    SummaryComplete,
    TutorialInit,
    TutorialUpdate,
    TutorialMission,
    StaffInit,
    StaffUpdate,
    StaffReturn,
    StaffCancel,
    Backdrop,
    ClearToTitle,
    EnterTitle,
    CancelTitle,
    BeginGameplay,
    ConfigureMission,
    FrameTail,
};
enum class FlowModule { AlertWindow, Advertise, Summary };
struct Rgba {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;
};
// NOTSA: one invocation context; no source addresses or fixture availability lives here.
struct FlowContext {
    GameFlowState &state;
    FlowPhase phase = FlowPhase::Dispatch;
    NativeServices *native = nullptr;
};
// NOTSA: enter a logical flow step, resetting adapter-local provenance counters.
void enter_phase(FlowContext &, FlowPhase);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
GameMode read_mode(FlowContext &);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_mode(FlowContext &, GameMode);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_pause_state(FlowContext &, std::uint32_t);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::int32_t read_save_phase(FlowContext &);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_save_ready(FlowContext &, std::uint8_t);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
MenuResource read_message_resource(FlowContext &);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_message_resource(FlowContext &, MenuResource);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::uint8_t read_action_restart_requested(FlowContext &);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_action_restart_requested(FlowContext &, std::uint8_t);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_title_mode(FlowContext &, std::uint8_t);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::uint8_t read_title_load_phase(FlowContext &);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_title_load_phase(FlowContext &, std::uint8_t);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_playing_super_sonic(FlowContext &, std::uint8_t);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::uint8_t read_tutorial_variant(FlowContext &);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::uint8_t read_alert_flag(FlowContext &);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::uint8_t read_alert_blocked(FlowContext &);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::uint32_t read_movie_number(FlowContext &);
// NOTSA: publishes an actual store immediately, including same-value stores.
void write_movie_number(FlowContext &, std::uint32_t);
// NOTSA: semantic state access; the reference adapter validates missing facts lazily.
std::uint32_t read_menu_message_enabled(FlowContext &);
// NOTSA: the active sequence can be absent; validate it at the actual level use.
std::int16_t current_sequence_level(FlowContext &);
// NOTSA: composition seam for bootstrap::start.
void initialize_game(FlowContext &flow);
// NOTSA: composition seam for bootstrap::initialize_advertise.
void initialize_demo_state(FlowContext &flow);
// NOTSA: composition seam for bootstrap::loading_status.
std::uint32_t startup_status(FlowContext &flow);
// NOTSA: composition seam for bootstrap::loading_ready.
std::int32_t startup_readiness(FlowContext &flow);
// STUB: SONIC 0x4221e0 FUN_004221e0
void request_file_module(FlowContext &flow, FlowModule module);
// STUB: SONIC 0x4f88a0 OnEdit
void on_edit(FlowContext &flow);
// NOTSA: composition seam for logo::initialize_sega.
void initialize_sega_logo(FlowContext &flow);
// NOTSA: composition seam for the recovered logo update; native wiring is still incomplete.
std::uint32_t run_logos(FlowContext &flow);
// NOTSA: composition seam for recovered dsInitInt; its FUNCTION belongs to frame control.
void set_frame_rate_mode(FlowContext &flow, std::int32_t mode, std::int32_t multiplier);
// STUB: SONIC 0x42e890 FUN_0042e890
std::uint32_t run_title(FlowContext &flow);
// STUB: SONIC 0x4212e0 LoadRegObjTextures
void load_regular_object_textures(FlowContext &flow, std::int32_t kind);
// NOTSA: composition seam for bootstrap::initialize_game.
std::uint32_t initialize_level_game(FlowContext &flow);
// STUB: SONIC 0x42f590 FUN_0042f590
void initialize_player_selection(FlowContext &flow);
// STUB: SONIC 0x40fdc0 GetSwitchData
void get_switch_data(FlowContext &flow);
// STUB: SONIC 0x40ef40 PadReadOn
void enable_pad_reading(FlowContext &flow);
// STUB: SONIC 0x42f240 FUN_0042f240
std::int16_t run_player_selection(FlowContext &flow);
// STUB: SONIC 0x40bf30 Clear
void clear_game(FlowContext &flow);
// STUB: SONIC 0x40bf40 Reset
void reset_game(FlowContext &flow);
// STUB: SONIC 0x416610 GameStateHandler_Trial
std::int16_t run_action(FlowContext &flow);
// STUB: SONIC 0x42f080 FUN_0042f080
void prepare_action_return(FlowContext &flow);
// STUB: SONIC 0x42caa0 dsReturnCheck
std::uint32_t check_return_mode(FlowContext &flow);
// STUB: SONIC 0x4173b0 GameStateHandler_Adventure
std::int16_t run_adventure(FlowContext &flow);
// NOTSA: session composition delegates to the recovered movie system.
std::int32_t run_movie(FlowContext &flow, std::uint32_t movie);
// STUB: SONIC 0x40ef70 PadReadOnP
void enable_player_pad_reading(FlowContext &flow, std::uint32_t player);
// STUB: SONIC 0x415af0 GameStateHandler_Mission
std::int16_t run_mission(FlowContext &flow);
// STUB: SONIC 0x4228f0 RunLevelDestructor
void run_level_destructor(FlowContext &flow, std::uint32_t kind);
// STUB: SONIC 0x780fb0 syFree
void free_message_resource(FlowContext &flow, MenuResource resource);
// STUB: SONIC 0x420f40 FreeRegObjTexlists
void free_regular_object_texlists(FlowContext &flow);
// STUB: SONIC 0x420ef0 FreeCharacterTexlists
void free_character_texlists(FlowContext &flow);
// STUB: SONIC 0x406f00 njReleaseTextureAll_
void release_all_textures(FlowContext &flow);
// STUB: SONIC 0x5034a0 FUN_005034a0
void prepare_title_assets(FlowContext &flow);
// STUB: SONIC 0x5033f0 FUN_005033f0
void prepare_title_dialogs(FlowContext &flow);
// STUB: SONIC 0x5032e0 FUN_005032e0
void initialize_title_menu(FlowContext &flow);
// NOTSA: composition seam for recovered njSetWaitVsyncCount; frame control owns its FUNCTION.
void set_wait_vsync_count(FlowContext &flow, std::int32_t count);
// STUB: SONIC 0x424c80 LoadLevelSounds
void load_level_sounds(FlowContext &flow);
// STUB: SONIC 0x5033b0 FUN_005033b0
std::int32_t run_title_menu(FlowContext &flow);
// STUB: SONIC 0x503380 FUN_00503380
void close_title_menu(FlowContext &flow, std::int32_t value);
// STUB: SONIC 0x42e780 FUN_0042e780
void prepare_title(FlowContext &flow);
// STUB: SONIC 0x42c630 ScreenFade_Start_CopyAlpha
void start_screen_fade(FlowContext &flow);
// STUB: SONIC 0x4144e0 GetPlayerNumber
std::uint32_t current_player_number(FlowContext &flow);
// NOTSA: composition seam for bootstrap::setup_character.
void setup_character(FlowContext &flow, std::uint32_t character);
// NOTSA: composition seam for bootstrap::reset_rings.
void reset_player_work(FlowContext &flow);
// STUB: SONIC 0x427ec0 FUN_00427ec0
void reset_player_parameters(FlowContext &flow);
// STUB: SONIC 0x42d770 LoadSave
void load_save(FlowContext &flow);
// STUB: SONIC 0x412ac0 FUN_00412ac0
void prepare_sequence(FlowContext &flow);
// STUB: SONIC 0x413380 SeqSetPlayer
void set_sequence_player(FlowContext &flow, std::uint32_t character);
// STUB: SONIC 0x4133e0 FUN_004133e0
void apply_sequence_level(FlowContext &flow, std::int16_t level);
// STUB: SONIC 0x414570 SetStageNumber
void set_stage_number(FlowContext &flow, std::int32_t level, std::int32_t act);
// STUB: SONIC 0x4147f0 SetEntranceNumber
void set_entrance_number(FlowContext &flow, std::int32_t entrance);
// STUB: SONIC 0x412c00 SeqSetTime
void set_sequence_time(FlowContext &flow, std::int32_t time);
// STUB: SONIC 0x42c820 dsGoDemoLoop
void go_demo_loop(FlowContext &flow);
// STUB: SONIC 0x642640 FUN_00642640
std::uint32_t initialize_summary(FlowContext &flow, std::int32_t value);
// STUB: SONIC 0x412d20 SeqCheckFlag
std::uint32_t check_sequence_flag(FlowContext &flow, std::int32_t flag);
// STUB: SONIC 0x6428d0 FUN_006428d0
std::uint32_t run_summary(FlowContext &flow, std::int32_t value);
// STUB: SONIC 0x413bc0 GetLevelType
std::uint32_t current_level_type(FlowContext &flow);
// STUB: SONIC 0x642170 FUN_00642170
void initialize_tutorial(FlowContext &flow, std::int32_t value);
// STUB: SONIC 0x641c60 FUN_00641c60
std::uint32_t run_tutorial(FlowContext &flow, std::int32_t value);
// STUB: SONIC 0x641a80 FUN_00641a80
void initialize_staff_roll(FlowContext &flow, std::int32_t value);
// STUB: SONIC 0x640f20 FUN_00640f20
std::int32_t run_staff_roll(FlowContext &flow, std::int32_t value);
// NOTSA: composition seam for logo::load_sega_textures.
void load_sega_logo(FlowContext &flow);
// STUB: SONIC 0x402f10 ___njSetBackColor
void set_background_color(FlowContext &flow, Rgba top, Rgba middle, Rgba bottom);
// STUB: SONIC 0x42ca10 FUN_0042ca10
void prepare_title_initialization(FlowContext &flow);
// STUB: SONIC 0x785330 pdVibMxStop
void stop_vibration(FlowContext &flow, std::uint32_t port);
// STUB: SONIC 0x424320 dsPause_all
void pause_all_sound(FlowContext &flow);
// STUB: SONIC 0x590650 GetMiClearStatus
std::uint32_t mission_clear_status(FlowContext &flow);
// STUB: SONIC 0x590660 Release_MIS_C_TEXLIST
void release_mission_clear_texlist(FlowContext &flow);
// STUB: SONIC 0x40d490 MSG_Disp
void display_message(FlowContext &flow);
// STUB: SONIC 0x42c4f0 ScreenFade_DrawColor
void draw_screen_fade(FlowContext &flow);
// STUB: SONIC 0x426050 IncrementFrameCounter
void increment_frame_counter(FlowContext &flow);
} // namespace sadx::flow
