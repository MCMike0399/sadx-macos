#pragma once
#ifndef __OBJC__
#error "metal_sprite.hpp is the Objective-C++ sprite consumer boundary"
#endif
#include "platform/mac/metal_texture.hpp"
#include "ninja/ninja_sprite_queue.hpp"
#include "ninja/ninja_texture_selection.hpp"
#include <map>
namespace sadx {
// A snapshot of the actual D3D8 backend state, separate from the packed Ninja
// blend byte. Missing entries are unknown, never assumed API defaults.
struct MetalSpriteSourceState {
    std::map<std::uint32_t,std::uint32_t> render;
    std::map<std::uint32_t,std::uint32_t> stage_zero;
};
struct MetalSpriteDraw {
    NinjaSpriteDrawCommand command;
    NinjaTextureBindCommand texture_binding; // actual selected surface snapshot
    std::shared_ptr<const MetalTextureResource> texture;
    MetalSpriteSourceState state;
    NinjaQueueRecordHandle source_record; // optional origin, retained through GPU completion
};
enum class MetalSpriteFailure {none,missing_device,missing_queue,device_mismatch,
    unsupported_memory_profile,shader_failed,missing_target,unsupported_target,
    missing_source_state,unsupported_source_state,unsupported_vertices,nonfinite_vertices,
    missing_texture,texture_not_ready,texture_identity_conflict,missing_depth_target,
    allocation_failed,pipeline_failed,encoder_failed,command_failed,not_completed,empty_draws};
class MetalSpriteSubmission;
class MetalSpriteRenderer;
struct MetalSpriteResult {
    MetalSpriteFailure failure=MetalSpriteFailure::none;
    std::string detail;
    std::optional<std::size_t> pending_draw;
    std::optional<std::uint32_t> source_state;
    bool source_state_is_stage_zero=false;
    std::size_t encoded_draws=0;
    std::shared_ptr<MetalSpriteSubmission> submission;
    explicit operator bool()const noexcept{return failure==MetalSpriteFailure::none;}
};
struct MetalSpriteContextResult {
    MetalSpriteFailure failure=MetalSpriteFailure::none;
    std::string detail;
    std::shared_ptr<MetalSpriteRenderer> renderer;
    explicit operator bool()const noexcept{return failure==MetalSpriteFailure::none&&renderer!=nullptr;}
};
class MetalSpriteSubmission {
public:
    ~MetalSpriteSubmission();
    bool completed()const noexcept;
    MetalSpriteFailure wait();
    std::size_t draw_count()const noexcept;
private:
    struct Impl;std::shared_ptr<Impl> impl;
    explicit MetalSpriteSubmission(std::shared_ptr<Impl>);
    friend class MetalSpriteRenderer;
};
class MetalSpriteRenderer {
public:
    ~MetalSpriteRenderer();
    static MetalSpriteContextResult create(NinjaRenderDeviceHandle,id<MTLDevice>,id<MTLCommandQueue>);
    // Caller supplies its real attachment/load/store policy. Supported screen
    // vertices are already source-produced; no depth/project/matrix is invented.
    // A later failure commits an encoded prefix, preserving its real effects.
    MetalSpriteResult submit(std::vector<MetalSpriteDraw>,MTLRenderPassDescriptor*,MTLViewport);
private:
    struct Impl;std::unique_ptr<Impl> impl;
    explicit MetalSpriteRenderer(std::unique_ptr<Impl>);
};
const char* metal_sprite_shader_source()noexcept;
} // namespace sadx
