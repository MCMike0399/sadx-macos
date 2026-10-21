#pragma once
#include "engine/graphics_frame.hpp"
#include "engine/ninja_texture.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sadx::texture_lifetime {
// [platform:texture] One manager reference to the native equivalent of a D3D texture.
// Submitted graphics::Draw values retain the immutable image independently.
struct Surface {
    std::shared_ptr<const graphics::Texture> texture;
    // NOTSA: keep the native source-image view valid until this manager owner releases it.
    std::shared_ptr<const std::vector<std::uint8_t>> source_bytes;
};
struct PhysicalAllocation {
    std::unique_ptr<std::vector<std::uint8_t>> bytes;
};
// NJS_TEXMEMLIST, expressed with native resource owners instead of source pointer words.
struct Memory {
    std::int32_t global_index = -1, bank = -1;
    std::optional<std::uint32_t> tsp_parameters = 0, texture_parameters = 0;
    std::uint32_t texture_flags = 0;
    const void *image_data = nullptr;
    std::uint32_t type = 0, bit_depth = 0, pixel_format = 0;
    std::uint32_t width = 0, height = 0, texture_size = 0, surface_flags = 0;
    std::shared_ptr<Surface> surface;
    const void *virtual_data = nullptr;
    std::shared_ptr<PhysicalAllocation> physical_data;
    std::uint16_t references = 0;
    std::optional<std::uint16_t> palette_version = 65535;
};
using Filename = std::shared_ptr<const std::string>;
struct List {
    std::vector<Memory *> entries;
    bool first_filename_invalid = false;
    // Source NJS_TEXNAME members. Some older native callers have not recovered
    // filename construction yet; release never reads this metadata.
    std::vector<Filename> filenames{};
    std::vector<std::uint32_t> attributes{};
};
struct State {
    std::vector<Memory> &pool;
    std::int32_t &cached_global_index;
    Memory *&active_memory;
    const List *&current_list;
    // Only clearing is recovered; wider temporary-info contents remain opaque.
    std::array<std::uint32_t, 8> &temporary_info;
};
struct Services {
    std::function<void()> unbind_texture;
};
// FUNCTION: SONIC 0x77f5b0 njSearchEmptyTexMemList
Memory *find_memory(std::uint32_t global_index, State &);
// NOTSA: prepare the defined metadata for the current 16-bit PVR asset closure.
// The caller passes the PVM table index and the actual native source-image view.
Memory prepare_pvr(std::uint32_t global_index, const NinjaTextureRecord &, const void *image_data);
// NOTSA: the zero-initialized memory-image descriptor used by the movie caller,
// after njSetTextureInfo and njLoadTexturePVRTAnalize, for direct ARGB8888 upload.
Memory prepare_argb_image(std::uint32_t global_index, const void *image_data,
                          std::uint32_t width, std::uint32_t height);
// NOTSA: post-parse acquisition shared by the PVR/GVR loaders, with native upload.
// prepared carries actual parsed metadata; genuinely uninitialized stack fields
// are nullopt, not asserted zero. Immutable Draw snapshots retain older uploads.
Memory *acquire_prepared(Memory prepared, std::shared_ptr<const graphics::Texture>, State &,
                         std::shared_ptr<const std::vector<std::uint8_t>> source_bytes = {});
// NOTSA: update the existing native surface after a movie texture upload.
void update_surface(Memory &, std::shared_ptr<const graphics::Texture>);
// FUNCTION: SONIC 0x78cd50 njReleaseTextureLow
void release_low(Memory *, State &, Services &);
// FUNCTION: SONIC 0x78bba0 FUN_0078bba0
void unbind_cached_texture(State &, Services &);
// FUNCTION: SONIC 0x77fa70 njReleaseTexture_
std::int32_t release_list(List *, State &, Services &);
// FUNCTION: SONIC 0x403290 njReleaseTexture
std::int32_t release_texture_list(List *, State &, Services &);
// FUNCTION: SONIC 0x77fb30 njReleaseTextureAll
void release_all(State &, Services &);
// FUNCTION: SONIC 0x77fc50 FUN_0077fc50
void clear_temporary_info(State &, Services &);
// FUNCTION: SONIC 0x40d3b0 FUN_0040d3b0
void clear_registered_list_count(std::uint32_t &);
// FUNCTION: SONIC 0x406f00 njReleaseTextureAll_
void release_scene_textures(State &, Services &, std::int32_t &queue_mode,
                            std::uint32_t &registered_list_count);
} // namespace sadx::texture_lifetime
