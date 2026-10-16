#pragma once
#include <array>
#include <cstdint>

namespace sadx::runtime {
// D3D8 names describe the recovered table values; native renderers translate by meaning.
enum class BlendTarget : std::uint32_t { Source = 19, Destination = 20 };
enum class BlendFactor : std::uint32_t {
    Zero = 1,
    One,
    SourceColor,
    InverseSourceColor,
    SourceAlpha,
    InverseSourceAlpha,
    DestinationAlpha,
    InverseDestinationAlpha,
    DestinationColor,
    InverseDestinationColor
};
enum class DepthComparison : std::uint32_t {
    Never = 1,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};
enum class KeyTranslationKind : std::uint16_t { Key, Modifier, Ignore };
struct KeyTranslation {
    KeyTranslationKind kind;
    std::uint8_t code; // key usage or modifier mask, according to kind
};
struct SpriteCodeConstants {
    float uv_scale, color_scale, minimum_depth, zero, one;
};
struct ViewCodeConstants {
    float width_scale, height_scale, half, horizontal_extent, vertical_extent;
};
// NOTSA: immutable code tables grouped for session initialization. These contain no
// asset pointers, fixture availability, original layouts or executable identities.
struct EngineCodeTables {
    std::array<BlendTarget, 2> blend_targets;
    std::array<BlendFactor, 12> blend_modes;
    std::array<DepthComparison, 8> depth_comparisons;
    std::array<KeyTranslation, 237> keyboard; // indexed by the polled scan code
    std::array<float, 16> unit_matrix;
    SpriteCodeConstants sprite;
    float negative_one, fog_disabled;
    ViewCodeConstants view;
};
// NOTSA: maintained source defaults; the reference adapter may instead decode injected facts.
EngineCodeTables engine_code_tables();
} // namespace sadx::runtime
