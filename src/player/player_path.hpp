#pragma once
#include "ninja/ninja_math.hpp"
#include "cpu/crt_floor.hpp"
#include <cstddef>
#include <optional>
#include <vector>
namespace sadx {
struct PlayerPathPoint {
    std::int16_t source_angle0,source_angle2; // original point+0/+2
    float segment_length; // +4
    NinjaVector position; // +8/+12/+16
};
struct PlayerPathResource {
    std::uint8_t type;
    std::uint16_t point_count; // logical source count; physical storage can include sentinels
    float total_length;
    std::vector<PlayerPathPoint> points; // native owned storage, no guest-pointer cast
};
using PathVectorFacts=std::array<std::optional<float>,3>;
struct PlayerPathStatus {
    float distance; // source work+0x10, caller-supplied before query
    std::optional<std::int32_t> angle0={},angle4={},angle8={},anglec={};
    PathVectorFacts position_at14{},position_at20{},direction_at2c{},direction_at38{},tangent_at44{};
};
struct PathQueryConstants {
    CoordinateAxisConstants scalar;
    float range_factor; // source0x7dd57c
};
struct PlayerPathConstants { PathQueryConstants query; CrtFloorConstants floor; };
std::optional<PlayerPathConstants> decode_player_path_constants(const std::uint8_t*,std::size_t);
enum class PathQueryFailure { none, point_outside_storage, floor_unavailable, uninitialized_segment_length };
struct PathQueryResult {
    PathQueryFailure failure=PathQueryFailure::none;
    std::optional<bool> source_return={};
    std::uint32_t source_boundary=0;
    std::optional<std::int32_t> selected_index={};
    std::optional<std::int32_t> unavailable_index={};
    std::optional<float> accumulated_distance={};
    std::optional<CrtFloorResult> floor={};
    std::optional<double> floor_argument={};
    explicit operator bool() const noexcept {return failure==PathQueryFailure::none&&source_return.has_value();}
};
struct PathSegmentEndpoints {
    std::array<std::int16_t,2> begin_angles,end_angles;
    NinjaVector begin_position,end_position;
};
struct PathSegmentResult {
    PathQueryFailure failure=PathQueryFailure::none;
    std::optional<PathSegmentEndpoints> endpoints={};
    std::uint32_t source_boundary=0;
    std::optional<std::int32_t> unavailable_index={};
};
// Source49c210. Caller logical count/type control neighbor indices; unavailable
// physical words fail at the actual read, never clamp to invented endpoints.
PathSegmentResult path_segment_endpoints(const PlayerPathResource&,std::int32_t selected_index);
// Complete local49c330 with real floor/Ninja/CRT owners. Mutates only the source
// status fields written by each path, retaining caller facts on source false.
PathQueryResult query_player_path(const PlayerPathResource&,PlayerPathStatus&,
    const PathQueryConstants&,const CrtFloorConstants&,const CrtFloorRuntime&,
    const NinjaMath&,X87Context&);
} // namespace sadx
