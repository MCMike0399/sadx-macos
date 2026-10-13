#pragma once
#include "ninja/ninja_math.hpp"

namespace sadx {
struct NinjaPlane { NinjaVector point,normal; };
struct PlayerPlaneConstants {
    CoordinateAxisConstants axis;
    float parallel_cosine; // binary32 0x7e6c30
    double distance_scale; // binary64 0x7e0b40
};
// Source795820 performs source binary32 length/dot spills before comparing
// abs(dot/(length1*length2)) with the source threshold. Zero normals/NaNs
// follow the original arithmetic and comparison, without host normalization.
bool source_planes_parallel(const NinjaPlane&,const NinjaPlane&,
                            const PlayerPlaneConstants&,X87Context&);
// Source7887d0: output, when present, contains intersection point/direction.
// Near-parallel input clears all six output floats then returns the source
// point-to-plane distance; nonparallel input returns source zero. Aliasing
// first/second/output retains the original store/read order.
X87Value intersect_source_planes(const NinjaPlane&,const NinjaPlane&,NinjaPlane* output,
                                const PlayerPlaneConstants&,X87Context&);
} // namespace sadx
