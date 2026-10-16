#pragma once
#include "guest/frame_control_reference.hpp"
#include "runtime/engine_code_tables.hpp"
#include "runtime/session_control_defaults.hpp"
#include "input/input_initial_state.hpp"
#include "math/ninja_sine_table.hpp"
#include "game/audio_stream.hpp"
#include "loader/executable_image.hpp"
#include "guest/game_mode_reference.hpp"
#include "guest/game_mode_leaves_reference.hpp"
#include "player/player_scene_position.hpp"
#include "player/sonic_player.hpp"
#include "game/sound_manager.hpp"
#include "game/game_frame_owners.hpp"
#include "game/game_reset.hpp"
#include "game/game_reset_leaves.hpp"
#include "game/game_startup.hpp"
#include "game/game_view.hpp"
#include "engine/task.hpp"
#include "engine/task_work.hpp"
#include "input/input_mod_pad.hpp"
#include "engine/ninja_projection_range.hpp"
#include "engine/ninja_view.hpp"
#include "engine/ninja_fog.hpp"
#include "engine/ninja_fog_initial_state.hpp"
#include "engine/ninja_frame_calls.hpp"
#include "game/mod_loader_frame.hpp"
#include "game/game_winmain.hpp"
#include "game/game_library_init.hpp"
#include "engine/ninja_device_init.hpp"
#include "engine/ninja_system_init.hpp"
#include "game/game_window.hpp"
#include "engine/ninja_vibration.hpp"
#include "engine/ninja_input_devices.hpp"
#include "engine/ninja_keyboard.hpp"
#include "engine/ninja_queue_lifecycle.hpp"
#include "engine/ninja_screen.hpp"
#include "engine/ninja_screen_initial_state.hpp"
#include "game/sound_initial_state.hpp"
#include "game/game_view_initial_state.hpp"
#include "engine/ninja_queue_initial_state.hpp"
#include "engine/ninja_debug_text.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sadx {
// One composition of every ported owner behind GameLoop 40cad0. Owners that
// are not ported yet are explicit: stop_at_unported halts at the first one
// (real native execution); survey_unported_as_noop records each one, treats it
// as a completed no-op returning EAX 0 and continues. A survey is a worklist
// generator only: its later state is not game behaviour.
enum class OwnerPolicy {stop_at_unported,survey_unported_as_noop};
struct UnportedOwner {
    std::uint32_t callee=0,first_site=0;
    std::string path;      // owner chain, e.g. "40cad0>40c030>408630"
    std::uint32_t calls=0;
};
struct NativeGameReport {
    bool startup_completed=false;
    std::uint32_t frames_completed=0;
    bool exited=false;
    std::optional<UnportedOwner> stopped_at; // stop_at_unported
    std::vector<UnportedOwner> unported;     // first-reached order
    std::string failure;                     // non-owner failure, if any
    std::vector<std::string> assumptions;    // explicit native choices where a source fact is unavailable
};
// Native owner of the D3D8 state the source sets (what a Metal backend reads).
// It records state; it does not claim any draw happened.
struct D3D8StateOwner {
    std::map<std::uint32_t,std::uint32_t> render_states,stage0_states;
    std::map<std::uint32_t,NinjaMatrix> transforms;
    std::uint64_t render_calls=0,stage_calls=0,texture_binds=0,vertex_formats=0;
    std::vector<NinjaSpriteIssueCommand> draws; // issued sprite draws, in order
    std::vector<NinjaScreenPolygon> polygons;   // 78ea10 DrawPrimitiveUP fans (77dfc0), in order
    std::optional<std::array<std::uint32_t,6>> viewport; // the last SetViewport (7912e0)
    // Frame methods issued through 781020..781050 (recorded; no display backend implied).
    std::uint64_t begin_scenes=0,end_scenes=0,presents=0,clears=0;std::optional<std::uint32_t> last_clear_flags;
};
struct NativeGameConfig {
    OwnerPolicy policy=OwnerPolicy::stop_at_unported;
    std::uint32_t max_frames=1;
    // Explicit native code tables; absent selects the injectable PE-backed reference facts.
    std::optional<runtime::EngineCodeTables> engine_tables;
    // Explicit scalar load-time state; resource/callback facts remain separate.
    std::optional<runtime::SessionControlDefaults> control_defaults;
    // Explicit input values; name/extension/info facts remain image-backed.
    std::optional<input::InputInitialState> input_defaults;
    // Explicit native coefficients; absent retains all-or-unknown PE table decoding.
    std::optional<NinjaQuarterSineTable> sine_table;
    // Explicit native fog load-time values; device resources/capabilities remain separate.
    std::optional<NinjaFogInitialState> fog_defaults;
    // Numeric load-time screen values; resources and device facts remain separate.
    std::optional<NinjaScreenInitialState> screen_defaults;
    // Numeric sound defaults; queue identities and task/listener facts remain separate.
    std::optional<SoundInitialState> sound_defaults;
    // Numeric game-view/colors; pause controls and resources remain independently owned.
    std::optional<GameViewInitialState> view_defaults;
    // Numeric queue state; allocation, device and live controls remain separate.
    std::optional<NinjaQueueInitialState> queue_defaults;
    InputModPadConstants input_constants; // caller-supplied; normal boot uses maintained native values
    std::int32_t limiter_cadence=63;      // Speedrun FrameLimit "63FPS"
    SpeedrunLimiterClock clock;           // host counter owner (mac_clock in the app)
    // Load images: source globals and Input mod DLL data start from these.
    std::shared_ptr<const ExecutableImage> executable,input_mod_dll;
    // 401ac0 model DLLs by slot at their preferred base (null: absent, LoadLibraryA fails).
    std::array<std::shared_ptr<const ExecutableImage>,7> model_dlls{};
    // Input mod config.ini per pad (profile updated-speedrun.json).
    std::array<InputModPadSettings,8> pad_settings{};
    // D3D8 frame methods (a Metal backend). Unset methods use the recording owner,
    // which records the call in D3D8StateOwner and returns D3D_OK.
    D3D8FrameDevice frame_device;
    // Speedrun mod config.ini [QuickSaveSettings] Enabled (profile: false).
    bool speedrun_quick_save_enabled=false;
    // The game's .\sonicDX.ini bytes (WinMain's GetPrivateProfile reads); nullopt = file absent.
    std::optional<std::vector<std::uint8_t>> sonicdx_ini;
    WinMainLoaderSettings loader_settings; // mod loader TestSpawn text/voice languages (profile: 1/1)
};
class NativeGame {
public:
    explicit NativeGame(NativeGameConfig);
    NativeGameReport run();
    // Reference-only scalar facts in the session-control probe order; unknowns stay absent.
    std::array<std::optional<std::uint32_t>, 51> control_facts() const;
    // Reference-only snapshots before startup can overwrite the initialized data.
    std::array<std::optional<std::uint8_t>, 1869> input_data_facts() const;
    std::array<std::optional<std::uint8_t>, 252> input_resource_facts() const;
    std::array<std::optional<std::uint32_t>, 398> fog_facts() const;
    std::array<std::optional<std::uint32_t>, 90> screen_facts() const;
    // Reference-only POINT2COL resource identities; native numeric data does not replace them.
    std::array<std::optional<std::uint32_t>, 3> screen_resource_facts() const;
    std::array<std::optional<std::uint32_t>, 329> sound_facts() const;
    std::array<std::optional<std::uint32_t>, 73> sound_resource_facts() const;
    std::array<std::optional<std::uint32_t>, 30> view_facts() const;
    // Reference-only caller coefficient bits, including deliberately injected variants.
    std::array<std::uint32_t, 7> input_constant_facts() const;
    std::array<std::optional<std::uint32_t>, 21> queue_facts() const;
    // Reference allocation word and native device-presence fact before startup.
    std::array<std::optional<std::uint32_t>, 2> queue_resource_facts() const;
    const std::optional<NinjaMath::SineTable> &sine_facts() const noexcept { return sine_table_at_88af78_; }
    // Live owners for a front end (keyboard/controller delivery each frame).
    InputModKeyboardPad keyboard_pad{};
    std::array<InputModGamepadState,8> gamepads{};
    // Recorded D3D8 state the source has set (what a Metal backend reads).
    const D3D8StateOwner& d3d8()const noexcept{return d3d8_;}
    // Borrow the source GetDeviceCaps result; unknown bytes remain explicit before device setup.
    const SourceBytes<0x3d127c0,0xd4>& device_caps()const noexcept{return device_.caps_at_3d127c0;}
private:
    NativeGameConfig config_;
    // Engine state owners.
    GameLogicTimingState timing_;GameLogicState logic_{timing_,{},{}};
    GameLoopState loop_;GameModeState mode_state_;std::optional<std::int32_t> mode_at_3abdc7c_;
    GameLogicEdgeState edges_;GameFrameInputState input_{edges_.soft_reset_edge(),{},{},loop_.save_phase_at_3b291a4};
    GameFrameRendererState renderer_{};GameControllerInitGlobals controller_globals_;GameLogicController controller_template_;
    std::array<std::shared_ptr<GameLogicController>,4> controllers_raw_;GameLogicControllers controller_pointers_;
    InputModRawRecords input_raw_;std::array<InputModDreamPad,8> pads_{};InputModFrameState input_frame_;SseContext sse_{0x1f80};
    AudioStartupState audio_;MenuBlockState menu_;ExitCallbackList exit_callbacks_;
    std::optional<std::uint32_t> word_at_3abdc80_,word_at_3abdc78_,word_at_3abdc90_;
    std::optional<std::uint8_t> byte_at_3b280a8_;
    // Render queue and D3D8 state.
    D3D8StateOwner d3d8_;NinjaRenderState render_;std::optional<NinjaBlendTables> blend_tables_;std::array<std::uint32_t,8> depth_table_{};
    std::shared_ptr<NinjaRenderQueueArena> arena_=std::make_shared<NinjaRenderQueueArena>(std::vector<std::uint8_t>(0),0,0,0);NinjaSpriteQueue queue_{arena_};
    NinjaSpriteQueueRuntime queue_runtime_;NinjaQueueConsumerState consumer_;NinjaSpriteProjection projection_{};
    std::optional<std::array<float,4>> color_at_3b18230_;std::optional<std::uint32_t> depth_mode_at_8928c8_;
    std::array<std::optional<std::uint32_t>,3> words_at_900840_,words_at_3ab9898_,words_at_900858_,words_at_90084c_;
    float one_at_7dcb10_=1.0F,negative_one_at_7dcb0c_=-1.0F;X87Context fp_{0x7f}; // capture-tied main-thread CW
    std::optional<NinjaQueueLifecycleOwners> lifecycle_;
    CrtRandState rng_;std::optional<GameResetState> reset_;std::optional<ResetLeafState> leaves_;std::optional<GameViewState> view_;std::optional<std::array<float,4>> color_at_3b280ac_;std::optional<std::uint32_t> queue_base_at_3abd960_;
    std::optional<NinjaMath::SineTable> sine_table_at_88af78_;std::optional<float> fog_sentinel_at_7dce50_;
    NinjaFogDeviceState fog_device_;NinjaFogTable fog_table_at_3b10f78_{};std::optional<float> level_fog_start_at_3abdc60_,level_fog_end_at_3abdc64_;std::optional<std::uint32_t> level_fog_color_at_3abdc68_;
    NinjaFogOwners fog_owners();
    NinjaKeyboardState keyboard_;NinjaCallbackList callbacks_;std::optional<NinjaKeyboardTable> keyboard_table_;
    NinjaFrameCallState frame_calls_; // 411d00's D3D8 frame thunks (scene flag, hooks, vertex-buffer records)
    SpeedrunModState speedrun_mod_;   // Speedrun 2.0 DLL globals (OnFrame via the loader's 426063 ProcessCodes)
    GameWinMainState winmain_;        // WinMain 4025b0's settings globals
    std::int32_t cursor_display_count_=0; // host ShowCursor counter (Win32 starts at 0)
    GuestHeap heap_;NinjaMemoryState memory_;CdDriveState cd_drive_; // njMalloc's Win32 heap (389d628) and 401a20's 3a8af78
    GameLibraryInitState library_;NinjaDeviceState device_;std::optional<std::uint32_t> word_at_3d0f7a0_; // 781070, 791610, 77ff40
    NinjaSystemInitState system_;NinjaTexturePoolState texture_pool_;GuestHeap crt_heap_{0x00520000u,0x60000000u}; // 401830, 77f610, the CRT malloc heap
    std::optional<std::uint32_t> window_at_3d0fd30_,instance_at_3d0fd34_; // set by window creation (unported)
    GameWindowState window_;bool message_pump_reported_=false; // 789bd0's 3d08534
    NinjaScreenState screen_;NinjaScreenDevice screen_device_;bool screen_depth_reported_=false; // 780c30, 7912e0, 78dbe0, 77dfc0
    NinjaVibrationState vibration_; // 785190's entry fields
    InputModRumbleTasks rumble_tasks_; // the Input mod's rumble task pairs (DLL BSS: zero)
    NinjaInputDeviceState input_devices_; // 3d08814 joystick count, 3d08834 mouse device
    PlayerScenePositionGlobals scene_globals_;SonicPlayerGlobals sonic_globals_;std::optional<GameModeLeafState> mode_leaves_;
    ModelDllSet model_set_;ModelDllBindings model_bindings_;SoundRecordFields sound_fields_;
    // 4086f0 queue consumer composition (texture selection, sprite display).
    NinjaTextureSelectionState texture_selection_;std::array<std::uint32_t,4> texture_returns_at_389d458_{};
    std::optional<NinjaTextureSelectionRuntime> texture_runtime_;std::optional<NinjaSpriteConstants> sprite_constants_;std::optional<NinjaQueueConsumerConstants> consumer_constants_;
    std::optional<NinjaSpriteDisplayRuntime> display_runtime_;std::optional<NinjaQueueConsumerRuntime> consumer_runtime_;
    // Ninja matrix stack (base 3d0fa1c, current 3d0f818, capacity 3d0fa28, depth 3d0fa2c over 3a7cf48)
    // and its coordinate owner; created by 401830's njInitMatrix 780180 before 40cad0.
    GameFrameOwnersState frame_owners_;
    std::optional<NinjaMatrixStack> matrix_stack_;std::optional<PlayerCoordinates> coordinates_;std::optional<NinjaMatrix> unit_at_389d650_;
    NinjaProjectionRangeState projection_range_{};NinjaViewState ninja_view_;NinjaTransformOwner transform_owner_;
    // Task heap 40b1a0 and pool 40b460 (fresh owners each reset, as the source rebuilds them).
    std::shared_ptr<TaskWorkArena> task_arena_;std::unique_ptr<TaskScheduler> scheduler_;
    std::optional<std::uint32_t> word_at_3c839fc_,word_at_3c83a00_;std::array<std::optional<std::uint32_t>,4> words_at_3c60108_; // 64fc70, 51a740
    AudioStreamState audio_stream_;std::optional<std::uint32_t> window_at_3a8b088_;
    // Reporting.
    NativeGameReport report_;std::map<std::uint32_t,std::size_t> unported_index_;std::string failure_detail_; // e.g. the fact a stopped owner required
    GameLogicOwnerResult unported(std::uint32_t callee,std::uint32_t site,const std::string& path);
    runtime::ViewCodeConstants view_code_constants_{};
    // NOTSA: select native code tables or injected reference facts before wiring owners.
    void initialize_engine_code_tables();
    // NOTSA: apply native scalar defaults or the original injected image facts.
    void initialize_control_defaults();
    // NOTSA: decode input data or bind explicitly supplied native values.
    void initialize_input_defaults();
    // NOTSA: bind native fog values or preserve injectable reference facts.
    void initialize_fog_defaults();
    // NOTSA: bind numeric native screen state or preserve reference availability.
    void initialize_screen_defaults();
    // NOTSA: bind native sound numeric data or preserve reference availability.
    void initialize_sound_defaults();
    // NOTSA: bind native view/color values or preserve reference availability.
    void initialize_view_defaults();
    // NOTSA: bind numeric queue defaults or preserve independent PE availability.
    void initialize_queue_defaults();
    void initialize_source_facts();
    void report_screen_depth(); // the 78dbe0 assumptions, once
};
} // namespace sadx
