#pragma once
#include "game/game_logic.hpp"
#include "game/game_startup.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
namespace sadx {
// Sound-effect record fields (36 records at 3b292fc, stride 0x38) without
// another owner; +0 timers are AudioStartupState's, +8 flags ResetLeafState's.
struct SoundRecordFields {
    std::array<std::optional<std::uint32_t>,36> words_at_3b29300,ids_at_3b29308,frequency_at_3b2930c,volume_at_3b29310,pan_at_3b29318;
    std::array<std::optional<std::uint32_t>,36> current_volume_at_3b29c28; // last volume sent per record
    std::optional<std::uint32_t> listener_at_3b2cbb0;                     // 424b10's 3D listener (0 = none)
};
// One helper call in source order; 4244a0 takes its record index in EAX.
struct SoundManagerCall {std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,2> arguments={};};
struct SoundManagerOwnerResult {GameLogicOwnerResult owner;std::optional<std::uint32_t> eax;};
struct SoundManagerRuntime {
    AudioStartupState& audio;                                       // 3b29ce0, 3b29ce4, 3b29ce8, timers, 3b292a8
    std::array<std::optional<std::uint32_t>,36>& flags_at_3b29304;  // ResetLeafState
    std::array<std::optional<std::uint32_t>,36>& entities_at_3b29b90;
    SoundRecordFields& fields;
    std::function<SoundManagerOwnerResult(const SoundManagerCall&)> invoke;
};
enum class SoundManagerFailure {none,unknown_state,owner_failed,missing_owner,missing_return};
struct SoundManagerResult {
    SoundManagerFailure failure=SoundManagerFailure::none;std::uint32_t source_boundary=0,nested_source_boundary=0;std::optional<std::uint32_t> required_fact;
    std::optional<SoundManagerCall> pending_call;
    explicit operator bool()const noexcept{return failure==SoundManagerFailure::none;}
};
// Actual 4250d0 (called each frame from 40cb40), with its 424b10 tail. A
// completed helper whose return the source then needs but which supplies none
// (an unported survey owner) ends that record's processing.
SoundManagerResult run_sound_manager_4250d0(SoundManagerRuntime&);
} // namespace sadx
