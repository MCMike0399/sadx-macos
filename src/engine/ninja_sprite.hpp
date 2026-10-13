#pragma once
#include "engine/ninja_matrix.hpp"
#include <array>
#include <optional>
namespace sadx {
struct NinjaSpriteFrame {
    std::array<std::int16_t,8> dimensions_center_uv;
    std::int16_t texture_index,attributes;
};
std::optional<NinjaSpriteFrame> decode_ninja_sprite_frame(const std::uint8_t*,std::size_t);
struct NinjaSpriteConstants {float uv_scale,color_scale,minimum_depth,zero,one;};
std::optional<NinjaSpriteConstants> decode_ninja_sprite_constants(const std::uint8_t*,std::size_t);
struct NinjaSpriteInput {NinjaVector position;float scale_x,scale_y;std::int32_t angle;};
struct NinjaSpriteProjection {
    NinjaMatrixPrimitives* coordinates=nullptr; // borrowed real current matrix owner
    std::optional<float> scale_at_3d0f9e0,center_x_at_3d0fa0c,center_y_at_3d0fa10;
    std::optional<std::array<float,4>> color_argb_at_3d0f7f0;
};
enum class NinjaSpriteFailure {none,missing_coordinates,coordinate_failed,missing_projection_scale,missing_center_x,missing_center_y,missing_color,fp_failed,missing_projected_depth,missing_geometry};
struct NinjaSpriteLocalGeometry {
    std::array<std::optional<float>,4> bounds; // left,top,right,bottom source locals
    std::array<std::optional<float>,4> uv; // source u0,v0,u1,v1
    std::optional<NinjaVector> camera_position;
    std::optional<std::uint32_t> diffuse_argb;
    std::optional<std::uint32_t> flags;
    bool clipped=false;
};
struct NinjaSpriteGeometryResult {
    NinjaSpriteFailure failure=NinjaSpriteFailure::none;
    std::uint32_t source_boundary=0;
    NinjaSpriteLocalGeometry partial;
    std::optional<X87Failure> fp_failure;
    std::optional<CoordinateFailure> coordinate_failure;
    std::uint16_t fp_exceptions=0;
    explicit operator bool()const noexcept{return failure==NinjaSpriteFailure::none;}
};
// Exact77e3c9..77e5c2 numeric locals. Caller must perform real texture-list /
// texture-index binding first; this API does not claim those calls succeeded.
// Sprite.angle is not consumed by the actual77e390 body and is not invented.
NinjaSpriteGeometryResult ninja_sprite_geometry(const NinjaSpriteInput&,const NinjaSpriteFrame&,
    std::uint32_t flags,const NinjaSpriteConstants&,NinjaSpriteProjection&,X87Context&);
struct NinjaSpriteDrawCommand {
    std::array<std::uint8_t,112> vertex_bytes;
    std::uint32_t vertex_format,primitive_type=5,primitive_count=2,vertex_stride;
};
struct NinjaSpriteVerticesResult {
    NinjaSpriteFailure failure=NinjaSpriteFailure::none;
    std::uint32_t source_boundary=0;
    std::array<std::uint8_t,112> partial_bytes{};
    std::uint32_t written_words=0;
    std::optional<NinjaSpriteDrawCommand> command;
    std::optional<X87Failure> fp_failure;
    std::uint16_t fp_exceptions=0;
    explicit operator bool()const noexcept{return failure==NinjaSpriteFailure::none;}
};
// Actual77e63c..77e703 /77e733..77e7ff vertex stores and DrawPrimitiveUP
// shape, after required source render-state/transform/depth owners. For screen
// vertices depth is the real unrounded78dbe0 return, or explicit FLDzero path.
// A missing depth never becomes a guessed0,1/z or host projection substitute.
NinjaSpriteVerticesResult ninja_sprite_vertices(const NinjaSpriteLocalGeometry&,
    std::optional<X87Value> projected_depth,X87Context&);
} // namespace sadx
