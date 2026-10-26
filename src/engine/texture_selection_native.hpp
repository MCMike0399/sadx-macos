#pragma once
#include "engine/native_texture_lifetime.hpp"
#include "engine/native_texture_palette.hpp"
#include <functional>
#include <stdexcept>

namespace sadx::texture_selection {
using Memory = texture_lifetime::Memory;
using List = texture_lifetime::List;
using Surface = std::shared_ptr<texture_lifetime::Surface>;
using BindSurface = std::function<void(const Surface &)>;
// NOTSA: borrow the existing texture owners; this view owns no game storage.
struct State {
    const List *&current_list;
    Memory *&requested_memory;
    std::int32_t &cached_global_index;
    Memory *&active_memory;
    texture_palette::State &palettes;
};
enum class Boundary {
    CacheMemory,
    ListEntry,
    IndexEntry,
    PaletteVersion,
    PaletteSurface,
    PaletteUpload,
    DirectBinding,
    PaletteBinding
};
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, const char *fact) : std::runtime_error(fact), boundary(boundary) {}
    Boundary boundary;
};
// FUNCTION: SONIC 0x403070 njSetTexture
std::uint32_t select_queued_list(const List *, const List *&queued_list,
                                 std::uint32_t &queued_index, State &, const BindSurface &);
// FUNCTION: SONIC 0x77f3d0 Direct3D_SetTexList
std::uint32_t select_list(const List *, State &, const BindSurface &);
// FUNCTION: SONIC 0x77f440 njSetTextureNum_
std::uint32_t select_index(std::uint32_t, State &, const BindSurface &);
// FUNCTION: SONIC 0x78cf20 Direct3D_SetNJSTexture
std::uint32_t cache(Memory *, State &, const BindSurface &);
} // namespace sadx::texture_selection
