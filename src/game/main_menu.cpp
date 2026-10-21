#include "game/main_menu.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>

namespace sadx::main_menu {
void initialize(Work &work, InitState state, InitCalls &calls) {
    if ((calls.text_language() != 5 && calls.text_language() != state.text_language) ||
        calls.voice_language() != state.voice_language) {
        state.text_language = calls.text_language() == 5 ? 1 : calls.text_language();
        state.voice_language = calls.voice_language();
    }
    calls.load_mode_textures(Mode::TitleMenu);
    calls.request_menu_music(1);
    const auto selected = state.tutorial_variant;
    work.selected_menu = selected >= 1 && selected <= 3 ? selected : 0;
    work.submode = 0;
    ++state.title_count;
    state.title.next = Mode::TitleMenu;
    state.canonical_menu.presentation.previous = Mode::Title;
    work.frames = 0;
}

namespace {
// NOTSA: source branches publish the clear character before applying and copying the save.
void restore_save(ControlState state, ControlCalls &calls) {
    state.save.last_character = 0;
    calls.apply_save();
    state.backup = state.save;
}

// NOTSA: the two menus share this source music/dialog timeout predicate.
bool demo_ready(ControlCalls &calls) {
    if (calls.music_state() == 5)
        return calls.dialog_idle_frames() >= 480;
    calls.reset_dialog_idle_frames();
    return false;
}

// NOTSA: named control step split from the original long menu callback.
void decide(Work &work, ControlState state, ControlCalls &calls) {
    work.selected_menu = calls.dialog_result();
    if (work.selected_menu < 0 || state.ready != 1) {
        if (demo_ready(calls)) {
            restore_save(state, calls);
            work.submode = static_cast<std::int32_t>(ControlStage::ToDemo);
        }
        return;
    }
    if (!calls.optional_entry_enabled() && work.selected_menu >= 3)
        ++work.selected_menu;
    const auto selected = static_cast<std::uint32_t>(work.selected_menu);
    if (selected >= state.variant_mapping.size())
        throw std::out_of_range("main menu selection exceeds imported variant mapping");
    state.tutorial_variant = state.variant_mapping[selected];
    switch (selected) {
    case 0:
        calls.prepare_character_select(8);
        break;
    case 1: {
        std::array<std::int8_t, 3> unavailable{-1, -1, -1};
        std::size_t count = 0;
        if (!calls.trial_stages_enabled())
            unavailable[count++] = 0;
        if (!calls.minigames_enabled())
            unavailable[count] = 1;
        calls.open_dialog(Dialog::Trial, 0, unavailable, false);
        work.submode = static_cast<std::int32_t>(ControlStage::Trial);
        return;
    }
    case 2:
        calls.transition_menu(Mode::CharacterSelect);
        break;
    case 3:
        calls.transition_menu(Mode::Puti);
        break;
    case 4:
        calls.transition_menu(Mode::OptionSelect);
        break;
    case 5:
        state.exit_requested = 1;
        return;
    case 6:
        calls.transition_menu(work.presentation.previous);
        restore_save(state, calls);
        break;
    }
    work.submode = static_cast<std::int32_t>(ControlStage::Wait);
    return;
}

// NOTSA: named control step split from the original long menu callback.
void choose_trial(Work &work, ControlState state, ControlCalls &calls) {
    const auto selected = calls.dialog_result();
    if (selected < 0) {
        if (demo_ready(calls))
            work.submode = static_cast<std::int32_t>(ControlStage::ToDemo);
        return;
    }
    switch (selected) {
    case 0:
        calls.prepare_character_select(3);
        state.trial_subgame = 0;
        break;
    case 1:
        calls.prepare_character_select(9);
        state.trial_subgame = 1;
        break;
    case 2:
        calls.transition_menu(work.next_sub);
        break;
    default:
        work.submode = static_cast<std::int32_t>(ControlStage::Reopen);
        return;
    }
    work.submode = static_cast<std::int32_t>(ControlStage::Wait);
    return;
}
} // namespace

void update_controls(Work &work, ControlState state, ControlCalls &calls) {
    std::array<std::int8_t, 3> disabled{-1, -1, -1};
    std::size_t disabled_count = 0;
    if (!calls.trial_enabled())
        disabled[disabled_count++] = 1;
    if (!calls.mission_enabled())
        disabled[disabled_count] = 2;
    auto stage = static_cast<ControlStage>(work.submode);
    if (stage == ControlStage::Stay || stage == ControlStage::Reopen) {
        const auto dialog = calls.optional_entry_enabled() ? Dialog::Main : Dialog::MainWithoutPuti;
        calls.open_dialog(dialog, static_cast<std::int8_t>(work.selected_menu), disabled,
                          stage == ControlStage::Stay);
        work.submode = static_cast<std::int32_t>(ControlStage::Decide);
        if (stage == ControlStage::Reopen)
            return;
        stage = ControlStage::Decide;
    }
    switch (stage) {
    case ControlStage::Decide:
        decide(work, state, calls);
        return;
    case ControlStage::Trial:
        choose_trial(work, state, calls);
        return;
    case ControlStage::ToDemo:
        if (calls.close_dialog()) {
            work.frames = 0;
            work.submode = static_cast<std::int32_t>(ControlStage::DemoDelay);
        }
        return;
    case ControlStage::DemoDelay: {
        const auto previous = work.frames;
        work.frames = static_cast<std::int16_t>(static_cast<std::uint16_t>(previous) + 1U);
        if (previous >= 20) {
            state.sequence.return_value =
                static_cast<std::int32_t>(title_menu::MenuResult::TitleToDemo);
            calls.transition_menu(Mode::None);
            work.submode = static_cast<std::int32_t>(ControlStage::Wait);
        }
        return;
    }
    default:
        return;
    }
}

void update(Work &work, Sequence &sequence, InitState state, const std::uint32_t &logic_ticks,
            UpdateCalls &calls, X87Context &fp) {
    auto &presentation = work.presentation;
    if (sequence.mode == Mode::TitleMenu && presentation.phase == Phase::RequestWait) {
        initialize(work, state, calls);
        presentation.fade_progress = 0;
        presentation.phase = Phase::FadeIn;
    }
    if (sequence.old_mode == Mode::TitleMenu && presentation.phase == Phase::Keep) {
        presentation.phase = Phase::FadeOut;
        presentation.fade_progress = 0;
    }
    switch (presentation.phase) {
    case Phase::RequestWait:
        presentation.color = 0;
        return;
    case Phase::FadeIn:
    case Phase::FadeOut: {
        const bool leaving = presentation.phase == Phase::FadeOut;
        presentation.color = leaving ? calls.fade_in_color(presentation.fade_progress)
                                     : calls.fade_out_color(presentation.fade_progress);
        using fidelity::ExactFloat;
        const auto step = ExactFloat::integer(fp, static_cast<std::int32_t>(logic_ticks)) * 0.1f;
        const auto progress = step + presentation.fade_progress;
        presentation.fade_progress = progress.rounded();
        const auto comparison = progress.compare(1.0f);
        if (comparison != X87Comparison::equal && comparison != X87Comparison::greater)
            return;
        if (leaving) {
            sequence.fade_out = 1;
            calls.release_mode_textures(Mode::TitleMenu);
            sequence.fade_end = 1;
            presentation.phase = Phase::RequestWait;
        } else {
            presentation.phase = Phase::Keep;
        }
        return;
    }
    case Phase::Keep:
        work.wake = 0;
        sequence.fade_out = 0;
        presentation.color = 4294967295U;
        sequence.fixed_mode = Mode::TitleMenu;
        calls.update_controls(work);
        return;
    }
}

void draw_background(float depth, DrawState state, DrawCalls &calls, X87Context &fp) {
    calls.select_texture(TextureList::Common);
    calls.set_material(4294967295U);
    calls.select_texture(TextureList::MainMenu);
    using fidelity::ExactFloat;
    const auto lowered_depth = (ExactFloat(fp, depth) - 4.0f).rounded();
    constexpr std::array<float, 8> positions_x{0, 0, 256, 256, 512, 512, 512, 512};
    constexpr std::array<float, 8> positions_y{-18, 238, -18, 238, -18, 110, 238, 366};
    for (std::uint32_t index = 0; index < positions_x.size(); ++index) {
        const auto y = (ExactFloat(fp, state.vertical_scale) * positions_y[index]).rounded();
        const auto x = (ExactFloat(fp, state.horizontal_scale) * positions_x[index]).rounded();
        calls.draw_scaled(index, x, y, lowered_depth, state.horizontal_scale, state.vertical_scale);
    }
    calls.select_texture(TextureList::Common);
}

void draw(const Work &work, DrawState state, DrawCalls &calls, X87Context &fp) {
    if (work.presentation.color == 0)
        return;
    calls.default_blending();
    calls.set_base_color(work.presentation.color);
    draw_background(work.presentation.depth, state, calls, fp);
    state.window_after_menu = 1;
}

void draw_profiled(const Work &work, DrawState state, DrawCalls &calls, X87Context &fp) {
    if (work.presentation.color == 0)
        return;
    calls.default_blending();
    calls.set_base_color(work.presentation.color);
    // The loader pushes centered background scaling around this body. At the source
    // resolution its scale and translation are identity; other views remain explicit.
    if (state.screen_width != 640 || state.screen_height != 480 || state.horizontal_scale != 1.0f ||
        state.vertical_scale != 1.0f)
        throw std::out_of_range("main-menu background scaling outside qualified640x480");
    draw_background(work.presentation.depth, state, calls, fp);
    state.window_after_menu = 1;
}
} // namespace sadx::main_menu
