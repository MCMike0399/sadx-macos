#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
namespace sadx {
struct TextureByteRange {std::size_t offset=0,size=0;};
struct NinjaTextureChunk {std::array<std::uint8_t,4> tag;TextureByteRange bytes;};
struct NinjaTextureRecord {
    std::size_t ordinal=0;TextureByteRange chunk,payload;
    std::uint8_t pixel_format=0,data_format=0;std::uint16_t reserved=0,width=0,height=0;
    std::optional<std::uint16_t> table_index;
    std::optional<TextureByteRange> table_record;
    std::optional<std::array<std::uint8_t,28>> name;
    std::optional<std::array<std::uint8_t,2>> table_formats,table_dimensions;
    std::optional<std::uint32_t> table_global_index;
    // Every GBIX chunk encountered before this PVRT is retained. The last
    // index is a file-loader fact, distinct from the PVM table global index.
    std::vector<std::size_t> gbix_chunks;
    std::optional<std::uint32_t> gbix_global_index;
};
struct NinjaTextureArchive {
    std::vector<std::uint8_t> bytes; // sole immutable-by-convention asset owner
    std::optional<std::uint16_t> pvm_flags,pvm_count;
    std::optional<TextureByteRange> header,header_padding;
    std::vector<NinjaTextureChunk> chunks;
    std::vector<NinjaTextureRecord> records;
};
enum class TextureFailure {none,invalid_magic,truncated_chunk,invalid_chunk_length,
    truncated_metadata,texture_count_mismatch,missing_gbix_index,invalid_record,
    unsupported_pixel_format,unsupported_data_format,invalid_dimensions,
    truncated_pixels,resource_exhausted};
struct NinjaTextureParseResult {
    TextureFailure failure=TextureFailure::none;std::size_t source_offset=0;
    NinjaTextureArchive archive; // parsed prefix remains available on failure
    explicit operator bool()const noexcept{return failure==TextureFailure::none;}
};
// PVMH or standalone GBIX/PVRT resource. No archive name or format default is
// invented. Metadata flags follow the supplied sonic.exe, not a host enum.
NinjaTextureParseResult parse_ninja_texture_resource(std::vector<std::uint8_t>);
struct NinjaTextureLevel {
    std::uint16_t width=0,height=0;TextureByteRange pixels;
    std::vector<std::uint16_t> packed16; // exact row-major source-format texels
    // Straight-alpha RGBA, integer floor255/max expansion (PuyoTools policy).
    // This does not claim original GPU normalized-color/filtering equivalence.
    std::vector<std::uint8_t> rgba8;
};
struct NinjaTextureDecodeResult {
    TextureFailure failure=TextureFailure::none;std::size_t source_offset=0;
    std::size_t record_index=0;std::optional<TextureByteRange> codebook,prefix_padding,tail;
    std::vector<NinjaTextureLevel> levels; // largest first, including all format2 mips
    explicit operator bool()const noexcept{return failure==TextureFailure::none;}
};
// Needed asset profile: 16-bit1555/565/4444; rectangle9, twiddled1,
// twiddled-mipmaps2, VQ3. Other encodings remain explicit unsupported results.
NinjaTextureDecodeResult decode_ninja_texture(const NinjaTextureArchive&,std::size_t);
} // namespace sadx
