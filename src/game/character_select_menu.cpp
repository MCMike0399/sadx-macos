#include "game/character_select_menu.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>

namespace sadx::character_select_menu {
void initialize(Work &work, State state, Calls &calls) {
    calls.load_mode_textures(Mode::CharacterSelect);
    calls.create_preview();
    calls.request_music(state.variant == 0 ? 3 : 4);
    calls.request_voice(2);
    const auto selected = work.selected_character;
    work.cursor_angle = 0;
    work.angle_progress = 0;
    work.submode = 0;
    bool enabled = false;
    if (selected != 6 || state.variant != 2) {
        switch (state.variant) {
        case 0:
            enabled = calls.character_enabled(selected);
            break;
        case 1:
            enabled = calls.trial_enabled(selected);
            break;
        case 2:
            enabled = calls.character_cleared(selected);
            break;
        default: break;
        }
    }
    if (!enabled) {
        work.selected_character = 0;
        state.selected_character = 0;
    }
    calls.release_loading_font();
}
namespace {
// NOTSA: shared source control tail restores the resting carousel.
void rest(Work &work) {
    work.cursor_angle = 0;
    work.angle_progress = 0;
    work.submode = 0;
}
// NOTSA: names the original character-change tail shared by left and right.
void change_character(Work &work, State state, Calls &calls, bool previous) {
    const auto count = calls.character_count();
    const auto numerator = previous ? count - 1 + work.selected_character : work.selected_character + 1;
    const auto divisor = previous ? calls.character_count() : count;
    if (divisor == 0)
        throw std::domain_error("character carousel divides by zero");
    work.selected_character = numerator % divisor;
    state.selected_character = static_cast<std::uint8_t>(work.selected_character);
    state.player_character = 0;
    if (work.selected_character == 5 || work.selected_character == 6)
        state.player_character = 6;
    rest(work);
}
// NOTSA: names the confirm branch of the source control callback.
void confirm(Work &work, State state, Calls &calls) {
    if (!calls.selection_enabled())
        return;
    work.frames = 0;
    if (state.variant == 0) {
        if (work.selected_character == 6) {
            const bool cleared = calls.character_cleared(6);
            calls.open_dialog(cleared ? 10 : 9);
            work.submode = cleared ? 7 : 6;
            return;
        }
        if (calls.character_cleared(work.selected_character)) {
            calls.open_dialog(8);
            work.submode = 5;
            return;
        }
    } else if (state.variant == 1) {
        calls.character_voice(work.selected_character + (work.selected_character == 6 ? 1 : 0));
        calls.select_preview(1);
        work.submode = 3;
        return;
    }
    calls.open_dialog(7);
    work.submode = 4;
}
// NOTSA: names the four source dialog-result branches without changing their out-of-range behavior.
void dialog(Work &work, Calls &calls) {
    const auto choice = calls.dialog_result();
    if (choice < 0)
        return;
    switch (work.submode) {
    case 4:
        if (choice <= 1) {
            work.dialog_status = choice;
            work.submode = 8;
        } else if (choice == 2) {
            work.dialog_status = 3;
            work.submode = 8;
        }
        work.submode = 8;
        return;
    case 5:
        if (choice <= 3)
            work.dialog_status = choice;
        work.submode = 8;
        return;
    case 6:
        if (choice == 0 || choice == 1) {
            work.dialog_status = choice == 0 ? 0 : 3;
            work.submode = 8;
        }
        work.submode = 8;
        return;
    case 7:
        if (choice <= 2) {
            work.dialog_status = choice == 0 ? 0 : choice + 1;
            work.submode = 8;
        }
        work.submode = 8;
        return;
    default: return;
    }
}
// NOTSA: preserves signed-short wrap before testing the source delay.
bool delay(Work &work, State state) {
    work.frames = static_cast<std::int16_t>(static_cast<std::uint16_t>(work.frames) +
                                          static_cast<std::uint16_t>(state.logic_ticks));
    return work.frames > 299;
}
} // namespace
void update_controls(Work &work, State state, Calls &calls, X87Context &fp) {
    calls.calculate_ticks();
    state.frame_increment = 0;
    state.saved_frame_increment = state.logic_ticks;
    using fidelity::ExactFloat;
    switch (work.submode) {
    case 0:
        work.cursor_angle = static_cast<std::int32_t>(static_cast<std::uint32_t>(work.cursor_angle) +
                            static_cast<std::uint32_t>(state.logic_ticks) * 1456U);
        if ((state.pressed_buttons & 64U) != 0 || (state.pressed_buttons & 128U) != 0) {
            work.submode = (state.pressed_buttons & 64U) != 0 ? 1 : 2;
            calls.navigation_sound();
            return;
        }
        if (calls.confirm_pressed() && state.ready == 1) {
            confirm(work, state, calls);
            return;
        }
        if (calls.cancel_pressed() && state.ready == 1) {
            calls.transition_menu(work.presentation.previous);
            calls.cancel_sound();
            work.submode = 10;
        }
        return;
    case 1:
    case 2: {
        const bool previous = work.submode == 1;
        const auto step = ExactFloat::integer(fp, state.logic_ticks) * 0.05f;
        const auto progress = previous ? step + work.angle_progress : ExactFloat(fp, work.angle_progress) - step;
        work.angle_progress = progress.rounded();
        const auto comparison = progress.compare(previous ? 1.0f : -1.0f);
        if (previous ? comparison == X87Comparison::greater || comparison == X87Comparison::equal
                     : comparison == X87Comparison::less || comparison == X87Comparison::equal)
            change_character(work, state, calls, previous);
        return;
    }
    case 3:
        if (delay(work, state)) {
            calls.transition_menu(state.trial_subgame != 0 ? Mode::MinigameSelect : Mode::TrialActSelect);
            work.submode = 10;
        }
        return;
    case 4: case 5: case 6: case 7:
        dialog(work, calls);
        return;
    case 8:
        if (work.dialog_status == 3) {
            rest(work);
            return;
        }
        work.frames = 0;
        work.submode = 9;
        calls.character_voice(work.selected_character);
        calls.select_preview(1);
        return;
    case 9:
        if (delay(work, state)) {
            switch (work.dialog_status) {
            case 0:
                state.sequence.return_value = state.variant == 2 ? 102 : 101;
                break;
            case 1:
                state.sequence.return_value = 104;
                break;
            case 2:
                state.sequence.return_value = 103;
                break;
            default: break;
            }
            calls.transition_menu(Mode::None);
            work.submode = 10;
        }
        return;
    default: return;
    }
}
void update(Work &work, State state, Calls &calls, X87Context &fp) {
    auto &presentation = work.presentation;
    if (state.sequence.mode == Mode::CharacterSelect && presentation.phase == Phase::RequestWait) {
        calls.set_stage(0, 1);
        initialize(work, state, calls);
        presentation.fade_progress = 0;
        presentation.phase = Phase::FadeIn;
    }
    if (state.sequence.old_mode == Mode::CharacterSelect && presentation.phase == Phase::Keep) {
        calls.delete_preview();
        presentation.fade_progress = 0;
        presentation.phase = Phase::FadeOut;
    }
    work.selected_character = state.selected_character;
    state.character_cursor = state.selected_character;
    state.selected_character = static_cast<std::uint8_t>(work.selected_character);
    switch (presentation.phase) {
    case Phase::RequestWait:
        presentation.color = 0;
        return;
    case Phase::FadeIn: case Phase::FadeOut: {
        const bool leaving = presentation.phase == Phase::FadeOut;
        presentation.color = leaving ? calls.fade_in_color(presentation.fade_progress)
                                     : calls.fade_out_color(presentation.fade_progress);
        using fidelity::ExactFloat;
        const auto step = ExactFloat::integer(fp, state.logic_ticks) * 0.1f;
        const auto progress = step + presentation.fade_progress;
        presentation.fade_progress = progress.rounded();
        const auto comparison = progress.compare(1.0f);
        if (comparison != X87Comparison::equal && comparison != X87Comparison::greater)
            return;
        if (leaving) {
            state.sequence.fade_out = 1;
            calls.release_mode_textures(Mode::CharacterSelect);
            calls.delete_preview();
            state.sequence.fade_end = 1;
            presentation.phase = Phase::RequestWait;
        } else {
            presentation.phase = Phase::Keep;
        }
        return;
    }
    case Phase::Keep:
        work.wake = 0;
        state.sequence.fade_out = 0;
        presentation.color = 4294967295U;
        state.sequence.fixed_mode = Mode::CharacterSelect;
        update_controls(work, state, calls, fp);
        return;
    }
}
} // namespace sadx::character_select_menu

#include "math/ninja_math.hpp"
namespace sadx::character_select_menu {
namespace {
// NOTSA: the three source variant predicates share the same argument and return shape.
bool enabled(std::int32_t character, State state, Calls &calls) {
    switch (state.variant) {
    case 0: return calls.character_enabled(character);
    case 1: return calls.trial_enabled(character);
    case 2: return calls.character_cleared(character);
    default: return false;
    }
}
// NOTSA: names the source's repeated centered screen coordinates and binary32 stores.
class Projection {
  public:
    DrawState state;
    X87Context &fp;
    float x(float offset) { return (fidelity::ExactFloat(fp, state.horizontal_scale) * 320.0f + offset).rounded(); }
    float y(float offset) { return (fidelity::ExactFloat(fp, state.vertical_scale) * 240.0f + offset).rounded(); }
    float z(const Work &work, float offset) {
        return (fidelity::ExactFloat(fp, work.presentation.depth) - offset).rounded();
    }
};
}
void draw_background(const Work &work, State state, DrawState view, DrawCalls &calls, X87Context &fp) {
    Projection p{view, fp};
    calls.select_texture(TextureList::CharacterSelect);
    calls.material(4294967295U);
    calls.title_back(p.x(-320), p.y(-208), p.z(work, 8), 557, 41);
    calls.draw_texture(9, p.x(-294), p.y(-199), p.z(work, 14));
    calls.draw_texture(67108871U, p.x(-4), p.y(4), p.z(work, 10));
    calls.colors(2046786048U, 2046820096U, 2046786048U, 2046820096U);
    calls.draw_scaled(67108868U, p.x(-4), p.y(4), p.z(work, 8), 1.172f, 1.172f);
    calls.colors(4293096704U, 4294887424U, 4293096704U, 4294887424U);
    calls.draw_scaled(117440518U, p.x(-4), p.y(4), p.z(work, 10), 1.109f, 1.109f);
    calls.colors(4294887424U, 4293163264U, 4294887424U, 4293163264U);
    calls.draw_scaled(16777221U, p.x(-4), p.y(4), p.z(work, 10), 1.109f, 1.109f);
    calls.set_depth_comparison(7);
    calls.colors(4278195967U, 4278202111U, 4278195967U, 4278202111U);
    calls.draw_texture(16777219U, p.x(-4), p.y(72), p.z(work, 8));
    calls.colors(4278202111U, 4278242303U, 4278202111U, 4278242303U);
    calls.draw_texture(16777216U, p.x(-4), p.y(146), p.z(work, 8));
    calls.draw_texture(state.variant == 0 ? 33554433U : 33554434U, p.x(-127), p.y(146), p.z(work, 8));
    calls.draw_texture(state.variant == 0 ? 3221225473U : 3221225474U, p.x(129), p.y(146), p.z(work, 8));
    calls.draw_texture(3221225474U, p.x(109), p.y(146), p.z(work, 8));
    calls.restore_depth_comparison();
}
void draw_labels(const Work &work, State state, Calls &services, DrawCalls &calls, X87Context &fp) {
    const bool available = enabled(work.selected_character, state, services);
    calls.set_depth_comparison(7);
    calls.character_label(work.selected_character, available ? 4294967295U : 2160115904U,
                          163, 390, (fidelity::ExactFloat(fp, work.presentation.depth) - 12.0f).rounded());
    calls.restore_depth_comparison();
}
void draw_carousel(const Work &work, State state, DrawState view, Calls &services,
                    DrawCalls &calls, const NinjaMath &math, X87Context &fp) {
    using fidelity::ExactFloat;
    const auto count = services.character_count();
    if (count == 0)
        throw std::domain_error("character carousel divides by zero");
    const auto spacing = static_cast<std::int32_t>((360.0f / ExactFloat::integer(fp, count))
        .multiply_double(65536.0).multiply_double(0.002777777777777778).truncated_integer());
    const auto float_spacing = ExactFloat::integer(fp, spacing).rounded();
    const auto last = services.character_count() - 1;
    for (auto index = last; index >= 0; --index) {
        float progress = work.angle_progress;
        if (ExactFloat(fp, progress).compare(-1.0f) == X87Comparison::less)
            progress = -1;
        if (ExactFloat(fp, progress).compare(1.0f) == X87Comparison::greater)
            progress = 1;
        const auto divisor = services.character_count();
        if (divisor == 0)
            throw std::domain_error("character carousel divides by zero");
        const auto character = (work.selected_character + index) % divisor;
        const auto base_angle = static_cast<std::int32_t>(static_cast<std::uint32_t>(spacing) *
                                                        static_cast<std::uint32_t>(index));
        const auto angle = static_cast<std::int32_t>((ExactFloat(fp, float_spacing) * progress +
                                             ExactFloat::integer(fp, base_angle)).truncated_integer());
        const auto sine = ExactFloat::retained(fp, math.sin_extended(angle));
        const auto x = (sine * 126.0f + 316.0f).rounded();
        const auto cosine = ExactFloat::retained(fp, math.cos_extended(angle));
        const auto y = (cosine * 126.0f + 244.0f).rounded();
        float scale = 0.615f;
        std::uint32_t color = 4278190335U;
        if (character == work.selected_character) {
            color = calls.interpolate_colors(4294901760U, 4278190335U,
                                              fidelity::absolute_float(work.angle_progress));
            if (ExactFloat(fp, work.angle_progress).compare(0.0f) == X87Comparison::equal) {
                const auto pulse = ExactFloat::retained(fp, math.sin_extended(work.cursor_angle));
                scale = ((pulse + 1.0f) * 0.18450001f + 0.615f).rounded();
            }
            if (work.submode >= 3)
                scale = 1.107f;
        }
        const auto depth = (ExactFloat(fp, work.presentation.depth) - 12.0f).rounded();
        const auto y_offset = ExactFloat(fp, y) - 240.0f;
        const auto screen_y = (ExactFloat(fp, view.vertical_scale) * 240.0f + y_offset).rounded();
        const auto x_offset = ExactFloat(fp, x) - 320.0f;
        const auto screen_x = (ExactFloat(fp, view.horizontal_scale) * 320.0f + x_offset).rounded();
        calls.round_panel(color, screen_x, screen_y, depth, 0.75f, 0.75f);
        if (work.submode >= 3 && character != work.selected_character)
            continue;
        if (character < 0 || static_cast<std::size_t>(character) >= view.name_widths.size())
            throw std::out_of_range("character name width outside imported source table");
        const auto name_sine = ExactFloat::retained(fp, math.sin_extended(angle));
        const auto name_x = (ExactFloat(fp, view.name_widths[character]) * 0.5f * name_sine + x).rounded();
        const bool available = enabled(character, state, services);
        if (!available)
            scale = 0.615f;
        const auto name_depth = (ExactFloat(fp, work.presentation.depth) - 14.0f).rounded();
        const auto name_y = (ExactFloat(fp, y) - 28.0f).rounded();
        calls.character_name(character, available ? 4294967295U : 2160115904U,
                             name_x, name_y, name_depth, scale, scale);
    }
}
void draw_emblems(const Work &work, State state, DrawState view, DrawCalls &calls, X87Context &fp) {
    Projection p{view, fp};
    calls.select_texture(TextureList::CharacterSelect);
    calls.colors(4278202111U, 4278242303U, 4278202111U, 4278242303U);
    if (state.variant != 0)
        return;
    calls.set_depth_comparison(7);
    calls.round_panel(4294901760U, p.x(189), p.y(178), p.z(work, 14), 0.781f, 0.781f);
    calls.round_panel(4294901760U, p.x(-187), p.y(178), p.z(work, 14), 0.781f, 0.781f);
    calls.select_texture(TextureList::Digits);
    calls.material(4294967295U);
    calls.draw_texture(67108875U, p.x(206.5f), p.y(178.8f), p.z(work, 14));
    calls.select_texture(TextureList::CharacterSelect);
    calls.draw_texture(8, p.x(218), p.y(154), p.z(work, 12));
    const auto number_depth = p.z(work, 14);
    const auto completion = calls.character_completion(work.selected_character);
    calls.draw_number(completion, 3, false, 492, 418.8f, number_depth, 0.7f);
    calls.restore_depth_comparison();
}
} // namespace sadx::character_select_menu

namespace sadx::character_select_menu {
bool selection_enabled(std::int32_t character, State state, Calls &calls) {
    return enabled(character, state, calls);
}
void draw(const Work &work, State state, DrawState view, Calls &services,
          DrawCalls &calls, const NinjaMath &math, X87Context &fp) {
    if (work.presentation.color == 0)
        return;
    calls.default_blending();
    calls.base_color(work.presentation.color);
    calls.sky_background((fidelity::ExactFloat(fp, work.presentation.depth) - 4.0f).rounded());
    draw_background(work, state, view, calls, fp);
    draw_labels(work, state, services, calls, fp);
    draw_carousel(work, state, view, services, calls, math, fp);
    draw_emblems(work, state, view, calls, fp);
}
} // namespace sadx::character_select_menu

namespace sadx::character_select_menu {
std::int32_t character_count(menu_eligibility::Progress progress) {
    if (progress.tutorial_variant == 2)
        return 6;
    if (progress.unlock_override != 0)
        return 7;
    if (progress.tutorial_variant == 1)
        return 6 + (menu_eligibility::metal_sonic_enabled(progress) != 0);
    return 6 + (progress.story_flags[71] != 0);
}
bool character_cleared(std::uint32_t character, Availability availability) {
    if (availability.progress.unlock_override != 0)
        return true;
    return availability.progress.story_flags.at(availability.clear_flags.at(character)) != 0;
}
bool trial_character_enabled(std::uint32_t character, Availability availability) {
    if (availability.progress.unlock_override != 0)
        return true;
    if (menu_eligibility::character_enabled(character, availability.progress) == 0)
        return false;
    if (character == 6)
        return true;
    if (availability.trial_subgame != 0)
        return menu_eligibility::minigame_count(character, availability.progress,
                                               availability.completion) != 0;
    const auto source_character = availability.completion.catalog.character_ids.at(character);
    const auto stage = availability.completion.catalog.trial_stages.at(character).at(1).stage;
    return availability.completion.completed_stages.at(source_character * 43 + stage) != 0;
}
} // namespace sadx::character_select_menu
