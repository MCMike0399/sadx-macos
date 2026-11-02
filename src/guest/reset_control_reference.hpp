#pragma once
#include "engine/reset_control.hpp"
#include <optional>

namespace sadx::reference {
// NOTSA: plain per-world owners with private reference knowledge. Importing an
// unavailable word never exposes either the old value or an invented default.
class FrameMessageFact {
    control::FrameMessageState state_{};
    bool known_ = false;
  public:
    FrameMessageFact() = default;
    FrameMessageFact(std::uint32_t value) : state_{value}, known_(true) {}
    FrameMessageFact(std::optional<std::uint32_t> value) { *this = value; }
    FrameMessageFact &operator=(std::uint32_t value) {
        state_.work = value;
        known_ = true;
        return *this;
    }
    FrameMessageFact &operator=(std::optional<std::uint32_t> value) {
        known_ = value.has_value();
        if (value) state_.work = *value;
        return *this;
    }
    explicit operator bool() const { return known_; }
    std::uint32_t operator*() const {
        if (!known_) throw std::logic_error("unavailable frame message word");
        return state_.work;
    }
    void reset() { known_ = false; }
    std::uint32_t value_or(std::uint32_t other) const { return known_ ? state_.work : other; }
    std::optional<std::uint32_t> fact() const {
        return known_ ? std::optional{state_.work} : std::nullopt;
    }
    bool operator==(std::uint32_t value) const { return known_ && state_.work == value; }
};
class EditorFact {
    control::EditorState state_{};
    bool known_ = false;
  public:
    EditorFact() = default;
    EditorFact(std::optional<std::uint16_t> value) { *this = value; }
    EditorFact &operator=(std::optional<std::uint16_t> value) {
        known_ = value.has_value();
        if (value) state_.status = *value;
        return *this;
    }
    explicit operator bool() const { return known_; }
    std::uint16_t operator*() const {
        if (!known_) throw control::EditorReadUnavailable{};
        return state_.status;
    }
    void reset() { known_ = false; }
    std::uint16_t value_or(std::uint16_t other) const { return known_ ? state_.status : other; }
    bool operator==(std::uint16_t value) const { return known_ && state_.status == value; }
    control::EditorView borrow() const {
        return control::EditorView(state_, [this] {
            if (!known_) throw control::EditorReadUnavailable{};
        });
    }
};
} // namespace sadx::reference
