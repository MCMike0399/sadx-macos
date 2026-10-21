#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sadx::character_completion {
enum class FlagSource : std::int32_t { Story = 0, Cutscene = 1 };
// NOTSA: pointer-free imported three-word source progress declarations.
struct Row {
    std::int32_t value;
    FlagSource source;
    std::int32_t flag;
};
using Catalog = std::array<std::vector<Row>, 7>;
// NOTSA: borrows the existing live owners, separate from the packed save snapshots.
struct Flags {
    const std::array<std::uint8_t, 512> &story, &cutscene;
};
enum class Boundary {
    Character, // SONIC 0x512ca5 catalog count read
    StoryFlag, // SONIC 0x412d24 SeqCheckFlag byte read
    CutsceneFlag // SONIC 0x42fb14 signed cutscene byte read
};
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::int32_t index)
        : std::runtime_error("character completion declaration has no imported index"),
          boundary_(boundary), index_(index) {}
    Boundary boundary() const { return boundary_; }
    std::int32_t index() const { return index_; }
  private:
    Boundary boundary_;
    std::int32_t index_;
};
// FUNCTION: SONIC 0x512ca0 CharacterCompletion
std::int32_t value(std::int32_t character, const Catalog &, Flags);
} // namespace sadx::character_completion
