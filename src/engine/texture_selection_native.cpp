#include "engine/texture_selection_native.hpp"
#include "engine/texture_selection_internal.hpp"

namespace sadx::texture_selection {
namespace {
// NOTSA: typed access to the canonical owners used by the shared source algorithm.
struct NativeTextures {
    using Memory = texture_selection::Memory *;
    using List = const texture_selection::List *;
    State &state;
    const BindSurface &bind_surface;

    void require_memory(Memory memory) {
        if (!memory)
            throw Error(Boundary::CacheMemory, "texture memory is null");
    }
    std::uint32_t global_index(Memory memory) { return memory->global_index; }
    std::uint32_t palette_global_index(Memory memory) { return memory->global_index; }
    std::uint32_t palette_format(Memory memory) { return memory->pixel_format; }
    std::uint32_t format(Memory memory) { return memory->pixel_format; }
    std::uint32_t palette_bank(Memory memory) { return memory->bank; }
    Surface palette_surface(Memory memory) { return memory->surface; }
    Surface surface(Memory memory) { return memory->surface; }
    bool is_null(const Surface &surface) { return !surface; }
    std::uint32_t palette_bank_at(std::size_t index) { return state.palettes.entries[index].bank; }
    std::uint16_t palette_version_at(std::size_t index) {
        return state.palettes.entries[index].version;
    }
    std::uint16_t memory_palette_version(Memory memory) {
        if (!memory->palette_version)
            throw Error(Boundary::PaletteVersion, "texture palette revision is uninitialized");
        return *memory->palette_version;
    }
    void store_palette_version(Memory memory, std::uint16_t revision) {
        memory->palette_version = revision;
    }
    auto palette_colors(std::size_t index) { return state.palettes.entries[index].colors; }
    void require_palette_surface(const Surface &surface) {
        if (!surface)
            throw Error(Boundary::PaletteSurface, "palette texture surface is null");
    }
    [[noreturn]] void
    stop_palette_upload(const std::shared_ptr<const std::array<std::uint32_t, 256>> &) {
        throw Error(Boundary::PaletteUpload, "indexed palette surface lock/upload is unrecovered");
    }
    std::uint32_t cached_global_index(detail::BindingPath) { return state.cached_global_index; }
    void bind(Memory memory, const Surface &surface, std::uint32_t global_index,
              detail::BindingPath path) {
        if (!bind_surface)
            throw Error(path == detail::BindingPath::Palette ? Boundary::PaletteBinding
                                                             : Boundary::DirectBinding,
                        "texture device binding is unavailable");
        bind_surface(surface);
        // The device callback completes before these stores, ID first, then active memory.
        state.cached_global_index = static_cast<std::int32_t>(global_index);
        state.active_memory = memory;
    }
    std::uint32_t return_value(std::uint32_t index) {
        // DATA: SONIC 0x389d458 TextureFormatResults size=16
        constexpr std::array<std::uint32_t, 4> values{{1, 0, 1, 0}};
        return values[index];
    }
    void store_current_list(List list) { state.current_list = list; }
    List current_list() { return state.current_list; }
    Memory entry(List list, std::uint32_t offset, bool indexed) {
        const auto ordinal = offset / detail::kTextureNameSourceSize;
        if (offset % detail::kTextureNameSourceSize || ordinal >= list->entries.size())
            throw Error(indexed ? Boundary::IndexEntry : Boundary::ListEntry,
                        "texture name array entry is unavailable");
        return list->entries[ordinal];
    }
    void store_requested_memory(Memory memory) { state.requested_memory = memory; }
};
} // namespace
std::uint32_t select_queued_list(const List *list, const List *&queued_list,
                                 std::uint32_t &queued_index, State &state,
                                 const BindSurface &bind_surface) {
    queued_list = list;
    queued_index = 0;
    return select_list(list, state, bind_surface);
}
std::uint32_t select_list(const List *list, State &state, const BindSurface &bind_surface) {
    NativeTextures textures{state, bind_surface};
    return detail::select_list(textures, list);
}
std::uint32_t select_index(std::uint32_t index, State &state, const BindSurface &bind_surface) {
    NativeTextures textures{state, bind_surface};
    return detail::select_index(textures, index);
}
std::uint32_t set_texture_number(std::uint32_t index, State &state,
                                 const BindSurface &bind_surface) {
    const auto offset = index * detail::kTextureNameSourceSize;
    const auto *list = state.current_list;
    if (!list)
        throw Error(Boundary::ListEntry, "texture-number current list is null");
    NativeTextures textures{state, bind_surface};
    const auto memory = textures.entry(list, offset, true);
    return detail::cache(textures, memory);
}
std::uint32_t cache(Memory *memory, State &state, const BindSurface &bind_surface) {
    NativeTextures textures{state, bind_surface};
    return detail::cache(textures, memory);
}
} // namespace sadx::texture_selection
