#include "game/game_mode_internal.hpp"
#include <initializer_list>

namespace sadx::flow {
namespace {
// NOTSA: source mode increments wrap as 32-bit ADD, including callback-supplied values.
GameMode next_mode(GameMode mode) {
    return static_cast<GameMode>(static_cast<std::uint32_t>(mode) + 1U);
}

// NOTSA: the source shares this next-frame logo initialization transition.
void queue_logo_initialization(FlowContext &flow) { write_mode(flow, GameMode::LogoInit); }

// NOTSA: recovered shared title-entry tail.
void enter_title(FlowContext &flow) {
    enter_phase(flow, FlowPhase::EnterTitle);
    prepare_title_initialization(flow);
    write_mode(flow, GameMode::TitleInit);
    write_title_mode(flow, 0);
}

// NOTSA: recovered shared cleanup before title entry.
void clear_to_title(FlowContext &flow) {
    enter_phase(flow, FlowPhase::ClearToTitle);
    clear_game(flow);
    reset_game(flow);
    enter_title(flow);
}

// NOTSA: cancel paths use their shared cleanup and retain the source store order.
void cancel_to_title(FlowContext &flow) {
    enter_phase(flow, FlowPhase::CancelTitle);
    clear_game(flow);
    reset_game(flow);
    prepare_title_initialization(flow);
    write_title_mode(flow, 0);
    write_mode(flow, GameMode::TitleInit);
}

// NOTSA: recovered common action/adventure initialization tail.
void begin_gameplay(FlowContext &flow) {
    enter_phase(flow, FlowPhase::BeginGameplay);
    load_regular_object_textures(flow, 0);
    write_mode(flow, initialize_level_game(flow) == 1 ? GameMode::Adventure : GameMode::Action);
}

// NOTSA: recovered shared mission-stage setup; player queries remain separate calls.
void configure_mission(FlowContext &flow) {
    enter_phase(flow, FlowPhase::ConfigureMission);
    const auto character = static_cast<std::int32_t>(current_player_number(flow));
    if (character == 2 || (character > 4 && character <= 6)) {
        set_stage_number(flow, 33, 0);
    } else {
        set_stage_number(flow, 26, 3);
    }
    set_entrance_number(flow, 0);
    set_sequence_time(flow, 0);
    load_regular_object_textures(flow, 0);
    setup_character(flow, current_player_number(flow));
    initialize_level_game(flow);
    reset_player_work(flow);
    write_mode(flow, GameMode::Mission);
}

// NOTSA: named action-mode step, preserving AX narrowing at the callee seam.
void update_action(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Action);
    if (read_action_restart_requested(flow) != 0) {
        if (read_save_phase(flow) != 0)
            return;
        enter_phase(flow, FlowPhase::ActionRestart);
        clear_game(flow);
        reset_game(flow);
        write_mode(flow, GameMode::StaffRollInit);
        write_action_restart_requested(flow, 0);
        return;
    }
    const auto result = run_action(flow);
    if (result == -2) {
        write_save_ready(flow, 0);
        clear_to_title(flow);
    } else if (result == 1) {
        write_mode(flow, GameMode::Adventure);
    } else if (result == -1) {
        enter_phase(flow, FlowPhase::ActionReturn);
        clear_game(flow);
        reset_game(flow);
        prepare_action_return(flow);
        const auto mode = check_return_mode(flow);
        if (mode == 0)
            enter_title(flow);
        else
            write_mode(flow, static_cast<GameMode>(mode));
    } else if (result == 10) {
        write_action_restart_requested(flow, 1);
    }
}

// NOTSA: named adventure-mode step.
void update_adventure(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Adventure);
    const auto result = run_adventure(flow);
    if (result == -2) {
        write_save_ready(flow, 0);
        clear_to_title(flow);
    } else if (result == 2) {
        write_mode(flow, GameMode::Action);
    } else if (result == -1) {
        enter_phase(flow, FlowPhase::AdventureReturn);
        clear_game(flow);
        reset_game(flow);
        const auto mode = check_return_mode(flow);
        if (mode == 0)
            enter_title(flow);
        else
            write_mode(flow, static_cast<GameMode>(mode));
    } else if (result == 10) {
        clear_game(flow);
        reset_game(flow);
        write_mode(flow, GameMode::StaffRollInit);
    }
}

// NOTSA: named trial-mode step; cancellation differs from ordinary action cancellation.
void update_trial(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Trial);
    const auto result = run_action(flow);
    if (result == -2) {
        enter_phase(flow, FlowPhase::TrialCancel);
        write_save_ready(flow, 0);
        clear_game(flow);
        reset_game(flow);
        enable_player_pad_reading(flow, 2);
        enter_title(flow);
    } else if (result == 1) {
        enable_player_pad_reading(flow, 2);
        write_mode(flow, GameMode::TitleInit);
        write_title_mode(flow, 2);
    } else if (result == -1) {
        enter_phase(flow, FlowPhase::TrialFinish);
        write_mode(flow, GameMode::TitleInit);
        write_title_mode(flow, 2);
        enable_player_pad_reading(flow, 2);
    }
}

// NOTSA: named mission-mode step.
void update_mission(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Mission);
    const auto result = run_mission(flow);
    if (result == -2) {
        write_save_ready(flow, 0);
        clear_to_title(flow);
    } else if (result == -1) {
        enter_phase(flow, FlowPhase::MissionReturn);
        clear_game(flow);
        reset_game(flow);
        write_mode(flow, GameMode::TitleInit);
        write_title_mode(flow, 1);
        enable_player_pad_reading(flow, 2);
    } else {
        write_mode(flow, GameMode::Mission);
    }
}

// NOTSA: named title-menu result handling.
void update_title_menu(FlowContext &flow) {
    enter_phase(flow, FlowPhase::TitleMenu);
    const auto result = run_title_menu(flow);
    // Original quirk: unsigned range test includes the two negative control results.
    if (static_cast<std::uint32_t>(result) + 2U > 109U)
        return;
    switch (result) {
    case -2:
        cancel_to_title(flow);
        return;
    case -1:
        enter_phase(flow, FlowPhase::TitleBack);
        close_title_menu(flow, 0);
        write_mode(flow, GameMode::Title);
        prepare_title(flow);
        return;
    case 1:
        enter_phase(flow, FlowPhase::TitleTrial);
        close_title_menu(flow, 0);
        write_pause_state(flow, 1);
        load_regular_object_textures(flow, 0);
        initialize_level_game(flow);
        write_mode(flow, GameMode::Trial);
        return;
    case 100: {
        enter_phase(flow, FlowPhase::TitlePlayer);
        close_title_menu(flow, 0);
        start_screen_fade(flow);
        write_pause_state(flow, 1);
        load_regular_object_textures(flow, 0);
        const auto character = current_player_number(flow);
        setup_character(flow, character);
        initialize_level_game(flow);
        reset_player_work(flow);
        reset_player_parameters(flow);
        write_mode(flow, GameMode::Trial);
        return;
    }
    case 101:
        enter_phase(flow, FlowPhase::TitleSummary);
        close_title_menu(flow, 0);
        write_mode(flow, GameMode::SummaryInit);
        return;
    case 102: {
        enter_phase(flow, FlowPhase::TitleMission);
        close_title_menu(flow, 0);
        start_screen_fade(flow);
        write_pause_state(flow, 1);
        const auto character = current_player_number(flow);
        load_save(flow);
        prepare_sequence(flow);
        setup_character(flow, character);
        set_sequence_player(flow, character);
        apply_sequence_level(flow, current_sequence_level(flow));
        configure_mission(flow);
        return;
    }
    case 103:
        enter_phase(flow, FlowPhase::TitleStaff);
        close_title_menu(flow, 0);
        write_mode(flow, GameMode::StaffRollInit);
        return;
    case 104:
        enter_phase(flow, FlowPhase::TitleTutorial);
        close_title_menu(flow, 0);
        write_mode(flow, GameMode::TutorialInit);
        return;
    case 107:
        enter_phase(flow, FlowPhase::TitleDemo);
        go_demo_loop(flow);
        begin_gameplay(flow);
        return;
    default:
        return;
    }
}

// NOTSA: title initialization may finish the menu in this same frame.
void initialize_title(FlowContext &flow) {
    enter_phase(flow, FlowPhase::TitleInit);
    if (read_title_load_phase(flow) != 2) {
        for (const auto kind : {4U, 0U, 5U, 7U, 6U, 8U, 9U})
            run_level_destructor(flow, kind);
        const auto resource = read_message_resource(flow);
        if (resource.value != 0) {
            free_message_resource(flow, resource);
            write_message_resource(flow, {});
        }
        free_regular_object_texlists(flow);
        free_character_texlists(flow);
        release_all_textures(flow);
        prepare_title_assets(flow);
        request_file_module(flow, FlowModule::Advertise);
        enter_phase(flow, FlowPhase::TitleResources);
        if (read_title_load_phase(flow) == 1) {
            prepare_title_dialogs(flow);
            write_title_load_phase(flow, 2);
            return;
        }
    }
    enter_phase(flow, FlowPhase::TitleLoading);
    initialize_title_menu(flow);
    set_wait_vsync_count(flow, 1);
    write_mode(flow, next_mode(read_mode(flow)));
    write_playing_super_sonic(flow, 0);
    load_level_sounds(flow);
    if (read_title_load_phase(flow) == 2) {
        write_title_load_phase(flow, 0);
        return;
    }
    update_title_menu(flow);
}

// NOTSA: named summary update and completion step.
void update_summary(FlowContext &flow) {
    enter_phase(flow, FlowPhase::SummaryUpdate);
    const auto result = run_summary(flow, 0);
    if (result == 1) {
        enter_phase(flow, FlowPhase::SummaryComplete);
        const auto character = current_player_number(flow);
        reset_player_work(flow);
        load_save(flow);
        prepare_sequence(flow);
        setup_character(flow, character);
        set_sequence_player(flow, character);
        apply_sequence_level(flow, current_sequence_level(flow));
        write_pause_state(flow, 1);
        load_regular_object_textures(flow, 0);
        write_mode(flow, current_level_type(flow) != 2 ? GameMode::Adventure : GameMode::Action);
    } else if (result == static_cast<std::uint32_t>(-1)) {
        write_title_mode(flow, 1);
        write_mode(flow, GameMode::TitleInit);
    } else if (result == static_cast<std::uint32_t>(-2)) {
        cancel_to_title(flow);
    }
}

// NOTSA: summary initialization can immediately run the first summary update.
void start_summary(FlowContext &flow) {
    enter_phase(flow, FlowPhase::SummaryInit);
    request_file_module(flow, FlowModule::Summary);
    if (initialize_summary(flow, 0) != static_cast<std::uint32_t>(-1)) {
        write_mode(flow, next_mode(read_mode(flow)));
        update_summary(flow);
        return;
    }
    enter_phase(flow, FlowPhase::SummaryFallback);
    write_mode(flow, GameMode::Adventure);
    reset_player_work(flow);
    const auto character = current_player_number(flow);
    load_save(flow);
    if (character == 3 && check_sequence_flag(flow, 74) == 0 && run_movie(flow, 5) < 0) {
        cancel_to_title(flow);
        return;
    }
    prepare_sequence(flow);
    setup_character(flow, character);
    set_sequence_player(flow, character);
    apply_sequence_level(flow, current_sequence_level(flow));
    write_pause_state(flow, 1);
    load_regular_object_textures(flow, 0);
}

// NOTSA: named tutorial update and completion step.
void update_tutorial(FlowContext &flow) {
    enter_phase(flow, FlowPhase::TutorialUpdate);
    const auto result = run_tutorial(flow, 0);
    if (result == 1) {
        if (read_tutorial_variant(flow) != 2) {
            write_mode(flow, GameMode::SummaryInit);
            return;
        }
        enter_phase(flow, FlowPhase::TutorialMission);
        start_screen_fade(flow);
        write_pause_state(flow, 1);
        const auto character = current_player_number(flow);
        load_save(flow);
        prepare_sequence(flow);
        setup_character(flow, character);
        set_sequence_player(flow, character);
        apply_sequence_level(flow, current_sequence_level(flow));
        configure_mission(flow);
    } else if (result == static_cast<std::uint32_t>(-1)) {
        write_title_mode(flow, 1);
        write_title_load_phase(flow, 1);
        write_mode(flow, GameMode::TitleInit);
    } else if (result == static_cast<std::uint32_t>(-2)) {
        cancel_to_title(flow);
    }
}

// NOTSA: named staff-roll update step.
void update_staff_roll(FlowContext &flow) {
    enter_phase(flow, FlowPhase::StaffUpdate);
    const auto result = run_staff_roll(flow, 0);
    if (result == -2) {
        enter_phase(flow, FlowPhase::StaffCancel);
        set_frame_rate_mode(flow, 1, 1);
        cancel_to_title(flow);
    } else if (result == -1) {
        enter_phase(flow, FlowPhase::StaffReturn);
        set_frame_rate_mode(flow, 1, 1);
        write_mode(flow, GameMode::TitleInit);
        write_title_mode(flow, 1);
    } else if (result == 1) {
        load_sega_logo(flow);
        write_mode(flow, GameMode::Logo);
    }
}

// NOTSA: named logo update, including its two completion transitions.
void update_logo(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Logo);
    const auto result = run_logos(flow);
    if (result == 1) {
        enter_phase(flow, FlowPhase::LogoTitle);
        set_frame_rate_mode(flow, 1, 1);
        enter_title(flow);
    } else if (result == 3) {
        enter_phase(flow, FlowPhase::LogoMovie);
        set_frame_rate_mode(flow, 1, 1);
        write_movie_number(flow, 0);
        write_mode(flow, GameMode::Movie);
    }
}

// NOTSA: named title update; input refresh follows both completion branches.
void update_title(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Title);
    write_pause_state(flow, 0);
    const auto result = run_title(flow);
    if (result == 1) {
        enter_phase(flow, FlowPhase::TitleAccepted);
        write_pause_state(flow, 1);
        load_regular_object_textures(flow, 0);
        write_mode(flow, initialize_level_game(flow) == 1 ? GameMode::Adventure : GameMode::Action);
    } else if (result == static_cast<std::uint32_t>(-1)) {
        write_mode(flow, GameMode::SelectPlayer);
        initialize_player_selection(flow);
    }
    get_switch_data(flow);
    enable_pad_reading(flow);
}

// NOTSA: named startup step and next-frame logo decision.
void start_game(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Start);
    initialize_game(flow);
    initialize_demo_state(flow);
    if (startup_status(flow) != 0 || startup_readiness(flow) >= 1) {
        queue_logo_initialization(flow);
    } else {
        write_mode(flow, GameMode::NoSaveWindowInit);
    }
}

// NOTSA: the shared per-frame tail preserves short-circuit reads and callback rereads.
void finish_frame(FlowContext &flow) {
    enter_phase(flow, FlowPhase::FrameTail);
    if (read_menu_message_enabled(flow) != 0 && read_alert_flag(flow) != 0 &&
        read_alert_blocked(flow) == 0 && read_save_phase(flow) == 0) {
        stop_vibration(flow, 0);
        pause_all_sound(flow);
        if (read_mode(flow) == GameMode::Mission && mission_clear_status(flow) != 0) {
            release_mission_clear_texlist(flow);
            display_message(flow);
        }
    }
    draw_screen_fade(flow);
    increment_frame_counter(flow);
}
} // namespace

// NOTSA: both entry points share the original dispatcher and frame tail.
static void advance(FlowContext &flow) {
    enter_phase(flow, FlowPhase::Dispatch);
    switch (read_mode(flow)) {
    case GameMode::Start:
        start_game(flow);
        break;
    case GameMode::NoSaveWindowInit:
        enter_phase(flow, FlowPhase::NoSave);
        request_file_module(flow, FlowModule::AlertWindow);
        on_edit(flow);
        write_mode(flow, GameMode::TvSettingEnd);
        break;
    case GameMode::NoSaveWindow:
        write_mode(flow, GameMode::TvSettingEnd);
        break;
    case GameMode::TvSettingEnd:
        queue_logo_initialization(flow);
        break;
    case GameMode::LogoInit:
        enter_phase(flow, FlowPhase::LogoInit);
        initialize_sega_logo(flow);
        write_mode(flow, GameMode::Logo);
        [[fallthrough]];
    case GameMode::Logo:
        update_logo(flow);
        break;
    case GameMode::Title:
        update_title(flow);
        break;
    case GameMode::SelectPlayer:
        enter_phase(flow, FlowPhase::SelectPlayer);
        if (run_player_selection(flow) == 1)
            begin_gameplay(flow);
        break;
    case GameMode::Action:
        update_action(flow);
        break;
    case GameMode::Adventure:
        update_adventure(flow);
        break;
    case GameMode::Movie:
        enter_phase(flow, FlowPhase::Movie);
        if (run_movie(flow, read_movie_number(flow)) != 0) {
            clear_to_title(flow);
        } else {
            clear_game(flow);
            reset_game(flow);
            write_pause_state(flow, 1);
            begin_gameplay(flow);
        }
        break;
    case GameMode::Trial:
        update_trial(flow);
        break;
    case GameMode::Mission:
        update_mission(flow);
        break;
    case GameMode::TitleInit:
        initialize_title(flow);
        break;
    case GameMode::TitleMenu:
        update_title_menu(flow);
        break;
    case GameMode::SummaryInit:
        start_summary(flow);
        break;
    case GameMode::Summary:
        update_summary(flow);
        break;
    case GameMode::TutorialInit:
        enter_phase(flow, FlowPhase::TutorialInit);
        request_file_module(flow, FlowModule::Summary);
        initialize_tutorial(flow, 0);
        write_mode(flow, next_mode(read_mode(flow)));
        [[fallthrough]];
    case GameMode::Tutorial:
        update_tutorial(flow);
        break;
    case GameMode::StaffRollInit: {
        enter_phase(flow, FlowPhase::StaffInit);
        request_file_module(flow, FlowModule::Summary);
        initialize_staff_roll(flow, 0);
        const auto current = read_mode(flow);
        write_pause_state(flow, 0);
        write_mode(flow, next_mode(current));
        set_frame_rate_mode(flow, 1, 1);
        set_wait_vsync_count(flow, 1);
        [[fallthrough]];
    }
    case GameMode::StaffRoll:
        update_staff_roll(flow);
        break;
    case GameMode::Backdrop:
        enter_phase(flow, FlowPhase::Backdrop);
        set_background_color(flow, {0, 0, 34, 255}, {102, 0, 0, 255}, {0, 0, 0, 255});
        reset_game(flow);
        enter_title(flow);
        break;
    default:
        break;
    }
    finish_frame(flow);
}

void advance_game_mode(GameFlowState &state) {
    FlowContext flow{state, FlowPhase::Dispatch};
    advance(flow);
}

void advance_game_mode(GameFlowState &state, NativeServices &services) {
    FlowContext flow{state, FlowPhase::Dispatch, &services};
    advance(flow);
}
} // namespace sadx::flow
