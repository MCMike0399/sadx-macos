// Frozen test-only fixture support is extracted from the qualified current unit.
#include "fixture.hpp"
#include <sstream>
#include <vector>
namespace {
float real(std::uint32_t x) { float y;std::memcpy(&y,&x,4);return y; }
void optional(std::ostream& out,const std::optional<std::uint32_t>& x) {
    if(x)out<<*x;else out<<"null";
}
void handle(std::ostream& out,const TaskWorkHandle& h) {
    if(h)out<<'['<<h->data_offset()<<','<<h->requested_size()<<','<<h->released()<<']';else out<<"null";
}
void snapshot(std::ostream& out,Fixture& f,const PlayerAnimationResult& r) {
    out<<"{\"result\":["<<static_cast<unsigned>(r.failure)<<','<<r.source_boundary<<','<<r.callee_address<<','<<r.nested_source_boundary<<','<<r.completed<<','<<r.wrap_iterations<<',';
    if(r.work_failure)out<<static_cast<unsigned>(*r.work_failure);else out<<"null";
    out<<"],\"fp\":["<<f.fp.control_word()<<','<<f.fp.exception_flags()<<','<<(f.fp.exception_flags() & ~f.fp.control_word() & 63)<<"],\"state\":[";
    auto value=[&](const auto& x){if(x)out<<+*x;else out<<"null";out<<',';};
    value(f.player.animation_mode_at_120);value(f.player.requested_animation_at_124);value(f.player.current_animation_at_126);value(f.player.previous_animation_at_128);
    value(f.tail.next_at_12a);value(f.tail.timer_at_12c);value(f.tail.flags_at_12e);
    if(f.tail.frame_at_130)out<<bits(*f.tail.frame_at_130);else out<<"null";
    out<<',';if(f.tail.start_at_134)out<<bits(*f.tail.start_at_134);else out<<"null";
    out<<"],\"table\":["<<f.tail.table_at_140.has_value()<<','<<(f.tail.table_at_140&&*f.tail.table_at_140?1:0)<<','<<(f.tail.table_at_140&&*f.tail.table_at_140&&(*f.tail.table_at_140).get()==f.table.get())<<"],\"header\":[";
    out<<f.tail.buffer_at_148.has_value()<<',';auto a=f.tail.buffer_at_148.value_or(nullptr);handle(out,a?a->allocation:TaskWorkHandle{});
    out<<','<<(a&&a->object.has_value())<<',';if(a&&a->object&&a->object->id)out<<*a->object->id;else out<<"null";
    out<<','<<(a&&a->motion.has_value())<<',';auto m=a?a->motion.value_or(nullptr):nullptr;handle(out,m?m->allocation:TaskWorkHandle{});
    out<<','<<(m&&m->data.has_value())<<',';handle(out,m?m->data.value_or(TaskWorkHandle{}):TaskWorkHandle{});
    out<<',';if(m){value(m->frame_count);value(m->type);if(m->input_function)out<<*m->input_function;else out<<"null";}else out<<"null,null,null";
    out<<"],\"blocks\":[";bool comma=false;
    for(const auto& b:f.arena.blocks()) {if(comma)out<<',';comma=true;out<<'['<<b.offset<<',';optional(out,b.free_next);out<<',';optional(out,b.free_previous);out<<',';optional(out,b.physical_next);out<<',';optional(out,b.physical_previous);out<<']';}
    // Lossless run encoding covers every arena byte and its definedness, including
    // the pressure allocation and sentinel. No unobserved allocation byte is dropped.
    out<<"],\"arena_runs\":[";comma=false;const auto& bytes=f.arena.source_bytes();const auto& known=f.arena.known_bytes();
    for(std::size_t i=0;i<bytes.size();) {auto j=i+1;while(j<bytes.size()&&bytes[j]==bytes[i]&&known[j]==known[i])++j;if(comma)out<<',';comma=true;out<<'['<<i<<','<<j-i<<','<<+bytes[i]<<','<<+known[i]<<']';i=j;}
    out<<"],\"references\":[";comma=false;
    if(m&&m->data&&*m->data)for(unsigned i=0;i<18;++i){auto ref=f.arena.reference_view(*m->data,i*4);if(comma)out<<',';comma=true;out<<'['<<i*4<<','<<static_cast<unsigned>(ref.failure)<<','<<ref.byte_offset<<',';handle(out,ref.work);out<<']';}
    out<<"]}";
}
}
// 102 explicit words. Same construction, availability and defined output fields
// are compiled against both immutable Source snapshots.
extern "C" const char* final_interface(const std::uint32_t* in) {
    static std::string result;
    Fixture f; f.fp.set_control_word(static_cast<std::uint16_t>(in[1]));
    f.player.animation_mode_at_120=static_cast<std::int16_t>(in[3]);
    f.player.requested_animation_at_124=static_cast<std::uint16_t>(in[4]);f.player.current_animation_at_126=static_cast<std::uint16_t>(in[5]);f.player.previous_animation_at_128=static_cast<std::uint16_t>(in[6]);
    f.tail.next_at_12a=static_cast<std::uint16_t>(in[7]);f.tail.frame_at_130=real(in[8]);f.tail.start_at_134=real(in[9]);f.tail.timer_at_12c=static_cast<std::uint16_t>(in[10]);f.tail.flags_at_12e=static_cast<std::uint16_t>(in[11]);
    f.player.motion.speed.x=real(in[15]);f.player.source_charge_at_0=real(in[16]);
    for(unsigned i=0;i<2;++i){f.table->records[i].frame_step=real(in[12]);f.table->records[i].transition_step=real(in[13]);f.table->records[i].next=static_cast<std::uint16_t>((in[100]&0xff00U)|(1-i));f.resources.motions[i].frame_count=in[14];}
    if(in[0]==1) {
        f.player.requested_animation_at_124=0;f.player.current_animation_at_126=1;
        f.resources.motions[1].type=static_cast<std::uint16_t>(in[21]);f.resources.motions[0].type=static_cast<std::uint16_t>(in[22]);
        for(unsigned i=0;i<2;++i)for(unsigned ch=0;ch<3;++ch) {
            auto& node=f.resources.node_arrays[i][0];node.scale_pointer_bits_at_8=in[23];node.counts[ch]=2;
            auto bytes=std::make_shared<std::vector<std::uint8_t>>(48);
            std::memcpy(bytes->data(),in+28+(i*3+ch)*12,48);
            f.resources.key_arrays[i*3+ch]={bytes,0,in[i?25:26]};
        }
        if(in[24]==3) {
            f.objects[0].child=SceneResourceReference{1};f.objects[0].sibling=SceneResourceReference{1};f.objects[1]=f.objects[0];f.objects[1].child=SceneResourceReference{};f.objects[1].sibling=SceneResourceReference{};
            f.table->records[0].object_count=3;
            for(unsigned i=0;i<2;++i)f.resources.node_arrays[i].resize(3,f.resources.node_arrays[i][0]);
        }
    }
    const auto fault=in[19];
    switch(fault) {
    case 1:f.player.current_animation_at_126.reset();break;
    case 2:f.player.requested_animation_at_124.reset();break;
    case 3:f.player.animation_mode_at_120.reset();break;
    case 4:f.tail.timer_at_12c.reset();break;
    case 5:f.tail.table_at_140.reset();break;
    case 6:f.tail.table_at_140=std::shared_ptr<const PlayerAnimationTable>{};break;
    case 7:f.table->records[0].frame_step.reset();break;
    case 8:f.tail.frame_at_130.reset();break;
    case 9:f.table->records[0].action.reset();break;
    case 10:f.table->records[0].action=AnimationResourceReference{};break;
    case 11:f.resources.actions[0].motion.reset();break;
    case 12:f.resources.motions[0].frame_count.reset();break;
    case 13:require(bool(f.arena.set_reference(f.work,0x138,{})),"null speed");break;
    case 14:require(bool(f.arena.set_reference_view(f.work,0x13c,f.work,4)),"offset charge");break;
    case 15:f.view.player_allocation.reset();break;
    case 16:f.tail.buffer_at_148.reset();break;
    case 17:f.action->motion.reset();break;
    case 18:f.motion->data.reset();break;
    case 19:f.resources.node_arrays[1][0].keys[0].reset();break;
    case 20:f.resources.node_arrays[1][0].keys[1].reset();break;
    case 21:f.resources.node_arrays[1][0].keys[2].reset();break;
    case 22:f.resources.node_arrays[1][0].scale_pointer_bits_at_8.reset();break;
    case 23:f.resources.node_arrays[1][0].counts[0].reset();break;
    case 24:f.resources.key_arrays[3].bytes.reset();break;
    case 25:f.resources.actions[0].object.reset();break;
    case 26:f.objects[0].child=SceneResourceReference{0};break;
    case 27:f.resources.node_arrays[0][0].keys[1]=AnimationResourceReference{};break;
    case 28:f.resources.node_arrays[1][0].keys[0]=AnimationResourceReference{};break;
    case 29:f.table->records[0].object_count.reset();break;
    case 30:f.resources.motions[1].data=AnimationResourceReference{};break;
    case 31:f.runtime.arena=nullptr;break;
    case 32:f.player.animation_mode_at_120=0;f.tail.buffer_at_148.reset();{
        auto head=*f.arena.free_head();auto blocks=f.arena.blocks();auto block=std::find_if(blocks.begin(),blocks.end(),[&](const auto& b){return b.offset==head;});
        require(block!=blocks.end()&&block->physical_next.has_value(),"pressure block");require(bool(f.arena.allocate(*block->physical_next-head-in[20]-32)),"pressure");}break;
    case 33:f.player.animation_mode_at_120=0;f.resources.actions[0].object.reset();break;
    case 34:f.player.animation_mode_at_120=0;f.arena.set_payload_transport({},[](auto,const auto*,auto n){return n!=111*24;});break;
    default:break;
    }
    auto invoke=[&](){switch(in[2]){case 1:return reset_player_animation_frame(f.view,f.runtime,f.fp);case 2:return next_player_animation(f.view,f.runtime,static_cast<std::uint16_t>(in[4]),f.fp);case 3:return blend_player_animation(f.view,f.runtime,f.fp);case 4:return release_animation_keys(f.view,f.runtime);case 5:return build_player_animation_transition(f.view,f.runtime,f.fp);default:return f.update();}};
    std::ostringstream out;out<<'[';auto r=invoke();snapshot(out,f,r);
    // Second call mutates the live canonical values, preserving allocation aliases.
    f.player.motion.speed.x=real(in[17]);f.player.source_charge_at_0=real(in[18]);
    out<<',';r=invoke();snapshot(out,f,r);out<<']';result=out.str();return result.c_str();
}
