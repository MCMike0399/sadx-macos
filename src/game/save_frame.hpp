#pragma once
#include "game/menu_caption.hpp"
#include <cstdint>
namespace sadx::save {
// NOTSA: borrowed canonical native-world fields; this view owns no source globals.
// No default values or unknown reference facts live here.
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
    // The saving-message owner allocates and renders its real resources.
    virtual void display_saving_message(menu_caption::Context &) = 0;
    // Closes the same message context through its real release queue.
    virtual void close_saving_message(menu_caption::Context &) = 0;
    // Runs the real writer.
    virtual void write_save_file() = 0;
};
namespace detail {
constexpr std::int32_t kSaveIdle = 0;
constexpr std::int32_t kSaveRequested = 1;
// The source also displays the saving message for -1. Its producer and wider
// meaning are unrecovered; retain this distinct value rather than infer failure.
constexpr std::int32_t kLegacySavingMessagePhase = -1;
// These names describe the observed completion transition only. The wider pause
// mode meanings are not established by this scheduling slice.
constexpr std::uint32_t kPauseBeforeSaveCompletion = 2;
constexpr std::uint32_t kPauseAfterSaveCompletion = 1;
// NOTSA: one scheduling policy for borrowed native fields and lazy reference
// field adapters. A field is read only by its expression below; writes never
// require a prior read. Calls retain the caller's actual message identity.
template<class State, class Calls>
void before_frame_completion(State state, Calls &calls) {
    if (state.ready != 0) {
        state.count = static_cast<std::uint8_t>(state.count + 1U);
        state.now_saving = kSaveRequested;
        state.exec = 1;
    }
    const std::int32_t phase = state.now_saving;
    if (phase == kSaveRequested || phase == kLegacySavingMessagePhase) {
        calls.display_saving_message(state.message);
    } else if (state.pending_message != 0) {
        calls.close_saving_message(state.message);
        state.pending_message = 0;
    }
}
// NOTSA: the completion/render interval belongs to the caller. In particular,
// every read here observes any changes made during completion or the writer.
template<class State, class Calls>
void after_frame_completion(State state, Calls &calls) {
    if (state.exec != 0) {
        state.exec = 0;
        calls.write_save_file();
    }
    if (state.now_saving == kSaveIdle) {
        if (state.pending_message != 0) {
            calls.close_saving_message(state.message);
            state.pending_message = 0;
        }
        if (state.pause_state == kPauseBeforeSaveCompletion)
            state.pause_state = kPauseAfterSaveCompletion;
    }
}
} // namespace detail
// NOTSA: split scheduling around the caller's rendering and frame completion.
void before_frame_completion(FrameStateView, FrameCalls &);
void after_frame_completion(FrameStateView, FrameCalls &);
} // namespace sadx::save
