#include "runtime/native_title_scene.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "engine/frame_loop_native.hpp"
#include "game/sound_effects.hpp"
#include "math/ninja_sine_table.hpp"
#include <stdexcept>
#include <string>

namespace sadx::native {
void TitleScene::require_complete(title_menu::MenuCallbackResult result, const char *menu) {
    if (result == title_menu::MenuCallbackResult::Complete) return;
    static constexpr std::array<const char *,6> names{
        "", "_initialization", "_leave", "_fade_in", "_control", "_fade_out"};
    const auto name = std::string(menu) + names.at(static_cast<std::size_t>(result));
    throw flow::UnrecoveredGameFlowCall(name.c_str());
}
void TitleScene::require_drawn(title_menu::MenuDrawResult result, const char *menu) {
    if (result != title_menu::MenuDrawResult::Complete)
        throw flow::UnrecoveredGameFlowCall(menu);
}
TaskCallback TitleScene::menu_exec(title_menu::MenuMode mode) {
    switch (mode) {
    case title_menu::MenuMode::CharacterSelect: return exec_character_select;
    case title_menu::MenuMode::FileSelect: return exec_file_select;
    case title_menu::MenuMode::TrialActSelect: return exec_level_select;
    case title_menu::MenuMode::SoundTest: return exec_sound_test;
    case title_menu::MenuMode::TitleMenu: return exec_main_menu;
    case title_menu::MenuMode::OptionSelect: return exec_options;
    case title_menu::MenuMode::Explanation: return exec_explanation;
    case title_menu::MenuMode::MinigameSelect: return exec_subgame_list;
    case title_menu::MenuMode::EmblemView: return exec_emblem_results;
    case title_menu::MenuMode::InternetDemo: return exec_internet_demo;
    case title_menu::MenuMode::TvSettings: return exec_tv_settings;
    case title_menu::MenuMode::Title: return exec_title;
    case title_menu::MenuMode::Puti: return exec_puti;
    default: throw std::runtime_error("unsupported native menu callback");
    }
}
TaskCallback TitleScene::menu_display(title_menu::MenuMode mode) {
    switch (mode) {
    case title_menu::MenuMode::CharacterSelect: return display_character_select;
    case title_menu::MenuMode::FileSelect: return display_file_select;
    case title_menu::MenuMode::TrialActSelect: return display_level_select;
    case title_menu::MenuMode::SoundTest: return display_sound_test;
    case title_menu::MenuMode::TitleMenu: return display_main_menu;
    case title_menu::MenuMode::OptionSelect: return display_options;
    case title_menu::MenuMode::Explanation: return display_explanation;
    case title_menu::MenuMode::MinigameSelect: return display_subgame_list;
    case title_menu::MenuMode::EmblemView: return display_emblem_results;
    case title_menu::MenuMode::InternetDemo: return display_internet_demo;
    case title_menu::MenuMode::TvSettings: return display_tv_settings;
    case title_menu::MenuMode::Title: return display_title;
    default: throw std::runtime_error("unsupported native menu display callback");
    }
}
TaskHandle TitleScene::create_menu_task(title_menu::MenuMode mode, std::uint32_t flags,
                                       std::uint32_t level) {
    return create_task(flags, static_cast<int>(level), menu_exec(mode));
}
void TitleScene::install_menu_callbacks(title_menu::MenuMode mode, const TaskHandle &task) {
    const auto display = menu_display(mode);
    register_callback(display);
    task->exec = menu_exec(mode);
    task->display = display;
}
template<class Work>
std::shared_ptr<Work> TitleScene::allocate_menu_work(const TaskHandle &task, std::uint32_t size) {
    if (!allocate_auxiliary(task, size, true)) return {};
    auto work = std::make_shared<Work>();
    task->auxiliary_native_work = work;
    return work;
}
title_menu::MenuFactoryInputs TitleScene::factory_inputs() {
    return {session_.bootstrap_.menu.character_cursor, session_.bootstrap_.menu.selected_file,
            session_.bootstrap_.card_operation, session_.bootstrap_.card_state,
            session_.flow_.tutorial_variant};
}
title_menu::StageSelectionInputs TitleScene::selection_inputs() {
    return {session_.bootstrap_.menu.player_character, session_.bootstrap_.menu.flags[0],
            session_.bootstrap_.completed_stages, assets_.selection};
}
void TitleScene::calculate_ticks() {
    const auto demo_mode = session_.logo_.demo_mode();
    frame_loop::TickState state{session_.timing_, session_.secondary_loop_count_,
        session_.loop_.elapsed_ticks, session_.loop_.loop_starting, demo_mode};
    const auto result = frame_loop::calculate_ticks(state);
    if (result != frame_loop::TickFailure::None)
        throw std::runtime_error("native menu timing divide fault");
}
void TitleScene::exec_character_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_character_select);
    auto work = std::static_pointer_cast<title_menu::CharacterSelectWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native character_select task has no work");
    scene.require_complete(title_menu::update_character_select(*work, *scene.state_.sequence_work,
        scene.session_.bootstrap_.menu.character_cursor, scene.session_.bootstrap_.menu.player_character),
                            "native_character_select");
}
void TitleScene::display_character_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_character_select);
    auto work = std::static_pointer_cast<title_menu::CharacterSelectWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native character_select task has no work");
    scene.require_drawn(title_menu::draw_character_select(*work), "native_character_select_drawing");
}
void TitleScene::exec_file_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_file_select);
    auto work = std::static_pointer_cast<title_menu::FileSelectWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native file_select task has no work");
    file_select::update(*work, scene.state_, scene.session_.secondary_loop_count_,
        scene.session_.file_select_fade_in_marker_, scene, scene.display_.arithmetic());
}
void TitleScene::display_file_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_file_select);
    auto work = std::static_pointer_cast<title_menu::FileSelectWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native file_select task has no work");
    scene.draw_file_select(*work);
}
void TitleScene::exec_level_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_level_select);
    auto work = std::static_pointer_cast<title_menu::StageSelectWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native level_select task has no work");
    scene.require_complete(title_menu::update_level_select(*work, *scene.state_.sequence_work),
                            "native_level_select");
}
void TitleScene::display_level_select(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_level_select);
    auto work = std::static_pointer_cast<title_menu::StageSelectWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native level_select task has no work");
    scene.require_drawn(title_menu::draw_level_select(*work), "native_level_select_drawing");
}
void TitleScene::exec_sound_test(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_sound_test);
    auto work = std::static_pointer_cast<title_menu::SoundTestWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native sound_test task has no work");
    scene.require_complete(title_menu::update_sound_test(*work, *scene.state_.sequence_work),
                            "native_sound_test");
}
void TitleScene::display_sound_test(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_sound_test);
    auto work = std::static_pointer_cast<title_menu::SoundTestWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native sound_test task has no work");
    scene.require_drawn(title_menu::draw_sound_test(*work), "native_sound_test_drawing");
}
void TitleScene::exec_main_menu(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_main_menu);
    auto work = std::static_pointer_cast<title_menu::MainMenuWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native main_menu task has no work");
    scene.require_complete(title_menu::update_main_menu(*work, *scene.state_.sequence_work),
                            "native_main_menu");
}
void TitleScene::display_main_menu(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_main_menu);
    auto work = std::static_pointer_cast<title_menu::MainMenuWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native main_menu task has no work");
    scene.require_drawn(title_menu::draw_main_menu(*work), "native_main_menu_drawing");
}
void TitleScene::exec_options(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_options);
    auto work = std::static_pointer_cast<title_menu::OptionWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native options task has no work");
    scene.require_complete(title_menu::update_options(*work, *scene.state_.sequence_work),
                            "native_options");
}
void TitleScene::display_options(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_options);
    auto work = std::static_pointer_cast<title_menu::OptionWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native options task has no work");
    scene.require_drawn(title_menu::draw_options(*work), "native_options_drawing");
}
void TitleScene::exec_explanation(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_explanation);
    auto work = std::static_pointer_cast<title_menu::ExplanationWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native explanation task has no work");
    scene.require_complete(title_menu::update_explanation(*work, *scene.state_.sequence_work),
                            "native_explanation");
}
void TitleScene::display_explanation(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_explanation);
    auto work = std::static_pointer_cast<title_menu::ExplanationWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native explanation task has no work");
    scene.require_drawn(title_menu::draw_explanation(*work), "native_explanation_drawing");
}
void TitleScene::exec_subgame_list(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_subgame_list);
    auto work = std::static_pointer_cast<title_menu::StageSelectWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native subgame_list task has no work");
    scene.require_complete(title_menu::update_subgame_list(*work, *scene.state_.sequence_work),
                            "native_subgame_list");
}
void TitleScene::display_subgame_list(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_subgame_list);
    auto work = std::static_pointer_cast<title_menu::StageSelectWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native subgame_list task has no work");
    scene.require_drawn(title_menu::draw_subgame_list(*work), "native_subgame_list_drawing");
}
void TitleScene::exec_emblem_results(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_emblem_results);
    auto work = std::static_pointer_cast<title_menu::EmblemWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native emblem_results task has no work");
    scene.require_complete(title_menu::update_emblem_results(*work, *scene.state_.sequence_work),
                            "native_emblem_results");
}
void TitleScene::display_emblem_results(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_emblem_results);
    auto work = std::static_pointer_cast<title_menu::EmblemWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native emblem_results task has no work");
    scene.require_drawn(title_menu::draw_emblem_results(*work), "native_emblem_results_drawing");
}
void TitleScene::exec_internet_demo(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_internet_demo);
    auto work = std::static_pointer_cast<title_menu::InternetDemoWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native internet_demo task has no work");
    scene.require_complete(title_menu::update_internet_demo(*work, *scene.state_.sequence_work),
                            "native_internet_demo");
}
void TitleScene::display_internet_demo(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_internet_demo);
    auto work = std::static_pointer_cast<title_menu::InternetDemoWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native internet_demo task has no work");
    scene.require_drawn(title_menu::draw_internet_demo(*work), "native_internet_demo_drawing");
}
void TitleScene::exec_tv_settings(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_tv_settings);
    auto work = std::static_pointer_cast<title_menu::TvSettingsWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native tv_settings task has no work");
    scene.require_complete(title_menu::update_tv_settings(*work, *scene.state_.sequence_work),
                            "native_tv_settings");
}
void TitleScene::display_tv_settings(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_tv_settings);
    auto work = std::static_pointer_cast<title_menu::TvSettingsWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native tv_settings task has no work");
    scene.require_drawn(title_menu::draw_tv_settings(*work), "native_tv_settings_drawing");
}
std::shared_ptr<title_menu::CharacterSelectWork> TitleScene::allocate_character_select(const TaskHandle &task) {
    return allocate_menu_work<title_menu::CharacterSelectWork>(task, 56);
}
std::shared_ptr<title_menu::FileSelectWork> TitleScene::allocate_file_select(const TaskHandle &task) {
    return allocate_menu_work<title_menu::FileSelectWork>(task, 56);
}
std::shared_ptr<title_menu::StageSelectWork> TitleScene::allocate_level_select(const TaskHandle &task) {
    return allocate_menu_work<title_menu::StageSelectWork>(task, 208);
}
std::shared_ptr<title_menu::SoundTestWork> TitleScene::allocate_sound_test(const TaskHandle &task) {
    return allocate_menu_work<title_menu::SoundTestWork>(task, 72);
}
std::shared_ptr<title_menu::MainMenuWork> TitleScene::allocate_main_menu(const TaskHandle &task) {
    return allocate_menu_work<title_menu::MainMenuWork>(task, 40);
}
std::shared_ptr<title_menu::OptionWork> TitleScene::allocate_options(const TaskHandle &task) {
    return allocate_menu_work<title_menu::OptionWork>(task, 84);
}
std::shared_ptr<title_menu::ExplanationWork> TitleScene::allocate_explanation(const TaskHandle &task) {
    return allocate_menu_work<title_menu::ExplanationWork>(task, 32);
}
std::shared_ptr<title_menu::StageSelectWork> TitleScene::allocate_subgame(const TaskHandle &task) {
    return allocate_menu_work<title_menu::StageSelectWork>(task, 208);
}
std::shared_ptr<title_menu::EmblemWork> TitleScene::allocate_emblem(const TaskHandle &task) {
    return allocate_menu_work<title_menu::EmblemWork>(task, 60);
}
std::shared_ptr<title_menu::InternetDemoWork> TitleScene::allocate_internet_demo(const TaskHandle &task) {
    return allocate_menu_work<title_menu::InternetDemoWork>(task, 40);
}
std::shared_ptr<title_menu::TvSettingsWork> TitleScene::allocate_tv_settings(const TaskHandle &task) {
    return allocate_menu_work<title_menu::TvSettingsWork>(task, 56);
}
TaskHandle TitleScene::create_character_select(const title_menu::ModeSelection &transition) {
    return title_menu::create_character_select(state_.factories, transition, factory_inputs(), *this);
}
TaskHandle TitleScene::create_file_select(const title_menu::ModeSelection &transition) {
    auto input = factory_inputs();
    return title_menu::create_file_select(state_.factories, transition, input, *this);
}
TaskHandle TitleScene::create_level_select(const title_menu::ModeSelection &transition) {
    return title_menu::create_level_select(state_.factories, transition, selection_inputs(), *this);
}
TaskHandle TitleScene::create_sound_test(const title_menu::ModeSelection &transition) {
    return title_menu::create_sound_test(state_.factories, transition, *this);
}
TaskHandle TitleScene::create_title_menu(const title_menu::ModeSelection &transition) {
    return title_menu::create_main_menu(state_.factories, transition, factory_inputs(), *this);
}
TaskHandle TitleScene::create_options(const title_menu::ModeSelection &transition) {
    return title_menu::create_options(state_.factories, transition, *this);
}
TaskHandle TitleScene::create_explanation(const title_menu::ModeSelection &transition) {
    return title_menu::create_explanation(state_.factories, transition, *this);
}
TaskHandle TitleScene::create_subgame_list(const title_menu::ModeSelection &transition) {
    return title_menu::create_subgame_list(state_.factories, transition, selection_inputs(), *this);
}
TaskHandle TitleScene::create_emblem_results(const title_menu::ModeSelection &transition) {
    return title_menu::create_emblem_results(state_.factories, transition, *this);
}
TaskHandle TitleScene::create_internet_demo(const title_menu::ModeSelection &transition) {
    return title_menu::create_internet_demo(state_.factories, transition, *this);
}
TaskHandle TitleScene::create_tv_settings(const title_menu::ModeSelection &transition) {
    return title_menu::create_tv_settings(state_.factories, transition, *this);
}
TaskHandle TitleScene::create_puti(const title_menu::ModeSelection &) {
    return title_menu::create_puti(state_.factories, *this);
}
std::shared_ptr<title_menu::PutiWork> TitleScene::allocate_puti_work(const TaskHandle &task) {
    if (!allocate_auxiliary(task, 16, false)) return {};
    auto work = std::make_shared<title_menu::PutiWork>();
    task->auxiliary_native_work = work;
    return work;
}
void TitleScene::install_puti_callbacks(const TaskHandle &task) {
    register_callback(delete_puti);
    task->destructor = delete_puti;
    task->display = TaskScheduler::source_no_op;
}
void TitleScene::exec_puti(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_puti);
    if (!scene.state_.sequence_work) throw std::runtime_error("native puti has no sequence");
    scene.require_complete(title_menu::update_puti(task,
        *scene.state_.sequence_work, scene), "native_puti");
}
void TitleScene::delete_puti(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_puti_destructor");
}
TaskHandle TitleScene::create_title(const title_menu::ModeSelection &transition) {
    return title_menu::create_title(state_, transition, *this);
}
TaskHandle TitleScene::create_title_task() { return create_task(8, 3, exec_title); }
std::shared_ptr<title_menu::TitleWork> TitleScene::allocate_title_work(const TaskHandle &task) {
    return allocate_menu_work<title_menu::TitleWork>(task, 60);
}
void TitleScene::install_title_callbacks(const TaskHandle &task) {
    install_menu_callbacks(title_menu::MenuMode::Title, task);
}
void TitleScene::exec_title(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_title);
    auto work = std::static_pointer_cast<title_menu::TitleWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native title task has no work");
    title_menu::TitleTiming timing{scene.session_.loop_.elapsed_ticks,
        scene.session_.secondary_loop_count_, scene.session_.title_logic_ticks_,
        scene.session_.text_language_};
    title_menu::update_title(*work, scene.state_, timing, scene, scene.display_.arithmetic());
}
void TitleScene::display_title(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_title);
    auto work = std::static_pointer_cast<title_menu::TitleWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native title task has no work");
    title_menu::draw_title(*work, scene);
}
void TitleScene::initialize_title(title_menu::TitleWork &work) {
    if (!state_.title_work) throw std::runtime_error("native title task has no canonical work");
    title_menu::TitleInitState shared{session_.text_language_, session_.bootstrap_.menu.title_count,
        session_.bootstrap_.menu.flags[3], *state_.title_work};
    title_menu::initialize_title(work, shared, *this);
}
void TitleScene::set_stage(std::uint8_t stage, std::uint8_t act) {
    session_.set_stage_number(stage, act);
}
void TitleScene::initialize_character_lighting() {
    if (!session_.lighting_) throw flow::UnrecoveredGameFlowCall("native_stage_lighting_tables");
    lighting::TimeServices time{
        [this] {
            const auto *sequence = session_.bootstrap_.current_sequence;
            return lighting::sequence_time(sequence ? &sequence->time : nullptr,
                                            session_.logo_.demo_event());
        },
        []() -> std::int32_t {
            throw flow::UnrecoveredGameFlowCall("native_hotel_lighting_state");
        }};
    lighting::update_character_light({session_.stage_, session_.act_}, *session_.lighting_, time);
}
void TitleScene::load_mode_textures(title_menu::MenuMode mode) {
    if (!state_.sequence_work)
        throw std::runtime_error("native title mode loading has no sequence");
    title_menu::load_mode_textures(mode, textures_, assets_.mode_textures,
                                   *state_.sequence_work, *this);
}
void TitleScene::load_japanese_title_textures() { session_.load_japanese_title_textures(); }
void TitleScene::prepare_menu_voice() {
    menu_dialog::restore_saved_character_cursor(session_.save_.last_character,
                                                session_.bootstrap_.menu.flags[2]);
}
void TitleScene::set_title_menu_previous() {
    if (!state_.factories.main_menu_work)
        throw std::runtime_error("native main menu task has no canonical work");
    title_menu::set_main_menu_previous(*state_.factories.main_menu_work);
}
std::uint32_t TitleScene::fade_out_color(float progress) {
    return title_menu::fade_out_color(progress, *this, display_.arithmetic());
}
std::uint32_t TitleScene::fade_in_color(float progress) {
    return title_menu::fade_in_color(progress, *this, display_.arithmetic());
}
void TitleScene::disable_alpha_test() { display_.set_alpha_test_enabled(false); }
void TitleScene::update_title_input(title_menu::TitleWork &work) {
    if (!state_.sequence_work || !session_.audio_configuration_)
        throw std::runtime_error("native title input has no canonical sequence/audio owners");
    title_menu::TitleInputState shared{session_.secondary_loop_count_, session_.title_logic_ticks_,
        session_.timing_.pending_logic_ticks, session_.bootstrap_.menu.flags[3],
        session_.saving_disabled_, session_.save_word_after_selection_,
        session_.save_word_before_selection_, session_.quit_, textures_.ready,
        session_.audio_configuration_->music_enabled, session_.demo_wait_ticks_};
    title_menu::update_title_input(work, *state_.sequence_work, shared, *this);
}
std::int32_t TitleScene::key_pressed(std::int32_t key) {
    if (!input_.key_pressed) throw flow::UnrecoveredGameFlowCall("native_title_key_pressed");
    return input_.key_pressed(key);
}
std::uint32_t TitleScene::pressed_buttons() {
    if (!input_.pressed_buttons) throw flow::UnrecoveredGameFlowCall("native_title_pressed_buttons");
    return input_.pressed_buttons();
}
std::int32_t TitleScene::audio_ready() {
    return static_cast<std::int32_t>(session_.sound_.stream_initialized);
}
std::uint32_t TitleScene::music_state() {
    return music::query_music_state(session_.tracks_);
}
music::TaskServices TitleScene::music_task_services() {
    return {
        [this](const TaskHandle &task) {
            if (!session_.logo_.task_scheduler().destroy_task(task))
                throw std::runtime_error("native menu music task destruction failed");
        },
        [this] { return create_task(0, 1, load_music_manager); },
        [this](const TaskHandle &task) {
            register_callback(update_music_manager);
            register_callback(delete_music_manager);
            task->exec = update_music_manager;
            task->destructor = delete_music_manager;
        },
        [this] { return create_task(0, 4, update_music_request); },
        [this](const TaskHandle &task) {
            return allocate_menu_work<music::RequestWork>(task, 8);
        },
        [this] { stop_music(); }
    };
}
void TitleScene::request_menu_music(std::int32_t index) {
    if (!session_.music_catalog_)
        throw flow::UnrecoveredGameFlowCall("native_menu_music_catalog");
    auto services = music_task_services();
    music::request_menu_music(index, *session_.music_catalog_, sound_, session_.tracks_, services);
}
menu_voice::TaskServices TitleScene::voice_task_services() {
    return {
        [this] { return create_task(0, 4, update_voice_request); },
        [this](const TaskHandle &task) { return allocate_menu_work<menu_voice::RequestWork>(task, 8); },
        [this](const TaskHandle &task) {
            if (!session_.logo_.task_scheduler().destroy_task(task))
                throw std::runtime_error("native menu voice request destruction failed");
        }
    };
}
void TitleScene::request_menu_voice(std::int32_t index) {
    auto services = voice_task_services();
    // The canonical source slot is named volume_task in the earlier menu recovery.
    menu_voice::request_character_voice(index, session_.bootstrap_.menu.flags[2],
        session_.menu_voices_, sound_.volume_task, services);
}
void TitleScene::update_voice_request(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, update_voice_request);
    auto services = scene.voice_task_services();
    menu_voice::update_request(task, scene.sound_.volume_task, scene.session_.tracks_,
        scene.session_.voice_requested_enabled_, scene.textures_.ready, services);
}
void TitleScene::initialize_file_select(title_menu::FileSelectWork &work) {
    file_select::InitState state{
        session_.bootstrap_.menu.selected_file, session_.bootstrap_.card_state,
        session_.bootstrap_.card_operation, session_.save_word_before_selection_,
        session_.first_visible_file_, session_.saving_disabled_, session_.selected_card_index_,
        session_.displayed_save_index_};
    file_select::initialize(work, state, *this);
}
std::uint8_t TitleScene::count_save_files() {
    if (!session_.save_files_)
        throw flow::UnrecoveredGameFlowCall("native_save_file_provider");
    return file_select::count_save_files(session_.list_data_, session_.save_, *session_.save_files_);
}
void TitleScene::load_selected_save() {
    if (!session_.save_files_)
        throw flow::UnrecoveredGameFlowCall("native_save_file_provider");
    file_select::LoadState state{session_.saving_disabled_, session_.bootstrap_.card_state,
        session_.bootstrap_.save_name, session_.list_data_, session_.save_, session_.rings_};
    file_select::load_selected_save(state, *session_.save_files_);
}
void TitleScene::load_texture(texture_lifetime::List &list) {
    if (!session_.regular_texture_state_)
        throw flow::UnrecoveredGameFlowCall("native_font_texture_loader");
    auto services = session_.regular_texture_services();
    regular_textures::load_individual(list, *session_.regular_texture_state_, services);
}
void TitleScene::load_font() {
    if (!session_.font_assets_)
        throw flow::UnrecoveredGameFlowCall("native_ascii_font_assets");
    font::load(&session_.font_info_, session_.font_state_, *this);
}
void TitleScene::copy_font() { font::copy(session_.font_info_, session_.font_state_); }
void TitleScene::apply_save() { session_.load_save(); }
void TitleScene::release_font() {
    font::release(session_.font_info_, [this](auto &list) { display_.release_texture_list(list); });
}
void TitleScene::free_save_list() { file_select::free_save_list(session_.list_data_); }
void TitleScene::update_controls(title_menu::FileSelectWork &work) {
    file_select::ControlState state{
        session_.bootstrap_.card_operation, session_.bootstrap_.menu.selected_file,
        session_.displayed_save_index_, session_.bootstrap_.card_state,
        session_.first_visible_file_, session_.selected_card_index_,
        session_.file_operation_pending_, session_.flow_.alert_blocked, textures_.ready,
        session_.secondary_loop_count_, session_.bootstrap_.character,
        session_.new_file_requested_};
    auto &fp = display_.arithmetic();
    const NinjaMath math(ninja_sine_table(), fp);
    file_select::update_controls(work, state, *this, math, fp);
}
void TitleScene::update_confirmation(title_menu::FileSelectWork &work,
                                      file_select::ControlState &state) {
    menu_dialog::ConfirmationState confirmation{state, session_.file_selection_backup_,
        session_.file_select_fade_in_marker_, session_.save_.last_character,
        session_.bootstrap_.menu.flags[2], session_.list_data_, session_.save_name_.value};
    menu_dialog::confirm_selection(work, confirmation, *this);
}
void TitleScene::request_dialog(std::int32_t index) {
    if (!session_.menu_dialog_assets_)
        throw flow::UnrecoveredGameFlowCall("native_menu_dialog_assets");
    menu_dialog::request(index, session_.menu_dialog_assets_->definitions,
                          {state_.dialog_task}, *this);
}
void TitleScene::prepare_new_save() {
    throw flow::UnrecoveredGameFlowCall("native_file_select_prepare_new_save");
}
void TitleScene::request_save() {
    throw flow::UnrecoveredGameFlowCall("native_file_select_request_save");
}
std::int32_t TitleScene::close_warning() {
    throw flow::UnrecoveredGameFlowCall("native_file_select_close_warning");
}
void TitleScene::play_navigation_sound() {
    sound_effects::Queue queue{session_.sound_, session_.bank_channels_, session_.sound_effects_};
    sound_effects::play_one_shot(1, nullptr, 0, 0, queue);
}
void TitleScene::play_cancel_sound() {
    sound_effects::Queue queue{session_.sound_, session_.bank_channels_, session_.sound_effects_};
    sound_effects::play_one_shot(3, nullptr, 0, 0, queue);
}
void TitleScene::load_music_manager(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, load_music_manager);
    auto services = scene.music_task_services();
    music::initialize_manager_task(task, scene.session_.sound_manager_, services);
}
void TitleScene::delete_music_manager(TaskScheduler &scheduler, const TaskHandle &) {
    auto &scene = callback_scene(scheduler, delete_music_manager);
    title_menu::delete_sound_manager(scene.session_.sound_manager_, scene.session_.tracks_, scene);
}
void TitleScene::update_music_request(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, update_music_request);
    auto services = scene.music_task_services();
    music::update_request(task, scene.sound_, scene.session_.tracks_,
        scene.session_.music_requested_enabled_, scene.textures_.ready, services);
}
void TitleScene::update_music_manager(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, update_music_manager);
    auto &session = scene.session_;
    if (!session.music_catalog_ || !session.audio_configuration_)
        throw flow::UnrecoveredGameFlowCall("native_menu_music_catalog_configuration");
    auto &configuration = *session.audio_configuration_;
    music::ManagerState state{session.tracks_, session.voice_timing_, configuration.music_enabled,
        configuration.voice_enabled, session.voice_language_};
    music::ManagerServices services{
        [&session] { return session.pcm_.initialized; },
        [&session, &configuration](std::string_view path, std::int32_t loop) {
            return music::play_music_file(path, loop, session.media_,
                configuration.music_attenuation, session.music_media_services_);
        },
        [&session, &configuration] {
            music::restart_music(session.media_, configuration.music_attenuation,
                session.music_media_services_);
        },
        [&session] { return music::music_status(session.media_); },
        [&session, &configuration](std::string_view path) {
            return voice_playback::play_file(path, session.media_, configuration.voice_attenuation,
                                            session.voice_services_);
        },
        [&session] { return music::voice_status(session.media_, session.music_media_services_); },
        [&session] {
            if (!session.music_milliseconds_)
                throw flow::UnrecoveredGameFlowCall("native_music_millisecond_clock");
            return session.music_milliseconds_();
        },
        [&scheduler](const TaskHandle &parent) {
            if (!scheduler.run_children(parent))
                throw std::runtime_error("native music manager child dispatch failed");
        }
    };
    music::update_manager(task, state, *session.music_catalog_, session.cd_sound_prefix_, services);
}
void TitleScene::stop_menu_music() {
    auto services = music_task_services();
    music::stop_menu_music(sound_, services);
}
void TitleScene::transition_menu(title_menu::MenuMode mode) {
    // Keep the sequence snapshot across sound-task destruction, as the source does.
    const auto sequence = state_.sequence_work;
    if (!sequence) throw std::runtime_error("native title transition has no sequence work");
    title_menu::request_menu_transition(*sequence, mode, *this);
}
void TitleScene::play_title_confirm_sound() {
    sound_effects::Queue queue{session_.sound_, session_.bank_channels_, session_.sound_effects_};
    sound_effects::play_title_confirmation(queue);
}
void TitleScene::release_mode_textures(title_menu::MenuMode) {
    const auto sequence = state_.sequence_work;
    if (!sequence)
        throw std::runtime_error("native title texture release has no sequence work");
    auto textures = texture_release_state();
    auto services = texture_release_services();
    menu_resource_lifetime::release_mode_textures(*sequence, textures, services);
}
void TitleScene::release_japanese_title_textures() {
    auto *list = session_.japanese_title_texture_.textures;
    if (!list)
        throw std::runtime_error("native Japanese title texture owner is unavailable");
    auto services = texture_release_services();
    menu_resource_lifetime::release_japanese_title(*list, services);
}
menu_resource_lifetime::State TitleScene::texture_release_state() {
    return {textures_, session_.text_language_, session_.menu_texture_catalog_, assets_.mode_textures};
}
menu_resource_lifetime::Services TitleScene::texture_release_services() {
    return {
        [this](texture_lifetime::List *list) { display_.release_texture_list(list); },
        [this](const TaskHandle &task) {
            if (!session_.logo_.task_scheduler().destroy_task(task))
                throw std::runtime_error("native menu resource task destruction failed");
        },
        [this](const TaskHandle &task) {
            if (!session_.logo_.task_scheduler().free_task(task))
                throw std::runtime_error("native menu resource deferred destruction failed");
        }
    };
}
void TitleScene::default_blending() { session_.logo_.default_blending(); }
void TitleScene::set_base_color(std::uint32_t color) { session_.logo_.set_base_color(color); }
void TitleScene::draw_title_background(title_menu::TitleWork &work) {
    const auto frame = display_.frame_state();
    const title_menu::BackgroundView view{frame.horizontal_scale, frame.vertical_scale,
        session_.loop_.elapsed_ticks, session_.secondary_loop_count_, session_.text_language_};
    title_menu::draw_title_background(work, view, assets_.background_layout,
                                       *this, display_.arithmetic());
}
void TitleScene::set_material(std::uint32_t color) { session_.logo_.set_material(color); }
void TitleScene::draw_scaled_texture(std::uint32_t index, float x, float y, float depth,
                                      float scale_x, float scale_y) {
    session_.logo_.draw_scaled_texture(index, x, y, depth, scale_x, scale_y);
}
void TitleScene::select_title_texture(title_menu::TitleTextureList list) {
    session_.select_title_texture(list);
}
std::int32_t TitleScene::run() {
    auto owners = shared();
    return title_menu::run_title_menu(state_, owners, session_.flow_.pause_state, *this);
}
std::int32_t TitleScene::close() { return title_menu::close_title_menu(state_, *this); }
void TitleScene::set_depth_compare(std::int32_t mode) {
    display_.set_depth_compare_mode(static_cast<std::uint32_t>(mode));
}
void TitleScene::poll_controls() {
    if (!input_.poll_controls) throw flow::UnrecoveredGameFlowCall("native_title_controller_read");
    input_.poll_controls();
}
void TitleScene::enable_control() {
    if (!input_.enable_control) throw flow::UnrecoveredGameFlowCall("native_title_pad_read_enable");
    input_.enable_control();
}
void TitleScene::run_tasks() {
    if (!session_.logo_.task_scheduler().run_game_tasks())
        throw std::runtime_error("native title exec traversal failed");
}
void TitleScene::disable_fog() { display_.disable_fog(); }
void TitleScene::display_tasks() {
    if (!session_.logo_.task_scheduler().display_game_tasks([this] { display_.reset_material(); }))
        throw std::runtime_error("native title display traversal failed");
}
std::int32_t TitleScene::soft_reset_check(std::int32_t mode) {
    if (!input_.soft_reset_check) throw flow::UnrecoveredGameFlowCall("native_title_soft_reset_check");
    return input_.soft_reset_check(mode);
}
void TitleScene::delete_all_tasks() {
    if (!session_.logo_.task_scheduler().delete_all_tasks())
        throw std::runtime_error("native title task deletion failed");
}
void TitleScene::free_regular_object_textures() { session_.free_regular_object_texlists(); }
void TitleScene::destroy_sequence_task(const TaskHandle &task) {
    if (!session_.logo_.task_scheduler().destroy_task(task))
        throw std::runtime_error("native title sequence destruction failed");
}
} // namespace sadx::native
