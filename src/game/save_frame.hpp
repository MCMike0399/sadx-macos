#pragma once
#include "game/menu_caption.hpp"
#include <cstdint>
namespace sadx::save {
// NOTSA: borrowed canonical native-world fields; this view owns no source globals.
// Legacy reference optional fields remain separate reference validation until a
// coherent whole-owner migration. No default values or unknown facts live here.
struct FrameStateView {
    std::uint8_t &ready;
    std::uint8_t &count;
    std::int32_t &now_saving;
    std::uint8_t &exec;
    std::uint8_t &pending_message;
    std::uint32_t &pause_state;
    menu_caption::Context &message;
};
struct FrameCalls {
    virtual ~FrameCalls() = default;
    // Source40be40: actual saving-message owner must allocate/render its resources.
    virtual void display_saving_message(menu_caption::Context &) = 0;
    // Source40e2c0: closes the SAME3abdca8 MSGC through its real release queue.
    virtual void close_saving_message(menu_caption::Context &) = 0;
    // Source421fd0: actual writer, not completion-flag emulation.
    virtual void write_save_file() = 0;
};
// NOTSA: readable40cad0 scheduling slices. Caller must preserve the intervening
// render/tail order and call source411d00 frame completion BETWEEN these helpers.
// before:40cba4..40cbf1, after queue/fog pass and before40cbf2+message/render tail.
void before_frame_completion(FrameStateView, FrameCalls &);
// after:40cc1d..40cc61, AFTER411d00 returns and before presented-frame increment.
void after_frame_completion(FrameStateView, FrameCalls &);
} // namespace sadx::save
