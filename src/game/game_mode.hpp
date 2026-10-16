#pragma once
#include <cstdint>
#include <stdexcept>

namespace sadx::flow {
// MD_GameMode; values outside this list still execute the common frame tail.
enum class GameMode : std::int32_t {
    Start = 0,
    Logo = 1,
    Title = 2,
    SelectPlayer = 3,
    Action = 4,
    Adventure = 5,
    Continue = 6,
    End = 7,
    Movie = 8,
    Trial = 9,
    Mission = 10,
    TitleInit = 11,
    TitleMenu = 12,
    NoSaveWindowInit = 13,
    NoSaveWindow = 14,
    CharacterSelectInit = 15,
    CharacterSelect = 16,
    SummaryInit = 17,
    Summary = 18,
    TutorialInit = 19,
    Tutorial = 20,
    StaffRollInit = 21,
    StaffRoll = 22,
    TvSettingEnd = 23,
    LogoInit = 24,
    // NOTSA: descriptive name for the additional backdrop case, absent from MD_GameMode.
    Backdrop = 26
};

struct MenuResource {
    std::uint32_t value = 0;
};
struct SequenceSelection {
    std::int16_t level = 0;
};

// NOTSA: plain value view for game flow. Session/reference storage owns source globals;
// this view introduces no second source-global owner. Accessors expose only validated facts.
struct GameFlowState {
    GameMode mode = GameMode::Start;
    std::uint32_t pause_state = 0;
    std::int32_t save_phase = 0;
    std::uint8_t save_ready = 0;
    MenuResource message_resource{};
    std::uint8_t action_restart_requested = 0;
    std::uint8_t title_mode = 0;
    std::uint8_t title_load_phase = 0;
    std::uint8_t playing_super_sonic = 0;
    std::uint8_t tutorial_variant = 0;
    std::uint8_t alert_flag = 0;
    std::uint8_t alert_blocked = 0;
    std::uint32_t movie_number = 0;
    std::uint32_t menu_message_enabled = 0;
    const SequenceSelection *sequence = nullptr;
};

// NOTSA: native callers get an explicit failure at an unrecovered link-time seam.
class UnrecoveredGameFlowCall : public std::runtime_error {
  public:
    explicit UnrecoveredGameFlowCall(const char *name) : std::runtime_error(name) {}
};

// FUNCTION: SONIC 0x40c090 GameModeHandler
void advance_game_mode(GameFlowState &state);
} // namespace sadx::flow
