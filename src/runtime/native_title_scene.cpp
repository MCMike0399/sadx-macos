#include "runtime/native_title_scene.hpp"
#include "runtime/native_display.hpp"
#include "runtime/native_session.hpp"
#include "math/ninja_sine_table.hpp"

namespace sadx::native {
struct TitleScene::CallbackContext final : TaskCallbackContext {
    explicit CallbackContext(TitleScene &owner) : scene(&owner) {}
    TitleScene *scene;
};
TitleScene::TitleScene(Session &session, Display &display, const TitleAssets &assets)
    : session_(session), display_(display), assets_(assets),
      callback_context_(std::make_shared<CallbackContext>(*this)) {}
TitleScene::~TitleScene() { callback_context_->scene = nullptr; }

TitleScene &TitleScene::callback_scene(TaskScheduler &scheduler, TaskCallback callback) {
    const auto context =
        std::dynamic_pointer_cast<CallbackContext>(scheduler.callback_context(callback));
    if (!context || !context->scene)
        throw flow::UnrecoveredGameFlowCall("native_title_callback_owner");
    return *context->scene;
}
void TitleScene::register_callback(TaskCallback callback) {
    if (!session_.logo_.task_scheduler().register_callback_context(callback, callback_context_))
        throw std::runtime_error("native title callback registration failed");
}
TaskHandle TitleScene::create_task(std::uint32_t flags, int level, TaskCallback callback) {
    register_callback(callback);
    const auto created = session_.logo_.task_scheduler().create_element(flags, level, callback);
    if (created.failure == TaskFailure::source_pool_exhausted)
        return {};
    if (!created)
        throw std::runtime_error("native title task creation failed");
    return created.task;
}
bool TitleScene::allocate_auxiliary(const TaskHandle &task, std::uint32_t size, bool zero) {
    auto &arena = *session_.logo_.task_scheduler().work_arena();
    const auto allocation = arena.allocate(size);
    if (allocation && zero && !arena.fill(allocation.work, 0, 0, size))
        throw std::runtime_error("native title auxiliary initialization failed");
    // The title factory overwrites the flag8 allocation without freeing it in the source.
    task->auxiliary_work = allocation.work;
    task->auxiliary_native_work.reset();
    return static_cast<bool>(allocation);
}
void TitleScene::update_sequence(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, update_sequence);
    auto work = std::static_pointer_cast<title_menu::SequenceWork>(task->auxiliary_native_work);
    if (!work || !scene.state_.sequence_work)
        throw std::runtime_error("native title sequence has no work");
    auto startup = scene.session_.logo_.startup_state();
    title_menu::update_sequence(*work, *scene.state_.sequence_work, startup.demo_mode, scene);
}
void TitleScene::update_child(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, update_child);
    auto work = std::static_pointer_cast<title_menu::SequenceChildWork>(task->auxiliary_native_work);
    if (!work)
        throw std::runtime_error("native title sequence child has no work");
    title_menu::update_sequence_child(task, *work, scene);
}
void TitleScene::delete_sequence(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, delete_sequence);
    const auto work = std::static_pointer_cast<title_menu::SequenceWork>(task->auxiliary_native_work);
    if (!work)
        throw std::runtime_error("native title sequence destructor has no task-owned work");
    auto startup = scene.session_.logo_.startup_state();
    auto textures = scene.texture_release_state();
    auto services = scene.texture_release_services();
    menu_resource_lifetime::destroy_sequence(*work, startup.demo_mode, scene.state_, textures, services);
}
void TitleScene::update_dialog(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, update_dialog);
    auto work = std::static_pointer_cast<title_menu::DialogWork>(task->auxiliary_native_work);
    if (!work)
        throw std::runtime_error("native title dialog has no work");
    if (!scene.session_.menu_dialog_assets_)
        throw flow::UnrecoveredGameFlowCall("native_menu_dialog_assets");
    auto &fp = scene.display_.arithmetic();
    const NinjaMath math(ninja_sine_table(), fp);
    menu_dialog::update(*work, {scene.state_.dialog_task},
        scene.session_.menu_dialog_assets_->tables, scene.session_.secondary_loop_count_,
        scene, math, fp);
}
void TitleScene::display_dialog(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_dialog);
    auto work = std::static_pointer_cast<title_menu::DialogWork>(task->auxiliary_native_work);
    if (!work) throw std::runtime_error("native title dialog has no work");
    const auto frame = scene.display_.frame_state();
    menu_dialog::draw(*work, {scene.state_.dialog_task},
        {frame.horizontal_scale, frame.vertical_scale, scene.session_.dialog_window_after_content_},
        scene, scene.display_.arithmetic());
}

title_menu::SharedState TitleScene::shared() {
    return {font_loaded_, session_.bootstrap_.menu.flags[2],
            session_.bootstrap_.menu.character_cursor, session_.secondary_loop_count_};
}
void TitleScene::initialize() {
    auto owners = shared();
    const auto &flags = session_.bootstrap_.menu.flags;
    title_menu::initialize_title_menu(state_, owners, {flags[0], flags[1]}, *this);
}
void TitleScene::stop_music() {
    title_menu::close_sound_manager(session_.sound_manager_, *this);
}
void TitleScene::initialize_menu_sound() { title_menu::initialize_menu_sound(sound_, *this); }
void TitleScene::select_sound_bank(std::int32_t index) { session_.select_sound_bank(index); }
void TitleScene::destroy_sound_manager(const TaskHandle &task) {
    if (!session_.logo_.task_scheduler().destroy_task(task))
        throw std::runtime_error("native sound manager destruction failed");
}
void TitleScene::close_media(std::uint32_t mode) {
    if (!audio::close_media(session_.media_, mode))
        throw std::runtime_error("native title media cleanup failed");
}
void TitleScene::set_frame_rate(std::int32_t render, std::int32_t logic) {
    session_.set_frame_rate_mode(render, logic);
}
void TitleScene::set_wait_vsync(std::int32_t count) { session_.set_wait_vsync_count(count); }
void TitleScene::initialize_heap() { session_.logo_.initialize_heap(); }
void TitleScene::initialize_tasks() { session_.logo_.initialize_tasks(); }
void TitleScene::set_background(std::uint32_t top, std::uint32_t middle, std::uint32_t bottom) {
    display_.output().set_background(top, middle, bottom);
}
void TitleScene::set_fade_color(std::uint32_t color) { session_.logo_.set_fade_color(color); }
void TitleScene::release_loading_font() {
    auto owners = shared();
    title_menu::release_loading_font(owners, *this);
}
void TitleScene::free_character_textures() { session_.free_character_texlists(); }
void TitleScene::release_all_textures() { session_.release_all_textures(); }
void TitleScene::initialize_texture_drawing() { session_.logo_.initialize_texture_drawing(); }
void TitleScene::initialize_render_state() {
    if (!session_.lighting_)
        throw flow::UnrecoveredGameFlowCall("native_stage_lighting_tables");
    lighting::TimeServices time{
        [this] {
            const auto *sequence = session_.bootstrap_.current_sequence;
            return lighting::sequence_time(sequence ? &sequence->time : nullptr,
                                            session_.logo_.demo_event());
        },
        []() -> std::int32_t {
            throw flow::UnrecoveredGameFlowCall("native_hotel_lighting_state");
        }};
    lighting::initialize_render_state({session_.stage_, session_.act_}, *session_.lighting_, time);
}
void TitleScene::set_lighting(bool enabled) { display_.set_lighting(enabled); }
void TitleScene::set_view_angle(std::int32_t angle) {
    display_.set_view_angle(angle, session_.flow_.pause_state);
}
void TitleScene::set_clip(float near_distance, float far_distance) {
    display_.set_clip_range(near_distance, far_distance);
}
void TitleScene::set_camera() { display_.set_camera(); }
void TitleScene::load_regular_object_textures(std::int32_t group) {
    session_.load_regular_object_textures(static_cast<std::uint32_t>(group));
}
void TitleScene::load_player_motion(std::int32_t character) {
    bootstrap::load_player_motion_data(static_cast<std::uint16_t>(character),
                                      session_.bootstrap_, session_.actions_);
}
void TitleScene::load_loading_font() {
    throw flow::UnrecoveredGameFlowCall("native_loading_font_loading");
}
void TitleScene::copy_loading_font() {
    throw flow::UnrecoveredGameFlowCall("native_loading_font_copy");
}
void TitleScene::release_font_resource() {
    throw flow::UnrecoveredGameFlowCall("native_loading_font_release");
}
void TitleScene::set_font_color(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {
    throw flow::UnrecoveredGameFlowCall("native_loading_font_color");
}
void TitleScene::draw_text(std::string_view, float, float, float) {
    throw flow::UnrecoveredGameFlowCall("native_title_text_drawing");
}
TaskHandle TitleScene::create_sequence_task() {
    return create_task(0, 3, update_sequence);
}
std::shared_ptr<title_menu::SequenceWork> TitleScene::allocate_sequence_work(const TaskHandle &task) {
    if (!allocate_auxiliary(task, 36, true))
        return {};
    auto work = std::make_shared<title_menu::SequenceWork>();
    task->auxiliary_native_work = work;
    return work;
}
void TitleScene::initialize_menu_data() {
    title_menu::initialize_menu_data(textures_, assets_.initial_textures, *this);
}
void TitleScene::reset_selected_trial_level() {
    title_menu::reset_selected_trial_level(state_.trial);
}
const title_menu::SequenceData &TitleScene::sequence_data(std::int32_t entry) {
    return assets_.sequences.at(assets_.entries.at(static_cast<std::size_t>(entry)));
}
TaskHandle TitleScene::create_sequence_child() {
    return create_task(0, 3, update_child);
}
std::shared_ptr<title_menu::SequenceChildWork> TitleScene::allocate_child_work(const TaskHandle &task) {
    if (!allocate_auxiliary(task, 8, true))
        return {};
    auto work = std::make_shared<title_menu::SequenceChildWork>();
    task->auxiliary_native_work = work;
    return work;
}
void TitleScene::install_child_callback(const TaskHandle &task) {
    task->exec = update_child;
}
TaskHandle TitleScene::initialize_dialog() {
    return title_menu::create_dialog(state_, session_.secondary_loop_count_, *this);
}
void TitleScene::install_sequence_callbacks(const TaskHandle &task) {
    register_callback(delete_sequence);
    task->exec = update_sequence;
    task->destructor = delete_sequence;
}
void TitleScene::sequence_already_active() {
    throw flow::UnrecoveredGameFlowCall("title_sequence_already_active");
}
void TitleScene::load_menu_texture(title_menu::MenuTextureId id) {
    session_.load_menu_texture(id.value);
}
TaskHandle TitleScene::create_dialog_task() { return create_task(0, 5, update_dialog); }
std::shared_ptr<title_menu::DialogWork> TitleScene::allocate_dialog_work(const TaskHandle &task) {
    if (!allocate_auxiliary(task, 68, false))
        return {};
    auto work = std::make_shared<title_menu::DialogWork>();
    task->auxiliary_native_work = work;
    return work;
}
void TitleScene::install_dialog_callbacks(const TaskHandle &task) {
    register_callback(display_dialog);
    task->exec = update_dialog;
    task->display = display_dialog;
}
void TitleScene::set_entrance(std::int32_t) {
    throw flow::UnrecoveredGameFlowCall("native_title_entrance_selection");
}
void TitleScene::release_menu_textures() {
    auto textures = texture_release_state();
    auto services = texture_release_services();
    menu_resource_lifetime::release_all(textures, services);
}
void TitleScene::finalize_selection() {
    throw flow::UnrecoveredGameFlowCall("native_title_selection_finalization");
}
void TitleScene::create_menu_objects(const title_menu::SequenceData *data) {
    if (!data) throw std::runtime_error("native title sequence has no imported data");
    title_menu::create_menu_objects(*data, session_.timing_.pending_logic_ticks, *this);
}
void TitleScene::destroy_sequence_child(const TaskHandle &task) {
    if (!session_.logo_.task_scheduler().destroy_task(task))
        throw std::runtime_error("native title child destruction failed");
}
} // namespace sadx::native
