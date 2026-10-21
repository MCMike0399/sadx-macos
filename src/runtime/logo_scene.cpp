#include "runtime/logo_scene.hpp"
#include <stdexcept>

namespace sadx::native {
LogoScene::LogoScene(LogoOutput &output, frame::ModeState &mode,
                     frame::RendererState &renderer, save::Data &save,
                     std::int16_t &rings, LogoFrame frame, X87Context &fp)
    : output_(output), frame_mode_(mode), renderer_(renderer), save_(save), rings_(rings),
      frame_(frame), arithmetic_(fp) {}

void LogoScene::initialize() {
    logo::StartupState startup{state_, demo_mode_, demo_event_, demo_reset_};
    logo::initialize_sega(startup, *this);
}
void LogoScene::load() { logo::load_sega_textures(state_, *this); }
logo::Completion LogoScene::advance() { return logo::advance(state_, *this); }

void LogoScene::initialize_heap() {
    // InitHeap resets the source arena; old task resources remain alive until their
    // scheduler is replaced by the following InitMasterObjectArray call.
    arena_ = std::make_shared<TaskWorkArena>();
}
void LogoScene::initialize_tasks() {
    task_auxiliary_.fill(0);
    tasks_ = std::make_unique<TaskScheduler>();
    const auto result = tasks_->initialize_source_pool(arena_, current_cooperative_thread_);
    if (!result)
        throw std::runtime_error("native logo task-pool initialization failed");
}
TaskScheduler &LogoScene::task_scheduler() {
    if (!tasks_)
        throw std::runtime_error("native task scheduler used before initialization");
    return *tasks_;
}
void LogoScene::set_character_selection(std::int32_t value) { character_selection_ = value; }
void LogoScene::set_background(std::uint32_t top, std::uint32_t middle, std::uint32_t bottom) {
    output_.set_background(top, middle, bottom);
}
void LogoScene::load_textures(std::string_view name) { output_.load_logo_archive(name); }
void LogoScene::set_frame_rate(std::int32_t render_ratio, std::int32_t logic_ratio) {
    frame::set_frame_rate_mode(render_ratio, logic_ratio, frame_mode_, renderer_);
}
void LogoScene::set_wait_vsync(std::int32_t count) {
    frame::set_frame_interval_multiplier(count, renderer_);
}
void LogoScene::set_fade_color(std::uint32_t color) { fade_color_ = color; }
void LogoScene::initialize_texture_drawing() { pvr::initialize(drawing_); }
void LogoScene::initialize_save_data() { save::initialize(save_, rings_); }
void LogoScene::run_objects() {
    if (!tasks_)
        throw std::runtime_error("native logo objects used before task initialization");
    const auto result = tasks_->run_game_tasks();
    if (!result)
        throw std::runtime_error("native logo task dispatch failed");
}
void LogoScene::display(std::uint8_t first_texture) {
    logo::DisplayState display{state_.frames, frame_.horizontal_scale, frame_.vertical_scale,
                               drawing_.vertices[0].color};
    logo::display_profile_640(first_texture, display, *this, arithmetic_, 640, 480);
}
void LogoScene::release_textures() { output_.release_logo_archive(); }
void LogoScene::select_logo_textures() { output_.select_logo_archive(); }
void LogoScene::set_material(std::uint32_t color) { pvr::set_material(color, drawing_); }
void LogoScene::default_blending() {
    pvr::default_blending([this](std::uint32_t side, std::uint32_t mode) {
        output_.set_blending(side, mode);
    });
}
void LogoScene::set_base_color(std::uint32_t color) { pvr::set_base_color(color, drawing_); }
void LogoScene::draw_texture(std::uint32_t index, float x, float y, float depth) {
    pvr::draw_texture(index, x, y, depth, drawing_, frame_.display_disabled, output_, arithmetic_);
}
void LogoScene::draw_scaled_texture(std::uint32_t index, float x, float y, float depth,
                                    float scale_x, float scale_y) {
    pvr::draw_scaled_texture(index, x, y, depth, scale_x, scale_y, drawing_,
                             frame_.display_disabled, output_, arithmetic_);
}
void LogoScene::draw_box(float width, float height, float depth, std::uint32_t color) {
    pvr::draw_box_fill_2d(0, 0, width, height, depth, color, frame_.display_disabled,
                         frame_.fog_emulation, frame_.fog_enabled, frame_.queued_depth_offset,
                         output_, arithmetic_);
}
} // namespace sadx::native
