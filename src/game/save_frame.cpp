#include "game/save_frame.hpp"
namespace sadx::save {
void before_frame_completion(FrameStateView state, FrameCalls &calls) {
    detail::before_frame_completion(state, calls);
}
void after_frame_completion(FrameStateView state, FrameCalls &calls) {
    detail::after_frame_completion(state, calls);
}
} // namespace sadx::save
