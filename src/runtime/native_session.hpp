#pragma once
#include "engine/character_pack.hpp"
#include "engine/graphics_frame.hpp"
#include "game/game_bootstrap.hpp"
#include "game/game_mode_services.hpp"
#include "runtime/logo_scene.hpp"

namespace sadx::native {
// NOTSA: native game session owns canonical flow/bootstrap/save state and imported actions.
// Starts at the original Start mode; unrecovered later modes keep their explicit failures.
class Session final : private flow::NativeServices {
  public:
    Session(LogoOutput &, std::shared_ptr<const character_pack::CharacterPack>,
            LogoFrame, X87Context &, std::function<void(std::uint32_t)> stop_vibration);
    void advance();
    const flow::GameFlowState &flow_state() const { return flow_; }
    const bootstrap::State &game_state() const { return bootstrap_; }
    const save::Data &save_data() const { return save_; }
    const LogoScene &logo_scene() const { return logo_; }
    std::uint32_t global_timer() const { return global_timer_; }
    std::int16_t stage() const { return stage_; }
    std::int16_t rings() const { return rings_; }
    frame::RendererState &renderer_timing() { return renderer_; }
    const frame::ModeState &frame_mode() const { return timing_; }

  private:
    flow::GameFlowState flow_;
    std::uint32_t global_timer_ = 0;
    std::int16_t rings_ = 0, stage_ = 0;
    std::uint16_t game_mode_ = 0;
    bootstrap::State bootstrap_;
    save::Data save_;
    std::shared_ptr<const character_pack::CharacterPack> character_pack_;
    bootstrap::SonicActions actions_;
    bootstrap::Services services_;
    frame::ModeState timing_;
    frame::RendererState renderer_;
    LogoScene logo_;
    // Imported texture declarations are distinct from currently loaded native resources.
    // At startup no character texture has been acquired; later loads remain explicit seams.
    std::array<std::vector<std::shared_ptr<const graphics::Texture>>, 8> character_textures_;

    void initialize_game() override;
    void initialize_demo_state() override;
    std::uint32_t startup_status() override;
    std::int32_t startup_readiness() override;
    void initialize_sega_logo() override;
    std::uint32_t run_logos() override;
    void load_sega_logo() override;
    void set_frame_rate_mode(std::int32_t, std::int32_t) override;
    void set_wait_vsync_count(std::int32_t) override;
    void draw_screen_fade() override;
    void increment_frame_counter() override;
    std::uint32_t initialize_level_game() override;
    void setup_character(std::uint32_t) override;
    void reset_player_work() override;
};
} // namespace sadx::native
