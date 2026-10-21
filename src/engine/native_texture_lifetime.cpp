#include "engine/native_texture_lifetime.hpp"
#include <stdexcept>

namespace sadx::texture_lifetime {
namespace {
// NOTSA: build the defined portion of the loader stack record.
Memory prepared_metadata(std::uint32_t global_index, const void *image_data) {
    Memory memory;
    memory.global_index = static_cast<std::int32_t>(global_index);
    memory.image_data = image_data;
    // The original stack record does not initialize these three fields.
    memory.tsp_parameters.reset();
    memory.texture_parameters.reset();
    memory.palette_version.reset();
    return memory;
}
} // namespace
Memory prepare_pvr(std::uint32_t global_index, const NinjaTextureRecord &record,
                    const void *image_data) {
    if (record.pixel_format > 2 || (record.data_format != 1 && record.data_format != 2 &&
        record.data_format != 3 && record.data_format != 9))
        throw std::runtime_error("native pool preparation requires the recovered 16-bit PVR path");
    auto memory = prepared_metadata(global_index, image_data);
    memory.type = (static_cast<std::uint32_t>(record.pixel_format) |
                   (static_cast<std::uint32_t>(record.data_format) << 8)) << 16;
    memory.pixel_format = static_cast<std::uint32_t>(record.pixel_format) * 134217728U;
    memory.width = record.width;
    memory.height = record.height;
    memory.texture_size = memory.width * memory.height * 2U;
    return memory;
}
Memory prepare_argb_image(std::uint32_t global_index, const void *image_data,
                          std::uint32_t width, std::uint32_t height) {
    auto memory = prepared_metadata(global_index, image_data);
    memory.type = 151388160U;
    memory.pixel_format = 939524096U;
    memory.width = width;
    memory.height = height;
    memory.texture_size = width * height * 4U;
    return memory;
}

Memory *find_memory(std::uint32_t global_index, State &state) {
    for (auto &memory : state.pool) {
        if (static_cast<std::uint32_t>(memory.global_index) == global_index)
            return &memory;
    }
    for (auto &memory : state.pool) {
        if (memory.references == 0)
            return &memory;
    }
    return nullptr;
}
Memory *acquire_prepared(Memory prepared, std::shared_ptr<const graphics::Texture> texture,
                         State &state,
                         std::shared_ptr<const std::vector<std::uint8_t>> source_bytes) {
    auto *memory = find_memory(static_cast<std::uint32_t>(prepared.global_index), state);
    if (memory == nullptr)
        return nullptr;
    if (memory->references == 0) {
        if (memory->surface != nullptr || memory->physical_data != nullptr)
            throw std::runtime_error("zero-count source slot still owns an unrecovered orphan allocation");
        if (texture == nullptr)
            throw std::runtime_error("native texture acquisition requires an uploaded image");
        prepared.references = 0;
        *memory = std::move(prepared);
        memory->surface = std::make_shared<Surface>(Surface{std::move(texture), std::move(source_bytes)});
    }
    ++memory->references;
    return memory;
}
void update_surface(Memory &memory, std::shared_ptr<const graphics::Texture> texture) {
    if (memory.surface == nullptr || texture == nullptr)
        throw std::runtime_error("native texture upload requires a live surface");
    memory.surface->texture = std::move(texture);
}
void release_low(Memory *memory, State &state, Services &services) {
    if (memory == nullptr)
        return;
    if (state.cached_global_index == memory->global_index && state.cached_global_index != -1) {
        services.unbind_texture();
        state.cached_global_index = -1;
        state.active_memory = nullptr;
    }
    if (memory->surface != nullptr)
        memory->surface->texture.reset();
    // The source publishes this clear after releasing the surface, even when it was null.
    memory->surface.reset();
    if (memory->physical_data != nullptr) {
        memory->physical_data->bytes.reset();
        memory->physical_data.reset();
    }
}
namespace {
// NOTSA: the identical field-reset sequence shared by list and pool release.
void reset_memory(Memory &memory) {
    memory.global_index = -1;
    memory.bank = -1;
    memory.tsp_parameters = 0;
    memory.texture_parameters = 0;
    memory.texture_flags = 0;
    memory.references = 0;
    memory.palette_version = 65535;
    memory.image_data = nullptr;
    memory.type = 0;
    memory.bit_depth = 0;
    memory.pixel_format = 0;
    memory.width = 0;
    memory.height = 0;
    memory.texture_size = 0;
    memory.surface_flags = 0;
    memory.surface.reset();
    memory.virtual_data = nullptr;
    memory.physical_data.reset();
}
// NOTSA: the identical reference-decrement sequence shared by both release loops.
void release_reference(Memory &memory, State &state, Services &services) {
    if (memory.references == 0)
        return;
    --memory.references;
    if (memory.references != 0)
        return;
    release_low(&memory, state, services);
    reset_memory(memory);
}
} // namespace
void unbind_cached_texture(State &state, Services &services) {
    if (state.cached_global_index != -1) {
        services.unbind_texture();
        state.cached_global_index = -1;
        state.active_memory = nullptr;
    }
}
std::int32_t release_list(List *list, State &state, Services &services) {
    if (list == nullptr)
        return -1;
    if (state.current_list == list) {
        state.current_list = nullptr;
        unbind_cached_texture(state, services);
    }
    for (auto *memory : list->entries) {
        if (memory == nullptr)
            throw std::runtime_error("njReleaseTexture_ requires each texture memory binding");
        release_reference(*memory, state, services);
    }
    return 1;
}
std::int32_t release_texture_list(List *list, State &state, Services &services) {
    if (list == nullptr || list->entries.empty())
        return 1;
    for (auto *memory : list->entries) {
        if (memory == nullptr)
            return 1;
    }
    if (list->first_filename_invalid)
        return 1;
    return release_list(list, state, services);
}
void release_all(State &state, Services &services) {
    for (auto &memory : state.pool)
        release_reference(memory, state, services);
}
void clear_temporary_info(State &state, Services &services) {
    release_all(state, services);
    state.temporary_info.fill(0);
}
void clear_registered_list_count(std::uint32_t &count) { count = 0; }
void release_scene_textures(State &state, Services &services, std::int32_t &queue_mode,
                            std::uint32_t &registered_list_count) {
    release_all(state, services);
    queue_mode = 2;
    clear_registered_list_count(registered_list_count);
}
} // namespace sadx::texture_lifetime
