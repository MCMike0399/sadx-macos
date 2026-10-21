#pragma once
#include "game/game_frame_owners.hpp"
#include <unordered_map>

namespace sadx::reference {
// Reference transport only: observation flags and source-pointer identities never enter game state.
class CaptionFrameReference {
  public:
    explicit CaptionFrameReference(menu_caption::State &state) : state_(state) {}

    void observe_timer(std::optional<std::uint32_t> value) {
        timer_observed_ = value.has_value();
        if (value)
            state_.timer = *value;
    }
    void observe_buffer(std::optional<std::uint32_t> value) {
        buffer_observed_ = value.has_value();
        if (!value)
            return;
        if (*value == 0) {
            state_.buffer.reset();
            return;
        }
        for (auto entry = resources_.begin(); entry != resources_.end();) {
            if (entry->second.expired())
                entry = resources_.erase(entry);
            else
                ++entry;
        }
        auto resource = resources_[*value].lock();
        // Only identity is observed by this reference boundary. No buffer bytes are read.
        if (!resource) {
            resource = std::make_shared<menu_caption::Buffer>(0);
            resources_[*value] = resource;
        }
        state_.buffer = resource;
    }
    std::optional<std::uint32_t> timer_word() const {
        return timer_observed_ ? std::optional<std::uint32_t>{state_.timer} : std::nullopt;
    }
    std::optional<std::uint32_t> buffer_word() const {
        if (!buffer_observed_)
            return std::nullopt;
        return identity(state_.buffer);
    }
    GameFrameOwnerResult complete_frame(const GameFrameOwnerInvoke &invoke) {
        GameFrameOwnerResult result;
        if (!timer_observed_)
            return missing(0x40bcf0, 0x3abdf88);
        menu_caption::advance_timer(state_);
        if (!buffer_observed_)
            return missing(0x40bcff, 0x3abdf84);
        ReceiptCalls calls(*this, invoke, result);
        try {
            menu_caption::release_expired(state_, calls);
        } catch (const Stop &) {
        }
        return result;
    }

  private:
    struct Stop {};
    menu_caption::State &state_;
    bool timer_observed_ = false, buffer_observed_ = false;
    std::unordered_map<std::uint32_t, std::weak_ptr<menu_caption::Buffer>> resources_;

    static GameFrameOwnerResult missing(std::uint32_t boundary, std::uint32_t fact) {
        return {GameFrameOwnerFailure::unknown_state, boundary, fact, {}, {}};
    }
    std::uint32_t identity(const std::shared_ptr<menu_caption::Buffer> &buffer) const {
        if (!buffer)
            return 0;
        for (const auto &entry : resources_) {
            if (entry.second.lock() == buffer)
                return entry.first;
        }
        throw std::runtime_error("reference caption buffer has no source-pointer identity");
    }
    struct ReceiptCalls final : menu_caption::FrameCalls {
        CaptionFrameReference &adapter;
        const GameFrameOwnerInvoke &invoke;
        GameFrameOwnerResult &result;
        ReceiptCalls(CaptionFrameReference &a, const GameFrameOwnerInvoke &i,
                     GameFrameOwnerResult &r)
            : adapter(a), invoke(i), result(r) {}
        void call(std::uint32_t site, std::uint32_t callee, std::uint32_t argument) {
            result.pending_call = GameFrameOwnerCall{site, callee, {argument, {}}};
            if (!invoke) {
                result.failure = GameFrameOwnerFailure::missing_owner;
                result.source_boundary = site;
                throw Stop{};
            }
            const auto receipt = invoke(*result.pending_call);
            if (receipt.state != GameLogicOwnerState::completed) {
                result.failure = receipt.state == GameLogicOwnerState::failed
                                     ? GameFrameOwnerFailure::owner_failed
                                     : GameFrameOwnerFailure::missing_owner;
                result.source_boundary = site;
                throw Stop{};
            }
            result.pending_call.reset();
        }
        void close_message() override {
            call(0x40bd12, 0x40e2c0, 0x3abdc18);
        }
        void free_buffer(std::shared_ptr<menu_caption::Buffer> buffer) override {
            if (!adapter.buffer_observed_) {
                result = missing(0x40bd17, 0x3abdf84);
                throw Stop{};
            }
            call(0x40bd1d, 0x780fb0, adapter.identity(buffer));
        }
    };
};
} // namespace sadx::reference
