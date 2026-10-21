#pragma once
#include "engine/pvr_draw.hpp"
#include <stdexcept>
namespace sadx::queue {
enum class BlankFrameBoundary {
    // Source boundary: SONIC 0x409024 calls the actual default-depth restore.
    DefaultDepthRestore
};
class BlankFrameError : public std::runtime_error {
  public:
    BlankFrameError()
        : std::runtime_error("queue blank frame requires actual default-depth restoration") {}
    BlankFrameBoundary boundary = BlankFrameBoundary::DefaultDepthRestore;
};
// Borrow the SAME canonical renderer fields as the consumer/ordinary box caller.
struct BlankFrameView {
    const float &horizontal_scale, &vertical_scale;
    std::int32_t &display_disabled;
    std::uint8_t &fog_emulation, &fog_enabled;
    float &queued_depth_offset;
};
// NOTSA: exact4086f0 mode2 body AFTER its depth-comparison7 call. The consumer
// publishes execution_mode1 only after this callback returns. This helper does
// not select a mode, clear records, disable a guard or synthesize a background.
void draw_blank_frame(BlankFrameView, pvr::Calls &, const std::function<void()> &restore_depth,
                      X87Context &);
} // namespace sadx::queue
