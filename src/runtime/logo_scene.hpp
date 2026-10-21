#pragma once
#include "engine/game_frame_control.hpp"
#include "engine/pvr_draw.hpp"
#include "engine/task.hpp"
#include "game/logo_startup.hpp"
#include "engine/ui_scale_profile.hpp"
#include "game/save_data.hpp"
#include <memory>
#include <string_view>

namespace sadx::native {
// NOTSA: imported texture ownership and native device submission used by the logo scene.
struct LogoOutput : pvr::Calls {
    virtual void load_logo_archive(std::string_view name) = 0;
    virtual void select_logo_archive() = 0;
    virtual void release_logo_archive() = 0;
    virtual void set_background(std::uint32_t, std::uint32_t, std::uint32_t) = 0;
    virtual void set_blending(std::uint32_t side, std::uint32_t mode) = 0;
};
// NOTSA: live frame/device owners; the scene never substitutes fixed rendering flags.
struct LogoFrame {
    float &horizontal_scale;
    float &vertical_scale;
    std::int32_t &display_disabled;
    std::uint8_t &fog_emulation;
    std::uint8_t &fog_enabled;
    float &queued_depth_offset;
};

// NOTSA: session composition of the recovered logo task, native save owner and draw path.
// The application must enter this scene through its game-mode startup; this is not a boot shortcut.
class LogoScene final : private logo::StartupCalls,
                        private logo::Calls,
                        private logo::DisplayCalls {
  public:
    LogoScene(LogoOutput &, frame::ModeState &, frame::RendererState &, save::Data &,
              std::int16_t &rings, LogoFrame, X87Context &);
    void attach_ui_scaling(ui_scale::State &state) { scaling_ = &state; }
    void initialize();
    void load();
    logo::Completion advance();
    const logo::State &state() const { return state_; }
    const pvr::State &drawing_state() const { return drawing_; }
    pvr::State &drawing_state() { return drawing_; }
    std::uint32_t fade_color() const { return fade_color_; }
    const TaskScheduler *tasks() const { return tasks_.get(); }
    TaskScheduler &task_scheduler();
    const std::shared_ptr<void> &current_cooperative_thread() const {
        return current_cooperative_thread_;
    }
    std::int32_t demo_mode() const { return demo_mode_; }
    std::int32_t demo_event() const { return demo_event_; }
    logo::StartupState startup_state() {
        return {state_, demo_mode_, demo_event_, demo_reset_};
    }
    void initialize_texture_drawing() override;
    void set_material(std::uint32_t) override;
    void default_blending() override;
    void set_base_color(std::uint32_t) override;
    void draw_scaled_texture(std::uint32_t index, float x, float y, float depth,
                             float scale_x, float scale_y);
    void initialize_heap() override;
    void initialize_tasks() override;
    void set_fade_color(std::uint32_t) override;

  private:
    ui_scale::State *scaling_ = nullptr;
    LogoOutput &output_;
    frame::ModeState &frame_mode_;
    frame::RendererState &renderer_;
    save::Data &save_;
    std::int16_t &rings_;
    LogoFrame frame_;
    X87Context &arithmetic_;
    logo::State state_{logo::Mode::Inactive, 0, 0, 0};
    pvr::State drawing_{};
    std::int32_t demo_mode_ = 0, demo_event_ = 0, demo_reset_ = 0;
    std::int32_t character_selection_ = 0;
    std::uint32_t fade_color_ = 0;
    // The task-pool initializer clears four auxiliary words; their wider role is unrecovered.
    std::array<std::uint32_t, 4> task_auxiliary_{};
    std::shared_ptr<TaskWorkArena> arena_;
    // BSS starts null. TH_Init first sets a sentinel during LoadLevel, after the startup menus.
    // GLOBAL: SONIC 0x38f6e08 CurrentCooperativeThread size=4
    std::shared_ptr<void> current_cooperative_thread_;
    std::unique_ptr<TaskScheduler> tasks_;

    void set_character_selection(std::int32_t) override;
    void set_background(std::uint32_t, std::uint32_t, std::uint32_t) override;
    void load_textures(std::string_view) override;
    void set_frame_rate(std::int32_t, std::int32_t) override;
    void set_wait_vsync(std::int32_t) override;
    void initialize_save_data() override;
    void run_objects() override;
    void display(std::uint8_t) override;
    void release_textures() override;
    void select_logo_textures() override;
    void draw_texture(std::uint32_t, float, float, float) override;
    void draw_box(float, float, float, std::uint32_t) override;
};
} // namespace sadx::native
