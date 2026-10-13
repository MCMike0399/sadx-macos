#include "player/player_coordinates.hpp"
#include <cstddef>
#include <cstring>
#include <utility>
namespace {
float number(std::uint32_t w){float v;std::memcpy(&v,&w,4);return v;}
std::int32_t integer(std::uint32_t w){std::int32_t v;std::memcpy(&v,&w,4);return v;}
std::uint32_t bits(float v){std::uint32_t w;std::memcpy(&w,&v,4);return w;}
void store(std::uint32_t* w,const sadx::NinjaVector& v){w[0]=bits(v.x);w[1]=bits(v.y);w[2]=bits(v.z);}
}
extern "C" int sadx_native_axis_projection(const std::uint8_t* table,std::size_t table_size,
    const std::uint8_t* unit,std::size_t unit_size,const std::uint8_t* constants,std::size_t constants_size,
    const std::uint32_t* w,std::size_t count,std::uint16_t cw,std::uint32_t* out,std::size_t out_count){
    if(!table||table_size!=4100||!unit||unit_size!=64||!constants||constants_size!=8||
       !w||count!=109||!out||out_count!=110||w[0]>1)return 1;
    try{
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);
        sadx::NinjaMatrix identity;std::memcpy(identity.data(),unit,64);
        sadx::CoordinateAxisConstants c;std::memcpy(&c,constants,8);
        std::vector<sadx::NinjaMatrix> frames(6);
        for(std::size_t r=0;r<6;++r)for(std::size_t i=0;i<16;++i)frames[r][i]=number(w[13+r*16+i]);
        sadx::NinjaMatrixStack stack(std::move(frames),w[11],w[12]);sadx::PlayerCoordinates coordinates(sine,identity,stack);
        const sadx::NinjaVector axis{number(w[1]),number(w[2]),number(w[3])};
        sadx::X87Context fp(cw);std::memset(out,0,110*4);
        if(w[0]==0)coordinates.rotate_axis(stack.current(),axis,integer(w[4]),c,fp);
        else{
            const sadx::PlayerOrientation task{0,{integer(w[5]),integer(w[6]),integer(w[7])}};
            const auto result=coordinates.project_input_to_floor(task,axis,integer(w[4]),integer(w[8]),integer(w[9]),fp.from_float(number(w[10])),c,fp);
            out[0]=static_cast<std::uint32_t>(result.coordinate.failure);out[1]=result.source_boundary;
            if(result.camera_direction){out[13]|=1;store(out+4,*result.camera_direction);}
            if(result.projected_direction){out[13]|=2;store(out+7,*result.projected_direction);}
            if(result.forward_direction){out[13]|=4;store(out+10,*result.forward_direction);}
        }
        out[2]=stack.depth();out[3]=fp.exception_flags();
        for(std::size_t r=0;r<6;++r)for(std::size_t i=0;i<16;++i)out[14+r*16+i]=bits(stack.slots()[r][i]);return 0;
    }catch(const sadx::X87Error&){return 2;}catch(const sadx::CoordinateError&){return 3;}
}
