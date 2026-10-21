#include "game/title_menu.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>

namespace sadx::title_menu {
std::uint32_t interpolate_colors(std::uint32_t first, std::uint32_t second,
                                  float progress, X87Context &fp) {
    using fidelity::ExactFloat;
    const auto inverse = ExactFloat(fp, 1.0f) - progress;
    std::array<std::uint32_t,4> channels{};
    constexpr std::array<unsigned,4> shifts{16,24,8,0};
    for (std::size_t index = 0; index < channels.size(); ++index) {
        const auto shift = shifts[index];
        const auto first_channel = ExactFloat::integer(fp, (first >> shift) & 255U) * inverse;
        const auto second_channel = ExactFloat::integer(fp, (second >> shift) & 255U) * progress;
        channels[index] = static_cast<std::uint32_t>((first_channel + second_channel).truncated_integer());
    }
    // Original quirk: converted channels are not narrowed before the ORs.
    return (channels[1] << 24) | (channels[0] << 16) | (channels[2] << 8) | channels[3];
}
std::uint32_t fade_out_color(float progress, FadeCalls &calls, X87Context &fp) {
    calls.disable_alpha_test();
    return interpolate_colors(16777215U, 4294967295U, progress, fp);
}
std::uint32_t fade_in_color(float progress, FadeCalls &calls, X87Context &fp) {
    calls.disable_alpha_test();
    using fidelity::ExactFloat;
    if (ExactFloat(fp, progress).compare(0.75f) != X87Comparison::less) return 0;
    const auto alpha = (ExactFloat(fp, 0.75f) - progress) * 255.0f;
    const auto zero_term = ExactFloat(fp, progress) * 0.0f;
    const auto converted = static_cast<std::uint32_t>((alpha + zero_term).truncated_integer());
    return (converted << 24) | 16777215U;
}

MenuDrawResult draw_character_select(const CharacterSelectWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_file_select(const FileSelectWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_level_select(const StageSelectWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_sound_test(const SoundTestWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_main_menu(const MainMenuWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_options(const OptionWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_explanation(const ExplanationWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_subgame_list(const StageSelectWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_emblem_results(const EmblemWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_internet_demo(const InternetDemoWork &work) {
    return work.presentation.color != 0 && work.submode >= 2 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_tv_settings(const TvSettingsWork &work) {
    return work.presentation.color != 0 ? MenuDrawResult::NeedsDrawing : MenuDrawResult::Complete;
}
MenuDrawResult draw_dialog(const DialogWork &work, X87Context &fp) {
    if (work.color == 0) return MenuDrawResult::Complete;
    if (fidelity::ExactFloat(fp, work.scale_x).compare(0.0f) != X87Comparison::equal ||
        fidelity::ExactFloat(fp, work.scale_y).compare(0.0f) != X87Comparison::equal) {
        return MenuDrawResult::NeedsDrawing;
    }
    return MenuDrawResult::Complete;
}

namespace {
// NOTSA: common source guards, used by the individual menu exec functions.
MenuCallbackResult transition_menu(MenuPresentation &work, const SequenceWork &sequence,
                                    MenuMode mode, bool leave_call = false) {
    if (sequence.mode == mode && work.phase == TitlePhase::RequestWait) {
        return MenuCallbackResult::NeedsInitialization;
    }
    if (sequence.old_mode == mode && work.phase == TitlePhase::Keep) {
        if (leave_call) return MenuCallbackResult::NeedsLeave;
        work.phase = TitlePhase::FadeOut;
        work.fade_progress = 0;
    }
    return MenuCallbackResult::Complete;
}
// NOTSA: common switch branches, with the original per-menu wait-store difference.
MenuCallbackResult menu_phase(MenuPresentation &work, bool reset_wait_fade = false) {
    switch (work.phase) {
    case TitlePhase::RequestWait:
        if (reset_wait_fade) work.fade_progress = 0;
        work.color = 0;
        return MenuCallbackResult::Complete;
    case TitlePhase::FadeIn: return MenuCallbackResult::NeedsFadeIn;
    case TitlePhase::Keep: return MenuCallbackResult::NeedsControl;
    case TitlePhase::FadeOut: return MenuCallbackResult::NeedsFadeOut;
    }
    return MenuCallbackResult::Complete;
}
// NOTSA: one source byte-counter operation shared by seven option animations.
void advance_animation(MenuAnimation &animation, std::uint8_t frames, std::uint8_t images) {
    animation.frames = static_cast<std::uint8_t>(animation.frames + 1);
    if (animation.frames >= frames) {
        animation.frame = static_cast<std::uint8_t>(animation.frame + 1);
        animation.frames = 0;
        if (animation.frame >= images) animation.frame = 0;
    }
}
} // namespace

MenuCallbackResult update_puti(const TaskHandle &task, const SequenceWork &sequence,
                                PutiCalls &calls) {
    auto current = std::static_pointer_cast<PutiWork>(task->auxiliary_native_work);
    if (!current) {
        current = calls.allocate_puti_work(task);
        if (!current) throw std::runtime_error("puti allocation returned null");
        task->auxiliary_native_work = current;
        current->phase = 1;
        current->resource.reset();
        current->waiting_frames = 0;
        calls.install_puti_callbacks(task);
    }
    auto &work = *current;
    switch (work.phase) {
    case 0: {
        const auto previous = work.waiting_frames;
        ++work.waiting_frames;
        if (static_cast<std::int32_t>(previous) >= 120) {
            work.waiting_frames = 0;
            if (sequence.mode == MenuMode::Puti) return MenuCallbackResult::NeedsInitialization;
        }
        break;
    }
    case 1:
        if (sequence.mode == MenuMode::Puti) work.phase = 0;
        break;
    case 2: return MenuCallbackResult::NeedsControl;
    case 3: return MenuCallbackResult::NeedsFadeOut;
    }
    return MenuCallbackResult::Complete;
}

MenuCallbackResult update_character_select(CharacterSelectWork &work, SequenceWork &sequence,
                                            std::uint8_t &character_cursor,
                                            std::uint8_t &player_character) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::CharacterSelect, true);
    if (transition != MenuCallbackResult::Complete) return transition;
    work.selected_character = player_character;
    character_cursor = player_character;
    player_character = static_cast<std::uint8_t>(work.selected_character);
    return menu_phase(work.presentation);
}
MenuCallbackResult update_file_select(FileSelectWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::FileSelect);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation) : transition;
}
MenuCallbackResult update_level_select(StageSelectWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::TrialActSelect, true);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation, true) : transition;
}
MenuCallbackResult update_sound_test(SoundTestWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::SoundTest);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation) : transition;
}
MenuCallbackResult update_main_menu(MainMenuWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::TitleMenu);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation) : transition;
}
void update_option_animation(OptionWork &work) {
    advance_animation(work.eight_frame_cycle, 8, 7);
    advance_animation(work.twelve_frame_cycle, 12, 3);
    advance_animation(work.four_frame_cycle, 4, 2);
    advance_animation(work.first_long_cycle, 32, 2);
    advance_animation(work.second_long_cycle, 32, 7);
    advance_animation(work.five_frame_cycle, 5, 12);
    advance_animation(work.twenty_four_frame_cycle, 24, 5);
    work.rotation_angle += 1456;
}
MenuCallbackResult update_options(OptionWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::OptionSelect);
    if (transition != MenuCallbackResult::Complete) return transition;
    update_option_animation(work);
    return menu_phase(work.presentation);
}
MenuCallbackResult update_explanation(ExplanationWork &work, SequenceWork &sequence) {
    if (sequence.old_mode == MenuMode::Explanation && work.presentation.phase == TitlePhase::Keep) {
        sequence.advance = MenuResult::Summary;
    }
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::Explanation);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation) : transition;
}
MenuCallbackResult update_subgame_list(StageSelectWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::MinigameSelect, true);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation) : transition;
}
MenuCallbackResult update_emblem_results(EmblemWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::EmblemView);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation, true) : transition;
}
MenuCallbackResult update_internet_demo(InternetDemoWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::InternetDemo);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation) : transition;
}
MenuCallbackResult update_tv_settings(TvSettingsWork &work, SequenceWork &sequence) {
    const auto transition = transition_menu(work.presentation, sequence, MenuMode::TvSettings,
                                             work.mode_changed != 0);
    return transition == MenuCallbackResult::Complete ? menu_phase(work.presentation, true) : transition;
}

void build_trial_stage_list(StageSelectWork &work, const StageSelectionInputs &input) {
    const auto &stages = input.data.trial_stages[input.character];
    std::uint8_t count = 0;
    if (input.include_all != 0) {
        for (const auto &stage : stages) work.available_stages[count++] = stage;
    } else {
        const auto character = static_cast<std::uint8_t>(input.data.character_ids[input.character]);
        const auto *cleared = input.completed_stages.data() + character * 43;
        if (cleared[stages.front().stage] != 0) {
            for (std::size_t index = 1; index < stages.size(); ++index) {
                if (cleared[stages[index].stage] != 0) {
                    work.available_stages[count++] = stages[index - 1];
                }
            }
            if (cleared[stages.back().stage] != 0) {
                work.available_stages[count] = stages.back();
                // Original quirk: publishes the catalog size, even if earlier entries were skipped.
                count = static_cast<std::uint8_t>(stages.size());
            }
        }
    }
    work.available_count = count;
}

void build_subgame_stage_list(StageSelectWork &work, const StageSelectionInputs &input) {
    const auto &stages = input.data.subgame_stages[input.character];
    std::uint8_t count = 0;
    const auto *cleared = input.completed_stages.data() +
        input.data.character_ids[input.character] * 43;
    for (const auto &stage : stages) {
        // The source reads the save flag before the include-all flag.
        if (cleared[stage.stage] != 0 || input.include_all != 0) {
            work.available_stages[count++] = stage;
        }
    }
    work.available_count = count;
}

void build_display_stage_list(StageSelectWork &work, const StageSelectionData &data) {
    work.stage_count = 0;
    for (std::uint8_t index = 0; index < work.available_count; ++index) {
        work.stage_ids[work.stage_count++] = data.display_stage_ids[work.available_stages[index].stage];
    }
}

void initialize_level_select(StageSelectWork &work, const ModeSelection &transition,
                             const StageSelectionInputs &input) {
    work.presentation.previous = transition.previous;
    work.presentation.next = transition.next;
    work.presentation.depth = 5800.0f;
    work.presentation.phase = TitlePhase::RequestWait;
    build_trial_stage_list(work, input);
    build_display_stage_list(work, input.data);
    work.submode = 0;
    work.cursor_angle = 0;
    work.cursor_scale = 1;
    work.cursor_alpha = 255;
}

void initialize_subgame_select(StageSelectWork &work, const ModeSelection &transition,
                               const StageSelectionInputs &input) {
    work.presentation.previous = transition.previous;
    work.presentation.next = transition.next;
    work.presentation.depth = 5100.0f;
    work.presentation.phase = TitlePhase::RequestWait;
    build_subgame_stage_list(work, input);
    build_display_stage_list(work, input.data);
    work.cursor_angle = 0;
    work.cursor_scale = 1;
    work.cursor_alpha = 255;
    work.submode = 0;
}

void initialize_options(OptionWork &work, const ModeSelection &transition) {
    work.presentation.previous = transition.previous;
    work.presentation.next = transition.next;
    work.presentation.depth = 5500.0f;
    work.presentation.phase = TitlePhase::RequestWait;
    work.submode = 0;
    work.cursor_angle = 0;
    work.cursor_scale = 1;
    work.cursor_alpha = 255;
    work.rotation_angle = 0;
    work.four_frame_cycle = {};
    work.first_long_cycle = {};
    work.second_long_cycle = {};
    work.eight_frame_cycle.frame = 0;
    work.twelve_frame_cycle.frame = 0;
    work.five_frame_cycle = {};
    work.content = OptionContent::Menu;
    work.item_count = 6;
}

void initialize_emblem_selection(EmblemWork &work) {
    work.entry_count = 0;
    for (auto &entry : work.selected_entries) {
        entry = 0;
        ++work.entry_count;
    }
    work.submode = 0;
    work.cursor_angle = 0;
    work.cursor_scale = 1;
    work.cursor_alpha = 255;
}

TaskHandle create_level_select(MenuFactoryState &state, const ModeSelection &transition,
                               const StageSelectionInputs &input, MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::TrialActSelect, 0, 4);
    state.level_select_task = task;
    if (task) {
        const auto work = calls.allocate_level_select(task);
        state.level_select_work = work;
        if (!work) throw std::runtime_error("level select allocation returned null");
        initialize_level_select(*work, transition, input);
        calls.install_menu_callbacks(MenuMode::TrialActSelect, task);
    }
    return task;
}

TaskHandle create_subgame_list(MenuFactoryState &state, const ModeSelection &transition,
                               const StageSelectionInputs &input, MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::MinigameSelect, 0, 4);
    state.subgame_task = task;
    if (task) {
        const auto work = calls.allocate_subgame(task);
        state.subgame_work = work;
        if (!work) throw std::runtime_error("subgame select allocation returned null");
        initialize_subgame_select(*work, transition, input);
        calls.install_menu_callbacks(MenuMode::MinigameSelect, task);
    }
    return task;
}

TaskHandle create_sound_test(MenuFactoryState &state, const ModeSelection &transition,
                             MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::SoundTest, 0, 3);
    state.sound_test_task = task;
    if (task) {
        const auto work = calls.allocate_sound_test(task);
        state.sound_test_work = work;
        if (!work) throw std::runtime_error("sound test allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->presentation.depth = 5600.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        work->submode = 0;
        calls.install_menu_callbacks(MenuMode::SoundTest, task);
    }
    return task;
}

TaskHandle create_options(MenuFactoryState &state, const ModeSelection &transition,
                          MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::OptionSelect, 0, 3);
    state.options_task = task;
    if (task) {
        const auto work = calls.allocate_options(task);
        state.options_work = work;
        if (!work) throw std::runtime_error("options allocation returned null");
        initialize_options(*work, transition);
        calls.install_menu_callbacks(MenuMode::OptionSelect, task);
    }
    return task;
}

TaskHandle create_emblem_results(MenuFactoryState &state, const ModeSelection &transition,
                                 MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::EmblemView, 0, 4);
    state.emblem_task = task;
    if (task) {
        const auto work = calls.allocate_emblem(task);
        state.emblem_work = work;
        if (!work) throw std::runtime_error("emblem allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->presentation.depth = 5800.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        initialize_emblem_selection(*work);
        calls.install_menu_callbacks(MenuMode::EmblemView, task);
    }
    return task;
}

TaskHandle create_tv_settings(MenuFactoryState &state, const ModeSelection &transition,
                             MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::TvSettings, 0, 4);
    state.tv_settings_task = task;
    if (task) {
        const auto work = calls.allocate_tv_settings(task);
        state.tv_settings_work = work;
        if (!work) throw std::runtime_error("TV settings allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->presentation.depth = 5800.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        work->submode = 0;
        calls.install_menu_callbacks(MenuMode::TvSettings, task);
    }
    return task;
}

TaskHandle create_character_select(MenuFactoryState &state, const ModeSelection &transition,
                                   const MenuFactoryInputs &input, MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::CharacterSelect, 0, 3);
    state.character_select_task = task;
    if (task) {
        const auto work = calls.allocate_character_select(task);
        state.character_select_work = work;
        if (!work) throw std::runtime_error("character select allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->presentation.depth = 6000.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        work->selected_character = input.character_cursor;
        work->selected_file = 0;
        work->wake = 0;
        work->cursor_angle = 0;
        work->angle_progress = 0;
        work->submode = 0;
        calls.install_menu_callbacks(MenuMode::CharacterSelect, task);
    }
    return task;
}

TaskHandle create_file_select(MenuFactoryState &state, const ModeSelection &transition,
                              MenuFactoryInputs &input, MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::FileSelect, 0, 3);
    state.file_select_task = task;
    if (task) {
        const auto work = calls.allocate_file_select(task);
        state.file_select_work = work;
        if (!work) throw std::runtime_error("file select allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->presentation.depth = 5900.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        work->submode = 0;
        input.selected_file = input.memory_card_ready == 0 ? 0 :
            static_cast<std::uint8_t>(input.memory_card_file);
        work->selected_character = 0;
        work->cursor_angle = 0;
        work->cursor_color = 4294901760U;
        calls.install_menu_callbacks(MenuMode::FileSelect, task);
    }
    return task;
}

TaskHandle create_main_menu(MenuFactoryState &state, const ModeSelection &transition,
                            const MenuFactoryInputs &input, MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::TitleMenu, 0, 4);
    state.main_menu_task = task;
    if (task) {
        const auto work = calls.allocate_main_menu(task);
        state.main_menu_work = work;
        if (!work) throw std::runtime_error("main menu allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->next_sub = transition.next_sub;
        work->presentation.depth = 5300.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        work->wake = 0;
        const auto selected = input.tutorial_variant;
        work->selected_menu = selected >= 1 && selected <= 3 ? selected : 0;
        work->submode = 0;
        calls.install_menu_callbacks(MenuMode::TitleMenu, task);
    }
    return task;
}

TaskHandle create_explanation(MenuFactoryState &state, const ModeSelection &transition,
                              MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::Explanation, 0, 3);
    state.explanation_task = task;
    if (task) {
        const auto work = calls.allocate_explanation(task);
        state.explanation_work = work;
        if (!work) throw std::runtime_error("explanation allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->presentation.depth = 5200.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        work->submode = 0;
        calls.install_menu_callbacks(MenuMode::Explanation, task);
    }
    return task;
}

TaskHandle create_internet_demo(MenuFactoryState &state, const ModeSelection &transition,
                                MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::InternetDemo, 8, 3);
    state.internet_demo_task = task;
    if (task) {
        const auto work = calls.allocate_internet_demo(task);
        state.internet_demo_work = work;
        if (!work) throw std::runtime_error("internet demo allocation returned null");
        work->presentation.previous = transition.previous;
        work->presentation.next = transition.next;
        work->presentation.depth = 5400.0f;
        work->presentation.phase = TitlePhase::RequestWait;
        work->submode = 0;
        calls.install_menu_callbacks(MenuMode::InternetDemo, task);
    }
    return task;
}

TaskHandle create_puti(MenuFactoryState &state, MenuFactoryCalls &calls) {
    const auto task = calls.create_menu_task(MenuMode::Puti, 0, 4);
    state.puti_task = task;
    // The original installs only exec during task creation: no work or display callback.
    return task;
}

void reset_selected_trial_level(TrialSelection &selection) {
    selection.stage = 0;
    selection.act = 0;
}

std::int32_t run_title_tasks(State &menus, std::uint32_t &pause_mode, RunCalls &calls) {
    calls.poll_controls();
    calls.enable_control();
    // Both compatibility entries between the task calls return immediately.
    calls.run_tasks();
    calls.disable_fog();
    calls.display_tasks();
    pause_mode = 0;
    if (calls.soft_reset_check(0) != 0) {
        const auto mode = menus.sequence_work->fixed_mode;
        calls.delete_all_tasks();
        return static_cast<std::int32_t>(mode == MenuMode::Title
                                             ? MenuResult::SoftResetInTitle : MenuResult::SoftReset);
    }
    return 0;
}
void draw_loading_text(const SharedState &shared, RunCalls &calls) {
    if (shared.font_loaded == 1) {
        calls.draw_text("Now Loading...", 400.0f, 400.0f, 1.0f);
    }
}
std::int32_t run_title_menu(State &menus, SharedState &shared, std::uint32_t &pause_mode,
                            RunCalls &calls) {
    calls.set_depth_compare(3);
    auto result = run_title_tasks(menus, pause_mode, calls);
    if (result == 0) {
        if (!menus.sequence_task) {
            throw std::runtime_error("title menu sequence task is null");
        }
        result = menus.sequence_work ? menus.sequence_work->return_value : -2;
    }
    draw_loading_text(shared, calls);
    return result;
}
void destroy_sequence_manager(State &menus, RunCalls &calls) {
    if (menus.sequence_task) {
        calls.destroy_sequence_task(menus.sequence_task);
        menus.sequence_task.reset();
    }
}
std::int32_t close_title_menu(State &menus, RunCalls &calls) {
    calls.set_depth_compare(1);
    calls.free_regular_object_textures();
    destroy_sequence_manager(menus, calls);
    calls.delete_all_tasks();
    // The trailing debug-print entry is a RET in the shipped executable.
    return 0;
}

namespace {
// NOTSA: names the repeated signed timing adjustment at title-task entry.
void adjust_title_timing(TitleTiming &timing) {
    const auto count = static_cast<std::int32_t>(timing.loop_count);
    const auto elapsed = timing.elapsed_ticks;
    if (elapsed < count) {
        const auto difference = static_cast<std::int32_t>(
            timing.loop_count - static_cast<std::uint32_t>(elapsed));
        if (difference > 1 && count > 1) {
            --timing.loop_count;
        }
    } else if (count < elapsed) {
        const auto difference = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(elapsed) - timing.loop_count);
        if (difference > 1 && count < 6) {
            ++timing.loop_count;
        }
    }
    timing.adventure_loop_count = timing.loop_count;
}
// NOTSA: the fade-in and fade-out branches emit the identical retained arithmetic sequence.
bool advance_title_fade(TitleWork &work, std::uint32_t ticks, X87Context &fp) {
    const auto increment = fidelity::ExactFloat::integer(fp, static_cast<std::int32_t>(ticks)) * 0.1f;
    const auto progress = increment + work.fade_progress;
    work.fade_progress = progress.rounded();
    const auto comparison = progress.compare(1.0f);
    return comparison == X87Comparison::equal || comparison == X87Comparison::greater;
}
} // namespace
void update_title(TitleWork &work, State &menus, TitleTiming &timing, TitleUpdateCalls &calls,
                  X87Context &fp) {
    adjust_title_timing(timing);
    if (menus.sequence_work->mode == MenuMode::Title && work.phase == TitlePhase::RequestWait) {
        calls.initialize_title(work);
        work.fade_progress = 0;
        work.phase = TitlePhase::FadeIn;
    }
    if (menus.sequence_work->old_mode == MenuMode::Title && work.phase == TitlePhase::Keep) {
        work.phase = TitlePhase::FadeOut;
        work.fade_progress = 0;
    }
    switch (work.phase) {
    case TitlePhase::RequestWait:
        work.color = 0;
        break;
    case TitlePhase::FadeIn:
        work.color = calls.fade_out_color(work.fade_progress);
        if (advance_title_fade(work, timing.loop_count, fp)) {
            work.phase = TitlePhase::Keep;
        }
        break;
    case TitlePhase::Keep:
        work.color = 4294967295U;
        menus.sequence_work->fixed_mode = MenuMode::Title;
        calls.update_title_input(work);
        work.blink_timer += timing.loop_count * 8U;
        break;
    case TitlePhase::FadeOut:
        work.blink_timer = 0;
        work.color = calls.fade_in_color(work.fade_progress);
        if (advance_title_fade(work, timing.loop_count, fp)) {
            calls.release_mode_textures(MenuMode::Title);
            if (timing.language == 0) {
                calls.release_japanese_title_textures();
            }
            menus.sequence_work->fade_end = 1;
            work.phase = TitlePhase::RequestWait;
        }
        break;
    }
}
void request_menu_transition(SequenceWork &sequence, MenuMode next, ModeTransitionCalls &calls) {
    calls.stop_menu_music();
    sequence.waiting = 1;
    sequence.waiting_mode = next;
}

void update_title_input(TitleWork &work, SequenceWork &sequence, TitleInputState &shared,
                        TitleInputCalls &calls) {
    if (work.submode == TitleSubmode::Stay) {
        work.submode = TitleSubmode::StayMain;
        calls.calculate_ticks();
        shared.pending_logic_ticks = 0;
        shared.title_logic_ticks = shared.loop_count;
        work.timer = 0;
        work.blink_timer = 0;
        shared.skip_title = 0;
        shared.saving_disabled = 1;
        shared.save_word_after_selection = 0;
        shared.save_word_before_selection = 0;
        return;
    }
    if (work.submode != TitleSubmode::StayMain) return;
    shared.loop_count = shared.title_logic_ticks;
    if (work.music_started == 0) {
        work.music_started = 1;
        calls.request_menu_music(0);
    }
    if (work.timer < 150) {
        const auto timer = static_cast<std::uint32_t>(work.timer) + shared.loop_count;
        work.blink_timer = 0;
        work.timer = static_cast<std::int32_t>(timer);
        return;
    }
    if (calls.key_pressed(41) != 0) shared.exit_requested = 1;
    if ((calls.pressed_buttons() & 8U) != 0 && shared.menu_ready == 1) {
        calls.transition_menu(work.next);
        calls.play_title_confirm_sound();
        work.submode = TitleSubmode::Wait;
        return;
    }
    if (calls.audio_ready() == 0 || shared.bgm_enabled == 0) {
        shared.demo_wait_ticks += shared.loop_count;
        if (static_cast<std::int32_t>(shared.demo_wait_ticks) < 300) return;
        shared.demo_wait_ticks = 0;
    } else if (calls.music_state() != 5) {
        return;
    }
    sequence.advance = MenuResult::TitleToDemo;
    calls.transition_menu(MenuMode::None);
    work.submode = TitleSubmode::Wait;
}

namespace {
using fidelity::ExactFloat;
// NOTSA: named cloud-animation step from the long title-background function.
void advance_title_clouds(TitleWork &work, const BackgroundView &view, bool compact,
                           X87Context &fp) {
    const auto ticks = static_cast<std::int32_t>(static_cast<std::uint32_t>(view.elapsed_ticks) + 1U);
    auto motion = ExactFloat::integer(fp, ticks);
    if (compact) motion = motion * view.horizontal_scale;
    motion = motion * (compact ? 2.105f : 4.21f);
    const auto next = motion + work.cloud_offset;
    work.cloud_offset = next.rounded();
    const auto comparison = next.compare(256.0f);
    if (comparison == X87Comparison::equal || comparison == X87Comparison::greater) {
        work.cloud_offset = 0;
        work.cloud_index = static_cast<std::uint8_t>(work.cloud_index + 1);
        if (static_cast<std::int8_t>(work.cloud_index) >= (compact ? 5 : 10)) work.cloud_index = 0;
    }
}
// NOTSA: source byte addition/wrap precedes the signed cloud-frame comparison.
std::int32_t title_cloud_frame(std::uint8_t first, unsigned column, std::int32_t count) {
    auto frame = static_cast<std::int8_t>(static_cast<std::uint8_t>(first + column));
    if (frame >= count) frame = static_cast<std::int8_t>(frame - count);
    return frame;
}
// NOTSA: named compact cloud/water pass; each argument is stored as binary32 before drawing.
void draw_compact_title_sky(TitleWork &work, const BackgroundView &view,
                             BackgroundCalls &calls, X87Context &fp) {
    calls.select_title_texture(TitleTextureList::Small);
    for (unsigned column = 0; column < 4; ++column) {
        const auto frame = title_cloud_frame(work.cloud_index, column, 5);
        const auto origin = (ExactFloat::integer(fp, column) * 256.0f - 64.0f).rounded();
        for (unsigned row = 0; row < 2; ++row) {
            const auto scale_y = ExactFloat(fp, view.vertical_scale).rounded();
            const auto scale_x = ExactFloat(fp, view.horizontal_scale).rounded();
            const auto y = (ExactFloat(fp, view.vertical_scale) * (row == 0 ? -16.0f : 240.0f)).rounded();
            const auto x = ((ExactFloat(fp, origin) - work.cloud_offset) * view.horizontal_scale).rounded();
            calls.draw_scaled_texture(static_cast<std::uint32_t>(frame + 11 + row * 5),
                                        x, y, 2.0f, scale_x, scale_y);
        }
    }
    advance_title_clouds(work, view, true, fp);
    for (unsigned row = 0; row < 2; ++row) {
        const auto origin_y = (ExactFloat::integer(fp, row) * 256.0f - 16.0f).rounded();
        for (unsigned column = 0; column < 3; ++column) {
            const auto scale_y = ExactFloat(fp, view.vertical_scale).rounded();
            const auto scale_x = ExactFloat(fp, view.horizontal_scale).rounded();
            const auto y = (ExactFloat(fp, origin_y) * view.vertical_scale).rounded();
            const auto x = ((ExactFloat::integer(fp, column) * 256.0f - 64.0f) *
                            view.horizontal_scale).rounded();
            calls.draw_scaled_texture(row * 3 + column, x, y, 1.3f, scale_x, scale_y);
        }
    }
}
// NOTSA: named full-size water pass, retaining the source's separately added center offsets.
void draw_title_water(const BackgroundView &view, BackgroundCalls &calls, X87Context &fp) {
    for (unsigned row = 0; row < 4; ++row) {
        const unsigned count = row == 0 || row == 3 ? 5 : 2;
        const unsigned first = row == 0 ? 0 : row == 1 ? 5 : row == 2 ? 7 : 9;
        for (unsigned column = 0; column < count; ++column) {
            auto width = ExactFloat(fp, view.horizontal_scale) * 128.0f;
            const auto scale_y = (ExactFloat(fp, view.vertical_scale) * 0.46874997f).rounded();
            const auto scale_x = (ExactFloat(fp, view.horizontal_scale) * 0.5f).rounded();
            const auto y = row == 0 ? 0.0f :
                (ExactFloat(fp, view.vertical_scale) * (row == 1 ? 120.0f : row == 2 ? 240.0f : 360.0f)).rounded();
            auto x = width * ExactFloat::integer(fp, column);
            if (row == 1 || row == 2) {
                const auto first_term = ExactFloat::integer(fp, column) * width;
                const auto second_term = width + width;
                x = first_term + second_term;
            }
            calls.draw_scaled_texture(first + column, x.rounded(), y, 1.3f, scale_x, scale_y);
        }
    }
}
// NOTSA: named full-size cloud pass from the same source function.
void draw_large_title_sky(TitleWork &work, const BackgroundView &view,
                           BackgroundCalls &calls, X87Context &fp) {
    for (unsigned column = 0; column < 9; ++column) {
        const auto frame = title_cloud_frame(work.cloud_index, column, 10);
        const auto scale_x = (ExactFloat(fp, view.horizontal_scale) * 0.5f).rounded();
        const auto origin = (ExactFloat::integer(fp, column) * 256.0f - 64.0f).rounded();
        for (unsigned row = 0; row < 4; ++row) {
            const auto scale_y = (ExactFloat(fp, view.vertical_scale) * 0.46874997f).rounded();
            const auto current_scale_x = row == 0 ? scale_x :
                (ExactFloat(fp, view.horizontal_scale) * 0.5f).rounded();
            const auto y = row == 0 ? -16.0f :
                (ExactFloat(fp, scale_y) * (row == 1 ? 256.0f : row == 2 ? 512.0f : 768.0f) - 16.0f).rounded();
            const auto x = ((ExactFloat(fp, origin) - work.cloud_offset) * current_scale_x).rounded();
            calls.draw_scaled_texture(static_cast<std::uint32_t>(frame + 27 + row * 10),
                                        x, y, 2.0f, current_scale_x, scale_y);
        }
    }
    advance_title_clouds(work, view, false, fp);
    draw_title_water(view, calls, fp);
}
// NOTSA: compact description of the source's four unrolled logo-quadrant calls.
void draw_compact_title_logo(float logo_x, const BackgroundView &view,
                              const BackgroundLayout &layout, BackgroundCalls &calls,
                              X87Context &fp) {
    constexpr std::array<std::uint32_t,4> tiles{134217735U,100663304U,33554441U,10U};
    for (const auto tile : tiles) {
        const auto scale_y = (ExactFloat(fp, view.vertical_scale) * 1.025f).rounded();
        const auto scale_x = (ExactFloat(fp, view.horizontal_scale) * 0.935f).rounded();
        const auto y = (ExactFloat(fp, layout.compact_logo_y) * view.vertical_scale).rounded();
        const auto x = (ExactFloat(fp, logo_x) * view.horizontal_scale).rounded();
        calls.draw_scaled_texture(tile, x, y, 1.2f, scale_x, scale_y);
    }
}
// NOTSA: source's unrolled logo tile positions, expressed as rows/columns of named draw steps.
void draw_large_title_logo(float logo_x, const BackgroundView &view,
                            const BackgroundLayout &layout, BackgroundCalls &calls,
                            X87Context &fp) {
    constexpr std::array<unsigned,4> first_columns{0,1,0,0};
    constexpr std::array<unsigned,4> end_columns{3,3,4,4};
    constexpr std::array<float,4> horizontal_offsets{0,119.68f,239.36f,359.04f};
    constexpr std::array<float,4> vertical_offsets{0,123.0f,246.0f,369.0f};
    unsigned tile = 14;
    for (unsigned row = 0; row < 4; ++row) {
        for (unsigned column = first_columns[row]; column < end_columns[row]; ++column) {
            const auto scale_y =
                (ExactFloat(fp, view.vertical_scale) * 0.46874997f * 1.025f).rounded();
            const auto scale_x =
                (ExactFloat(fp, view.horizontal_scale) * 0.5f * 0.935f).rounded();
            auto y = ExactFloat(fp, layout.logo_y) * view.vertical_scale;
            if (row != 0) y = y + ExactFloat(fp, view.vertical_scale) * vertical_offsets[row];
            const auto y_value = y.rounded();
            auto x = ExactFloat(fp, logo_x) * view.horizontal_scale;
            if (column != 0) x = x + ExactFloat(fp, view.horizontal_scale) * horizontal_offsets[column];
            calls.draw_scaled_texture(tile++, x.rounded(), y_value, 1.2f, scale_x, scale_y);
        }
    }
}
// NOTSA: named eight-piece title lettering pass from the source function.
void draw_title_lettering(float text_x, const BackgroundView &view,
                           const BackgroundLayout &layout, BackgroundCalls &calls, X87Context &fp) {
    calls.select_title_texture(TitleTextureList::Title);
    for (unsigned row = 0; row < 2; ++row) {
        for (unsigned column = 0; column < 4; ++column) {
            const auto scale_y = (ExactFloat(fp, view.vertical_scale) * 0.46874997f).rounded();
            const auto scale_x = (ExactFloat(fp, view.horizontal_scale) * 0.5f).rounded();
            auto y = ExactFloat(fp, layout.text_y) * view.vertical_scale;
            if (row != 0) y = y + ExactFloat(fp, view.vertical_scale) * 120.0f;
            const auto y_value = y.rounded();
            auto x = ExactFloat(fp, text_x) * view.horizontal_scale;
            if (column != 0) x = x + ExactFloat(fp, view.horizontal_scale) * (column * 128.0f);
            calls.draw_scaled_texture(row * 4 + column, x.rounded(), y_value, 1.1f, scale_x, scale_y);
        }
    }
}
// NOTSA: named logo-introduction step; motion follows the source frame thresholds and overshoot.
void draw_title_logo(TitleWork &work, const BackgroundView &view, const BackgroundLayout &layout,
                       BackgroundCalls &calls, X87Context &fp) {
    if (work.wave_timer < 50) return;
    float logo_x;
    if (work.wave_timer >= 80) {
        calls.set_material(4294967295U);
        calls.select_title_texture(TitleTextureList::Common);
        const auto compact = ExactFloat(fp, view.horizontal_scale).compare(1.0f) == X87Comparison::equal;
        logo_x = compact ? layout.compact_logo_x : layout.logo_x;
        if (compact) {
            calls.select_title_texture(TitleTextureList::Small);
            draw_compact_title_logo(logo_x, view, layout, calls, fp);
        } else draw_large_title_logo(logo_x, view, layout, calls, fp);
        draw_title_lettering(layout.text_x, view, layout, calls, fp);
        return;
    }
    const auto compact_position = ExactFloat(fp, view.horizontal_scale).compare(1.0f) == X87Comparison::equal;
    const auto target = compact_position ? layout.compact_logo_x : layout.logo_x;
    if (work.wave_timer < 55) {
        const auto difference = ExactFloat(fp, target) - layout.logo_start_x;
        const auto progress = ExactFloat::integer(fp, work.wave_timer - 50);
        logo_x = (difference * progress * 0.2f + layout.logo_start_x).rounded();
    } else logo_x = target;
    calls.select_title_texture(TitleTextureList::Common);
    if (ExactFloat(fp, view.horizontal_scale).compare(1.0f) == X87Comparison::equal) {
        calls.select_title_texture(TitleTextureList::Small);
        draw_compact_title_logo(logo_x, view, layout, calls, fp);
    } else draw_large_title_logo(logo_x, view, layout, calls, fp);
    if (work.wave_timer < 70) return;
    const auto frames = work.wave_timer <= 77 ? work.wave_timer - 70 : 84U - work.wave_timer;
    const auto text_x = (ExactFloat::integer(fp, frames) * layout.text_step + layout.text_start_x).rounded();
    draw_title_lettering(text_x, view, layout, calls, fp);
}
// NOTSA: named blinking prompt pass; depth comparison is explicitly set to1 after it.
void draw_title_prompt(const TitleWork &work, const BackgroundView &view,
                        BackgroundCalls &calls, X87Context &fp) {
    if (work.timer < 150) return;
    calls.select_title_texture(view.language == 0 ? TitleTextureList::JapaneseTitle : TitleTextureList::Title);
    const auto blink = work.blink_timer;
    auto alpha = static_cast<std::uint8_t>(blink);
    if ((blink & 511U) >= 256) alpha = static_cast<std::uint8_t>(255U - alpha);
    calls.set_material((static_cast<std::uint32_t>(alpha) << 24) + 16777215U);
    calls.set_depth_compare(7);
    const auto prompt_x = view.language == 0 ? 260.0f : 278.0f;
    const auto prompt_y = view.language == 0 ? 333.0f : 342.0f;
    const auto count = view.language == 0 ? 3U : 2U;
    for (unsigned column = 0; column < count; ++column) {
        const auto scale_y = (ExactFloat(fp, view.vertical_scale) * 0.46874997f).rounded();
        const auto scale_x = (ExactFloat(fp, view.horizontal_scale) * 0.5f).rounded();
        const auto y = (ExactFloat(fp, view.vertical_scale) * prompt_y).rounded();
        auto x = ExactFloat(fp, view.horizontal_scale) * prompt_x;
        if (column != 0) x = x + ExactFloat(fp, view.horizontal_scale) * (column * 128.0f);
        calls.draw_scaled_texture(8 + column, x.rounded(), y, 1.1f, scale_x, scale_y);
    }
    calls.set_depth_compare(1);
}
} // namespace

void draw_title_background(TitleWork &work, const BackgroundView &view, const BackgroundLayout &layout,
                            BackgroundCalls &calls, X87Context &fp) {
    const auto alpha = work.wave_timer < 60 ? (work.wave_timer * 255U) / 60U : 255U;
    calls.set_base_color((alpha << 24) + 16777215U);
    calls.set_material(4294967295U);
    calls.select_title_texture(TitleTextureList::Common);
    if (ExactFloat(fp, view.horizontal_scale).compare(1.0f) == X87Comparison::equal)
        draw_compact_title_sky(work, view, calls, fp);
    else draw_large_title_sky(work, view, calls, fp);
    draw_title_logo(work, view, layout, calls, fp);
    draw_title_prompt(work, view, calls, fp);
    work.wave_timer += view.loop_count;
}

void draw_title(TitleWork &work, TitleDrawCalls &calls) {
    if (work.color != 0 && static_cast<std::int32_t>(work.submode) >= 0) {
        calls.default_blending();
        calls.set_base_color(work.color);
        // The compatibility render-mode entry returns immediately in the original.
        calls.draw_title_background(work);
    }
}

void initialize_title(TitleWork &work, TitleInitState &shared, TitleInitCalls &calls) {
    calls.set_stage(0, 0);
    calls.initialize_character_lighting();
    work.timer = 0;
    work.wave_timer = 0;
    work.cloud_offset = 0;
    work.cloud_index = 0;
    calls.load_mode_textures(MenuMode::Title);
    if (shared.language == 0) {
        calls.load_japanese_title_textures();
    }
    if (shared.character_selection == 0 && shared.skip_title != 1) {
        work.submode = TitleSubmode::Stay;
        work.music_started = 0;
        return;
    }
    calls.prepare_menu_voice();
    shared.canonical.next = MenuMode::TitleMenu;
    calls.set_title_menu_previous();
    work.music_started = 0;
    work.submode = TitleSubmode::StayMain;
}

void create_menu_objects(const SequenceData &data, std::int32_t &pending_ticks, ObjectCalls &calls) {
    using Factory = TaskHandle (ObjectCalls::*)(const ModeSelection &);
    constexpr std::array<Factory, 14> factories{
        nullptr, &ObjectCalls::create_character_select, &ObjectCalls::create_file_select,
        &ObjectCalls::create_level_select, &ObjectCalls::create_sound_test,
        &ObjectCalls::create_title, &ObjectCalls::create_title_menu, &ObjectCalls::create_options,
        &ObjectCalls::create_explanation, &ObjectCalls::create_subgame_list,
        &ObjectCalls::create_emblem_results, &ObjectCalls::create_internet_demo,
        &ObjectCalls::create_tv_settings, &ObjectCalls::create_puti};
    calls.calculate_ticks();
    pending_ticks = 0;
    if (data.born_mode == MenuMode::None) {
        return;
    }
    const auto first = factories[static_cast<std::size_t>(data.born_mode)];
    (calls.*first)(data.initial_transition);
    for (const auto &entry : data.following) {
        const auto factory = factories[static_cast<std::size_t>(entry.mode)];
        (calls.*factory)(entry.transition);
    }
}

TaskHandle create_title(State &state, const ModeSelection &transition, TitleCalls &calls) {
    const auto task = calls.create_title_task();
    state.title_task = task;
    if (task) {
        const auto work = calls.allocate_title_work(task);
        state.title_work = work;
        if (!work) {
            throw std::runtime_error("title allocation returned null");
        }
        work->previous = transition.previous;
        work->next = transition.next;
        work->depth = 5400.0f;
        work->phase = TitlePhase::RequestWait;
        work->submode = TitleSubmode::Stay;
        calls.install_title_callbacks(task);
    }
    return task;
}

void update_sequence(SequenceWork &work, SequenceWork &canonical, std::int32_t &demo_mode,
                     SequenceCalls &calls) {
    demo_mode = 1;
    if (work.waiting != 0) {
        work.frames = 10;
        work.waiting = 0;
    }
    if (work.frames != 0) {
        work.frames = static_cast<std::int16_t>(static_cast<std::uint16_t>(work.frames) - 1);
        if (work.frames < 1) {
            const auto next_mode = work.waiting_mode == MenuMode::None
                                       ? MenuMode::AwaitFade : work.waiting_mode;
            if (canonical.mode != next_mode) {
                canonical.old_mode = canonical.mode;
                canonical.mode = next_mode;
            }
        }
    }
    if (work.mode == MenuMode::AwaitFade) {
        if (work.fade_end == 1) {
            work.mode = MenuMode::ReturnResult;
        }
        return;
    }
    if (work.mode != MenuMode::ReturnResult) {
        work.fade_end = 0;
        return;
    }
    switch (work.advance) {
    case MenuResult::Adventure:
    case MenuResult::Trial:
    case MenuResult::Summary:
    case MenuResult::Mission:
    case MenuResult::Ending:
    case MenuResult::Tutorial:
    case MenuResult::HiddenSelect:
    case MenuResult::TitleToDemo:
        canonical.return_value = static_cast<std::int32_t>(work.advance);
        break;
    default:
        canonical.return_value = -1;
        break;
    }
    if (work.advance == MenuResult::TitleToDemo) {
        calls.set_entrance(0);
    }
    calls.release_menu_textures();
    calls.finalize_selection();
    work.mode = MenuMode::Finished;
}

void update_sequence_child(const TaskHandle &task, SequenceChildWork &work, SequenceCalls &calls) {
    work.frames = static_cast<std::int32_t>(static_cast<std::uint32_t>(work.frames) - 1);
    if (work.frames < 1) {
        calls.create_menu_objects(work.data);
        calls.destroy_sequence_child(task);
    }
}

void update_dialog_animation(std::uint8_t &value, std::optional<std::uint32_t> &angle,
                              float &fade, std::uint32_t loop_count, X87Context &fp) {
    if (angle) *angle += 728;
    using fidelity::ExactFloat;
    const auto comparison = ExactFloat(fp, fade).compare(1.0f);
    const auto next = comparison == X87Comparison::greater
        ? fade - ExactFloat::integer(fp, static_cast<std::int32_t>(loop_count)) * 0.025f
        : ExactFloat(fp, 1.0f);
    fade = next.rounded();
    const auto difference = next - 1.0f;
    const auto root_input = (difference + difference).rounded();
    const auto root = ExactFloat::signed_square_root(fp, root_input);
    const auto alpha = (1.0f - root) * 255.0f;
    value = static_cast<std::uint8_t>(alpha.truncated_integer());
}

DialogUpdateResult update_dialog(DialogWork &work, DialogWork &canonical,
                                  std::uint32_t loop_count, X87Context &fp) {
    update_dialog_animation(work.animation_value, work.animation_angle, work.fade, loop_count, fp);
    work.upper_color = 4294901760U;
    if (!work.parameters || work.parameters->style == DialogStyle::Cyan ||
        work.parameters->style == DialogStyle::CyanSecond) {
        work.lower_color = 4279416063U;
    } else {
        switch (work.parameters->style) {
        case DialogStyle::Blue:
        case DialogStyle::BlueSecond:
        case DialogStyle::BlueThird:
        case DialogStyle::BlueFourth:
            work.lower_color = 4278190335U;
            break;
        default: break;
        }
    }
    switch (work.phase) {
    case 0:
        reset_dialog_work(work, canonical);
        return DialogUpdateResult::Complete;
    case 1: return DialogUpdateResult::NeedsOpening;
    case 2: return DialogUpdateResult::NeedsControl;
    case 3: return DialogUpdateResult::NeedsClosePause;
    case 4: return DialogUpdateResult::NeedsClosing;
    case 5: return DialogUpdateResult::NeedsCleanup;
    }
    return DialogUpdateResult::Complete;
}

void reset_dialog_work(DialogWork &work, DialogWork &canonical) {
    work.color = 0;
    work.scale_x = 0;
    work.scale_y = 0;
    work.repeat_frames = 0;
    work.opening_frames = 0;
    work.fade = 1.0f;
    canonical.choice_ids.fill(-1);
    work.previous_choice = -1;
}

void initialize_dialog_work(DialogWork &work, DialogWork &canonical, std::uint32_t loop_count) {
    work.parameters = nullptr;
    work.phase = 0;
    work.blink_step = loop_count == 0 ? 1 : loop_count;
    // The original emits the same reset tail at both entry points.
    reset_dialog_work(work, canonical);
}

TaskHandle create_dialog(State &state, std::uint32_t loop_count, DialogCalls &calls) {
    const auto task = calls.create_dialog_task();
    state.dialog_task = task;
    if (task) {
        const auto work = calls.allocate_dialog_work(task);
        state.dialog_work = work;
        if (!work) {
            throw std::runtime_error("dialog allocation returned null");
        }
        initialize_dialog_work(*work, *state.dialog_work, loop_count);
        calls.install_dialog_callbacks(task);
    }
    return task;
}

void close_sound_manager(TaskHandle &manager, SoundManagerCalls &calls) {
    if (manager) {
        calls.destroy_sound_manager(manager);
        manager.reset();
    }
}

void delete_sound_manager(TaskHandle &manager, cleanup::TrackState &tracks,
                          SoundManagerCalls &calls) {
    calls.close_media(0);
    calls.close_media(1);
    tracks.current_song = -1;
    tracks.song_queue_state = 0;
    tracks.previous_song = -1;
    tracks.current_voice = -1;
    tracks.voice_queue_state = 0;
    manager.reset();
}

void load_mode_textures(MenuMode mode, MenuTextureState &textures, const ModeTextureLists &lists,
                         const SequenceWork &sequence, MenuTextureCalls &calls) {
    load_menu_textures(textures, lists[static_cast<std::size_t>(mode)], calls);
    if (sequence.old_mode != MenuMode::None) textures.ready = 0;
}

void set_main_menu_previous(MainMenuWork &work) { work.presentation.previous = MenuMode::Title; }

void load_menu_textures(MenuTextureState &state, const std::vector<MenuTextureId> & textures,
                        MenuTextureCalls &calls) {
    for (const auto texture : textures) {
        state.loaded[texture.value / 32] |= 1U << (texture.value % 32);
        calls.load_menu_texture(texture);
    }
}

void initialize_menu_data(MenuTextureState &state, const std::vector<MenuTextureId> & initial_textures,
                          MenuTextureCalls &calls) {
    state.loaded[0] = 0;
    state.loaded[1] = 0;
    load_menu_textures(state, initial_textures, calls);
    state.ready = 1;
}

void initialize_menu_sound(MenuSoundState &sound, SoundCalls &calls) {
    calls.select_sound_bank(42);
    sound.volume_task.reset();
    sound.music_task.reset();
}

void release_loading_font(SharedState &shared, Calls &calls) {
    if (shared.font_loaded == 1) {
        calls.release_font_resource();
    }
    shared.font_loaded = 0;
}

void prepare_title_assets(ActionBindings &bindings, const CharacterActions &actions) {
    bindings.sonic_clear = actions.sonic_clear;
    bindings.sonic_transition = actions.sonic_transition;
    bindings.knuckles_clear = actions.knuckles_clear;
    bindings.knuckles_transition = actions.knuckles_transition;
    bindings.big_sit = actions.big_sit;
    bindings.big_sit_repeat = actions.big_sit;
    bindings.big_prepare = actions.big_prepare;
    bindings.big_cast = actions.big_cast;
    bindings.big_cast_repeat = actions.big_cast;
}

void prepare_title_dialogs(SharedState &shared, Calls &calls) {
    if (shared.font_loaded == 0) {
        calls.load_loading_font();
        calls.copy_loading_font();
        calls.set_font_color(4294967295U, 4294967295U, 4294967295U, 4294967295U);
    }
    shared.font_loaded = 1;
    calls.draw_text("Now Loading...", 400.0f, 400.0f, 1.0f);
}

void reset_title_resources(Calls &calls) {
    calls.stop_music();
    calls.initialize_menu_sound();
    calls.set_frame_rate(1, 1);
    calls.set_wait_vsync(1);
    calls.initialize_heap();
    calls.initialize_tasks();
    calls.set_background(4278190080U, 4278190080U, 4278190080U);
    calls.set_fade_color(0);
    calls.release_loading_font();
    calls.free_character_textures();
    calls.release_all_textures();
    calls.initialize_texture_drawing();
    calls.initialize_render_state();
    calls.set_lighting(false);
}

TaskHandle initialize_sequence(State &state, const std::shared_ptr<SequenceWork> &work,
                               std::int32_t entry, Calls &calls) {
    calls.initialize_menu_data();
    calls.reset_selected_trial_level();
    work->old_mode = MenuMode::None;
    work->advance = MenuResult::Cancel;
    work->return_value = 0;
    work->fade_end = 0;
    work->fade_out = 1;
    work->fade_out_secondary = 1;
    work->data = &calls.sequence_data(entry);
    work->mode = work->data->born_mode;
    work->fixed_mode = work->data->born_mode;
    const auto child = calls.create_sequence_child();
    if (child) {
        const auto child_work = calls.allocate_child_work(child);
        if (!child_work) {
            throw std::runtime_error("title sequence child allocation returned null");
        }
        child_work->data = work->data;
        child_work->frames = 1;
        calls.install_child_callback(child);
    }
    work->waiting = 0;
    work->frames = 0;
    state.dialog_task = calls.initialize_dialog();
    return state.dialog_task;
}

TaskHandle create_sequence(State &state, std::int32_t entry, Calls &calls) {
    if (state.sequence_task) {
        calls.sequence_already_active();
        // The original loops forever. A native service reports this nonreturning boundary.
        throw std::runtime_error("title sequence already active");
    }
    const auto task = calls.create_sequence_task();
    state.sequence_task = task;
    if (task) {
        const auto work = calls.allocate_sequence_work(task);
        state.sequence_work = work;
        if (!work) {
            throw std::runtime_error("title sequence allocation returned null");
        }
        initialize_sequence(state, work, entry, calls);
        calls.install_sequence_callbacks(task);
    }
    return task;
}

std::int32_t initialize_title_menu(State &state, SharedState &shared,
                                  const std::array<std::uint8_t, 2> &entry_flags, Calls &calls) {
    reset_title_resources(calls);
    calls.set_view_angle(12743);
    calls.set_clip(-1.0f, -65535.0f);
    calls.set_camera();
    calls.load_regular_object_textures(0);
    calls.load_player_motion(0);
    state.sequence_task.reset();
    std::int32_t entry = 0;
    if (entry_flags[0] == 1) {
        entry = 1;
    } else if (entry_flags[0] == 2) {
        entry = 2;
    } else {
        shared.menu_voice = 0;
        shared.menu_voice_state = 0;
        shared.loop_count = 0;
    }
    create_sequence(state, entry, calls);
    if (entry_flags[1] != 0) {
        prepare_title_dialogs(shared, calls);
    }
    return 0;
}
} // namespace sadx::title_menu
