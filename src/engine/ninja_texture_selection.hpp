#pragma once
#include "engine/ninja_sprite_queue.hpp"
namespace sadx {
struct NinjaTextureSelectionState {
    std::optional<std::shared_ptr<NinjaTextureListOwner>> current_list_at_3d0fa24;
    std::optional<std::shared_ptr<const NinjaTextureMemoryReference>> requested_memory_at_3ce7128,active_memory_at_3d129a0;
    std::optional<std::uint32_t> global_id_at_3d0fdf4;
};
struct NinjaTexturePaletteEntry {
    std::optional<std::uint32_t> id_at_0;
    std::optional<std::uint16_t> version_at_c;
    std::optional<std::shared_ptr<const std::array<std::uint32_t,256>>> colors_at_8;
};
using NinjaTexturePaletteTable=std::array<NinjaTexturePaletteEntry,128>;
struct NinjaTextureBindCommand {
    NinjaRenderDeviceHandle device;std::uint32_t stage=0,source_call=0;
    std::shared_ptr<const NinjaTextureMemoryReference> memory;
    bool surface_is_null=false;
    std::optional<std::uint32_t> source_surface_word;
    std::optional<std::shared_ptr<const void>> surface_owner;
};
struct NinjaTextureSelectionRuntime {
    NinjaRenderState& render;
    const std::array<std::uint32_t,4>* return_table_at_389d458;
    const NinjaTexturePaletteTable* palette_table_at_3d11f80;
    std::function<NinjaRenderCallResult(const NinjaTextureBindCommand&)> set_texture;
};
enum class NinjaTextureSelectionFailure {none,unknown_list,unknown_array,null_array,array_short,
    unsupported_array_word,unknown_memory,null_memory,unknown_global_id,unknown_palette_id,
    unknown_format,unknown_surface,surface_owner_conflict,unknown_cache_id,missing_return_table,
    unknown_palette_table,unknown_palette_entry,unknown_palette_version,unknown_palette_colors,
    null_palette_colors,null_surface,palette_lock_unavailable,unknown_device,null_device,
    missing_renderer,renderer_unavailable};
struct NinjaTextureSelectionResult {
    NinjaTextureSelectionFailure failure=NinjaTextureSelectionFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::optional<std::uint32_t> outcome,array_byte_offset;
    std::optional<std::size_t> selected_ordinal,palette_index;
    std::optional<bool> palette_changed;
    std::vector<NinjaTextureBindCommand> binds;
    std::optional<NinjaTextureBindCommand> pending_bind;
    std::vector<std::optional<std::int32_t>> ignored_hresults;
    explicit operator bool()const noexcept{return failure==NinjaTextureSelectionFailure::none&&outcome.has_value();}
};
NinjaTextureSelectionResult ninja_select_texture_list(const std::optional<std::shared_ptr<NinjaTextureListOwner>>&,NinjaTextureSelectionState&,NinjaTextureSelectionRuntime&); //77f3d0
NinjaTextureSelectionResult ninja_select_texture_index(std::uint32_t,NinjaTextureSelectionState&,NinjaTextureSelectionRuntime&); //77f440, modulo32 byte addressing
NinjaTextureSelectionResult ninja_select_cached_texture(const std::optional<std::shared_ptr<const NinjaTextureMemoryReference>>&,NinjaTextureSelectionState&,NinjaTextureSelectionRuntime&); //78cf20 +78cdc0 closed source paths
NinjaRenderResult ninja_prepare_texture_draw_states(NinjaRenderState&); //78b840
} // namespace sadx
