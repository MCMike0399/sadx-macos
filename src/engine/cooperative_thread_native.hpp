#pragma once
#include <memory>

namespace sadx::cooperative_threads {
// NOTSA: preserves the explicit boundary before an unrecovered native continuation store.
enum class WaitResult { Completed, NeedsContinuation };
// FUNCTION: SONIC 0x4014b0 TH_Wait
WaitResult wait(const std::shared_ptr<void> &current_thread) noexcept;
} // namespace sadx::cooperative_threads
