#pragma once
#include "game/regular_textures.hpp"
#include "game/title_menu.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace sadx::menu_resource_lifetime {
// NOTSA: this is the same native binding shape as the imported menu catalog;
// archive names are unused by release, and equal List pointers remain aliases.
using TextureCatalog = std::array<std::array<regular_textures::Binding, 30>, 5>;
struct State {
    title_menu::MenuTextureState &textures;
    const std::int32_t &language;
    const TextureCatalog &catalog;
    const title_menu::ModeTextureLists &mode_textures;
};
struct Services {
    std::function<void(texture_lifetime::List *)> release_texture;
    std::function<void(const TaskHandle &)> destroy_task;
    std::function<void(const TaskHandle &)> free_task;
};
enum class Boundary { TextureId, Language, Mode, TemporaryCapacity, SequenceData, TaskMode };
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, std::string text)
        : std::runtime_error(std::move(text)), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }
  private:
    Boundary boundary_;
};
// FUNCTION: SONIC 0x505f40 FUN_00505f40
void release_ids(const std::vector<title_menu::MenuTextureId> *, State &, Services &);
// FUNCTION: SONIC 0x505fe0 FUN_00505fe0
void release_all(State &, Services &);
// FUNCTION: SONIC 0x506040 AvaReleaseTexForEachMode
void release_mode_textures(const title_menu::SequenceWork &, State &, Services &);
// NOTSA: the direct Japanese-list release call in title_new_exec, without a
// loaded-ID flag or a separate source wrapper. The caller owns its language guard.
void release_japanese_title(texture_lifetime::List &, Services &);
// NOTSA: semantic dispatch of the imported sequence's original cleanup table.
void destroy_mode_task(title_menu::MenuMode, title_menu::State &, Services &);
// FUNCTION: SONIC 0x432530 FUN_00432530
void destroy_dialog(title_menu::State &, Services &);
// FUNCTION: SONIC 0x505ae0 MenuSequence_Delete
void destroy_sequence(const title_menu::SequenceWork &, std::int32_t &demo_mode,
                         title_menu::State &, State &, Services &);
} // namespace sadx::menu_resource_lifetime
