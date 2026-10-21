#include "engine/cooperative_thread_native.hpp"

namespace sadx::cooperative_threads {
WaitResult wait(const std::shared_ptr<void> &current_thread) noexcept {
    if (current_thread) return WaitResult::NeedsContinuation;
    return WaitResult::Completed;
}
} // namespace sadx::cooperative_threads
