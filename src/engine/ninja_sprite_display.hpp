#pragma once
#include "engine/ninja_texture_selection.hpp"
namespace sadx {
struct NinjaSpriteDepthResult {
    std::optional<X87Value> value;
    std::uint32_t source_boundary=0;
    explicit operator bool()const noexcept{return value.has_value();}
};
struct NinjaSpriteVertexFormatCommand {NinjaRenderDeviceHandle device;std::uint32_t format,source_call;};
struct NinjaSpriteIssueCommand {
    NinjaRenderDeviceHandle device;
    NinjaSpriteDrawCommand vertices;
    std::optional<std::shared_ptr<const NinjaTextureMemoryReference>> active_texture;
    NinjaQueueRecordHandle source_record;
    std::uint32_t source_call;
};
struct NinjaSpriteDisplayRuntime {
    NinjaSpriteProjection& projection;
    NinjaTextureSelectionState& textures;
    NinjaTextureSelectionRuntime& texture_runtime;
    const NinjaSpriteConstants& constants;
    // Real78dbe0 required when source Z comparison takes the projection path.
    // There is no identity/host divide/default depth provider.
    std::function<NinjaSpriteDepthResult(float,X87Context&)> projected_depth;
    std::function<NinjaRenderCallResult(const NinjaSpriteVertexFormatCommand&)> set_vertex_format;
    std::function<NinjaRenderCallResult(const NinjaSpriteIssueCommand&)> draw;
};
enum class NinjaSpriteDisplayFailure {none,unknown_input,unknown_texture_list,missing_animation,
    unsupported_animation_offset,animation_short,texture_failed,geometry_failed,render_failed,
    world_transform_unavailable,depth_unavailable,fp_failed,unknown_device,null_device,
    missing_vertex_backend,vertex_backend_unavailable,missing_draw_backend,draw_backend_unavailable};
struct NinjaSpriteDisplayResult {
    NinjaSpriteDisplayFailure failure=NinjaSpriteDisplayFailure::none;
    std::uint32_t source_boundary=0,nested_source_boundary=0;
    std::vector<NinjaTextureSelectionResult> texture_calls;
    std::optional<NinjaSpriteGeometryResult> geometry;
    NinjaRenderResult render;
    NinjaRenderStageResult stage;
    std::optional<float> reciprocal;
    std::optional<NinjaSpriteDepthResult> depth;
    std::optional<NinjaSpriteVerticesResult> vertices;
    std::optional<NinjaSpriteVertexFormatCommand> format;
    std::optional<NinjaSpriteIssueCommand> issue;
    std::optional<std::int32_t> ignored_format_hresult,ignored_draw_hresult;
    std::optional<X87Failure> fp_failure;
    std::uint16_t fp_exceptions=0;
    bool returned=false;
    explicit operator bool()const noexcept{return failure==NinjaSpriteDisplayFailure::none&&returned;}
};
// Actual77e390 screen branch, including texture selection before numeric
// locals, ordered state/vertex/draw calls and verified78c5a0 RET. World branch
// preserves the precise unlifted transform boundary, never fake completion.
NinjaSpriteDisplayResult ninja_display_sprite(const std::optional<std::shared_ptr<const NinjaQueuedSpriteInput>>&,
    std::int32_t animation_index,std::uint32_t flags,NinjaQueueRecordHandle,
    NinjaSpriteDisplayRuntime&,X87Context&);
} // namespace sadx
