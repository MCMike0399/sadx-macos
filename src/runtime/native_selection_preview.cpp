#include "runtime/native_title_scene.hpp"
#include "runtime/native_session.hpp"
#include "runtime/native_display.hpp"
#include "player/player_initialization.hpp"

namespace sadx::native {
// NOTSA: adapts the source preview to the actual scheduler, primary/motion owners
// and imported texture declarations. It owns no actor pose or player motion.
struct TitleScene::PreviewServices final : selection_preview::Calls {
    explicit PreviewServices(TitleScene &owner) : scene(owner) {
        for (std::size_t character = 0; character < lists.size(); ++character) {
            const auto &archives = scene.session_.texture_assets_[character];
            auto &textures = scene.session_.character_textures_[character];
            if (archives.size() != textures.size())
                throw flow::UnrecoveredGameFlowCall("native_character_texture_declarations");
            for (std::size_t index = 0; index < archives.size(); ++index)
                lists[character].push_back({archives[index].archive, &textures[index]});
            list_views[character] = &lists[character];
        }
    }
    TitleScene &scene;
    std::array<std::vector<regular_textures::Binding>, 9> lists;
    std::array<const std::vector<regular_textures::Binding> *, 9> list_views{};
    const CharacterSelectionAssets &assets() {
        if (!scene.session_.character_selection_assets_)
            throw flow::UnrecoveredGameFlowCall("native_character_selection_assets");
        return *scene.session_.character_selection_assets_;
    }
    selection_preview::Assets declarations() {
        return {assets().characters, assets().positions, assets().sonic_aim_y};
    }
    selection_preview::View view() {
        return {scene.display_.frame_state().horizontal_scale,
                scene.session_.bootstrap_.menu.player_character, scene.display_.saved_fov()};
    }
    selection_preview::TextureView textures() {
        return {list_views, scene.session_.character_textures_loaded_};
    }
    TaskScheduler &scheduler() { return scene.session_.logo_.task_scheduler(); }
    TaskHandle create_preview_task(std::uint32_t flags, std::int32_t level) override {
        return scene.create_task(flags, level, exec_preview);
    }
    std::shared_ptr<selection_preview::Work> allocate_work(const TaskHandle &task) override {
        return scene.allocate_menu_work<selection_preview::Work>(task, 56);
    }
    void install_callbacks(const TaskHandle &task) override {
        scene.register_callback(display_preview);
        scene.register_callback(delete_preview);
        task->display = display_preview;
        task->destructor = delete_preview;
    }
    void select_palette(std::int32_t value) override {
        if (!scene.session_.lighting_)
            throw flow::UnrecoveredGameFlowCall("native_stage_lighting_tables");
        lighting::TimeServices time{
            [this] {
                const auto *sequence = scene.session_.bootstrap_.current_sequence;
                return lighting::sequence_time(sequence ? &sequence->time : nullptr,
                                               scene.session_.logo_.demo_event());
            },
            []() -> std::int32_t {
                throw flow::UnrecoveredGameFlowCall("native_hotel_lighting_state");
            }};
        scene.display_.set_lighting_palette(value, *scene.session_.lighting_,
                                            {scene.session_.stage_, scene.session_.act_}, time);
    }
    TaskHandle create_camera(std::uint32_t flags, std::int32_t level) override {
        const auto task = scene.create_task(flags, level, exec_selection_camera);
        if (task)
            player_initialization::establish_zeroed_primary(scheduler(), task);
        return task;
    }
    selection_preview::Pose camera_pose(const TaskHandle &task) override {
        const auto primary = player_initialization::bind_primary_view(task);
        return {primary.position, primary.rotation};
    }
    TaskHandle create_actor(selection_preview::ActorKind kind, std::uint32_t flags,
                            std::int32_t level) override {
        TaskCallback callback = nullptr;
        switch (kind) {
        case selection_preview::ActorKind::Sonic:
            callback = exec_selection_sonic;
            break;
        case selection_preview::ActorKind::Miles:
            callback = exec_selection_miles;
            break;
        case selection_preview::ActorKind::Knuckles:
            callback = exec_selection_knuckles;
            break;
        case selection_preview::ActorKind::Amy:
            callback = exec_selection_amy;
            break;
        case selection_preview::ActorKind::Big:
            callback = exec_selection_big;
            break;
        case selection_preview::ActorKind::Gamma:
            callback = exec_selection_gamma;
            break;
        case selection_preview::ActorKind::QuestionMark:
            callback = exec_selection_question;
            break;
        }
        if (!callback)
            throw flow::UnrecoveredGameFlowCall("native_character_preview_actor_kind");
        const auto task = scene.create_task(flags, level, callback);
        if (task) {
            player_initialization::establish_zeroed_primary(scheduler(), task);
            player_initialization::establish_zeroed_motion(scheduler(), task);
        }
        return task;
    }
    selection_preview::Actor actor(const TaskHandle &task) override {
        const auto primary = player_initialization::bind_primary_view(task);
        const auto motion = player_initialization::bind_existing_motion(task);
        return {{primary.position, primary.rotation}, primary.id, primary.player_index,
                primary.character, motion->target_yaw};
    }
    void load_character_textures(std::uint16_t character) override {
        selection_preview::load_textures(character, textures(), *this);
    }
    void load_named_texture(std::string_view name, regular_textures::List &list) override {
        if (!scene.session_.regular_texture_state_)
            throw flow::UnrecoveredGameFlowCall("native_regular_texture_declarations");
        auto services = scene.session_.regular_texture_services();
        regular_textures::load_archive(name, &list, *scene.session_.regular_texture_state_, services);
    }
    void load_individual_texture(regular_textures::List &list) override { scene.load_texture(list); }
    void release_texture(regular_textures::List &list) override { scene.display_.release_texture_list(list); }
    void release_actor_textures(std::uint16_t character) override {
        selection_preview::release_textures(character, textures(), *this);
    }
    void destroy_task(const TaskHandle &task) override {
        if (!scheduler().destroy_task(task))
            throw flow::UnrecoveredGameFlowCall("native_character_preview_task_destruction");
    }
    void set_perspective(std::int32_t angle) override {
        scene.display_.set_perspective(angle, scene.session_.flow_.pause_state);
    }
    void update_actor_actions(selection_preview::Work &) override {
        throw flow::UnrecoveredGameFlowCall("native_character_preview_event_actions");
    }
};
void TitleScene::create_character_preview() {
    PreviewServices services(*this);
    selection_preview::create(preview_, services.declarations(), services.view(), services,
                               display_.arithmetic());
}
void TitleScene::remove_character_preview() {
    PreviewServices services(*this);
    selection_preview::remove(preview_, services);
}
void TitleScene::exec_preview(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_preview);
    PreviewServices services(scene);
    selection_preview::update(task, scene.preview_, services.declarations(), services.view(), services);
}
void TitleScene::display_preview(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, display_preview);
    PreviewServices services(scene);
    selection_preview::draw(task, services.view(), services);
}
void TitleScene::delete_preview(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, delete_preview);
    PreviewServices services(scene);
    selection_preview::destroy(task, services.declarations(), services);
}
void TitleScene::exec_selection_camera(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_selection_camera);
    const auto primary = player_initialization::bind_primary_view(task);
    scene.display_.update_selection_camera({primary.rotation, primary.position});
}
// These are the source task callback boundaries, reached by the real scheduler.
// No actor update or render result is fabricated while its implementation is missing.
void TitleScene::exec_selection_sonic(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_sonic_player_controller");
}
void TitleScene::exec_selection_miles(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_miles_player_controller");
}
void TitleScene::exec_selection_knuckles(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_knuckles_player_controller");
}
void TitleScene::exec_selection_amy(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_amy_player_controller");
}
void TitleScene::exec_selection_big(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_big_player_controller");
}
void TitleScene::exec_selection_gamma(TaskScheduler &, const TaskHandle &) {
    throw flow::UnrecoveredGameFlowCall("native_gamma_player_controller");
}
void TitleScene::exec_selection_question(TaskScheduler &scheduler, const TaskHandle &task) {
    auto &scene = callback_scene(scheduler, exec_selection_question);
    if (!scene.session_.character_selection_assets_)
        throw flow::UnrecoveredGameFlowCall("native_character_selection_assets");
    const auto primary = player_initialization::bind_primary_view(task);
    const auto &assets = *scene.session_.character_selection_assets_;
    if (!scene.session_.lighting_)
        throw flow::UnrecoveredGameFlowCall("native_stage_lighting_tables");
    const ModelDrawEnvironment environment{
        *scene.session_.character_pack_, scene.session_.character_scene_,
        scene.session_.bootstrap_.material_callback, *scene.session_.lighting_};
    scene.display_.draw_selection_question({primary.rotation, primary.position},
        scene.session_.secondary_loop_count_, {assets.question_object, assets.question_menu_texture},
        environment,
        [&scene](std::uint32_t texture) {
            scene.select_menu_texture(static_cast<file_select_display::MenuTexture>(texture));
        });
}
} // namespace sadx::native
