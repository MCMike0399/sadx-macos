#include "game/main_menu.hpp"
#include "game/main_menu_eligibility.hpp"
#include "game/menu_dialog_control.hpp"
#include "game/menu_navigation.hpp"
#include "runtime/main_menu_assets.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_title_scene.hpp"

namespace sadx::native {
// NOTSA: adapts narrower source return widths without adding duplicate native owners.
struct TitleScene::MainMenuServices final : main_menu::UpdateCalls,
                                            main_menu::ControlCalls,
                                            main_menu::DrawCalls,
                                            main_menu_content::Calls,
                                            menu_dialog_control::Definitions,
                                            menu_navigation::Calls {
    explicit MainMenuServices(TitleScene &scene) : scene(scene) {}
    TitleScene &scene;
    Session &session() { return scene.session_; }
    menu_eligibility::Progress progress() {
        auto &state = session();
        return {state.save_, state.bootstrap_.sequence_variables, state.bootstrap_.menu.flags[0],
                state.flow_.tutorial_variant};
    }
    menu_eligibility::Completion completion() {
        return {session().bootstrap_.completed_stages, scene.assets_.selection};
    }
    const MainMenuAssets &assets() {
        if (!session().main_menu_assets_)
            throw flow::UnrecoveredGameFlowCall("native_main_menu_assets");
        return *session().main_menu_assets_;
    }
    const menu_dialog::Parameters &parameters(std::int32_t identity) override {
        return assets().definition(identity);
    }
    main_menu::InitState initialization() {
        const auto title_task = scene.state_.title_task;
        const auto menu_task = scene.state_.factories.main_menu_task;
        const auto title =
            title_task
                ? std::static_pointer_cast<title_menu::TitleWork>(title_task->auxiliary_native_work)
                : std::shared_ptr<title_menu::TitleWork>{};
        const auto menu =
            menu_task ? std::static_pointer_cast<main_menu::Work>(menu_task->auxiliary_native_work)
                      : std::shared_ptr<main_menu::Work>{};
        if (!title || !menu)
            throw std::runtime_error("native main menu canonical task has no auxiliary work");
        auto &state = session();
        return {state.text_language_,
                state.voice_language_,
                state.flow_.tutorial_variant,
                state.bootstrap_.menu.title_count,
                *title,
                *menu};
    }
    std::shared_ptr<main_menu::Sequence> sequence() {
        const auto task = scene.state_.sequence_task;
        const auto work =
            task ? std::static_pointer_cast<main_menu::Sequence>(task->auxiliary_native_work)
                 : std::shared_ptr<main_menu::Sequence>{};
        if (!work)
            throw std::runtime_error("native main menu canonical sequence task has no work");
        return work;
    }
    std::int8_t text_language() override {
        return menu_eligibility::text_language(session().save_, session().text_language_);
    }
    std::int8_t voice_language() override {
        return static_cast<std::int8_t>(
            menu_eligibility::voice_language(session().save_, session().text_language_));
    }
    void load_mode_textures(main_menu::Mode mode) override { scene.load_mode_textures(mode); }
    void request_menu_music(std::int32_t cue) override { scene.request_menu_music(cue); }
    std::uint32_t fade_out_color(float progress) override { return scene.fade_out_color(progress); }
    std::uint32_t fade_in_color(float progress) override { return scene.fade_in_color(progress); }
    void release_mode_textures(main_menu::Mode mode) override { scene.release_mode_textures(mode); }
    void update_controls(main_menu::Work &work) override {
        auto &state = session();
        const auto current_sequence = sequence();
        main_menu::ControlState control{scene.textures_.ready,
                                        state.flow_.tutorial_variant,
                                        state.bootstrap_.menu.flags[1],
                                        state.quit_,
                                        state.save_,
                                        state.save_backup_,
                                        *current_sequence,
                                        assets().variant_mapping};
        main_menu::update_controls(work, control, *this);
    }
    bool trial_enabled() override {
        return menu_eligibility::trial_enabled(progress(), completion()) != 0;
    }
    bool mission_enabled() override { return menu_eligibility::mission_enabled(progress()) != 0; }
    bool optional_entry_enabled() override {
        return menu_eligibility::optional_entry_enabled(progress(), session().missions_) != 0;
    }
    bool optional_entry_available() override { return optional_entry_enabled(); }
    bool trial_stages_enabled() override {
        return menu_eligibility::total_trial_stage_count(
                   progress(), completion(), session().bootstrap_.menu.player_character) != 0;
    }
    bool minigames_enabled() override {
        return menu_eligibility::total_minigame_count(progress(), completion()) != 0;
    }
    void open_dialog(main_menu::Dialog identity, std::int8_t cursor,
                     const std::array<std::int8_t, 3> &disabled, bool quick) override {
        const menu_dialog::Choices choices{disabled.data(), disabled.size()};
        const menu_dialog::Owner owner{scene.state_.dialog_task};
        if (quick)
            menu_dialog_control::open_advanced_quick(static_cast<std::int32_t>(identity), cursor,
                                                     choices, *this, owner);
        else
            menu_dialog_control::open_advanced(static_cast<std::int32_t>(identity), cursor, choices,
                                               *this, owner, scene);
    }
    std::int8_t dialog_result() override { return static_cast<std::int8_t>(scene.dialog_result()); }
    std::int32_t music_state() override { return static_cast<std::int32_t>(scene.music_state()); }
    std::int16_t dialog_idle_frames() override {
        return menu_dialog_control::idle_frames({scene.state_.dialog_task});
    }
    void reset_dialog_idle_frames() override {
        menu_dialog_control::reset_idle_frames(0, {scene.state_.dialog_task});
    }
    bool close_dialog() override { return menu_dialog_control::close({scene.state_.dialog_task}); }
    void apply_save() override { scene.apply_save(); }
    void transition_menu(main_menu::Mode mode) override { scene.transition_menu(mode); }
    void stop_menu_music() override { scene.stop_menu_music(); }
    void prepare_character_select(std::int32_t next) override {
        menu_navigation::prepare_character_select(
            static_cast<main_menu::Mode>(next),
            {scene.state_.factories.character_select_task, scene.state_.sequence_task}, *this);
    }
    void default_blending() override { scene.default_blending(); }
    void set_base_color(std::uint32_t color) override { scene.set_base_color(color); }
    void set_material(std::uint32_t color) override { scene.set_material(color); }
    void select_common_texture() override {
        scene.select_menu_texture(static_cast<file_select_display::MenuTexture>(19));
    }
    void select_texture(main_menu::TextureList list) override {
        const auto identity = list == main_menu::TextureList::Common ? 19 : 20;
        scene.select_menu_texture(static_cast<file_select_display::MenuTexture>(identity));
    }
    void draw_scaled(std::uint32_t texture, float x, float y, float depth, float scale_x,
                     float scale_y) override {
        scene.draw_scaled_texture(texture, x, y, depth, scale_x, scale_y);
    }
};
void TitleScene::exec_main_menu(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_main_menu);
    const auto work = std::static_pointer_cast<main_menu::Work>(task->auxiliary_native_work);
    if (!work)
        throw std::runtime_error("native main menu task has no work");
    MainMenuServices services(scene);
    const auto current_sequence = services.sequence();
    main_menu::update(*work, *current_sequence, services.initialization(),
                      scene.session_.secondary_loop_count_, services, scene.display_.arithmetic());
}
void TitleScene::display_main_menu(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_main_menu);
    const auto work = std::static_pointer_cast<main_menu::Work>(task->auxiliary_native_work);
    if (!work)
        throw std::runtime_error("native main menu task has no work");
    const auto frame = scene.display_.frame_state();
    const auto &viewport = scene.display_.viewport();
    MainMenuServices services(scene);
    main_menu::draw_profiled(*work,
                             {frame.horizontal_scale, frame.vertical_scale,
                              scene.session_.dialog_window_after_content_, viewport.width,
                              viewport.height},
                             services, scene.display_.arithmetic());
}
void Session::attach_main_menu(MainMenuAssets assets) {
    if (main_menu_assets_)
        throw std::runtime_error("native MainMenu assets are already attached");
    const auto &binding = menu_texture_binding(19);
    if (!binding.textures)
        throw std::runtime_error("native MainMenu common texture list is unavailable");
    for (auto &definition : assets.definitions)
        definition.textures = binding.textures;
    main_menu_assets_ = std::make_unique<MainMenuAssets>(std::move(assets));
}
void TitleScene::draw_main_menu_content(title_menu::DialogContent content,
                                        const menu_dialog::ContentArguments &arguments) {
    MainMenuServices services(*this);
    const auto frame = display_.frame_state();
    main_menu_content::State state{frame.horizontal_scale, frame.vertical_scale,
                                   services.assets().trial_layout};
    if (content == title_menu::DialogContent::MainMenuBackdrop)
        main_menu_content::draw_main(arguments, state, services, display_.arithmetic());
    else if (content == title_menu::DialogContent::TrialMenuBackdrop)
        main_menu_content::draw_trial(arguments, state, services, display_.arithmetic());
    else
        throw std::runtime_error("native MainMenu callback has an invalid content identity");
}
} // namespace sadx::native
