#include "runtime/native_session.hpp"
#include "runtime/movie_scene.hpp"
#include <stdexcept>

namespace sadx::native {
namespace {
// NOTSA: imported handles are resolved before the session starts, never synthesized.
bootstrap::SonicActions actions(const std::shared_ptr<const character_pack::CharacterPack> &pack) {
    if (!pack)
        throw std::runtime_error("native startup requires an imported character pack");
    return {pack->action(character_pack::StartupAction::ClearPose),
            pack->action(character_pack::StartupAction::ClearPoseTransition)};
}
} // namespace

Session::Session(LogoOutput &output, std::shared_ptr<const character_pack::CharacterPack> pack,
                 LogoFrame frame, X87Context &fp, std::function<void(std::uint32_t)> stop)
    : bootstrap_({card_result_, global_timer_, rings_, flow_.movie_number, game_mode_}),
      character_pack_(std::move(pack)), actions_(actions(character_pack_)),
      services_{[this](std::uint16_t character) {
                    character_textures_.at(character).clear();
                },
                std::move(stop)},
      logo_(output, timing_, renderer_, save_, rings_, frame, fp) {
    // OnInitGameLoop selects one render/logic tick before its first GameModeHandler call.
    frame::set_frame_rate_mode(1, 1, timing_, renderer_);
}

void Session::advance() { flow::advance_game_mode(flow_, *this); }
void Session::initialize_game() {
    bootstrap::start(bootstrap_, save_, actions_, stage_, services_);
}
void Session::initialize_demo_state() {
    bootstrap::initialize_advertise(bootstrap_.menu, flow_.tutorial_variant);
}
std::uint32_t Session::startup_status() { return bootstrap::loading_status(); }
std::int32_t Session::startup_readiness() {
    return static_cast<std::int32_t>(bootstrap::loading_ready());
}
void Session::initialize_sega_logo() { logo_.initialize(); }
std::uint32_t Session::run_logos() { return static_cast<std::uint32_t>(logo_.advance()); }
void Session::load_sega_logo() { logo_.load(); }
void Session::set_frame_rate_mode(std::int32_t render_ratio, std::int32_t logic_ratio) {
    frame::set_frame_rate_mode(render_ratio, logic_ratio, timing_, renderer_);
}
void Session::set_wait_vsync_count(std::int32_t count) {
    frame::set_frame_interval_multiplier(count, renderer_);
}
void Session::draw_screen_fade() {
    if (logo_.fade_color() >> 24)
        throw flow::UnrecoveredGameFlowCall("native_screen_fade_queue");
}
void Session::increment_frame_counter() {
    global_timer_ += static_cast<std::uint32_t>(timing_.logic_increment);
}
std::uint32_t Session::initialize_level_game() {
    return bootstrap::initialize_game(bootstrap_, save_, stage_, services_);
}
void Session::setup_character(std::uint32_t character) {
    bootstrap::setup_character(static_cast<std::uint16_t>(character), bootstrap_,
                                save_, actions_, services_);
}
void Session::reset_player_work() { bootstrap::reset_rings(bootstrap_); }
std::int32_t Session::run_movie(std::uint32_t movie_id) {
    if (!movie_)
        throw flow::UnrecoveredGameFlowCall("native_movie_scene");
    return movie_->play(movie_id);
}
} // namespace sadx::native
