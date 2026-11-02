#pragma once
#include <cstdint>
#include <functional>
#include <stdexcept>

namespace sadx::control {
struct FrameMessageState {
    // GLOBAL: SONIC 0x3abdf80 frame_message_work size=4
    std::uint32_t work = 0;
};
struct EditorState {
    // GLOBAL: SONIC 0x3b29d08 ssEditorStatus size=2
    std::uint16_t status = 0;
};
// NOTSA: a lazy observation can stop a read; it cannot replace the plain value.
struct EditorReadUnavailable : std::runtime_error {
    EditorReadUnavailable() : std::runtime_error("editor status unavailable") {}
};
class EditorView {
    const EditorState *state_ = nullptr;
    std::function<void()> before_read_{};
  public:
    EditorView() = default;
    explicit EditorView(const EditorState &state, std::function<void()> before_read = {})
        : state_(&state), before_read_(std::move(before_read)) {}
    std::int32_t signed_status() const {
        if (!state_)
            throw EditorReadUnavailable{};
        if (before_read_)
            before_read_();
        return static_cast<std::int16_t>(state_->status);
    }
    bool aliases(const EditorView &other) const { return state_ == other.state_; }
};
} // namespace sadx::control
