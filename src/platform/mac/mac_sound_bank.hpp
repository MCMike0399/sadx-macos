#pragma once
#include "game/audio_playback.hpp"
#include "game/sound_bank.hpp"
#include "platform/mac/mac_audio.hpp"
#include <filesystem>

namespace sadx::sound_bank::mac {
struct OpenAttempt {
    std::string logical_path;
    bool opened;
    std::string detail;
};
// NOTSA: imported native bank data and decoded PCM. No executable/DLL reads.
class Banks {
  public:
    explicit Banks(const std::filesystem::path &imported_directory);
    ~Banks();
    Banks(const Banks &) = delete;
    Banks &operator=(const Banks &) = delete;
    const Catalog &catalog() const;
    std::int32_t mod_index(std::string_view logical_path) const;
    Handle open_pack(std::string_view logical_path, std::uint32_t slot);
    Handle open_file(std::string_view logical_path, std::uint32_t slot);
    void close(const Handle &);
    Services services(Channels, audio::PcmState &);
    // Ordinals refer to effective loader entries after underscore filtering.
    // Cue IDs are represented by the recovered normalized sample names.
    std::size_t sample_count(const Handle &) const;
    std::shared_ptr<const audio::mac::AudioClip> sample(const Handle &, std::size_t ordinal) const;
    std::string_view sample_name(const Handle &, std::size_t ordinal) const;
    std::size_t source_ordinal(const Handle &, std::size_t ordinal) const;
    const std::vector<OpenAttempt> &attempts() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace sadx::sound_bank::mac
