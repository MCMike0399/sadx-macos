#include "runtime/native_title_scene.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_display.hpp"
#include "game/character_select_menu.hpp"
#include "game/character_select_labels.hpp"
#include "game/main_menu_eligibility.hpp"
#include "game/menu_dialog_control.hpp"
#include "math/ninja_sine_table.hpp"

namespace sadx::native {
// NOTSA: borrows canonical menu, save, frame and sequence owners at each callback.
struct TitleScene::CharacterMenuServices final : character_select_menu::Calls {
    CharacterMenuServices(TitleScene &owner, character_select_menu::Work &work)
        : scene(owner), work(work) {}
    TitleScene &scene;
    character_select_menu::Work &work;
    Session &session() { return scene.session_; }
    const CharacterSelectionAssets &assets() {
        if (!session().character_selection_assets_)
            throw flow::UnrecoveredGameFlowCall("native_character_selection_assets");
        return *session().character_selection_assets_;
    }
    menu_eligibility::Progress progress() {
        return {session().save_, session().bootstrap_.sequence_variables,
                session().bootstrap_.menu.flags[0], session().flow_.tutorial_variant};
    }
    character_select_menu::Availability availability() {
        return {progress(), {session().bootstrap_.completed_stages, scene.assets_.selection},
                session().bootstrap_.menu.flags[1], assets().clear_flags};
    }
    character_select_menu::State state(const std::uint32_t &buttons) {
        if (!scene.state_.sequence_work)
            throw flow::UnrecoveredGameFlowCall("native_character_selection_sequence");
        // NOTSA: corresponding signed/unsigned integer types may alias. The source
        // FILD reads this same 32-bit loop owner as signed; no second counter exists.
        const auto &ticks = reinterpret_cast<const std::int32_t &>(session().secondary_loop_count_);
        return {session().flow_.tutorial_variant, session().bootstrap_.menu.player_character,
                session().bootstrap_.menu.character_cursor, scene.textures_.ready, ticks,
                buttons, session().timing_.pending_logic_ticks,
                session().selection_saved_frame_increment_, session().bootstrap_.character,
                session().bootstrap_.menu.flags[1], *scene.state_.sequence_work};
    }
    void load_mode_textures(title_menu::MenuMode mode) override { scene.load_mode_textures(mode); }
    void create_preview() override { scene.create_character_preview(); }
    void request_music(std::int32_t cue) override { scene.request_menu_music(cue); }
    void request_voice(std::int32_t cue) override { scene.request_menu_voice(cue); }
    bool character_enabled(std::int32_t character) override {
        return menu_eligibility::character_enabled(character, progress()) != 0;
    }
    bool trial_enabled(std::int32_t character) override {
        return character_select_menu::trial_character_enabled(character, availability());
    }
    bool character_cleared(std::int32_t character) override {
        return character_select_menu::character_cleared(character, availability());
    }
    void release_loading_font() override {
        auto shared = scene.shared();
        title_menu::release_loading_font(shared, scene);
    }
    void calculate_ticks() override { scene.calculate_ticks(); }
    void navigation_sound() override { scene.play_navigation_sound(); }
    bool confirm_pressed() override {
        return file_select::confirm_button_mask(scene.pressed_buttons()) != 0;
    }
    bool cancel_pressed() override {
        return file_select::cancel_button_mask(scene.pressed_buttons()) != 0;
    }
    bool selection_enabled() override {
        const auto buttons = scene.pressed_buttons();
        return character_select_menu::selection_enabled(
            work.selected_character, state(buttons), *this);
    }
    void transition_menu(title_menu::MenuMode mode) override { scene.transition_menu(mode); }
    void cancel_sound() override { scene.play_cancel_sound(); }
    void open_dialog(std::int32_t identity) override {
        menu_dialog::open(assets().definition(identity), {scene.state_.dialog_task}, scene);
    }
    std::int8_t dialog_result() override { return static_cast<std::int8_t>(scene.dialog_result()); }
    void character_voice(std::int32_t character) override {
        if (character < 0 || static_cast<std::size_t>(character) >= assets().selected_voice_cues.size())
            throw flow::UnrecoveredGameFlowCall("native_selected_character_voice_index");
        auto services = scene.voice_task_services();
        menu_voice::request_voice(assets().selected_voice_cues[character], scene.sound_.volume_task,
                                  services);
    }
    void select_preview(std::int32_t) override {
        throw flow::UnrecoveredGameFlowCall("native_character_preview_motion_selection");
    }
    std::int32_t character_count() override { return character_select_menu::character_count(progress()); }
    void set_stage(std::int16_t stage, std::int16_t act) override {
        session().set_stage_number(static_cast<std::uint8_t>(stage), static_cast<std::uint8_t>(act));
    }
    void delete_preview() override { scene.remove_character_preview(); }
    std::uint32_t fade_in_color(float value) override { return scene.fade_in_color(value); }
    std::uint32_t fade_out_color(float value) override { return scene.fade_out_color(value); }
    void release_mode_textures(title_menu::MenuMode mode) override { scene.release_mode_textures(mode); }
};
void Session::attach_character_selection(CharacterSelectionAssets assets) {
    if (character_selection_assets_)
        throw std::runtime_error("native CharacterSelect assets already attached");
    const auto &binding = menu_texture_binding(assets.dialog_menu_texture);
    if (!binding.textures)
        throw flow::UnrecoveredGameFlowCall("native_character_selection_dialog_texture");
    for (auto &definition : assets.definitions)
        definition.textures = binding.textures;
    character_selection_assets_ = std::make_unique<CharacterSelectionAssets>(std::move(assets));
}
void TitleScene::exec_character_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_character_select);
    const auto work = std::static_pointer_cast<character_select_menu::Work>(task->auxiliary_native_work);
    const auto sequence = scene.state_.sequence_work;
    if (!work || !sequence)
        throw flow::UnrecoveredGameFlowCall("native_character_selection_work");
    CharacterMenuServices services(scene, *work);
    const auto buttons = scene.pressed_buttons();
    character_select_menu::update(*work, services.state(buttons), services, scene.display_.arithmetic());
}
void TitleScene::display_character_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_character_select);
    const auto work = std::static_pointer_cast<character_select_menu::Work>(task->auxiliary_native_work);
    if (!work)
        throw flow::UnrecoveredGameFlowCall("native_character_selection_work");
    scene.draw_character_menu(*work);
}
// NOTSA: shares the live PVR/menu device and source sky owner. Unrecovered
// label/panel/number helpers remain explicit at their actual draw calls.
void TitleScene::draw_character_menu(title_menu::CharacterSelectWork &work) {
    struct Draw final : character_select_menu::DrawCalls {
        TitleScene &scene;
        explicit Draw(TitleScene &value) : scene(value) {}
        pvr::State &state() { return scene.session_.logo_.drawing_state(); }
        void default_blending() override { scene.default_blending(); }
        void base_color(std::uint32_t value) override { scene.set_base_color(value); }
        void sky_background(float depth) override {
            if (!scene.session_.file_select_display_assets_)
                throw flow::UnrecoveredGameFlowCall("native_menu_sky_resources");
            const auto frame = scene.display_.frame_state();
            const auto &viewport = scene.display_.viewport();
            file_select_display::draw_sky_profiled(depth,
                {frame.horizontal_scale, frame.vertical_scale, frame.display_disabled,
                 viewport.width, viewport.height, scene.session_.file_select_display_assets_->tables.sky_tiles},
                state(), scene, scene.display_.arithmetic(), scene.display_.ui_scaling());
        }
        void select_texture(character_select_menu::TextureList list) override {
            scene.select_menu_texture(static_cast<file_select_display::MenuTexture>(
                list == character_select_menu::TextureList::CharacterSelect ? 2 : 3));
        }
        void material(std::uint32_t value) override { scene.set_material(value); }
        void colors(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d) override {
            menu_draw::set_vertex_colors({a, b, c, d}, state());
        }
        void draw_texture(std::uint32_t index, float x, float y, float depth) override {
            pvr::draw_texture(index, x, y, depth, state(), scene.display_.frame_state().display_disabled,
                              scene, scene.display_.arithmetic());
        }
        void draw_scaled(std::uint32_t index, float x, float y, float depth, float sx, float sy) override {
            scene.draw_scaled_texture(index, x, y, depth, sx, sy);
        }
        void title_back(float x, float y, float depth, float width, float height) override {
            menu_draw::draw_title_back(x, y, depth, width, height, state(),
                scene.display_.frame_state().display_disabled, scene, scene.display_.arithmetic());
        }
        void set_depth_comparison(std::int32_t mode) override {
            scene.display_.set_depth_comparison(static_cast<std::uint32_t>(mode));
        }
        void restore_depth_comparison() override { scene.display_.restore_depth_comparison(); }
        void round_panel(std::uint32_t color, float x, float y, float depth, float sx, float sy) override {
            menu_dialog::draw_round(color, x, y, depth, sx, sy, scene, scene.display_.arithmetic());
        }
        std::uint32_t interpolate_colors(std::uint32_t first, std::uint32_t second, float value) override {
            return title_menu::interpolate_colors(first, second, value, scene.display_.arithmetic());
        }
        character_select_labels::State label_state() {
            if (!scene.session_.character_label_textures_ || !scene.session_.file_select_display_assets_)
                throw flow::UnrecoveredGameFlowCall("native_character_label_resources");
            const auto frame = scene.display_.frame_state();
            return {frame.horizontal_scale, frame.vertical_scale, scene.session_.text_language_,
                    scene.session_.flow_.tutorial_variant, *scene.session_.character_label_textures_,
                    scene.session_.file_select_display_assets_->tables.character_names};
        }
        struct LabelCalls final : character_select_labels::Calls {
            Draw &draw;
            explicit LabelCalls(Draw &owner) : draw(owner) {}
            void select_texture(character_select_labels::Texture texture) override {
                std::uint32_t index;
                switch (texture) {
                case character_select_labels::Texture::Labels:
            index = 14;
            break;
                case character_select_labels::Texture::Names:
            index = 15;
            break;
                case character_select_labels::Texture::MetalName:
            index = 28;
            break;
                default: throw flow::UnrecoveredGameFlowCall("native_character_label_texture");
                }
                draw.scene.select_menu_texture(static_cast<file_select_display::MenuTexture>(index));
            }
            void material(std::uint32_t color) override { draw.material(color); }
            void draw_texture(std::uint32_t index, float x, float y, float depth) override {
                draw.draw_texture(index, x, y, depth);
            }
            void draw_scaled(std::uint32_t index, float x, float y, float depth, float sx, float sy) override {
                draw.draw_scaled(index, x, y, depth, sx, sy);
            }
        };
        void character_name(std::int32_t index, std::uint32_t color, float x, float y, float depth,
                            float sx, float sy) override {
            LabelCalls calls(*this);
            character_select_labels::draw_name(index, color, x, y, depth, sx, sy,
                                                label_state(), calls, scene.display_.arithmetic());
        }
        void character_label(std::int32_t index, std::uint32_t color, float x, float y, float depth) override {
            LabelCalls calls(*this);
            character_select_labels::draw_label(index, color, x, y, depth,
                                                 label_state(), calls, scene.display_.arithmetic());
        }
        std::uint32_t character_completion(std::int32_t character) override {
            if (!scene.session_.character_completion_)
                throw flow::UnrecoveredGameFlowCall("native_character_completion_resources");
            return static_cast<std::uint32_t>(sadx::character_completion::value(
                character, *scene.session_.character_completion_,
                {scene.session_.bootstrap_.sequence_variables, scene.session_.bootstrap_.cutscene_flags}));
        }
        void draw_number(std::uint32_t value, std::int32_t digits, bool zero_fill,
                         float x, float y, float depth, float spacing) override {
            if (!scene.session_.file_select_display_assets_)
                throw flow::UnrecoveredGameFlowCall("native_menu_digit_resources");
            const auto frame = scene.display_.frame_state();
            file_select_display::draw_number(static_cast<std::int32_t>(value), digits,
                zero_fill, x, y, depth, spacing,
                {frame.horizontal_scale, frame.vertical_scale, frame.display_disabled,
                 scene.session_.file_select_display_assets_->tables.digits},
                state(), scene, scene.display_.arithmetic());
        }
    } calls(*this);
    CharacterMenuServices services(*this, work);
    const auto buttons = pressed_buttons();
    const auto frame = display_.frame_state();
    const auto &assets = services.assets();
    const NinjaMath math(ninja_sine_table(), display_.arithmetic());
    character_select_menu::draw(work, services.state(buttons),
        {frame.horizontal_scale, frame.vertical_scale, assets.name_widths}, services, calls,
        math, display_.arithmetic());
}
} // namespace sadx::native
