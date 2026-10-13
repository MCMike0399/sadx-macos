#include "player/player_mesh_math.hpp"

namespace sadx {
void mesh_triangle_normal(const NinjaVector& first,const NinjaVector& second,
    const NinjaVector& third,NinjaVector& output,X87Context& fp) {
    const auto a=first,b=second,c=third;
    const auto subtract=[&](float x,float y){return fp.to_float(fp.subtract_float_memory(fp.from_float(x),y));};
    const NinjaVector edge_b{subtract(b.x,a.x),subtract(b.y,a.y),subtract(b.z,a.z)};
    const NinjaVector edge_c{subtract(a.x,c.x),subtract(a.y,c.y),subtract(a.z,c.z)};
    const auto product=[&](float x,float y){return fp.multiply_float_memory(fp.from_float(x),y);};
    const auto x_first=product(edge_c.y,edge_b.z);
    const auto x_second=product(edge_c.z,edge_b.y);
    output.x=fp.to_float(fp.subtract(x_first,x_second));
    const auto y_first=product(edge_c.z,edge_b.x);
    const auto y_second=product(edge_c.x,edge_b.z);
    output.y=fp.to_float(fp.subtract(y_first,y_second));
    const auto z_first=product(edge_c.x,edge_b.y);
    const auto z_second=product(edge_c.y,edge_b.x);
    output.z=fp.to_float(fp.subtract(z_first,z_second));
}
namespace {
MeshNormalizationResult normalize(bool up,NinjaVector& vector,const CoordinateAxisConstants& constants,
    const CrtAsinRuntime& runtime,X87Context& fp) {
    const auto z=fp.multiply_float_memory(fp.from_float(vector.z),vector.z);
    const auto y=fp.multiply_float_memory(fp.from_float(vector.y),vector.y);
    const auto x=fp.multiply_float_memory(fp.from_float(vector.x),vector.x);
    const auto xy=fp.add(y,x);const auto sum=fp.add(z,xy);
    MeshNormalizationResult result;result.squared_spill=fp.to_float(sum);
    result.sqrt_argument=fp.to_double(sum);result.sqrt=crt_sqrt(*result.sqrt_argument,runtime,fp);
    if(result.sqrt->failure!=CrtAsinFailure::none || !result.sqrt->value){result.source_boundary=up?0x450a05:0x450955;return result;}
    (void)fp.to_float(*result.sqrt->value); // actual first FST32, before FSTP32
    result.length=fp.to_float(*result.sqrt->value);
    const auto compared=fp.compare_float_memory(fp.from_float(*result.length),constants.zero);
    if(compared==X87Comparison::equal) {
        if(up){vector.z=0;vector.x=0;vector.y=1;}
        else{vector.z=0;vector.y=0;vector.x=0;}
    } else {
        result.reciprocal=fp.to_float(fp.divide_float_memory(fp.from_float(constants.one),*result.length));
        vector.x=fp.to_float(fp.multiply_float_memory(fp.from_float(*result.reciprocal),vector.x));
        vector.y=fp.to_float(fp.multiply_float_memory(fp.from_float(*result.reciprocal),vector.y));
        vector.z=fp.to_float(fp.multiply_float_memory(fp.from_float(*result.reciprocal),vector.z));
    }
    result.completed=true;return result;
}
}
MeshNormalizationResult normalize_mesh_vector_zero(NinjaVector& vector,const CoordinateAxisConstants& constants,
    const CrtAsinRuntime& runtime,X87Context& fp){return normalize(false,vector,constants,runtime,fp);}
MeshNormalizationResult normalize_mesh_vector_up(NinjaVector& vector,const CoordinateAxisConstants& constants,
    const CrtAsinRuntime& runtime,X87Context& fp){return normalize(true,vector,constants,runtime,fp);}
} // namespace sadx
