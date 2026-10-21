#pragma once
#include "engine/task.hpp"
#include "game/sound_initial_state.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sadx::sound_bank {
inline constexpr std::size_t kListCount = 123, kSlotCount = 16, kChannelCount = 36;
// NOTSA: typed imported SoundLists entries; source layout stays in the importer.
struct Entry {
    std::uint8_t slot;
    std::string name;
};
using Catalog = std::array<std::vector<Entry>, kListCount>;
// NOTSA: retained native bank data, never an original MDHANDLE pointer.
class Bank {
  public:
    explicit Bank(std::string path) : path_(std::move(path)) {}
    virtual ~Bank() = default;
    const std::string &loaded_path() const {
        return path_;
    }

  private:
    std::string path_;
};
using Handle = std::shared_ptr<Bank>;
struct State {
    std::array<Handle, kSlotCount> slots{};
};
// NOTSA: native bank-slot values and opaque native sound identities. Existing record
// numeric fields and positional tasks remain canonical session-owned borrows.
struct ChannelBindings {
    std::array<std::uint32_t, kChannelCount> bank_slots{};
    std::array<const void *, kChannelCount> identities{};
};
struct Channels {
    std::array<SoundRecordInitialValues, kChannelCount> &records;
    std::array<TaskHandle, kChannelCount> &positional_tasks;
    ChannelBindings &bindings;
};
struct Services {
    std::function<void(std::uint32_t)> release_samples;
    std::function<void(const Handle &)> close;
    std::function<std::int32_t(std::string_view)> mod_index;
    std::function<Handle(std::string_view, std::uint32_t)> open_pack;
    std::function<Handle(std::string_view, std::uint32_t)> open_file;
};
enum class Boundary { ListIndex, Slot, Path, ReleaseSamples, Close, Open, Decode, Priority, Pack };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string message)
        : std::runtime_error(std::move(message)), boundary_(boundary) {}
    Boundary boundary() const {
        return boundary_;
    }

  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x423890 ReleaseBankSounds
void release_samples(std::uint32_t slot, Channels &,
                     const std::function<void(std::int32_t)> &stop_pcm);
// Imported catalog content is immutable. Real missing files return null from
// open, including an empty-name bank; unavailable implementations throw.
// Pinned loader hook: this body replaces SONIC SetBankDir.
// FUNCTION: LOADER 0x100979a0 LoadSoundList_r
void select(std::int32_t index, const Catalog &, State &, const std::uint32_t &stream_initialized,
            bool voice_enabled, std::string_view cd_prefix, const Services &);
// NOTSA: the name/filter subset of the pinned loader file-bank normalization.
// nullopt means an intentionally ignored underscore-prefixed source entry.
std::optional<std::string> normalize_name(std::string_view source_name, std::uint32_t slot,
                                          std::int32_t &counter, bool force_adx);
} // namespace sadx::sound_bank
