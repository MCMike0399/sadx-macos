#pragma once
#include <optional>
#include "ninja/ninja_texture_selection.hpp"
#include "cpu/source_bytes.hpp"
#include <array>
#include <cstdint>
namespace sadx {
// The texture memory list pool njInitTexture 77f610 (401830 calls it with
// 3a8b0a0, 0x800) sets up: the 0x44-byte records LoadPVM's 77f5b0/77fbd0 later
// search and fill, their base 3d0f844 and count 3d0f80c, and the texture
// globals it clears. Texlist texaddr words point into these records.
struct NinjaTexturePoolState {
    SourceBytes<0x3a8b0a0,0x22000> records_at_3a8b0a0;     // 0x800 x 0x44
    std::optional<std::uint32_t> base_at_3d0f844,count_at_3d0f80c;
    std::optional<std::uint32_t> word_at_3d0f830,word_at_3d0fa14,word_at_3d0f960;
    std::optional<std::uint8_t> byte_at_3d0f860;
    std::array<std::optional<std::uint32_t>,8> words_at_3d0f7c0;
};
enum class NinjaTexturePoolFailure {none,records_outside_owner};
struct NinjaTexturePoolResult {
    NinjaTexturePoolFailure failure=NinjaTexturePoolFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    explicit operator bool()const noexcept{return failure==NinjaTexturePoolFailure::none;}
};
// Actual 77f610(records, count). 3d0fa24 and 3ce7128 are NinjaTextureSelectionState's (cleared to null).
NinjaTexturePoolResult nj_init_texture_77f610(NinjaTexturePoolState&,NinjaTextureSelectionState&,std::uint32_t records,std::uint32_t count);
} // namespace sadx
