#pragma once
#include "engine/native_texture_lifetime.hpp"
#include "engine/native_texture_palette.hpp"

namespace sadx::message_image {
// NOTSA: bounded native device bridge for the MSG memory/global-index ARGB4444 image.
// State is borrowed from Display; there is no separate texture pool.
std::int32_t load(texture_lifetime::List &, std::uint32_t global_index,
                  const std::uint16_t *pixels, std::uint32_t width, std::uint32_t height,
                  texture_lifetime::State &, texture_palette::State &);
// FUNCTION: SONIC 0x77fa10 njReLoadTextureNumG
std::int32_t reload(std::uint32_t global_index, const std::uint16_t *pixels,
                    texture_lifetime::State &, texture_palette::State &);
} // namespace sadx::message_image
