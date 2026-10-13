#include "path_probe_data.hpp"
#include <cstddef>
#include <cstring>
extern "C" int sadx_native_player_path(const std::uint8_t* table,std::size_t table_size,
    const std::uint8_t* constants,std::size_t constants_size,const std::uint32_t* w,std::size_t count,
    std::uint16_t cw,std::uint32_t* out,std::size_t out_count){
    if(!table||table_size!=4100||!constants||constants_size!=92||!w||count<29||w[2]>(count-29)/5||count!=29+w[2]*5||!out||out_count!=34)return 1;
    try{
        const auto c=sadx::decode_player_path_constants(constants,92);if(!c)return 2;
        sadx::NinjaMath::SineTable sine;std::memcpy(sine.data(),table,4100);
        auto path=sadx_probe::resource(w[0],w[1],sadx_probe::number(w[3]),w+29,w[2]);auto status=sadx_probe::status(w+9);
        sadx::X87Context fp(cw);sadx::NinjaMath math(sine,fp);sadx::SseContext sse(w[6]);sadx::CrtFloorRuntime runtime;
        if(w[4])runtime.cpu_sse2=w[5];runtime.sse=&sse;if(w[7])runtime.fallback_control_word=static_cast<std::uint16_t>(w[8]);
        const auto result=sadx::query_player_path(path,status,c->query,c->floor,runtime,math,fp);std::memset(out,0,34*4);
        out[0]=static_cast<std::uint32_t>(result.failure);out[1]=result.source_return.has_value();out[2]=result.source_return.value_or(false);
        out[3]=result.source_boundary;out[4]=fp.control_word();out[5]=fp.exception_flags();out[6]=sse.mxcsr();
        out[7]=result.selected_index.has_value();if(result.selected_index)out[8]=static_cast<std::uint32_t>(*result.selected_index);
        out[9]=result.accumulated_distance.has_value();if(result.accumulated_distance)out[10]=sadx_probe::bits(*result.accumulated_distance);
        if(result.floor){out[11]=static_cast<std::uint32_t>(result.floor->failure);out[12]=result.floor->source_boundary;}
        out[13]=sadx_probe::store_status(status,out+14);return 0;
    }catch(const sadx::X87Error&){return 3;}catch(const sadx::SseError&){return 4;}
}
