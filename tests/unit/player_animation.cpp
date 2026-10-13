#include "player/player_animation_decode.hpp"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
namespace {
std::vector<std::uint8_t> load(const char* path){std::ifstream file(path,std::ios::binary);assert(file);return {std::istreambuf_iterator<char>(file),{}};}
std::uint32_t read(sadx::TaskWorkArena& arena,const sadx::TaskWorkHandle& work,std::uint32_t offset){std::uint32_t value;assert(arena.read(work,offset,&value,4));return value;}
}
int main(int argc,char** argv){using namespace sadx;assert(argc==3);auto source=load(argv[1]),constant_bytes=load(argv[2]);assert(constant_bytes.size()==12);PlayerAnimationConstants constants;std::memcpy(&constants,constant_bytes.data(),12);
    auto malformed=decode_sonic_animations(nullptr,0,10000);assert(!malformed&&malformed.failure==AnimationDecodeFailure::invalid_pe);
    auto decoded=decode_sonic_animations(source.data(),source.size(),10000);if(!decoded){std::cerr<<"Decode failure "<<static_cast<int>(decoded.failure)<<" at "<<std::hex<<decoded.requested_source_address<<'\n';return 1;}
    auto& assets=*decoded.decoded;assert(assets.sonic_actions.size()==149);auto table=std::make_shared<PlayerAnimationTable>();auto exported=std::make_shared<const SonicAnimationExport>(assets.sonic_actions);assert(initialize_sonic_animation_table(*table,exported));assert(table->records.size()==147);assert(table->records.at(75).action&&!table->records.at(75).action->id);assert(table->records.at(76).action&&!table->records.at(76).action->id);
    for(unsigned pc:{0U,0x200U,0x300U})for(unsigned rc:{0U,0x400U,0x800U,0xc00U}){
        PlayerPhysicsState player{};player.animation_mode_at_120=0;player.current_animation_at_126=0;player.requested_animation_at_124=0;PlayerAnimationTail tail;tail.timer_at_12c=65535;tail.table_at_140=table;TaskWorkArena arena;auto work=arena.allocate(512).work;assert(arena.fill(work,0,0,512));assert(arena.set_reference_view(work,0x138,work,0x38));assert(arena.set_reference(work,0x13c,work));PlayerAnimationView view{player,tail,work};PlayerAnimationRuntime runtime{&arena,assets.resources,constants};X87Context fp(static_cast<std::uint16_t>(pc|rc|0x7f));
        assert(update_player_animation(view,runtime,fp));assert(tail.timer_at_12c==0&&player.animation_mode_at_120==1&&tail.buffer_at_148&&*tail.buffer_at_148);auto buffer=*tail.buffer_at_148;auto motion=*buffer->motion;assert(motion->frame_count==2&&motion->input_function==2&&motion->data&&*motion->data);auto data=*motion->data;
        assert(update_player_animation(view,runtime,fp));assert(player.animation_mode_at_120==2&&tail.frame_at_130==0.0625F);assert(motion->input_function==2||motion->input_function==3);const auto channels=*motion->input_function;unsigned allocations=0;for(unsigned node=0;node<73;++node)for(unsigned ch=0;ch<channels;++ch){const auto offset=node*channels*8+ch*4;auto ref=arena.reference(data,offset);assert(ref);if(ref.work){++allocations;assert(read(arena,ref.work,0)==0&&read(arena,ref.work,16)==1);assert(read(arena,data,node*channels*8+(channels+ch)*4)==2);}}
        assert(allocations>0);for(unsigned tick=0;tick<15;++tick)assert(update_player_animation(view,runtime,fp));assert(player.animation_mode_at_120==3&&tail.frame_at_130==0);for(unsigned node=0;node<73;++node)for(unsigned ch=0;ch<channels;++ch){const auto offset=node*channels*8+ch*4;auto ref=arena.reference(data,offset);assert(ref&&!ref.work);assert(read(arena,data,node*channels*8+(channels+ch)*4)==0);}
        player.animation_mode_at_120=14;assert(update_player_animation(view,runtime,fp));assert(!*tail.buffer_at_148&&buffer->allocation->released()&&motion->allocation->released()&&data->released());
        player.animation_mode_at_120=1;tail.buffer_at_148.reset();auto failure=update_player_animation(view,runtime,fp);assert(!failure&&failure.failure==PlayerAnimationFailure::unknown_field);assert(player.previous_animation_at_128==0&&tail.flags_at_12e==0);
    }
    // A real MDATA3 outgoing motion must use its scale-pointer word as the
    // position count. Bounded capture stops at the consuming next-frame load;
    // the explicitly captured contiguous PE range permits the real read.
    for(bool bounded:{true,false}) {
        PlayerPhysicsState player{};player.animation_mode_at_120=0;player.current_animation_at_126=0;player.requested_animation_at_124=0;
        PlayerAnimationTail tail;tail.timer_at_12c=0;tail.table_at_140=table;TaskWorkArena arena;auto work=arena.allocate(512).work;
        PlayerAnimationView view{player,tail,work};PlayerAnimationRuntime runtime{&arena,assets.resources,constants};X87Context fp(0x007f);
        assert(update_player_animation(view,runtime,fp));player.current_animation_at_126=19;tail.frame_at_130=15.5F;
        const auto action=*table->records.at(19).action->id;const auto motion_id=*assets.resources.actions.at(action).motion->id;
        auto& motion=assets.resources.motions.at(motion_id);assert(*motion.type==7);auto& node=assets.resources.node_arrays.at(*motion.data->id).at(0);
        assert(node.scale_pointer_bits_at_8==0&&node.counts[0]==16&&node.keys[0]->id);
        auto& capture=assets.resources.key_arrays.at(*node.keys[0]->id);const auto original_extent=capture.byte_extent;
        if(bounded)capture.byte_extent=*node.counts[0]*16;
        const auto result=build_player_animation_transition(view,runtime,fp);capture.byte_extent=original_extent;
        if(bounded){assert(!result&&result.failure==PlayerAnimationFailure::unavailable_key&&result.source_boundary==0x43f3cc);auto generated=*(*tail.buffer_at_148)->motion;auto partial=arena.reference(*generated->data,0);assert(partial&&partial.work&&read(arena,partial.work,0)==0);}
        else assert(result);
    }
    for(unsigned outgoing:{19U,27U,47U,58U,59U,61U,66U,67U,68U,142U}) {
        for(bool at_end:{false,true}) {
            PlayerPhysicsState player{};player.animation_mode_at_120=0;player.current_animation_at_126=0;player.requested_animation_at_124=0;
            PlayerAnimationTail tail;tail.timer_at_12c=0;tail.table_at_140=table;TaskWorkArena arena;auto work=arena.allocate(512).work;
            PlayerAnimationView view{player,tail,work};PlayerAnimationRuntime runtime{&arena,assets.resources,constants};X87Context fp(0x007f);assert(update_player_animation(view,runtime,fp));
            const auto action=*table->records.at(static_cast<std::uint16_t>(outgoing)).action->id;const auto motion=*assets.resources.actions.at(action).motion->id;
            player.current_animation_at_126=static_cast<std::uint16_t>(outgoing);tail.frame_at_130=at_end?static_cast<float>(*assets.resources.motions.at(motion).frame_count)-0.5F:0.5F;
            const auto result=build_player_animation_transition(view,runtime,fp);if(!result){std::cerr<<"Outgoing animation "<<outgoing<<" unavailable at "<<std::hex<<result.source_boundary<<'\n';return 2;}assert(result);
        }
    }
    assert(animation_angle_delta(0,65535)==-1);assert(animation_angle_delta(32767,-32768)==1);assert(animation_unwrap_angle(32767,-32768)==32768);assert(animation_unwrap_angle(-32768,32767)==-32769);assert(animation_unwrap_angle(0,65536)==0);
    std::cout<<"Animation real DLL147-record table,73-node transition/key release and12 FP profiles passed; "<<assets.resources.actions.size()<<" actions, "<<assets.resources.motions.size()<<" motions, "<<assets.objects.size()<<" objects, "<<assets.resources.key_arrays.size()<<" key arrays decoded\n";
}
