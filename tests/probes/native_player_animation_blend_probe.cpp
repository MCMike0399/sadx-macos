#include "player/player_animation.hpp"
#include <cstring>
namespace {float f(std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;}std::int32_t s(std::uint32_t b){std::int32_t v;std::memcpy(&v,&b,4);return v;}}
extern "C" int sadx_native_player_animation_blend(const float* constants,const std::uint32_t* input,std::uint32_t* output){if(!constants||!input||!output)return 1;
    sadx::PlayerPhysicsState player;player.requested_animation_at_124=0;player.current_animation_at_126=1;sadx::PlayerAnimationTail tail;tail.frame_at_130=f(input[1]);auto table=std::make_shared<sadx::PlayerAnimationTable>();for(unsigned n=0;n<2;++n){auto& row=table->records[n];row.action=sadx::AnimationResourceReference{n};row.object_count=1;}tail.table_at_140=table;
    sadx::PlayerAnimationResources resources;std::unordered_map<std::size_t,sadx::SceneGeometryObject> objects;auto& object=objects[0];object.child=sadx::SceneResourceReference{};object.sibling=sadx::SceneResourceReference{};object.position=sadx::NinjaVector{f(input[53]),f(input[54]),f(input[55])};object.angles=std::array<std::int32_t,3>{s(input[56]),s(input[57]),s(input[58])};object.scale=sadx::NinjaVector{f(input[59]),f(input[60]),f(input[61])};resources.objects=&objects;
    // Caller fixture owns one contiguous KEY..ALLOC+32 byte capture, including
    // adjacent key arrays and its explicitly A5-filled allocator buffer. This
    // is not a production allocator/uninitialized-byte default.
    auto backing=std::make_shared<std::vector<std::uint8_t>>(0x1620,0);
    auto put=[&](std::size_t at,std::uint32_t bits){for(unsigned k=0;k<4;++k)(*backing)[at+k]=static_cast<std::uint8_t>(bits>>(k*8));};
    for(unsigned ch=0;ch<3;++ch){for(unsigned k=0;k<12;++k)put(0x100+ch*48+k*4,input[8+ch*12+k]);put(ch*48,0);for(unsigned k=0;k<3;++k)put(ch*48+4+k*4,input[44+ch*3+k]);}
    std::fill(backing->begin()+0x1600,backing->end(),0xa5);put(0x1600,0);
    for(unsigned n=0;n<2;++n){resources.actions[n].object=sadx::SceneResourceReference{0};resources.actions[n].motion=sadx::AnimationResourceReference{n};auto& m=resources.motions[n];m.frame_count=input[4];m.type=static_cast<std::uint16_t>(input[n?2:3]);m.input_function=(*m.type&4)?3:2;m.data=sadx::AnimationResourceReference{n};auto& nodes=resources.node_arrays[n];nodes.resize(1);const auto mask=input[n?5:6];if(*m.type&4)nodes[0].scale_pointer_bits_at_8=mask&4U?0x11000a60U+n*0x100U:0;
        for(unsigned ch=0;ch<3;++ch){const std::size_t id=n*3+ch;nodes[0].keys[ch]=mask&(1U<<ch)?sadx::AnimationResourceReference{id}:sadx::AnimationResourceReference{};nodes[0].counts[ch]=n?input[7]:1;const auto offset=n*0x100+ch*48;resources.key_arrays[id]={backing,offset,backing->size()-offset};}}
    sadx::TaskWorkArena arena;auto allocation=arena.allocate(512).work;auto a=std::make_shared<sadx::PlayerAnimationAllocatedAction>();a->allocation=arena.allocate(8).work;auto m=std::make_shared<sadx::PlayerAnimationAllocatedMotion>();m->allocation=arena.allocate(12).work;m->data=arena.allocate(24).work;if(!arena.fill(*m->data,0,0,24))return 2;m->input_function=2;m->type=3;a->motion=m;tail.buffer_at_148=a;
    sadx::PlayerAnimationConstants c{constants[0],constants[1],constants[2]};sadx::PlayerAnimationRuntime rt{&arena,resources,c};sadx::PlayerAnimationView view{player,tail,allocation};sadx::X87Context fp(static_cast<std::uint16_t>(input[0]));const auto result=sadx::build_player_animation_transition(view,rt,fp);std::memset(output,0,35*4);auto channels=(*m->input_function==3)?3U:2U;
    for(unsigned ch=0;ch<channels;++ch){auto ref=arena.reference(*m->data,ch*4);if(!ref)return 3;output[ch]=bool(ref.work);if(!arena.read(*m->data,(channels+ch)*4,output+3+ch,4))return 4;if(ref.work&&!arena.read(ref.work,0,output+6+ch*8,32))return 5;}
    output[30]=fp.exception_flags();output[31]=fp.control_word();output[32]=static_cast<std::uint32_t>(result.failure);output[33]=result.source_boundary;output[34]=result.completed;return 0;
}
