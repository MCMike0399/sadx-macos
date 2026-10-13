#include "engine/ninja_sprite_display.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Full77e390 with real texture list/index selection, geometry, render/stage
// commands, vertices and draw. 78dbe0 is either absent (required boundary) or
// an explicit seeded fixture matching tools/verify_original_ninja_sprite_display.py.
namespace {
template<class T>T load(const std::uint8_t* p){T v;std::memcpy(&v,p,sizeof v);return v;}
float as_float(std::uint32_t bits){float v;std::memcpy(&v,&bits,4);return v;}
std::uint32_t bits_of(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
}
extern "C" int sadx_native_ninja_sprite_display(const std::uint8_t* assets,std::size_t asset_size,const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    // assets: sprite constants40 + blend56 + trig4100 + unit64 + texture returns16
    if(!assets||asset_size!=4276||!in||count!=64||!out||capacity<128)return 0;
    const auto frame0=sadx::decode_ninja_sprite_frame(assets,20);const auto constants=sadx::decode_ninja_sprite_constants(assets+20,20);
    const auto blend=sadx::decode_ninja_blend_tables(assets+40,56);if(!frame0||!constants||!blend)return 0;
    sadx::NinjaMath::SineTable table;std::memcpy(table.data(),assets+96,4100);sadx::NinjaMatrix unit;std::memcpy(&unit,assets+4196,64);
    std::array<std::uint32_t,4> returns;std::memcpy(returns.data(),assets+4260,16);
    sadx::NinjaMatrix matrix;std::memcpy(&matrix,in+19,64);
    sadx::NinjaMatrixStack stack({matrix},1,0);sadx::PlayerCoordinates coordinates(table,unit,stack);sadx::X87Context fp(static_cast<std::uint16_t>(in[2]));
    std::vector<std::array<std::uint32_t,5>> trace;
    sadx::NinjaRenderState render;auto device=std::make_shared<sadx::NinjaRenderDevice>();device->identity=1;render.device_at_3d128b0=device;render.packed_blend_at_3ab9890=0xa8;render.blend_tables=&*blend;
    const auto hresult=static_cast<std::int32_t>(in[51]);
    render.set_render_state=[&](const auto& c){trace.push_back({0xc8,c.source_call,c.state,c.value,0});return sadx::NinjaRenderCallResult{true,hresult};};
    render.set_texture_stage_state=[&](const auto& c){trace.push_back({0xfc,c.source_call,c.stage,c.state,c.value});return sadx::NinjaRenderCallResult{true,hresult};};
    sadx::NinjaTextureSelectionState selection;sadx::NinjaTextureSelectionRuntime texture_runtime{render,&returns,nullptr,{}};
    texture_runtime.set_texture=[&](const auto& c){trace.push_back({0xf4,c.source_call,c.stage,c.source_surface_word.value_or(0xdeadbeef),0});return sadx::NinjaRenderCallResult{true,hresult};};
    std::array<std::shared_ptr<sadx::NinjaTextureMemoryReference>,2> memory;
    for(unsigned i=0;i<2;++i){auto m=std::make_shared<sadx::NinjaTextureMemoryReference>();m->record_ordinal=i;m->global_id_at_0=in[47+i];m->palette_id_at_4=0xffffffffU;m->format_flags_at_20=in[53];m->source_surface_word_at_34=in[45+i];
        if(in[45+i])m->runtime_owner=std::make_shared<const int>(int(i));else m->runtime_owner=std::shared_ptr<const void>{};memory[i]=m;}
    auto list=std::make_shared<sadx::NinjaTextureListOwner>();list->count=in[42];list->entries=std::make_shared<sadx::NinjaTextureListOwner::Entries>(in[42]);
    for(std::uint32_t i=0;i<in[42];++i)(**list->entries)[i]=std::shared_ptr<const sadx::NinjaTextureMemoryReference>(memory[i%2]);
    list->first_filename_is_invalid_sentinel=false;
    selection.current_list_at_3d0fa24=in[43]?list:std::shared_ptr<sadx::NinjaTextureListOwner>{};
    selection.requested_memory_at_3ce7128=std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};
    selection.active_memory_at_3d129a0=in[52]?std::shared_ptr<const sadx::NinjaTextureMemoryReference>(memory[in[52]-1]):std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};
    selection.global_id_at_3d0fdf4=in[44];
    sadx::NinjaSpriteProjection projection{&coordinates,as_float(in[35]),as_float(in[36]),as_float(in[37]),std::array<float,4>{as_float(in[38]),as_float(in[39]),as_float(in[40]),as_float(in[41])}};
    std::optional<std::uint32_t> recorded_reciprocal;
    sadx::NinjaSpriteDisplayRuntime runtime{projection,selection,texture_runtime,*constants,{},{},{}};
    if(in[49])runtime.projected_depth=[&](float reciprocal,sadx::X87Context& context){recorded_reciprocal=bits_of(reciprocal);return sadx::NinjaSpriteDepthResult{context.from_float(as_float(in[50])),0};};
    runtime.set_vertex_format=[&](const auto& c){trace.push_back({0x130,c.source_call,c.format,0,0});return sadx::NinjaRenderCallResult{true,hresult};};
    std::array<std::uint8_t,112> drawn{};
    runtime.draw=[&](const auto& c){trace.push_back({0x120,c.source_call,c.vertices.primitive_type,c.vertices.primitive_count,c.vertices.vertex_stride});drawn=c.vertices.vertex_bytes;return sadx::NinjaRenderCallResult{true,hresult};};
    auto packet=std::make_shared<sadx::NinjaQueuedSpriteInput>();
    packet->sprite={{as_float(in[3]),as_float(in[4]),as_float(in[5])},as_float(in[6]),as_float(in[7]),static_cast<std::int32_t>(in[8])};
    packet->texture_list=in[56]?std::shared_ptr<sadx::NinjaTextureListOwner>{}:list;
    std::vector<sadx::NinjaSpriteFrame> frames;
    for(unsigned f=0;f<2;++f){std::uint8_t raw[20];std::memcpy(raw,in+9+5*f,20);auto decoded=sadx::decode_ninja_sprite_frame(raw,20);if(!decoded)return 0;frames.push_back(*decoded);}
    packet->animation=std::make_shared<const std::vector<sadx::NinjaSpriteFrame>>(frames);
    const std::optional<std::shared_ptr<const sadx::NinjaQueuedSpriteInput>> argument=in[55]?std::shared_ptr<const sadx::NinjaQueuedSpriteInput>{}:std::shared_ptr<const sadx::NinjaQueuedSpriteInput>(packet);
    const auto r=sadx::ninja_display_sprite(argument,static_cast<std::int32_t>(in[1]),in[0],{},runtime,fp);
    std::fill(out,out+capacity,0);
    const auto index_of=[&](const std::optional<std::shared_ptr<const sadx::NinjaTextureMemoryReference>>& ref)->std::uint32_t{
        if(!ref)return 0xff;if(!*ref)return 0;for(unsigned i=0;i<2;++i)if(*ref==memory[i])return i+1;return 0xfe;};
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.nested_source_boundary;out[3]=r.returned;
    out[4]=recorded_reciprocal.has_value();out[5]=recorded_reciprocal.value_or(0);
    out[6]=selection.global_id_at_3d0fdf4.value_or(0xdeadbeef);out[7]=index_of(selection.active_memory_at_3d129a0);out[8]=index_of(selection.requested_memory_at_3ce7128);
    out[9]=selection.current_list_at_3d0fa24?(*selection.current_list_at_3d0fa24?1U:0U):0xffU;
    out[10]=fp.exception_flags()&0x3f;out[11]=fp.control_word();out[12]=static_cast<std::uint32_t>(std::min<std::size_t>(trace.size(),12));
    for(std::size_t i=0;i<trace.size()&&i<12;++i)for(unsigned k=0;k<5;++k)out[13+i*5+k]=trace[i][k];
    std::memcpy(out+73,drawn.data(),112);
    return 1;
}
