#include "engine/queue_blank_frame.hpp"
#include "cpu/fidelity_float.hpp"
#include <stdexcept>
namespace sadx::queue {
void draw_blank_frame(BlankFrameView view, pvr::Calls &calls,
                      const std::function<void()> &restore_depth, X87Context &fp) {
    const auto height = (fidelity::ExactFloat(fp, view.vertical_scale) * 480.0f).rounded();
    const auto width = (fidelity::ExactFloat(fp, view.horizontal_scale) * 640.0f).rounded();
    pvr::draw_box_fill_2d(0, 0, width, height, -1, 4278190080U, view.display_disabled,
                          view.fog_emulation, view.fog_enabled, view.queued_depth_offset, calls,
                          fp);
    if (!restore_depth)
        throw BlankFrameError();
    restore_depth();
}
} // namespace sadx::queue
