#include "game/lighting_palette.hpp"
#include "engine/device_lights.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
namespace {
const std::uint32_t *normalization_input = nullptr;
std::vector<std::uint32_t> *normalization_trace = nullptr;
float number(std::uint32_t word) {float x;std::memcpy(&x,&word,4);return x;}
std::uint32_t bits(float x) {std::uint32_t w;std::memcpy(&w,&x,4);return w;}
}
namespace sadx::lighting {
std::array<float,3> normalize_stage_direction(const std::array<float,3>& input) {
 normalization_trace->insert(normalization_trace->end(),{6,bits(input[0]),bits(input[1]),bits(input[2])});
 return {number(normalization_input[43]),number(normalization_input[44]),number(normalization_input[45])};
}
}
extern "C" int sadx_native_lighting_palette(const std::uint32_t *in,const std::uint8_t *data,
 const float *sine,const float *unit,std::uint32_t *out) {
 using namespace sadx;
 std::fill_n(out,16384,0);
 static_assert(sizeof(lighting::Tables)==36960 && sizeof(graphics::Light)==104);
 lighting::Tables tables;std::memcpy(&tables,data,36960);lighting::State scene(tables);
 std::memcpy(scene.active_stage.data(),data+36960,192);
 std::memcpy(&scene.active_palette,data+37152,96);
 std::memcpy(&scene.active_character,data+37248,48);
 scene.rotation_y=static_cast<int>(in[9]);scene.rotation_z=static_cast<int>(in[10]);
 std::vector<NinjaMatrix> slots(6);for(unsigned i=0;i<96;++i)slots[i/16][i%16]=number(in[64+i]);
 NinjaMatrixStack stack(std::move(slots),in[11],in[12]);NinjaMath::SineTable table;std::copy_n(sine,1025,table.begin());NinjaMatrix identity;std::copy_n(unit,16,identity.begin());
 NinjaMatrixPrimitives matrices(table,identity,stack);
 auto stage=static_cast<std::int16_t>(in[4]),act=static_cast<std::int16_t>(in[5]);auto loop=static_cast<std::int32_t>(in[6]);auto palette=static_cast<std::int32_t>(in[2]);auto copy=static_cast<std::uint8_t>(in[3]);
 lighting::DeviceState scratch;std::memcpy(&scratch.light,in+17,104);scratch.stage_direction={number(in[14]),number(in[15]),number(in[16])};
 lighting::PaletteView view{scene,{stage,act},loop,palette,copy,scratch,matrices};
 X87Context fp(static_cast<std::uint16_t>(in[13]));std::vector<std::uint32_t> trace;
 normalization_input=in;normalization_trace=&trace;
 graphics::Draw device;device_lights::State device_state;
 unsigned time_calls=0,hotel_calls=0;
 lighting::TimeServices time{[&]{++time_calls;trace.insert(trace.end(),{4,in[7]+time_calls-1});return static_cast<std::int32_t>(in[7]+time_calls-1);},[&]{++hotel_calls;trace.insert(trace.end(),{5,in[8]});return static_cast<std::int32_t>(in[8]);}};
 lighting::DeviceServices commands{
 [&](unsigned index,const graphics::Light &light){trace.push_back(1);trace.push_back(index);auto p=reinterpret_cast<const std::uint32_t *>(&light);trace.insert(trace.end(),p,p+26);device_lights::set_light(index,light,device_state,device);},
 [&](unsigned index,bool enabled){trace.insert(trace.end(),{2,index,enabled?1U:0U});device_lights::enable_light(index,enabled,device_state,device);},
 [&](unsigned key,unsigned value){trace.insert(trace.end(),{3,key,value});device.render_states[key]=value;}};
 try {
  if(in[0]==0)lighting::set_palette(static_cast<std::int32_t>(in[1]),view,time,commands,fp);
  else if(in[0]==1)lighting::apply_palette(view,commands,fp);
  else lighting::copy_palette_type(static_cast<std::int32_t>(in[1]),copy);
 } catch(const CoordinateError &){out[30]=1;}
 out[0]=static_cast<std::uint32_t>(palette);out[1]=copy;out[2]=static_cast<std::uint16_t>(stage);out[3]=static_cast<std::uint16_t>(act);out[4]=time_calls;out[5]=hotel_calls;out[6]=stack.depth();out[7]=fp.exception_flags();
 std::memcpy(out+32,&scratch.light,104);for(unsigned i=0;i<3;++i)out[58+i]=bits(scratch.stage_direction[i]);
 std::memcpy(out+64,scene.active_stage.data(),192);std::memcpy(out+112,&scene.active_palette,96);std::memcpy(out+136,&scene.active_character,48);
 for(unsigned i=0;i<96;++i)out[148+i]=bits(stack.slots()[i/16][i%16]);
 std::memcpy(out+256,&scene.tables,36960);out[31]=trace.size();std::copy(trace.begin(),trace.end(),out+10000);
 return 0;
}
