#include "ninja/ninja_sprite.hpp"
#include "cpu/crt_integer.hpp"
#include <cstring>
namespace sadx {
namespace {
std::uint16_t half(const std::uint8_t* p){return static_cast<std::uint16_t>(p[0]|(std::uint16_t(p[1])<<8));}
std::int16_t signed_half(std::uint16_t v){return v<0x8000?static_cast<std::int16_t>(v):static_cast<std::int16_t>(std::int32_t(v)-65536);}
float binary32(const std::uint8_t* p){std::uint32_t w=std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);float f;std::memcpy(&f,&w,4);return f;}
}
std::optional<NinjaSpriteFrame> decode_ninja_sprite_frame(const std::uint8_t* p,std::size_t n){
    if(!p||n!=20)return {};NinjaSpriteFrame result;
    for(unsigned i=0;i<8;++i)result.dimensions_center_uv[i]=signed_half(half(p+i*2));
    result.texture_index=signed_half(half(p+16));result.attributes=signed_half(half(p+18));return result;
}
std::optional<NinjaSpriteConstants> decode_ninja_sprite_constants(const std::uint8_t* p,std::size_t n){
    if(!p||n!=20)return {};return NinjaSpriteConstants{binary32(p),binary32(p+4),binary32(p+8),binary32(p+12),binary32(p+16)};
}
NinjaSpriteGeometryResult ninja_sprite_geometry(const NinjaSpriteInput& input,const NinjaSpriteFrame& frame,
    std::uint32_t flags,const NinjaSpriteConstants& constants,NinjaSpriteProjection& projection,X87Context& fp){
    NinjaSpriteGeometryResult result;auto& g=result.partial;g.flags=flags;std::uint32_t at=0x77e3e2;
    auto stop=[&](NinjaSpriteFailure failure,std::uint32_t site){result.failure=failure;result.source_boundary=site;return result;};
    try {
        const auto& d=frame.dimensions_center_uv;
        g.bounds[0]=fp.to_float(fp.from_int64(-std::int32_t(d[2])));at=0x77e3f9;
        g.bounds[1]=fp.to_float(fp.from_int64(-std::int32_t(d[3])));at=0x77e401;
        g.bounds[2]=fp.to_float(fp.add_float_memory(fp.from_int64(d[0]),*g.bounds[0]));at=0x77e411;
        auto retained_bottom=fp.add_float_memory(fp.from_int64(d[1]),*g.bounds[1]);g.bounds[3]=fp.to_float(retained_bottom);
        NinjaVector position=input.position;
        if(flags&0x10){
            if(!projection.coordinates)return stop(NinjaSpriteFailure::missing_coordinates,0x77e42a);
            at=0x77e42a;projection.coordinates->calculate_point(projection.coordinates->current_matrix(),input.position,position,fp);g.camera_position=position;
            at=0x77e42f;const auto comparison=fp.compare_float_memory(fp.from_float(position.z),constants.zero);
            if(comparison!=X87Comparison::greater){g.clipped=true;return result;}
            if(!projection.scale_at_3d0f9e0)return stop(NinjaSpriteFailure::missing_projection_scale,0x77e444);
            at=0x77e444;const auto ratio=fp.divide_float_memory(fp.from_float(*projection.scale_at_3d0f9e0),position.z);
            at=0x77e44e;const auto scaled_x=fp.multiply(fp.from_float(position.x),ratio);
            if(!projection.center_x_at_3d0fa0c)return stop(NinjaSpriteFailure::missing_center_x,0x77e454);
            at=0x77e454;position.x=fp.to_float(fp.add_float_memory(scaled_x,*projection.center_x_at_3d0fa0c));g.camera_position=position;
            at=0x77e45e;const auto scaled_y=fp.multiply(fp.from_float(position.y),ratio);
            if(!projection.center_y_at_3d0fa10)return stop(NinjaSpriteFailure::missing_center_y,0x77e464);
            at=0x77e464;position.y=fp.to_float(fp.add_float_memory(scaled_y,*projection.center_y_at_3d0fa10));g.camera_position=position;
            for(unsigned i=0;i<3;++i){at=0x77e46e + i*10;g.bounds[i]=fp.to_float(fp.multiply(fp.from_float(*g.bounds[i]),ratio));}
            at=0x77e48c;retained_bottom=fp.multiply_float_memory(ratio,*g.bounds[3]);
        }else g.camera_position=position;
        at=0x77e4a9;g.bounds[0]=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(fp.from_float(*g.bounds[0]),input.scale_x),position.x));
        at=0x77e4c4;g.bounds[1]=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(fp.from_float(*g.bounds[1]),input.scale_y),position.y));
        at=0x77e4db;g.bounds[2]=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(fp.from_float(*g.bounds[2]),input.scale_x),position.x));
        at=0x77e4ea;g.bounds[3]=fp.to_float(fp.add_float_memory(fp.multiply_float_memory(retained_bottom,input.scale_y),position.y));
        const std::uint32_t uv_sites[]={0x77e4f5,0x77e507,0x77e519,0x77e52b};
        for(unsigned i=0;i<4;++i){at=uv_sites[i];g.uv[i]=fp.to_float(fp.multiply_float_memory(fp.from_int64(d[4+i]),constants.uv_scale));}
        if(flags&2){
            if(!projection.color_argb_at_3d0f7f0)return stop(NinjaSpriteFailure::missing_color,0x77e53b);
            std::uint32_t color=0;const std::uint32_t sites[]={0x77e53b,0x77e54c,0x77e561,0x77e576};
            for(unsigned i=0;i<4;++i){at=sites[i];const auto value=fp.multiply_float_memory(fp.from_float((*projection.color_argb_at_3d0f7f0)[3-i]),constants.color_scale);const auto converted=crt_ftol_low32(value,fp)&255U;color|=converted<<(i*8);}
            g.diffuse_argb=color;
        }else g.diffuse_argb=0xffffffffU;
        if(flags&4){at=0x77e59d;const auto loaded=fp.from_float(*g.uv[0]);const auto opposite=*g.uv[2];g.uv[2]=fp.to_float(loaded);g.uv[0]=opposite;}
        if(flags&8){at=0x77e5b2;const auto loaded=fp.from_float(*g.uv[1]);const auto opposite=*g.uv[3];g.uv[3]=fp.to_float(loaded);g.uv[1]=opposite;}
        return result;
    }catch(const CoordinateError& e){result.failure=NinjaSpriteFailure::coordinate_failed;result.source_boundary=at;result.coordinate_failure=e.failure();return result;}
    catch(const X87Error& e){result.failure=NinjaSpriteFailure::fp_failed;result.source_boundary=at;result.fp_failure=e.failure();result.fp_exceptions=e.exceptions();return result;}
}
NinjaSpriteVerticesResult ninja_sprite_vertices(const NinjaSpriteLocalGeometry& g,std::optional<X87Value> depth,X87Context& fp){
    NinjaSpriteVerticesResult result;if(g.clipped)return result;
    for(unsigned i=0;i<4;++i)if(!g.bounds[i]||!g.uv[i]){result.failure=NinjaSpriteFailure::missing_geometry;return result;}
    if(!g.camera_position||!g.diffuse_argb||!g.flags){result.failure=NinjaSpriteFailure::missing_geometry;return result;}
    const bool screen=(*g.flags&0x10)!=0;if(screen && !depth){result.failure=NinjaSpriteFailure::missing_projected_depth;result.source_boundary=0x77e63c;return result;}
    auto store=[&](unsigned word,std::uint32_t value){for(unsigned b=0;b<4;++b)result.partial_bytes[word*4+b]=static_cast<std::uint8_t>(value>>(b*8));result.written_words|=1U<<word;};
    auto raw=[&](unsigned word,float value){std::uint32_t bits;std::memcpy(&bits,&value,4);store(word,bits);};
    const unsigned x[]={0,2,0,2},y[]={1,1,3,3},u[]={0,2,0,2},v[]={1,1,3,3};std::uint32_t at=0;
    try {
        for(unsigned i=0;i<4;++i){const unsigned w=i*(screen?7:6);
            if(screen){at=i==0?0x77e640:i==1?0x77e670:i==2?0x77e694:0x77e6bc;raw(w+2,fp.to_float(*depth));}
            raw(w,*g.bounds[x[i]]);raw(w+1,*g.bounds[y[i]]);
            if(screen){store(w+3,0x3f800000);store(w+4,*g.diffuse_argb);raw(w+5,*g.uv[u[i]]);raw(w+6,*g.uv[v[i]]);}
            else{raw(w+2,g.camera_position->z);store(w+3,*g.diffuse_argb);raw(w+4,*g.uv[u[i]]);raw(w+5,*g.uv[v[i]]);}
        }
    }catch(const X87Error& e){result.failure=NinjaSpriteFailure::fp_failed;result.source_boundary=at;result.fp_failure=e.failure();result.fp_exceptions=e.exceptions();return result;}
    result.command=NinjaSpriteDrawCommand{result.partial_bytes,screen?0x144U:0x142U,5,2,screen?28U:24U};return result;
}
} // namespace sadx
