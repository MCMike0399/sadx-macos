#pragma once
#include "player/sonic_animation_table.hpp"
#include "player/player_scene_decode.hpp"
namespace sadx {
enum class AnimationDecodeFailure {none,invalid_pe,missing_export,unavailable_range,forwarded_export,resource_identity_overflow,inconsistent_motion_layout};
struct DecodedPlayerAnimations {
    SonicAnimationExport sonic_actions;
    PlayerAnimationResources resources;
    std::unordered_map<std::size_t,SceneGeometryObject> objects;
    std::unordered_map<std::size_t,SceneArtifactLocation> locations;
    std::uint32_t export_source_address=0;
    std::size_t next_resource_id=0;
};
struct AnimationDecodeResult {
    AnimationDecodeFailure failure=AnimationDecodeFailure::none;
    std::uint32_t requested_source_address=0;
    std::unique_ptr<DecodedPlayerAnimations> decoded;
    explicit operator bool()const noexcept{return failure==AnimationDecodeFailure::none&&bool(decoded);}
};
// Reads the caller-selected PE32 artifact without loading/executing it. The
// caller assigns an artifact namespace via first_resource_id and retains SHA.
// All149 slots are required by actual7cec90. Graph/key records preserve bits.
// Model payloads remain the model decoder's boundary; object.model identity
// is retained, never replaced by a manufactured renderable model.
AnimationDecodeResult decode_sonic_animations(const std::uint8_t*,std::size_t,std::size_t first_resource_id);
}
