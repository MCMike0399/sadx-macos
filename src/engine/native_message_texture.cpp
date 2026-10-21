#include "engine/native_message_texture.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace sadx::message_image {
namespace {
struct Upload {
    std::shared_ptr<const std::vector<std::uint8_t>> bytes;
    std::shared_ptr<const graphics::Texture> texture;
};
// NOTSA: replace the D3D A4R4G4B4 linear surface with immutable native BGRA pixels.
// [platform:graphics] Normalized four-bit channels expand exactly by multiplying by 17.
Upload convert(const std::uint16_t *pixels, std::uint32_t width, std::uint32_t height) {
    const auto count = static_cast<std::uint64_t>(width) * height;
    if (!pixels || !width || !height || count > std::numeric_limits<std::uint32_t>::max() / 4U)
        throw std::runtime_error("native MSG image requires a valid linear ARGB4444 extent");
    auto bytes = std::make_shared<std::vector<std::uint8_t>>(count * 2U);
    std::memcpy(bytes->data(), pixels, bytes->size());
    auto texture = std::make_shared<graphics::Texture>();
    texture->width = width;
    texture->height = height;
    texture->bgra.resize(count * 4U);
    for (std::size_t index = 0; index < count; ++index) {
        const auto word = static_cast<std::uint16_t>((*bytes)[index * 2U]) |
                          (static_cast<std::uint16_t>((*bytes)[index * 2U + 1U]) << 8U);
        for (unsigned channel = 0; channel < 4; ++channel)
            texture->bgra[index * 4U + channel] = ((word >> (channel * 4U)) & 15U) * 17U;
    }
    return {std::move(bytes), std::move(texture)};
}
// NOTSA: defined descriptor fields copied by njLoadTexturePVRTAnalize for MSG type 2306.
texture_lifetime::Memory prepare(std::uint32_t global_index, const void *pixels,
                                 std::uint32_t width, std::uint32_t height) {
    texture_lifetime::Memory memory;
    memory.global_index = static_cast<std::int32_t>(global_index);
    memory.tsp_parameters.reset();
    memory.texture_parameters.reset();
    memory.palette_version.reset();
    memory.image_data = pixels;
    memory.type = 151126016;
    memory.pixel_format = 268435456;
    memory.width = width;
    memory.height = height;
    memory.texture_size = width * height * 2U;
    return memory;
}
} // namespace
std::int32_t load(texture_lifetime::List &list, std::uint32_t global_index,
                  const std::uint16_t *pixels, std::uint32_t width, std::uint32_t height,
                  texture_lifetime::State &state, texture_palette::State &palette) {
    texture_palette::select_bank(palette.upload_banks[0], palette);
    auto *memory = texture_lifetime::find_memory(global_index, state);
    if (memory && memory->references != 0) {
        // Source cached acquisition neither reads nor uploads the new image.
        auto prepared = prepare(global_index, pixels, width, height);
        memory = texture_lifetime::acquire_prepared(std::move(prepared), {}, state);
    } else if (memory) {
        const auto upload = convert(pixels, width, height);
        auto prepared = prepare(global_index, upload.bytes->data(), width, height);
        memory = texture_lifetime::acquire_prepared(std::move(prepared), upload.texture, state,
                                                  upload.bytes);
    }
    // Original replacement publishes null on pool exhaustion, then returns success.
    list.entries.assign(1, memory);
    return 1;
}
std::int32_t reload(std::uint32_t global_index, const std::uint16_t *pixels,
                    texture_lifetime::State &state, texture_palette::State &palette) {
    for (auto &memory : state.pool) {
        if (static_cast<std::uint32_t>(memory.global_index) != global_index)
            continue;
        texture_palette::select_bank(memory.bank, palette);
        if (memory.type != 151126016 || memory.pixel_format != 268435456)
            throw std::runtime_error("native MSG reload requires the recovered ARGB4444 descriptor");
        const auto upload = convert(pixels, memory.width, memory.height);
        texture_lifetime::update_surface(memory, upload.texture);
        // The source image descriptor pointer remains unchanged by reload. Its
        // original bitmap owner remains valid; only the surface snapshot changes.
        return 1;
    }
    return -1;
}
} // namespace sadx::message_image
