#pragma once
#include "engine/character_resources.hpp"
#include "engine/task.hpp"
#include "game/game_cleanup.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace sadx { class X87Context; }

namespace sadx::title_menu {
// NOTSA: borrowed animation entries from the imported character pack.
struct CharacterActions {
    ActionHandle sonic_clear;
    ActionHandle sonic_transition;
    ActionHandle knuckles_clear;
    ActionHandle knuckles_transition;
    ActionHandle big_sit;
    ActionHandle big_prepare;
    ActionHandle big_cast;
};
// NOTSA: borrows the live action slots shared with LoadPlayerMotionData.
struct ActionBindings {
    ActionHandle &sonic_clear;
    ActionHandle &sonic_transition;
    ActionHandle &knuckles_clear;
    ActionHandle &knuckles_transition;
    ActionHandle &big_sit;
    ActionHandle &big_sit_repeat;
    ActionHandle &big_prepare;
    ActionHandle &big_cast;
    ActionHandle &big_cast_repeat;
};
// AdvaModeEnum; values retain the original menu dispatch identities.
enum class MenuMode : std::int32_t {
    None = 0, CharacterSelect = 1, FileSelect = 2, TrialActSelect = 3, SoundTest = 4,
    Title = 5, TitleMenu = 6, OptionSelect = 7, Explanation = 8, MinigameSelect = 9,
    EmblemView = 10, InternetDemo = 11, TvSettings = 12, Puti = 13,
    TitleMenuSub = 14, TitleMenuLeaderboards = 15, UnusedTrialGame = 16,
    UnusedLeaderboards = 17, UnusedAchievements = 18, OptionMenuSub = 19,
    HowToPlay = 20, HowToPlaySecond = 21, LeaderboardMenu = 22, Credits = 23,
    Vanish = 24, End = 25, EndSecond = 26,
    // The original PC sequence uses these values as its completion phases.
    AwaitFade = 14, ReturnResult = 15, Finished = 16
};
// AdvaRetEnum.
enum class MenuResult : std::int32_t {
    SoftReset = -2, Cancel = -1, None = 0, Adventure = 1, Trial = 100, Summary = 101,
    Mission = 102, Ending = 103, Tutorial = 104, PlayerSelect = 105,
    HiddenSelect = 106, TitleToDemo = 107, SoftResetInTitle = 108,
    TutorialCharacterSelect = 109, Max = 110
};
struct ModeSelection {
    MenuMode previous = MenuMode::None;
    MenuMode next = MenuMode::None;
    MenuMode next_sub = MenuMode::None;
};
struct MenuObjectEntry {
    MenuMode mode = MenuMode::None;
    ModeSelection transition;
};
struct SequenceData {
    MenuMode born_mode = MenuMode::None;
    // Imported transition records exclude the source zero-mode terminator.
    ModeSelection initial_transition{};
    std::vector<MenuObjectEntry> following{};
};
struct ObjectCalls {
    virtual ~ObjectCalls() = default;
    virtual void calculate_ticks() = 0;
    virtual TaskHandle create_character_select(const ModeSelection &) = 0;
    virtual TaskHandle create_file_select(const ModeSelection &) = 0;
    virtual TaskHandle create_level_select(const ModeSelection &) = 0;
    virtual TaskHandle create_sound_test(const ModeSelection &) = 0;
    virtual TaskHandle create_title(const ModeSelection &) = 0;
    virtual TaskHandle create_title_menu(const ModeSelection &) = 0;
    virtual TaskHandle create_options(const ModeSelection &) = 0;
    virtual TaskHandle create_explanation(const ModeSelection &) = 0;
    virtual TaskHandle create_subgame_list(const ModeSelection &) = 0;
    virtual TaskHandle create_emblem_results(const ModeSelection &) = 0;
    virtual TaskHandle create_internet_demo(const ModeSelection &) = 0;
    virtual TaskHandle create_tv_settings(const ModeSelection &) = 0;
    virtual TaskHandle create_puti(const ModeSelection &) = 0;
};
struct SequenceWork {
    const SequenceData *data = nullptr;
    MenuMode mode = MenuMode::None;
    MenuMode old_mode = MenuMode::None;
    MenuMode fixed_mode = MenuMode::None;
    MenuResult advance = MenuResult::None;
    std::int32_t return_value = 0;
    MenuMode waiting_mode = MenuMode::None;
    std::int16_t frames = 0;
    std::uint8_t waiting = 0;
    std::uint8_t fade_end = 0;
    std::uint8_t fade_out = 0;
    std::uint8_t fade_out_secondary = 0;
};
struct SequenceChildWork {
    const SequenceData *data = nullptr;
    std::int32_t frames = 0;
};
// NOTSA: native task identities and typed work replace source heap pointers.
struct SoundManagerCalls {
    virtual ~SoundManagerCalls() = default;
    virtual void destroy_sound_manager(const TaskHandle &) = 0;
    virtual void close_media(std::uint32_t mode) = 0;
};
struct MenuSoundState {
    TaskHandle music_task;
    TaskHandle volume_task;
};
struct SoundCalls {
    virtual ~SoundCalls() = default;
    // STUB: SONIC 0x4238e0 SetBankDir
    virtual void select_sound_bank(std::int32_t) = 0;
};
// NOTSA: IDs are validated imported menu texture records, excluding the source terminator.
struct MenuTextureId {
    std::uint32_t value;
};
using ModeTextureLists = std::array<std::vector<MenuTextureId>, 14>;
struct MenuTextureState {
    std::array<std::uint32_t, 2> loaded{};
    std::int32_t ready = 0;
};
struct MenuTextureCalls {
    virtual ~MenuTextureCalls() = default;
    virtual void load_menu_texture(MenuTextureId) = 0;
};
struct SequenceCalls {
    virtual ~SequenceCalls() = default;
    virtual void set_entrance(std::int32_t) = 0;
    // NOTSA: composition seam; menu_resource_lifetime owns the recovered release.
    virtual void release_menu_textures() = 0;
    // STUB: SONIC 0x5064d0 FUN_005064d0
    virtual void finalize_selection() = 0;
    virtual void create_menu_objects(const SequenceData *) = 0;
    virtual void destroy_sequence_child(const TaskHandle &) = 0;
};
enum class DialogStyle : std::int32_t { Blue, Cyan, BlueSecond, BlueThird, CyanSecond, BlueFourth };
struct DialogPanel {
    float x = 0, y = 0;
    std::uint8_t texture = 0;
};
struct DialogSounds {
    std::int32_t open = 0, close = 0, move = 0, decide = 0, cancel = 0;
};
// NOTSA: semantic identity of the source EachDrawFnc; no code pointers enter assets.
enum class DialogContent { None, FileSelectHelp };
struct DialogParameters {
    DialogStyle style = DialogStyle::Blue;
    DialogContent content = DialogContent::None;
    texture_lifetime::List *textures = nullptr;
    std::shared_ptr<const std::vector<DialogPanel>> panels = {};
    std::shared_ptr<const DialogSounds> sounds = {};
    std::uint32_t upper_color = 0, lower_color = 0;
    float center_x = 0, center_y = 0, depth = 0, width = 0, height = 0;
    float scale_x = 0, scale_y = 0;
    std::int8_t choice_count = 0, cancel_choice = 0;
};
struct DialogWork {
    const DialogParameters *parameters = nullptr;
    std::int32_t phase = 0;
    std::uint32_t color = 0;
    float scale_x = 0, scale_y = 0, fade = 0;
    std::int16_t repeat_frames = 0, opening_frames = 0;
    std::array<std::int8_t, 16> choice_ids{};
    std::int32_t previous_choice = 0;
    std::uint32_t blink_step = 0;
    // This source MAlloc field is read before initialization, but overwritten before visible use.
    std::optional<std::uint32_t> animation_angle;
    std::uint32_t upper_color = 0, lower_color = 0;
    std::uint8_t cursor = 0, animation_value = 0;
};
struct DialogCalls {
    virtual ~DialogCalls() = default;
    virtual TaskHandle create_dialog_task() = 0;
    virtual std::shared_ptr<DialogWork> allocate_dialog_work(const TaskHandle &) = 0;
    virtual void install_dialog_callbacks(const TaskHandle &) = 0;
};
// AdvaStatEnum.
enum class TitlePhase : std::uint32_t { RequestWait, FadeIn, Keep, FadeOut };
// TitleNewSbMdEnum.
enum class TitleSubmode : std::int32_t { Stay, Alert, StayMain, Wait };
struct TitleWork {
    TitlePhase phase = TitlePhase::RequestWait;
    MenuMode previous = MenuMode::None, next = MenuMode::None;
    float depth = 0, fade_progress = 0;
    std::uint32_t color = 0;
    TitleSubmode submode = TitleSubmode::Stay;
    TaskHandle alert_task;
    std::int8_t vm_status = 0;
    std::int32_t timer = 0;
    std::uint32_t blink_timer = 0;
    std::uint32_t music_started = 0;
    std::uint32_t wave_timer = 0;
    float cloud_offset = 0;
    std::uint8_t cloud_index = 0;
};
// Shared presentation fields of the original menu work records.
struct MenuPresentation {
    TitlePhase phase = TitlePhase::RequestWait;
    MenuMode previous = MenuMode::None, next = MenuMode::None;
    float depth = 0, fade_progress = 0;
    std::uint32_t color = 0;
};
struct CharacterSelectWork {
    MenuPresentation presentation;
    std::int32_t cursor_angle = 0, submode = 0, selected_character = 0;
    std::uint8_t selected_file = 0;
    float angle_progress = 0;
    std::int16_t frames = 0;
    std::int32_t dialog_status = 0;
    std::uint8_t wake = 0;
};
struct FileSelectWork {
    MenuPresentation presentation;
    float cursor_scale = 0;
    std::uint32_t cursor_color = 0;
    std::int32_t cursor_angle = 0, submode = 0, player_backup = 0, selected_character = 0;
    std::int16_t wait_frames = 0;
    std::uint8_t cursor_alpha = 0;
    float offset_x = 0;
};
struct MainMenuWork {
    MenuPresentation presentation;
    MenuMode next_sub = MenuMode::None;
    std::int32_t submode = 0, selected_menu = 0;
    std::int16_t frames = 0;
    std::uint8_t wake = 0;
};
struct ExplanationWork {
    MenuPresentation presentation;
    std::int32_t submode = 0;
};
struct InternetDemoWork {
    MenuPresentation presentation;
    std::int32_t submode = 0;
};
struct StageAct {
    std::uint8_t stage = 0, act = 0;
};
struct StageSelectionData {
    std::array<std::vector<StageAct>, 7> trial_stages, subgame_stages;
    std::array<std::uint16_t, 7> character_ids{};
    std::array<std::uint8_t, 44> display_stage_ids{};
};
struct StageSelectWork {
    MenuPresentation presentation;
    std::int32_t submode = 0;
    std::array<StageAct, 10> available_stages{};
    std::uint8_t available_count = 0;
    std::array<std::int32_t, 33> stage_ids{};
    std::uint8_t stage_count = 0;
    std::uint32_t cursor_color = 0;
    float cursor_scale = 0;
    std::int32_t cursor_angle = 0;
    std::uint8_t cursor_alpha = 0, display_stage = 0, selected_stage = 0;
};
struct SoundTestWork {
    MenuPresentation presentation;
    std::int32_t submode = 0;
};
// Resource identity replaces the original options-content pointer, not its contents.
enum class OptionContent { None, Menu };
struct MenuAnimation {
    std::uint8_t frames = 0, frame = 0;
};
struct OptionWork {
    MenuPresentation presentation;
    std::int32_t submode = 0;
    float cursor_scale = 0;
    std::int32_t cursor_angle = 0;
    std::uint8_t cursor_alpha = 0;
    MenuAnimation eight_frame_cycle, twelve_frame_cycle, four_frame_cycle;
    MenuAnimation first_long_cycle, second_long_cycle, five_frame_cycle, twenty_four_frame_cycle;
    std::uint32_t rotation_angle = 0;
    OptionContent content = OptionContent::None;
    std::int32_t item_count = 0;
};
struct EmblemWork {
    MenuPresentation presentation;
    std::int32_t submode = 0;
    std::array<std::uint8_t, 10> selected_entries{};
    float cursor_scale = 0;
    std::int32_t cursor_angle = 0;
    std::uint8_t entry_count = 0, cursor_alpha = 0;
};
struct TvSettingsWork {
    MenuPresentation presentation;
    std::int32_t submode = 0;
    std::uint8_t mode_changed = 0;
};
struct PutiResource;
struct PutiWork {
    std::uint32_t phase = 0;
    std::shared_ptr<const PutiResource> resource;
    // MAlloc leaves this genuinely indeterminate until the source assigns the exit countdown.
    std::optional<std::uint32_t> exit_frames;
    std::uint32_t waiting_frames = 0;
};
struct PutiCalls {
    virtual ~PutiCalls() = default;
    virtual std::shared_ptr<PutiWork> allocate_puti_work(const TaskHandle &) = 0;
    virtual void install_puti_callbacks(const TaskHandle &) = 0;
};
struct MenuFactoryState {
    TaskHandle character_select_task, file_select_task, main_menu_task;
    TaskHandle level_select_task, sound_test_task, options_task, subgame_task;
    TaskHandle emblem_task, tv_settings_task;
    std::shared_ptr<StageSelectWork> level_select_work, subgame_work;
    std::shared_ptr<SoundTestWork> sound_test_work;
    std::shared_ptr<OptionWork> options_work;
    std::shared_ptr<EmblemWork> emblem_work;
    std::shared_ptr<TvSettingsWork> tv_settings_work;
    TaskHandle explanation_task, internet_demo_task, puti_task;
    std::shared_ptr<CharacterSelectWork> character_select_work;
    std::shared_ptr<FileSelectWork> file_select_work;
    std::shared_ptr<MainMenuWork> main_menu_work;
    std::shared_ptr<ExplanationWork> explanation_work;
    std::shared_ptr<InternetDemoWork> internet_demo_work;
};
// NOTSA: borrows canonical bootstrap/menu and memory-card state.
struct MenuFactoryInputs {
    const std::uint8_t &character_cursor;
    std::uint8_t &selected_file;
    const std::uint8_t &memory_card_ready;
    const std::uint8_t &memory_card_file;
    const std::uint8_t &tutorial_variant;
};
struct StageSelectionInputs {
    const std::uint8_t &character;
    const std::uint8_t &include_all;
    const std::array<std::uint8_t, 344> &completed_stages;
    const StageSelectionData &data;
};
struct MenuFactoryCalls {
    virtual ~MenuFactoryCalls() = default;
    // Native composition selects named executable callbacks for the given mode.
    virtual TaskHandle create_menu_task(MenuMode, std::uint32_t flags, std::uint32_t level) = 0;
    virtual void install_menu_callbacks(MenuMode, const TaskHandle &) = 0;
    virtual std::shared_ptr<CharacterSelectWork> allocate_character_select(const TaskHandle &) = 0;
    virtual std::shared_ptr<FileSelectWork> allocate_file_select(const TaskHandle &) = 0;
    virtual std::shared_ptr<MainMenuWork> allocate_main_menu(const TaskHandle &) = 0;
    virtual std::shared_ptr<ExplanationWork> allocate_explanation(const TaskHandle &) = 0;
    virtual std::shared_ptr<InternetDemoWork> allocate_internet_demo(const TaskHandle &) = 0;
    virtual std::shared_ptr<StageSelectWork> allocate_level_select(const TaskHandle &) = 0;
    virtual std::shared_ptr<StageSelectWork> allocate_subgame(const TaskHandle &) = 0;
    virtual std::shared_ptr<SoundTestWork> allocate_sound_test(const TaskHandle &) = 0;
    virtual std::shared_ptr<OptionWork> allocate_options(const TaskHandle &) = 0;
    virtual std::shared_ptr<EmblemWork> allocate_emblem(const TaskHandle &) = 0;
    virtual std::shared_ptr<TvSettingsWork> allocate_tv_settings(const TaskHandle &) = 0;
};
struct RunCalls {
    virtual ~RunCalls() = default;
    virtual void set_depth_compare(std::int32_t) = 0;
    virtual void poll_controls() = 0;
    virtual void enable_control() = 0;
    virtual void run_tasks() = 0;
    virtual void disable_fog() = 0;
    virtual void display_tasks() = 0;
    virtual std::int32_t soft_reset_check(std::int32_t) = 0;
    virtual void delete_all_tasks() = 0;
    virtual void free_regular_object_textures() = 0;
    virtual void destroy_sequence_task(const TaskHandle &) = 0;
    virtual void draw_text(std::string_view, float, float, float) = 0;
};
struct TitleInitState {
    const std::int32_t &language;
    const std::uint8_t &character_selection;
    const std::uint8_t &skip_title;
    TitleWork &canonical;
};
struct TitleInitCalls {
    virtual ~TitleInitCalls() = default;
    virtual void set_stage(std::uint8_t, std::uint8_t) = 0;
    // NOTSA: composition binds the recovered character-lighting initializer.
    virtual void initialize_character_lighting() = 0;
    virtual void load_mode_textures(MenuMode) = 0;
    virtual void load_japanese_title_textures() = 0;
    // NOTSA: saved-character restoration supplied by the native menu composition.
    virtual void prepare_menu_voice() = 0;
    virtual void set_title_menu_previous() = 0;
};
struct TitleTiming {
    const std::int32_t &elapsed_ticks;
    std::uint32_t &loop_count;
    std::uint32_t &adventure_loop_count;
    const std::int32_t &language;
};
struct TitleUpdateCalls {
    virtual ~TitleUpdateCalls() = default;
    virtual void initialize_title(TitleWork &) = 0;
    virtual std::uint32_t fade_out_color(float) = 0;
    virtual std::uint32_t fade_in_color(float) = 0;
    virtual void update_title_input(TitleWork &) = 0;
    // NOTSA: source ignores this argument and uses the canonical sequence work.
    virtual void release_mode_textures(MenuMode) = 0;
    virtual void release_japanese_title_textures() = 0;
};
// NOTSA: borrows live title-input owners; allocation/work state remains per task.
struct TitleInputState {
    std::uint32_t &loop_count;
    std::uint32_t &title_logic_ticks;
    std::int32_t &pending_logic_ticks;
    std::uint8_t &skip_title;
    std::int32_t &saving_disabled;
    std::uint32_t &save_word_after_selection;
    std::uint32_t &save_word_before_selection;
    std::uint32_t &exit_requested;
    const std::int32_t &menu_ready;
    const bool &bgm_enabled;
    std::uint32_t &demo_wait_ticks;
};
struct TitleInputCalls {
    virtual ~TitleInputCalls() = default;
    virtual void calculate_ticks() = 0;
    virtual void request_menu_music(std::int32_t) = 0;
    virtual std::int32_t key_pressed(std::int32_t) = 0;
    // NOTSA: reads live SWDATAE after the source keyboard callback.
    virtual std::uint32_t pressed_buttons() = 0;
    virtual void transition_menu(MenuMode) = 0;
    virtual void play_title_confirm_sound() = 0;
    virtual std::int32_t audio_ready() = 0;
    virtual std::uint32_t music_state() = 0;
};
struct ModeTransitionCalls {
    virtual ~ModeTransitionCalls() = default;
    virtual void stop_menu_music() = 0;
};
struct FadeCalls {
    virtual ~FadeCalls() = default;
    virtual void disable_alpha_test() = 0;
};
enum class TitleTextureList { Common, Small, Title, JapaneseTitle };
struct BackgroundLayout {
    float compact_logo_x, compact_logo_y, logo_x, logo_y, logo_start_x;
    float text_start_x, text_step, text_x, text_y;
};
// NOTSA: borrows the actual view/timing/language owners consumed by the source draw.
struct BackgroundView {
    const float &horizontal_scale, &vertical_scale;
    const std::int32_t &elapsed_ticks;
    const std::uint32_t &loop_count;
    const std::int32_t &language;
};
struct BackgroundCalls {
    virtual ~BackgroundCalls() = default;
    virtual void set_base_color(std::uint32_t) = 0;
    virtual void set_material(std::uint32_t) = 0;
    virtual void select_title_texture(TitleTextureList) = 0;
    virtual void draw_scaled_texture(std::uint32_t index, float x, float y, float depth,
                                      float scale_x, float scale_y) = 0;
    virtual void set_depth_compare(std::int32_t) = 0;
};
struct TitleDrawCalls {
    virtual ~TitleDrawCalls() = default;
    virtual void default_blending() = 0;
    virtual void set_base_color(std::uint32_t) = 0;
    virtual void draw_title_background(TitleWork &) = 0;
};
struct TitleCalls {
    virtual ~TitleCalls() = default;
    virtual TaskHandle create_title_task() = 0;
    virtual std::shared_ptr<TitleWork> allocate_title_work(const TaskHandle &) = 0;
    virtual void install_title_callbacks(const TaskHandle &) = 0;
};
struct TrialSelection {
    std::uint8_t stage = 0;
    std::uint8_t act = 0;
};
struct State {
    MenuFactoryState factories;
    TrialSelection trial;
    TaskHandle sequence_task;
    TaskHandle dialog_task;
    std::shared_ptr<SequenceWork> sequence_work;
    std::shared_ptr<DialogWork> dialog_work;
    TaskHandle title_task;
    std::shared_ptr<TitleWork> title_work;
};
// NOTSA: these owners are shared with sound, rendering and game flow.
struct SharedState {
    std::uint8_t &font_loaded;
    std::uint8_t &menu_voice;
    std::uint8_t &menu_voice_state;
    std::uint32_t &loop_count;
};
// NOTSA: original call boundaries; implementations must perform the operation or fail loudly.
struct Calls {
    virtual ~Calls() = default;
    virtual void stop_music() = 0;
    virtual void initialize_menu_sound() = 0;
    virtual void set_frame_rate(std::int32_t, std::int32_t) = 0;
    virtual void set_wait_vsync(std::int32_t) = 0;
    virtual void initialize_heap() = 0;
    virtual void initialize_tasks() = 0;
    virtual void set_background(std::uint32_t, std::uint32_t, std::uint32_t) = 0;
    virtual void set_fade_color(std::uint32_t) = 0;
    virtual void release_loading_font() = 0;
    virtual void free_character_textures() = 0;
    virtual void release_all_textures() = 0;
    virtual void initialize_texture_drawing() = 0;
    virtual void initialize_render_state() = 0;
    virtual void set_lighting(bool) = 0;
    virtual void set_view_angle(std::int32_t) = 0;
    virtual void set_clip(float, float) = 0;
    virtual void set_camera() = 0;
    virtual void load_regular_object_textures(std::int32_t) = 0;
    virtual void load_player_motion(std::int32_t) = 0;
    virtual void load_loading_font() = 0;
    virtual void copy_loading_font() = 0;
    // NOTSA: loading-screen composition of the recovered font resource release.
    virtual void release_font_resource() = 0;
    virtual void set_font_color(std::uint32_t, std::uint32_t, std::uint32_t,
                                std::uint32_t) = 0;
    virtual void draw_text(std::string_view, float, float, float) = 0;
    virtual TaskHandle create_sequence_task() = 0;
    virtual std::shared_ptr<SequenceWork> allocate_sequence_work(const TaskHandle &) = 0;
    virtual void initialize_menu_data() = 0;
    virtual void reset_selected_trial_level() = 0;
    virtual const SequenceData &sequence_data(std::int32_t) = 0;
    virtual TaskHandle create_sequence_child() = 0;
    virtual std::shared_ptr<SequenceChildWork> allocate_child_work(const TaskHandle &) = 0;
    virtual void install_child_callback(const TaskHandle &) = 0;
    virtual TaskHandle initialize_dialog() = 0;
    virtual void install_sequence_callbacks(const TaskHandle &) = 0;
    virtual void sequence_already_active() = 0;
};
enum class MenuDrawResult { Complete, NeedsDrawing };
// FUNCTION: SONIC 0x512450 CharacterSelectMenu_Display
MenuDrawResult draw_character_select(const CharacterSelectWork &);
// NOTSA: inactive-prefix adapter retained for the legacy title-menu oracle.
MenuDrawResult draw_file_select(const FileSelectWork &);
// FUNCTION: SONIC 0x50b410 trial_act_sel_disp
MenuDrawResult draw_level_select(const StageSelectWork &);
// FUNCTION: SONIC 0x511390 SoundTest_Display
MenuDrawResult draw_sound_test(const SoundTestWork &);
// FUNCTION: SONIC 0x50bcb0 title_menu_display
MenuDrawResult draw_main_menu(const MainMenuWork &);
// FUNCTION: SONIC 0x509810 OptionsMenu_Display
MenuDrawResult draw_options(const OptionWork &);
// FUNCTION: SONIC 0x50e230 explanation_display
MenuDrawResult draw_explanation(const ExplanationWork &);
// FUNCTION: SONIC 0x50a640 SubGameLevelList_Display
MenuDrawResult draw_subgame_list(const StageSelectWork &);
// FUNCTION: SONIC 0x50dfd0 EmblemResultMenu_Display
MenuDrawResult draw_emblem_results(const EmblemWork &);
// FUNCTION: SONIC 0x50d430 internet_demo_display
MenuDrawResult draw_internet_demo(const InternetDemoWork &);
// FUNCTION: SONIC 0x508450 tv_settings_display
MenuDrawResult draw_tv_settings(const TvSettingsWork &);
// NOTSA: inactive dialog prefix retained for the existing source-equivalence probe.
MenuDrawResult draw_dialog(const DialogWork &, X87Context &);
// A bounded native callback reports the first branch whose body is not recovered yet.
enum class MenuCallbackResult { Complete, NeedsInitialization, NeedsLeave,
                                NeedsFadeIn, NeedsControl, NeedsFadeOut };
// FUNCTION: SONIC 0x50c010 puti_exec
MenuCallbackResult update_puti(const TaskHandle &, const SequenceWork &, PutiCalls &);
// FUNCTION: SONIC 0x5122d0 CharacterSelectMenu_Main
MenuCallbackResult update_character_select(CharacterSelectWork &, SequenceWork &,
                                            std::uint8_t &character_cursor,
                                            std::uint8_t &player_character);
// NOTSA: retained inactive-prefix adapter; file_select::update owns the full source callback.
MenuCallbackResult update_file_select(FileSelectWork &, SequenceWork &);
// FUNCTION: SONIC 0x50b220 trial_act_sel_exec
MenuCallbackResult update_level_select(StageSelectWork &, SequenceWork &);
// FUNCTION: SONIC 0x511270 SoundTest_Main
MenuCallbackResult update_sound_test(SoundTestWork &, SequenceWork &);
// FUNCTION: SONIC 0x50bde0 title_menu_exec
MenuCallbackResult update_main_menu(MainMenuWork &, SequenceWork &);
// FUNCTION: SONIC 0x5086e0 FUN_005086e0
void update_option_animation(OptionWork &);
// FUNCTION: SONIC 0x5096f0 OptionsMenu_Main
MenuCallbackResult update_options(OptionWork &, SequenceWork &);
// FUNCTION: SONIC 0x50e110 explanation_exec
MenuCallbackResult update_explanation(ExplanationWork &, SequenceWork &);
// FUNCTION: SONIC 0x50a450 SubGameLevelList_Main
MenuCallbackResult update_subgame_list(StageSelectWork &, SequenceWork &);
// FUNCTION: SONIC 0x50dea0 EmblemResultMenu_Main
MenuCallbackResult update_emblem_results(EmblemWork &, SequenceWork &);
// FUNCTION: SONIC 0x50d310 internet_demo_exec
MenuCallbackResult update_internet_demo(InternetDemoWork &, SequenceWork &);
// FUNCTION: SONIC 0x508300 tv_settings_exec
MenuCallbackResult update_tv_settings(TvSettingsWork &, SequenceWork &);
// FUNCTION: SONIC 0x506230 FUN_00506230
void build_trial_stage_list(StageSelectWork &, const StageSelectionInputs &);
// FUNCTION: SONIC 0x506360 FUN_00506360
void build_subgame_stage_list(StageSelectWork &, const StageSelectionInputs &);
// FUNCTION: SONIC 0x509b40 FUN_00509b40
void build_display_stage_list(StageSelectWork &, const StageSelectionData &);
// FUNCTION: SONIC 0x50b1c0 FUN_0050b1c0
void initialize_level_select(StageSelectWork &, const ModeSelection &, const StageSelectionInputs &);
// FUNCTION: SONIC 0x50a3f0 FUN_0050a3f0
void initialize_subgame_select(StageSelectWork &, const ModeSelection &, const StageSelectionInputs &);
// FUNCTION: SONIC 0x509690 FUN_00509690
void initialize_options(OptionWork &, const ModeSelection &);
// FUNCTION: SONIC 0x50dc60 FUN_0050dc60
void initialize_emblem_selection(EmblemWork &);
// FUNCTION: SONIC 0x50b490 LevelSelect_Load
TaskHandle create_level_select(MenuFactoryState &, const ModeSelection &,
                               const StageSelectionInputs &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x511400 SoundTest_Load
TaskHandle create_sound_test(MenuFactoryState &, const ModeSelection &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x509860 OptionsMenu_Load
TaskHandle create_options(MenuFactoryState &, const ModeSelection &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x50a6c0 SubGameLevelList_Load
TaskHandle create_subgame_list(MenuFactoryState &, const ModeSelection &,
                               const StageSelectionInputs &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x50e020 EmblemResultMenu_Load
TaskHandle create_emblem_results(MenuFactoryState &, const ModeSelection &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x5084a0 tv_settings_create
TaskHandle create_tv_settings(MenuFactoryState &, const ModeSelection &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x5124a0 CharacterSelectMenu_Load
TaskHandle create_character_select(MenuFactoryState &, const ModeSelection &,
                                   const MenuFactoryInputs &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x505600 FileSelect_Load
TaskHandle create_file_select(MenuFactoryState &, const ModeSelection &,
                              MenuFactoryInputs &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x50bf30 title_menu_create
TaskHandle create_main_menu(MenuFactoryState &, const ModeSelection &,
                            const MenuFactoryInputs &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x50e270 explanation_create
TaskHandle create_explanation(MenuFactoryState &, const ModeSelection &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x50d470 internet_demo_create
TaskHandle create_internet_demo(MenuFactoryState &, const ModeSelection &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x50c140 puti_create
TaskHandle create_puti(MenuFactoryState &, MenuFactoryCalls &);
// FUNCTION: SONIC 0x5060e0 ResetSelectedTrialLevel
void reset_selected_trial_level(TrialSelection &);
// FUNCTION: SONIC 0x503270 FUN_00503270
std::int32_t run_title_tasks(State &, std::uint32_t &pause_mode, RunCalls &);
// FUNCTION: SONIC 0x503450 FUN_00503450
void draw_loading_text(const SharedState &, RunCalls &);
// FUNCTION: SONIC 0x5033b0 FUN_005033b0
std::int32_t run_title_menu(State &, SharedState &, std::uint32_t &pause_mode, RunCalls &);
// FUNCTION: SONIC 0x505b20 FUN_00505b20
void destroy_sequence_manager(State &, RunCalls &);
// FUNCTION: SONIC 0x503380 FUN_00503380
std::int32_t close_title_menu(State &, RunCalls &);
// FUNCTION: SONIC 0x4319d0 BetweenCol
std::uint32_t interpolate_colors(std::uint32_t first, std::uint32_t second,
                                  float progress, X87Context &);
// FUNCTION: SONIC 0x506e10 GetFadeOutColFromT
std::uint32_t fade_out_color(float, FadeCalls &, X87Context &);
// FUNCTION: SONIC 0x506e40 GetFadeInColFromT
std::uint32_t fade_in_color(float, FadeCalls &, X87Context &);
// FUNCTION: SONIC 0x5101a0 title_new_exec
void update_title(TitleWork &, State &, TitleTiming &, TitleUpdateCalls &, X87Context &);
// FUNCTION: SONIC 0x505b40 CmnAdvaModeProcedure
void request_menu_transition(SequenceWork &, MenuMode, ModeTransitionCalls &);
// FUNCTION: SONIC 0x50e300 title_new_control
void update_title_input(TitleWork &, SequenceWork &, TitleInputState &, TitleInputCalls &);
// FUNCTION: SONIC 0x50e470 FUN_0050e470
void draw_title_background(TitleWork &, const BackgroundView &, const BackgroundLayout &,
                            BackgroundCalls &, X87Context &);
// FUNCTION: SONIC 0x510350 title_new_display
void draw_title(TitleWork &, TitleDrawCalls &);
// FUNCTION: SONIC 0x510120 FUN_00510120
void initialize_title(TitleWork &, TitleInitState &, TitleInitCalls &);
// FUNCTION: SONIC 0x505a80 FUN_00505a80
void create_menu_objects(const SequenceData &, std::int32_t &pending_ticks, ObjectCalls &);
// FUNCTION: SONIC 0x510390 title_new_create
TaskHandle create_title(State &, const ModeSelection &, TitleCalls &);
// FUNCTION: SONIC 0x505c20 TitleMenuSequenceTask
void update_sequence(SequenceWork &, SequenceWork &canonical, std::int32_t &demo_mode, SequenceCalls &);
// FUNCTION: SONIC 0x505b60 FUN_00505b60
void update_sequence_child(const TaskHandle &, SequenceChildWork &, SequenceCalls &);
enum class DialogUpdateResult { Complete, NeedsOpening, NeedsControl, NeedsClosePause,
                                NeedsClosing, NeedsCleanup };
// FUNCTION: SONIC 0x431960 FUN_00431960
void update_dialog_animation(std::uint8_t &, std::optional<std::uint32_t> &angle,
                              float &fade, std::uint32_t loop_count, X87Context &);
// NOTSA: initialization/idle dialog prefix retained for the existing source-equivalence probe.
DialogUpdateResult update_dialog(DialogWork &, DialogWork &canonical,
                                  std::uint32_t loop_count, X87Context &);
// FUNCTION: SONIC 0x432650 FUN_00432650
void reset_dialog_work(DialogWork &, DialogWork &canonical);
// FUNCTION: SONIC 0x432690 FUN_00432690
void initialize_dialog_work(DialogWork &, DialogWork &canonical, std::uint32_t loop_count);
// FUNCTION: SONIC 0x432c60 LoadMenuObject
TaskHandle create_dialog(State &, std::uint32_t loop_count, DialogCalls &);
// FUNCTION: SONIC 0x425670 ADX_Close
void close_sound_manager(TaskHandle &, SoundManagerCalls &);
// FUNCTION: SONIC 0x425620 SoundManager_Delete
void delete_sound_manager(TaskHandle &, cleanup::TrackState &, SoundManagerCalls &);
// FUNCTION: SONIC 0x506010 AvaLoadTexForEachMode
void load_mode_textures(MenuMode, MenuTextureState &, const ModeTextureLists &,
                         const SequenceWork &, MenuTextureCalls &);
// FUNCTION: SONIC 0x50bd10 FUN_0050bd10
void set_main_menu_previous(MainMenuWork &);
// FUNCTION: SONIC 0x505ed0 FUN_00505ed0
void load_menu_textures(MenuTextureState &, const std::vector<MenuTextureId> &, MenuTextureCalls &);
// FUNCTION: SONIC 0x505fb0 FUN_00505fb0
void initialize_menu_data(MenuTextureState &, const std::vector<MenuTextureId> &, MenuTextureCalls &);
// FUNCTION: SONIC 0x505850 FUN_00505850
void initialize_menu_sound(MenuSoundState &, SoundCalls &);
// FUNCTION: SONIC 0x503480 FUN_00503480
void release_loading_font(SharedState &, Calls &);
// FUNCTION: SONIC 0x5034a0 FUN_005034a0
void prepare_title_assets(ActionBindings &, const CharacterActions &);
// FUNCTION: SONIC 0x5033f0 FUN_005033f0
void prepare_title_dialogs(SharedState &, Calls &);
// FUNCTION: SONIC 0x5031e0 FUN_005031e0
void reset_title_resources(Calls &);
// FUNCTION: SONIC 0x505b90 FUN_00505b90
TaskHandle initialize_sequence(State &, const std::shared_ptr<SequenceWork> &, std::int32_t,
                               Calls &);
// FUNCTION: SONIC 0x505e00 FUN_00505e00
TaskHandle create_sequence(State &, std::int32_t, Calls &);
// FUNCTION: SONIC 0x5032e0 FUN_005032e0
std::int32_t initialize_title_menu(State &, SharedState &, const std::array<std::uint8_t, 2> &,
                                  Calls &);
} // namespace sadx::title_menu
