#include "ninja/ninja_queue_consumer.hpp"
#include <algorithm>
#include <cstring>
#include <vector>
// Native 404760 submissions followed by 4086f0 consumption, matching
// tools/verify_original_ninja_queue_consumer.py. The D3D8 device and 78dbe0
// are explicit fixtures; unlifted callees remain native stop boundaries.
namespace {
float as_float(std::uint32_t bits){float v;std::memcpy(&v,&bits,4);return v;}
std::uint32_t bits_of(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
std::array<float,4> color(const std::uint32_t* w){return {as_float(w[0]),as_float(w[1]),as_float(w[2]),as_float(w[3])};}
}
extern "C" int sadx_native_ninja_queue_consumer(const std::uint8_t* assets,std::size_t asset_size,const std::uint32_t* in,std::size_t count,std::uint32_t* out,std::size_t capacity){
    // assets: frame20+sprite constants20 + blend56 + depth32 + trig4100 + unit64 + returns16 + consumer8
    if(!assets||asset_size!=4316||!in||count!=96||!out||capacity<320)return 0;
    const auto frame=sadx::decode_ninja_sprite_frame(assets,20);const auto constants=sadx::decode_ninja_sprite_constants(assets+20,20);
    const auto blend=sadx::decode_ninja_blend_tables(assets+40,88);const auto consumer_constants=sadx::decode_ninja_queue_consumer_constants(assets+4308,8);
    if(!frame||!constants||!blend||!consumer_constants)return 0;
    std::array<std::uint32_t,8> depth;std::memcpy(depth.data(),assets+96,32);
    sadx::NinjaMath::SineTable table;std::memcpy(table.data(),assets+128,4100);sadx::NinjaMatrix unit;std::memcpy(&unit,assets+4228,64);
    std::array<std::uint32_t,4> returns;std::memcpy(returns.data(),assets+4292,16);
    sadx::NinjaMatrix matrix=unit;matrix[12]=as_float(in[59]);matrix[13]=as_float(in[60]);matrix[14]=as_float(in[61]);
    sadx::NinjaMatrixStack stack({matrix},1,0);sadx::PlayerCoordinates coordinates(table,unit,stack);sadx::X87Context fp(static_cast<std::uint16_t>(in[0]));
    std::vector<std::array<std::uint32_t,5>> trace;
    sadx::NinjaRenderState render;auto device=std::make_shared<sadx::NinjaRenderDevice>();device->identity=1;render.device_at_3d128b0=device;render.packed_blend_at_3ab9890=static_cast<std::uint8_t>(in[62]);render.blend_tables=&*blend;
    render.set_render_state=[&](const auto& c){trace.push_back({0xc8,c.source_call,c.state,c.value,0});return sadx::NinjaRenderCallResult{true,0};};
    render.set_texture_stage_state=[&](const auto& c){trace.push_back({0xfc,c.source_call,c.stage,c.state,c.value});return sadx::NinjaRenderCallResult{true,0};};
    sadx::NinjaTextureSelectionState selection;sadx::NinjaTextureSelectionRuntime texture_runtime{render,&returns,nullptr,{}};
    texture_runtime.set_texture=[&](const auto& c){trace.push_back({0xf4,c.source_call,c.stage,c.source_surface_word.value_or(0xdeadbeef),0});return sadx::NinjaRenderCallResult{true,0};};
    auto reference=std::make_shared<sadx::NinjaTextureMemoryReference>();reference->record_ordinal=130;reference->global_id_at_0=7;reference->palette_id_at_4=0xffffffffU;reference->format_flags_at_20=0;reference->source_surface_word_at_34=0x11007000;reference->runtime_owner=std::make_shared<const int>(1);
    auto list=std::make_shared<sadx::NinjaTextureListOwner>();list->count=1;list->entries=std::make_shared<sadx::NinjaTextureListOwner::Entries>(131);
    (**list->entries)[0]=std::shared_ptr<const sadx::NinjaTextureMemoryReference>(reference);(**list->entries)[130]=std::shared_ptr<const sadx::NinjaTextureMemoryReference>(reference);list->first_filename_is_invalid_sentinel=in[51]!=0;
    selection.current_list_at_3d0fa24=std::shared_ptr<sadx::NinjaTextureListOwner>{};selection.requested_memory_at_3ce7128=std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};
    selection.active_memory_at_3d129a0=std::shared_ptr<const sadx::NinjaTextureMemoryReference>{};selection.global_id_at_3d0fdf4=in[50];
    sadx::NinjaSpriteProjection projection{&coordinates,as_float(in[52]),as_float(in[53]),as_float(in[54]),std::array<float,4>{0,0,0,0}};
    sadx::NinjaSpriteDisplayRuntime display{projection,selection,texture_runtime,*constants,{},{},{}};
    if(in[36])display.projected_depth=[&](float,sadx::X87Context& context){return sadx::NinjaSpriteDepthResult{context.from_float(as_float(in[37])),0};};
    display.set_vertex_format=[&](const auto& c){trace.push_back({0x130,c.source_call,c.format,0,0});return sadx::NinjaRenderCallResult{true,0};};
    std::array<std::uint8_t,112> drawn{};std::uint32_t draws=0;
    display.draw=[&](const auto& c){trace.push_back({0x120,c.source_call,c.vertices.primitive_type,c.vertices.primitive_count,c.vertices.vertex_stride});drawn=c.vertices.vertex_bytes;++draws;return sadx::NinjaRenderCallResult{true,0};};
    std::optional<std::int32_t> display_switch=static_cast<std::int32_t>(in[33]);std::optional<float> depth_bias=as_float(in[63]);
    auto arena=std::make_shared<sadx::NinjaRenderQueueArena>(std::vector<std::uint8_t>(65536,0xcd),0,65536,1);sadx::NinjaSpriteQueue queue(arena);queue.initialize_sort_buckets();
    sadx::NinjaSpriteQueueRuntime q;q.display_switch_at_3b1117c=&display_switch;q.depth_bias_at_3abd9b8=&depth_bias;q.render_state=&render;q.mode_at_3ab98ac=in[34];q.flag_at_3abdcfe=static_cast<std::uint8_t>(in[28]);
    q.captured_word_at_3abd950=0;q.captured_word_at_3d0f9c8=11;q.captured_word_at_3d0f840=22;q.captured_word_at_3d0f9c4=33;q.color_at_3abd9a8=color(in+55);q.color_at_3ab9864=color(in+42);q.flag_at_3b12208=in[35];
    q.sort_bias_at_3abd9c0=0;q.bucket_bias_at_7dcb60=as_float(in[64]); // source7dcb60 bytes supplied by the oracle
    sadx::NinjaQueueConsumerState s;s.special_flags_at_909eb4=static_cast<std::uint8_t>(in[26]);s.fog_profile_at_3abdc6c=static_cast<std::uint8_t>(in[27]);s.count_at_3abd9d4=static_cast<std::int32_t>(in[46]);s.maximum_at_3abd9d8=static_cast<std::int32_t>(in[47]);
    s.count_at_3abd9dc=static_cast<std::int32_t>(in[48]);s.maximum_at_3abd9e0=static_cast<std::int32_t>(in[49]);s.fog_enabled_at_3d12798=in[29];s.fog_supported_at_3d128a4=in[30];s.cached_fog_start_at_90a098=as_float(in[31]);s.cached_fog_end_at_90a09c=as_float(in[32]);
    const std::optional<std::array<float,4>> initial=color(in+38);
    sadx::NinjaQueueConsumerRuntime runtime{queue,q,s,display,*consumer_constants,initial,&depth};
    std::vector<std::shared_ptr<sadx::NinjaQueuedSpriteInput>> packets;std::uint32_t submit_failures=0;
    const auto sprites=std::min<std::uint32_t>(in[1],3);
    for(std::uint32_t i=0;i<sprites;++i){const auto* w=in+2+8*i;auto packet=std::make_shared<sadx::NinjaQueuedSpriteInput>();
        packet->sprite={{as_float(w[0]),as_float(w[1]),as_float(w[2])},as_float(w[3]),as_float(w[4]),0};packet->texture_list=list;packet->animation=std::make_shared<const std::vector<sadx::NinjaSpriteFrame>>(1,*frame);packets.push_back(packet);
        const auto submitted=queue.submit(std::shared_ptr<const sadx::NinjaQueuedSpriteInput>(packet),true,static_cast<std::int32_t>(w[5]),w[6],w[7],q,&coordinates,fp);
        if(!submitted)++submit_failures;}
    trace.clear();draws=0;
    const auto r=sadx::ninja_consume_queue(runtime,fp);
    std::fill(out,out+capacity,0);
    out[0]=static_cast<std::uint32_t>(r.failure);out[1]=r.source_boundary;out[2]=r.callee_boundary;out[3]=r.returned;out[4]=submit_failures;out[5]=draws;
    out[6]=q.flag_at_3b12208.value_or(0xdeadbeef);out[7]=s.mode_byte_at_3abdbc0.value_or(0xef);out[8]=q.mode_at_3ab98ac.value_or(0xdeadbeef);
    out[9]=static_cast<std::uint32_t>(s.maximum_at_3abd9d8.value_or(0));out[10]=static_cast<std::uint32_t>(s.maximum_at_3abd9e0.value_or(0));
    out[11]=q.captured_word_at_3d0f9c8.value_or(0xdeadbeef);out[12]=q.captured_word_at_3d0f840.value_or(0xdeadbeef);out[13]=q.captured_word_at_3d0f9c4.value_or(0xdeadbeef);
    out[14]=s.special_flags_at_909eb4.value_or(0xef);out[15]=q.flag_at_3abdcfe.value_or(0xef);out[16]=s.fog_enabled_at_3d12798.value_or(0xdeadbeef);out[17]=s.fog_mode_at_3d0849c.value_or(0xdeadbeef);
    for(unsigned i=0;i<4;++i){out[18+i]=q.color_at_3ab9864?bits_of((*q.color_at_3ab9864)[i]):0xdeadbeef;out[22+i]=projection.color_argb_at_3d0f7f0?bits_of((*projection.color_argb_at_3d0f7f0)[i]):0xdeadbeef;}
    out[26]=projection.scale_at_3d0f9e0?bits_of(*projection.scale_at_3d0f9e0):0xdeadbeef;out[27]=selection.global_id_at_3d0fdf4.value_or(0xdeadbeef);
    const auto current=coordinates.current_matrix();for(unsigned i=0;i<16;++i)out[28+i]=bits_of(current[i]);
    out[44]=fp.exception_flags()&0x3f;out[45]=fp.control_word();out[46]=static_cast<std::uint32_t>(std::min<std::size_t>(trace.size(),48));
    for(std::size_t i=0;i<trace.size()&&i<48;++i)for(unsigned k=0;k<5;++k)out[47+i*5+k]=trace[i][k];
    std::memcpy(out+287,drawn.data(),112);
    return 1;
}
