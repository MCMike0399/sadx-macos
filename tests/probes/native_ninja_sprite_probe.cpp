#include "ninja/ninja_sprite.hpp"
#include <cstring>
namespace {
float f(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float f){std::uint32_t w;std::memcpy(&w,&f,4);return w;}
}
extern "C" int sadx_native_ninja_sprite(const std::uint8_t* constants,std::size_t size,const float* table,std::size_t table_size,
    const float* unit,const float* matrix,const std::uint32_t* w,std::size_t words,std::uint16_t cw,std::uint32_t* out,std::size_t count,std::uint8_t* vertices,std::size_t vertex_bytes){
    if(!constants||size!=40||!table||table_size!=1025||!unit||!matrix||!w||words!=16||!out||count!=32||!vertices||vertex_bytes!=112)return 1;
    try{
        auto frame=sadx::decode_ninja_sprite_frame(constants,20);auto values=sadx::decode_ninja_sprite_constants(constants+20,20);if(!frame||!values)return 2;
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);sadx::NinjaMatrix source_unit,current;std::memcpy(source_unit.data(),unit,64);std::memcpy(current.data(),matrix,64);
        sadx::NinjaMatrixStack stack({current},1,0);sadx::PlayerCoordinates coordinates(sine,source_unit,stack);sadx::X87Context fp(cw);sadx::NinjaSpriteProjection projection;
        if(w[7]&1)projection.coordinates=&coordinates;if(w[7]&2)projection.scale_at_3d0f9e0=f(w[8]);if(w[7]&4)projection.center_x_at_3d0fa0c=f(w[9]);if(w[7]&8)projection.center_y_at_3d0fa10=f(w[10]);if(w[7]&16)projection.color_argb_at_3d0f7f0=std::array<float,4>{f(w[11]),f(w[12]),f(w[13]),f(w[14])};
        const sadx::NinjaSpriteInput input{{f(w[1]),f(w[2]),f(w[3])},f(w[4]),f(w[5]),static_cast<std::int32_t>(w[6])};
        auto result=sadx::ninja_sprite_geometry(input,*frame,w[0],*values,projection,fp);std::memset(out,0,count*4);std::memset(vertices,0,112);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_boundary;const auto& g=result.partial;
        for(unsigned i=0;i<4;++i){if(g.bounds[i]){out[4]|=1U<<i;out[16+i]=bits(*g.bounds[i]);}if(g.uv[i]){out[4]|=1U<<(i+4);out[20+i]=bits(*g.uv[i]);}}
        if(g.camera_position){out[4]|=7U<<8;out[24]=bits(g.camera_position->x);out[25]=bits(g.camera_position->y);out[26]=bits(g.camera_position->z);}
        out[5]=g.diffuse_argb.has_value();if(g.diffuse_argb)out[6]=*g.diffuse_argb;out[7]=g.clipped;out[8]=w[0];out[15]=result.fp_exceptions;
        if(result){std::optional<sadx::X87Value> depth;if((w[0]&0x10)&&!g.clipped)depth=fp.from_float(f(w[15]));auto stores=sadx::ninja_sprite_vertices(g,depth,fp);out[9]=static_cast<std::uint32_t>(stores.failure);out[10]=stores.written_words;out[28]=stores.source_boundary;std::memcpy(vertices,stores.partial_bytes.data(),112);
            if(stores.command){out[11]=stores.command->vertex_format;out[12]=stores.command->vertex_stride;out[13]=stores.command->primitive_type;out[14]=stores.command->primitive_count;}}
        out[2]=fp.exception_flags();out[3]=fp.control_word();return 0;
    }catch(const sadx::X87Error&){return 3;}
}
