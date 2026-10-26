#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace sadx::texture_selection::detail {
// NOTSA: resource access keeps reference-only absent facts out of the shared game algorithm.
enum class BindingPath { Palette, Direct };
constexpr std::uint32_t kIndexedPixelFormat = 0x28000000;
constexpr std::uint32_t kNoPaletteBank = 0xffffffff;
constexpr std::uint32_t kTextureNameSourceSize = 12;
constexpr std::size_t kPaletteCount = 128;
constexpr unsigned kFormatResultShift = 27;
constexpr std::uint32_t kFormatResultMask = 3;

// The upload boundary deliberately stops after publishing a changed revision.
template <class Textures>
// STUB: SONIC 0x78cdc0 Direct3D_ProbablyAppliesPalette prefix through palette surface lock
bool apply_palette(Textures &textures, const typename Textures::Memory &memory) {
    const auto format = textures.palette_format(memory);
    const auto bank = textures.palette_bank(memory);
    const auto global_index = textures.palette_global_index(memory);
    if (format != kIndexedPixelFormat)
        return false;
    auto surface = textures.palette_surface(memory);
    if (bank != kNoPaletteBank) {
        for (std::size_t index = 0; index < kPaletteCount; ++index) {
            if (textures.palette_bank_at(index) != bank)
                continue;
            const auto revision = textures.palette_version_at(index);
            if (revision != textures.memory_palette_version(memory)) {
                textures.store_palette_version(memory, revision);
                const auto colors = textures.palette_colors(index);
                textures.require_palette_surface(surface);
                textures.stop_palette_upload(colors);
            }
            break;
        }
    }
    // Unlike direct binding, this path passes a null surface to the device too.
    if (textures.cached_global_index(BindingPath::Palette) != global_index)
        textures.bind(memory, surface, global_index, BindingPath::Palette);
    return false;
}

template <class Textures>
std::uint32_t cache(Textures &textures, typename Textures::Memory memory) {
    textures.require_memory(memory);
    const auto global_index = textures.global_index(memory);
    const bool changed = apply_palette(textures, memory);
    if (changed || textures.cached_global_index(BindingPath::Direct) != global_index) {
        auto surface = textures.surface(memory);
        if (!textures.is_null(surface))
            textures.bind(memory, surface, global_index, BindingPath::Direct);
    }
    return textures.return_value((textures.format(memory) >> kFormatResultShift) &
                                 kFormatResultMask);
}

template <class Textures>
std::uint32_t select_list(Textures &textures, typename Textures::List list) {
    textures.store_current_list(list);
    if (list) {
        const auto memory = textures.entry(list, 0, false);
        textures.store_requested_memory(memory);
        cache(textures, memory);
    }
    return 1;
}

template <class Textures> std::uint32_t select_index(Textures &textures, std::uint32_t index) {
    const auto list = textures.current_list();
    if (!list)
        return kNoPaletteBank;
    // Original NJS_TEXNAME byte addressing wraps before the pointer read.
    const auto byte_offset = index * kTextureNameSourceSize;
    const auto memory = textures.entry(list, byte_offset, true);
    textures.store_requested_memory(memory);
    cache(textures, memory);
    return 1;
}

// NOTSA: share the recovered draw-state sequence with the reference receipt adapter.
template <class SetState> void prepare_draw_states(SetState set_state) {
    constexpr std::uint32_t kCullMode = 22, kLighting = 137, kColorVertex = 145;
    constexpr std::uint32_t kCullNone = 1;
    const std::array<std::uint32_t, 3> states{{kCullMode, kLighting, kColorVertex}};
    const std::array<std::uint32_t, 3> values{{kCullNone, 0, 1}};
    for (std::size_t index = 0; index < states.size(); ++index)
        set_state(index, states[index], values[index]);
}
} // namespace sadx::texture_selection::detail
