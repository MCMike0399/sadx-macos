#pragma once
#include "game/sound_bank.hpp"
#include "game/sound_initial_state.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sadx { class X87Context; }

namespace sadx::sound_effects {
// NOTSA: remaining _SEcallbuf fields; existing timer/tone/mode/angle/volume/pitch,
// opaque identities and bank numbers stay with their canonical session owners.
struct RecordWork {
    std::int32_t priority = 0;
    std::int32_t volume_limit = 0;
    std::array<float, 3> position{};
    std::int32_t queue_number = 0;
};
// NOTSA: selected bank resource replaces sndmemory, retaining its native owner.
class Sample {
  public:
    virtual ~Sample() = default;
};
struct SelectedSample {
    std::shared_ptr<const Sample> resource;
    std::uint32_t source_size = 0;
};
struct BankThreshold {
    std::int32_t tone;
    std::uint32_t bank;
};
using BankTable = std::array<BankThreshold, 64>;
enum class Boundary { ChannelCursor, BankTable, OverlapStorage, SampleName, SampleIndex };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string message)
        : std::runtime_error(std::move(message)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }
  private:
    Boundary boundary_;
};
struct State {
    std::array<RecordWork, 36> records{};
    std::int32_t alternating_pan = 8;
    // Original overlap scan also reaches the37th pair, beyond its72-byte reset range.
    std::array<std::uint8_t, 2> overlap_tail{};
    SelectedSample selected_sample{};
};
// NOTSA: borrowed canonical numeric queue and opaque identity owners.
struct Queue {
    SoundInitialState &sound;
    sound_bank::ChannelBindings &bindings;
    State &work;
};
struct OpponentState {
    const std::int32_t &character;
    const TaskHandle &task;
};
struct ServerView {
    Queue &queue;
    std::array<TaskHandle, 36> &positional_tasks;
    const BankTable &bank_table;
    OpponentState opponent;
    const TaskHandle &listener;
};
// NOTSA: live metadata views borrow the existing loaded-bank resource.
struct BankView {
    bool present;
    std::vector<std::string_view> names;
};
struct SampleCalls {
    virtual ~SampleCalls() = default;
    virtual BankView bank(std::uint32_t slot) = 0;
    virtual SelectedSample sample(std::uint32_t slot, std::size_t ordinal) = 0;
};
struct ServerCalls : SampleCalls {
    virtual void load_pcm(std::int32_t channel, const SelectedSample &, std::int32_t loop) = 0;
    virtual void load_3d_pcm(std::int32_t channel, const SelectedSample &) = 0;
    virtual void set_3d_min_max(std::int32_t channel, float minimum, float maximum) = 0;
    virtual std::int32_t distance_volume(std::int32_t channel) = 0;
    virtual void volume(std::int32_t value, std::int32_t channel) = 0;
    virtual void pan(std::int32_t value, std::int32_t channel) = 0;
    virtual void set_pan(std::int32_t channel, std::int32_t value) = 0;
    virtual void pitch(std::int32_t value, std::int32_t channel) = 0;
    virtual void play_pcm(std::int32_t channel) = 0;
    virtual void stop_pcm(std::int32_t channel) = 0;
    virtual void play_voice(std::int32_t voice) = 0;
    virtual void update_positions() = 0;
};
struct PcmParameterCalls {
    virtual ~PcmParameterCalls() = default;
    virtual void set_volume(std::int32_t channel, std::int32_t attenuation) = 0;
    virtual void set_pan(std::int32_t channel, std::int32_t pan) = 0;
    virtual void set_frequency(std::int32_t channel, std::uint32_t frequency) = 0;
    virtual void set_position(std::int32_t channel, float x, float y, float z) = 0;
};
// FUNCTION: SONIC 0x423c20 IsndVolume
void set_volume(std::int32_t value, std::int32_t channel, const Queue &, PcmParameterCalls &);
// FUNCTION: SONIC 0x423c80 IsndPan
void set_pan(std::int32_t value, std::int32_t channel, PcmParameterCalls &);
// FUNCTION: SONIC 0x423cc0 IsndPitch
void set_pitch(std::int32_t value, std::int32_t channel, PcmParameterCalls &, X87Context &);
// FUNCTION: SONIC 0x4246c0 dsEVboss
bool opponent_uses_bank(std::int32_t tone, const OpponentState &);
// FUNCTION: SONIC 0x424590 FUN_00424590
bool allow_overlap(std::uint32_t tone, Queue &);
// FUNCTION: SONIC 0x423ad0 FUN_00423ad0
std::int32_t find_sample_name(std::string_view, const BankView &);
// FUNCTION: SONIC 0x423b20 FUN_00423b20
void select_bank_sample(std::uint32_t bank, std::int32_t cue, State &, SampleCalls &);
// FUNCTION: SONIC 0x4250d0 dsSoundServer
void update_sound_server(ServerView &, ServerCalls &);
// FUNCTION: SONIC 0x423d10 SoundQueue_GetFreeIndex
std::int32_t find_free_channel(Queue &, std::int32_t priority);
// FUNCTION: SONIC 0x423d70 dsPlay_oneshot
std::int32_t play_one_shot(std::int32_t tone, const void *identity,
                           std::int32_t priority, std::int32_t volume_offset, Queue &);
// FUNCTION: SONIC 0x505810 FUN_00505810
void play_title_confirmation(Queue &);
} // namespace sadx::sound_effects
