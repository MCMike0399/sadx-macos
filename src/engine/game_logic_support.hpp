#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "engine/game_frame_control.hpp"
namespace sadx {
// Raw source namespace, not HID/virtual-key/player-input codes. Only lane0 at
// base3b0e3e2+3i is consumed; the other two fields keep their independent facts.
struct GameLogicEdgeState {
    std::array<std::array<std::optional<std::uint8_t>,3>,256> raw_at_3b0e3e2;
    std::array<std::optional<std::uint8_t>,256> aggregate_at_3b0eaf0;
    std::optional<std::uint8_t> special_at_3b0e3ba,aggregate_special_at_3b0eae8;
    // Neighbouring bytes cleared with the rows by 40f350 (no other known reader).
    std::array<std::optional<std::uint8_t>,2> bytes_at_3b0e3e0;
    std::optional<std::uint8_t> byte_at_3b0e3b9;
    // Borrow the existing strided byte consumed by the frame soft-reset logic.
    std::optional<std::uint8_t>& soft_reset_edge() noexcept {return raw_at_3b0e3e2[59][0];}
};
struct GameLogicTimingState : FrameRateModeState {
    std::optional<std::int32_t> word_at_3b11180;
    std::optional<std::int32_t> word_at_3b1118c,word_at_3b11198,word_at_3b2a2e4;
};
enum class GameLogicSupportFailure {none,unknown_raw_byte,unknown_aggregate_byte,
    unknown_special_byte,unknown_aggregate_special_byte,unknown_timing_word,
    divide_by_zero,divide_overflow};
struct GameLogicSupportResult {
    GameLogicSupportFailure failure=GameLogicSupportFailure::none;
    std::uint32_t source_boundary=0;
    std::optional<std::size_t> byte_index;
    std::size_t committed_bytes=0;bool completed=false;
    std::optional<std::int32_t> quotient,incremented_quotient,product;
    std::optional<bool> clamped;
    explicit operator bool()const noexcept{return failure==GameLogicSupportFailure::none&&completed;}
};
// Actual40ec80. Mode0 copies, mode1 accumulates, mode2 restores accumulated
// edges; other raw32-bit arguments follow the original complete no-op return.
GameLogicSupportResult aggregate_game_logic_edges(std::int32_t,GameLogicEdgeState&);
// Actual413920 signed IDIV/INC/low32 IMUL and source six-tick clamp. State is
// the caller's sole live owner; Win32 clock values are never supplied implicitly.
GameLogicSupportResult calculate_game_logic_ticks(GameLogicTimingState&);
} // namespace sadx
