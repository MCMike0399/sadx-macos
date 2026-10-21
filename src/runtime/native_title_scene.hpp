#pragma once
#include "game/title_menu.hpp"
#include "game/music.hpp"
#include "runtime/title_assets.hpp"
#include <functional>

namespace sadx::native {
class Session;
class Display;
// NOTSA: connects the recovered title sequence to the session's existing owners.
class TitleScene final : private title_menu::Calls,
                         private title_menu::SoundCalls,
                         private title_menu::SoundManagerCalls,
                         private title_menu::MenuTextureCalls,
                         private title_menu::DialogCalls,
                         private title_menu::SequenceCalls,
                         private title_menu::ObjectCalls,
                         private title_menu::MenuFactoryCalls,
                         private title_menu::PutiCalls,
                         private title_menu::TitleCalls,
                         private title_menu::TitleInitCalls,
                         private title_menu::TitleUpdateCalls,
                         private title_menu::TitleDrawCalls,
                         private title_menu::RunCalls,
                         private title_menu::FadeCalls,
                         private title_menu::BackgroundCalls,
                         private title_menu::TitleInputCalls,
                         private title_menu::ModeTransitionCalls {
  public:
    TitleScene(Session &, Display &, const TitleAssets &);
    ~TitleScene();
    struct InputServices {
        std::function<void()> poll_controls;
        std::function<void()> enable_control;
        std::function<std::int32_t(std::int32_t)> soft_reset_check;
        std::function<std::int32_t(std::int32_t)> key_pressed;
        std::function<std::uint32_t()> pressed_buttons;
    };
    void attach_input(InputServices input) { input_ = std::move(input); }
    void initialize();
    std::int32_t run();
    std::int32_t close();
    const title_menu::State &state() const { return state_; }

  private:
    InputServices input_;
    Session &session_;
    Display &display_;
    const TitleAssets &assets_;
    title_menu::State state_;
    title_menu::MenuSoundState sound_;
    title_menu::MenuTextureState textures_;
    struct CallbackContext;
    std::shared_ptr<CallbackContext> callback_context_;
    static TitleScene &callback_scene(TaskScheduler &, TaskCallback);
    static void update_sequence(TaskScheduler &, const TaskHandle &);
    static void update_child(TaskScheduler &, const TaskHandle &);
    static void delete_sequence(TaskScheduler &, const TaskHandle &);
    static void update_dialog(TaskScheduler &, const TaskHandle &);
    static void display_dialog(TaskScheduler &, const TaskHandle &);
    TaskHandle create_task(std::uint32_t flags, int level, TaskCallback);
    void register_callback(TaskCallback);
    bool allocate_auxiliary(const TaskHandle &, std::uint32_t size, bool zero);
    static TaskCallback menu_exec(title_menu::MenuMode);
    static TaskCallback menu_display(title_menu::MenuMode);
    static void exec_puti(TaskScheduler &, const TaskHandle &);
    static void delete_puti(TaskScheduler &, const TaskHandle &);
    template<class Work> std::shared_ptr<Work> allocate_menu_work(const TaskHandle &, std::uint32_t);
    title_menu::MenuFactoryInputs factory_inputs();
    title_menu::StageSelectionInputs selection_inputs();
    void require_complete(title_menu::MenuCallbackResult, const char *);
    void require_drawn(title_menu::MenuDrawResult, const char *);
    void calculate_ticks() override;
    TaskHandle create_menu_task(title_menu::MenuMode, std::uint32_t, std::uint32_t) override;
    void install_menu_callbacks(title_menu::MenuMode, const TaskHandle &) override;
    std::shared_ptr<title_menu::PutiWork> allocate_puti_work(const TaskHandle &) override;
    void install_puti_callbacks(const TaskHandle &) override;
    TaskHandle create_title_task() override;
    std::shared_ptr<title_menu::TitleWork> allocate_title_work(const TaskHandle &) override;
    void install_title_callbacks(const TaskHandle &) override;
    void set_stage(std::uint8_t, std::uint8_t) override;
    void initialize_character_lighting() override;
    void load_mode_textures(title_menu::MenuMode) override;
    void load_japanese_title_textures() override;
    void prepare_menu_voice() override;
    void set_title_menu_previous() override;
    void initialize_title(title_menu::TitleWork &) override;
    std::uint32_t fade_out_color(float) override;
    std::uint32_t fade_in_color(float) override;
    void update_title_input(title_menu::TitleWork &) override;
    void request_menu_music(std::int32_t) override;
    std::int32_t key_pressed(std::int32_t) override;
    std::uint32_t pressed_buttons() override;
    void transition_menu(title_menu::MenuMode) override;
    void stop_menu_music() override;
    void play_title_confirm_sound() override;
    std::int32_t audio_ready() override;
    std::uint32_t music_state() override;
    void release_mode_textures(title_menu::MenuMode) override;
    void release_japanese_title_textures() override;
    void disable_alpha_test() override;
    void default_blending() override;
    void set_base_color(std::uint32_t) override;
    void draw_title_background(title_menu::TitleWork &) override;
    void set_material(std::uint32_t) override;
    void select_title_texture(title_menu::TitleTextureList) override;
    void draw_scaled_texture(std::uint32_t, float, float, float, float, float) override;
    void set_depth_compare(std::int32_t) override;
    void poll_controls() override;
    void enable_control() override;
    void run_tasks() override;
    void disable_fog() override;
    void display_tasks() override;
    std::int32_t soft_reset_check(std::int32_t) override;
    void delete_all_tasks() override;
    void free_regular_object_textures() override;
    void destroy_sequence_task(const TaskHandle &) override;
    static void exec_character_select(TaskScheduler &, const TaskHandle &);
    static void display_character_select(TaskScheduler &, const TaskHandle &);
    static void exec_file_select(TaskScheduler &, const TaskHandle &);
    static void display_file_select(TaskScheduler &, const TaskHandle &);
    static void exec_level_select(TaskScheduler &, const TaskHandle &);
    static void display_level_select(TaskScheduler &, const TaskHandle &);
    static void exec_sound_test(TaskScheduler &, const TaskHandle &);
    static void display_sound_test(TaskScheduler &, const TaskHandle &);
    static void exec_main_menu(TaskScheduler &, const TaskHandle &);
    static void display_main_menu(TaskScheduler &, const TaskHandle &);
    static void exec_options(TaskScheduler &, const TaskHandle &);
    static void display_options(TaskScheduler &, const TaskHandle &);
    static void exec_explanation(TaskScheduler &, const TaskHandle &);
    static void display_explanation(TaskScheduler &, const TaskHandle &);
    static void exec_subgame_list(TaskScheduler &, const TaskHandle &);
    static void display_subgame_list(TaskScheduler &, const TaskHandle &);
    static void exec_emblem_results(TaskScheduler &, const TaskHandle &);
    static void display_emblem_results(TaskScheduler &, const TaskHandle &);
    static void exec_internet_demo(TaskScheduler &, const TaskHandle &);
    static void display_internet_demo(TaskScheduler &, const TaskHandle &);
    static void exec_tv_settings(TaskScheduler &, const TaskHandle &);
    static void display_tv_settings(TaskScheduler &, const TaskHandle &);
    music::TaskServices music_task_services();
    static void load_music_manager(TaskScheduler &, const TaskHandle &);
    static void update_music_manager(TaskScheduler &, const TaskHandle &);
    static void delete_music_manager(TaskScheduler &, const TaskHandle &);
    static void update_music_request(TaskScheduler &, const TaskHandle &);
    static void exec_title(TaskScheduler &, const TaskHandle &);
    static void display_title(TaskScheduler &, const TaskHandle &);
    TaskHandle create_character_select(const title_menu::ModeSelection &) override;
    TaskHandle create_file_select(const title_menu::ModeSelection &) override;
    TaskHandle create_level_select(const title_menu::ModeSelection &) override;
    TaskHandle create_sound_test(const title_menu::ModeSelection &) override;
    TaskHandle create_title(const title_menu::ModeSelection &) override;
    TaskHandle create_title_menu(const title_menu::ModeSelection &) override;
    TaskHandle create_options(const title_menu::ModeSelection &) override;
    TaskHandle create_explanation(const title_menu::ModeSelection &) override;
    TaskHandle create_subgame_list(const title_menu::ModeSelection &) override;
    TaskHandle create_emblem_results(const title_menu::ModeSelection &) override;
    TaskHandle create_internet_demo(const title_menu::ModeSelection &) override;
    TaskHandle create_tv_settings(const title_menu::ModeSelection &) override;
    TaskHandle create_puti(const title_menu::ModeSelection &) override;
    std::shared_ptr<title_menu::CharacterSelectWork> allocate_character_select(const TaskHandle &) override;
    std::shared_ptr<title_menu::FileSelectWork> allocate_file_select(const TaskHandle &) override;
    std::shared_ptr<title_menu::StageSelectWork> allocate_level_select(const TaskHandle &) override;
    std::shared_ptr<title_menu::SoundTestWork> allocate_sound_test(const TaskHandle &) override;
    std::shared_ptr<title_menu::MainMenuWork> allocate_main_menu(const TaskHandle &) override;
    std::shared_ptr<title_menu::OptionWork> allocate_options(const TaskHandle &) override;
    std::shared_ptr<title_menu::ExplanationWork> allocate_explanation(const TaskHandle &) override;
    std::shared_ptr<title_menu::StageSelectWork> allocate_subgame(const TaskHandle &) override;
    std::shared_ptr<title_menu::EmblemWork> allocate_emblem(const TaskHandle &) override;
    std::shared_ptr<title_menu::InternetDemoWork> allocate_internet_demo(const TaskHandle &) override;
    std::shared_ptr<title_menu::TvSettingsWork> allocate_tv_settings(const TaskHandle &) override;
    std::uint8_t font_loaded_ = 0;
    title_menu::SharedState shared();
    void stop_music() override;
    void initialize_menu_sound() override;
    void select_sound_bank(std::int32_t) override;
    void destroy_sound_manager(const TaskHandle &) override;
    void close_media(std::uint32_t) override;
    void set_frame_rate(std::int32_t, std::int32_t) override;
    void set_wait_vsync(std::int32_t) override;
    void initialize_heap() override;
    void initialize_tasks() override;
    void set_background(std::uint32_t, std::uint32_t, std::uint32_t) override;
    void set_fade_color(std::uint32_t) override;
    void release_loading_font() override;
    void free_character_textures() override;
    void release_all_textures() override;
    void initialize_texture_drawing() override;
    void initialize_render_state() override;
    void set_lighting(bool) override;
    void set_view_angle(std::int32_t) override;
    void set_clip(float, float) override;
    void set_camera() override;
    void load_regular_object_textures(std::int32_t) override;
    void load_player_motion(std::int32_t) override;
    void load_loading_font() override;
    void copy_loading_font() override;
    void release_font_resource() override;
    void set_font_color(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) override;
    void draw_text(std::string_view, float, float, float) override;
    TaskHandle create_sequence_task() override;
    std::shared_ptr<title_menu::SequenceWork> allocate_sequence_work(const TaskHandle &) override;
    void initialize_menu_data() override;
    void reset_selected_trial_level() override;
    const title_menu::SequenceData &sequence_data(std::int32_t) override;
    TaskHandle create_sequence_child() override;
    std::shared_ptr<title_menu::SequenceChildWork> allocate_child_work(const TaskHandle &) override;
    void install_child_callback(const TaskHandle &) override;
    TaskHandle initialize_dialog() override;
    void install_sequence_callbacks(const TaskHandle &) override;
    void sequence_already_active() override;
    void load_menu_texture(title_menu::MenuTextureId) override;
    TaskHandle create_dialog_task() override;
    std::shared_ptr<title_menu::DialogWork> allocate_dialog_work(const TaskHandle &) override;
    void install_dialog_callbacks(const TaskHandle &) override;
    void set_entrance(std::int32_t) override;
    void release_menu_textures() override;
    void finalize_selection() override;
    void create_menu_objects(const title_menu::SequenceData *) override;
    void destroy_sequence_child(const TaskHandle &) override;
};
} // namespace sadx::native
