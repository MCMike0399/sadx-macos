#pragma once
#include "game/sound_effects.hpp"
#include "platform/mac/mac_sound_bank.hpp"

namespace sadx::sound_effects::mac {
// NOTSA: resource adapter for the native sound server. Scheduling, selection and
// source parameter conversions remain in the game module.
class Output final : public ServerCalls, private PcmParameterCalls {
  public:
    Output(const std::filesystem::path &, Queue, sound_bank::State &,
           sound_bank::mac::Banks &, audio::PcmState &, audio::mac::AudioOutput &, X87Context &);
    ~Output() override;
    BankView bank(std::uint32_t) override;
    SelectedSample sample(std::uint32_t, std::size_t) override;
    void load_pcm(std::int32_t, const SelectedSample &, std::int32_t) override;
    void load_3d_pcm(std::int32_t, const SelectedSample &) override;
    void set_3d_min_max(std::int32_t, float, float) override;
    std::int32_t distance_volume(std::int32_t) override;
    void volume(std::int32_t, std::int32_t) override;
    void pan(std::int32_t, std::int32_t) override;
    void pitch(std::int32_t, std::int32_t) override;
    void play_pcm(std::int32_t) override;
    void stop_pcm(std::int32_t) override;
    void play_voice(std::int32_t) override;
    void update_positions() override;
    std::uint32_t load_events() const;
    std::uint32_t play_events() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void set_volume(std::int32_t, std::int32_t) override;
    void set_pan(std::int32_t, std::int32_t) override;
    void set_frequency(std::int32_t, std::uint32_t) override;
    void set_position(std::int32_t, float, float, float) override;
};
} // namespace sadx::sound_effects::mac
