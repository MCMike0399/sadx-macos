#pragma once
#include "engine/ninja_sprite_display.hpp"
namespace sadx {
struct NinjaQueueConsumerState {
    std::optional<std::uint8_t> special_flags_at_909eb4,fog_profile_at_3abdc6c,mode_byte_at_3abdbc0;
    std::optional<std::int32_t> count_at_3abd9d4,maximum_at_3abd9d8,count_at_3abd9dc,maximum_at_3abd9e0;
    std::optional<float> projection_width_at_3d0f9e4;
    std::optional<std::uint32_t> fog_enabled_at_3d12798,fog_supported_at_3d128a4,fog_mode_at_3d0849c;
    std::optional<float> cached_fog_start_at_90a098,cached_fog_end_at_90a09c;
};
struct NinjaQueueConsumerConstants {float negative_one_at_7dcb0c,fog_sentinel_at_7dce50;};
std::optional<NinjaQueueConsumerConstants> decode_ninja_queue_consumer_constants(const std::uint8_t*,std::size_t);
enum class NinjaQueueConsumerFailure {none,unknown_fact,queue_failed,render_failed,texture_failed,
    display_failed,unlifted_callee,missing_coordinates,coordinate_failed,fp_failed,
    missing_next,foreign_record,sentinel_mismatch,cycle,missing_sprite_owner,owner_mismatch,unlifted_dispatch};
struct NinjaQueueColorResult {
    std::array<std::optional<float>,4> source_stores;
    std::uint32_t source_boundary=0;
};
struct NinjaQueueConsumedRecord {
    NinjaQueueRecordHandle record;
    std::size_t bucket=0;
    std::int32_t pass=0;
    bool skipped_by_pass=false;
    std::optional<std::uint8_t> flags;
    std::optional<std::uint16_t> attribute;
    std::optional<NinjaTextureListResult> header_texture_gate,sprite_texture_gate;
    std::optional<NinjaTextureSelectionResult> header_texture_selection;
    std::optional<NinjaSpriteDisplayResult> sprite_display;
};
struct NinjaQueueConsumerRuntime {
    NinjaSpriteQueue& queue;
    NinjaSpriteQueueRuntime& queue_runtime;
    NinjaQueueConsumerState& state;
    NinjaSpriteDisplayRuntime& sprite;
    const NinjaQueueConsumerConstants& constants;
    const std::optional<std::array<float,4>>& initial_color_at_3b18230;
    const std::array<std::uint32_t,8>* depth_comparison_table;
};
struct NinjaQueueConsumerResult {
    NinjaQueueConsumerFailure failure=NinjaQueueConsumerFailure::none;
    std::uint32_t source_boundary=0,callee_boundary=0;
    std::optional<std::uint32_t> required_fact;
    NinjaRenderResult render;
    std::vector<NinjaQueueColorResult> colors;
    std::vector<NinjaQueueConsumedRecord> records;
    std::optional<NinjaQueueFailure> queue_failure;
    std::optional<CoordinateFailure> coordinate_failure;
    std::optional<X87Failure> fp_failure;
    std::uint16_t fp_exceptions=0;
    bool returned=false;
    explicit operator bool()const noexcept{return failure==NinjaQueueConsumerFailure::none&&returned;}
};
// Actual 7815c0 (with 787e30): scale 3d0f9e0 = width 3d0f9e4 / (2*tan(angle/2)),
// tan as f32 sine (788290, spilled) over extended cosine (7883c0) with the
// source table and the caller's x87 context. Missing coordinates or width stop.
struct NinjaFovResult {std::uint32_t source_boundary=0;std::optional<X87Failure> fp_failure;bool completed=false;explicit operator bool()const noexcept{return completed;}};
NinjaFovResult ninja_set_fov_7815c0(std::int32_t angle,const NinjaMath::SineTable& table_at_88af78,NinjaSpriteProjection&,const std::optional<float>& width_at_3d0f9e4,X87Context&);
// Actual 402f40: copy the colour into 3ab9864 store by store, clamp each
// component to [-1,1] with the source compares, then 77ff90's raw copy into
// the projection colour 3d0f7f0. Returns false (nothing stored) when the
// input is unknown; X87Error propagates with fp_site at the faulting load or compare.
struct NinjaQueueColorOwners {
    std::optional<std::array<float,4>>& color_at_3ab9864;
    std::optional<std::array<float,4>>& projection_color_at_3d0f7f0;
    float one_at_7dcb10,negative_one_at_7dcb0c;
};
bool ninja_queue_color_402f40(const std::optional<std::array<float,4>>& input,NinjaQueueColorOwners&,NinjaQueueColorResult& partial,X87Context&,std::uint32_t& fp_site);
// Actual4086f0 consumption. Supported type3 and empty/default records use real
// native bodies. Unlifted draw/fog branches stop at their exact required callee
// after prior stores; no skipped unknown dispatch or renderer success fallback.
NinjaQueueConsumerResult ninja_consume_queue(NinjaQueueConsumerRuntime&,X87Context&);
// NOTSA: compose queue fog with the selected fog storage and device diagnostics.
bool update_queue_fog(bool enabled, NinjaQueueConsumerRuntime &, X87Context &,
                      NinjaQueueConsumerResult &, std::uint32_t &last_fp_site);
} // namespace sadx
