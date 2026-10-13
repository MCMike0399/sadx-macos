#include "engine/ninja_projection_range.hpp"
#include <cstring>

namespace sadx {
namespace {
struct Stop{std::uint32_t at;NinjaProjectionRangeFailure failure;};
}
NinjaProjectionRangeResult ninja_clamp_projection_range_7815e0(float a,float b,NinjaProjectionRangeState& s,X87Context& fp){
    NinjaProjectionRangeResult r;std::uint32_t site=0x7815e0;
    try{
        // FCOMPS: only "greater" falls through 7815ef; only "less" falls through 78160a.
        site=0x7815e4;auto c=fp.compare_float_memory(fp.from_float(a),s.negative_one_at_7dcb0c);
        if(c==X87Comparison::greater){std::uint32_t bits=0xbf800000U;std::memcpy(&a,&bits,4);}
        else{site=0x7815ff;if(fp.compare_float_memory(fp.from_float(a),s.limit_at_7dce50)==X87Comparison::less){std::uint32_t bits=0xc77fff00U;std::memcpy(&a,&bits,4);}}
        site=0x781614;auto far=fp.from_float(b);site=0x781618;
        if(fp.compare_float_memory(far,s.negative_one_at_7dcb0c)==X87Comparison::greater){site=0x781627;far=fp.from_float(s.negative_one_at_7dcb0c);}
        else{site=0x781642;if(fp.compare_float_memory(far,s.limit_at_7dce50)==X87Comparison::less){site=0x781651;far=fp.from_float(s.limit_at_7dce50);}}
        site=0x78162d;const auto near=fp.from_float(a).negated();site=0x781633;s.near_at_3d0f808=fp.to_float(near);
        site=0x78163b;s.far_at_3d0f968=fp.to_float(far.negated());
        r.completed=true;
    }catch(const X87Error& e){r.failure=NinjaProjectionRangeFailure::fp_failed;r.fp_failure=e.failure();r.source_boundary=site;}
    return r;
}
NinjaProjectionRangeResult ninja_set_projection_range_403180(float near_value,float far_value,NinjaProjectionRangeState& s,const NinjaTransformOwner& t,X87Context& fp){
    auto r=ninja_clamp_projection_range_7815e0(near_value,far_value,s,fp);if(!r)return r;r.completed=false;
    std::uint32_t site=0x403192;
    const auto need=[&](const std::optional<float>& v,std::uint32_t at){if(!v)throw Stop{at,NinjaProjectionRangeFailure::unknown_fact};return *v;};
    try{
        const auto x=fp.from_float(need(s.near_at_3d0f808,0x403192)).negated();
        site=0x40319d;const auto y=fp.from_float(need(s.far_at_3d0f968,0x40319d)).negated();site=0x4031a5;const float yf=fp.to_float(y);
        std::uint32_t ybits;std::memcpy(&ybits,&yf,4);site=0x4031ad;s.x_at_90082c=fp.to_float(x);s.y_bits_at_900830=ybits;
        site=0x4031b9;const auto cx=fp.from_float(need(s.cache_x_at_3aad0c0,0x4031b9));site=0x4031bf;
        bool changed=fp.compare(cx,x)!=X87Comparison::equal;
        if(!changed){site=0x4031c8;const auto cy=fp.from_float(need(s.cache_y_at_3aad0c4,0x4031c8));site=0x4031ce;changed=fp.compare_float_memory(cy,yf)!=X87Comparison::equal;}
        if(changed){
            site=0x4031dd;s.cache_x_at_3aad0c0=fp.to_float(x);site=0x4031e9;const float l44=fp.to_float(x.negated());
            site=0x4031ee;const auto ny=fp.from_float(yf).negated();float yraw;std::memcpy(&yraw,&ybits,4);s.cache_y_at_3aad0c4=yraw;
            site=0x403200;const float l48=fp.to_float(ny);
            if(!t.get)throw Stop{0x403207,NinjaProjectionRangeFailure::transform_unavailable};
            auto matrix=t.get(3);if(!matrix)throw Stop{0x403207,NinjaProjectionRangeFailure::transform_unavailable};
            site=0x40320d;auto st=fp.from_float(l48);site=0x403211;st=fp.subtract_float_memory(st,l44);
            site=0x40321f;const auto q=fp.reverse_divide_float_memory(st,l48);site=0x403226;(*matrix)[10]=fp.to_float(q);
            site=0x40322a;const auto p=fp.multiply_float_memory(q,l44).negated();site=0x403230;(*matrix)[14]=fp.to_float(p);
            if(!t.set||!t.set(3,*matrix))throw Stop{0x403236,NinjaProjectionRangeFailure::transform_unavailable};
            r.transform_updated=true;
        }
        r.completed=true;
    }catch(const Stop& stop){r.failure=stop.failure;r.source_boundary=stop.at;}
    catch(const X87Error& e){r.failure=NinjaProjectionRangeFailure::fp_failed;r.fp_failure=e.failure();r.source_boundary=site;}
    return r;
}
} // namespace sadx
