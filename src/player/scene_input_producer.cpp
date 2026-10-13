#include "player/scene_input_producer.hpp"
#include <cstring>
namespace sadx {
namespace {
std::int16_t signed16(std::uint16_t bits){std::int16_t out;std::memcpy(&out,&bits,2);return out;}
std::int32_t signed32(std::uint32_t bits){std::int32_t out;std::memcpy(&out,&bits,4);return out;}
struct Unavailable{};
}
bool scene_input_hub_stage(std::uint16_t stage)noexcept{return stage>=0x27&&stage<=0x2a;}
SceneInputResult update_scene_directional_keys(std::uint32_t index,const GameLogicControllers& controllers,
    SceneDirectionalKeyState& s,const std::optional<std::uint8_t>& enabled){
    SceneInputResult result;result.slot=index;
    const auto stop=[&](SceneInputFailure why,std::uint32_t at){result.failure=why;result.source_boundary=at;throw Unavailable{};};
    const auto need=[&](const auto& v,SceneInputFailure why,std::uint32_t at){if(!v)stop(why,at);return *v;};
    try {
        if(index>=controllers.size())stop(SceneInputFailure::controller_index_unmapped,0x40f074);
        const auto raw=need(controllers[index],SceneInputFailure::unknown_controller_reference,0x40f074);
        if(!raw){result.completed=true;return result;}
        const auto y=signed16(need(raw->read_u16(0x1e),SceneInputFailure::unknown_controller_word,0x40f089));
        std::uint32_t directions=0;if(y>78)directions=0x20;if(y<-78)directions|=0x10;
        const auto x=signed16(need(raw->read_u16(0x1c),SceneInputFailure::unknown_controller_word,0x40f0a1));
        if(x>78)directions|=0x80;if(x<-78)directions|=0x40;
        const auto previous=need(s.held_work_at_3b0e3a8[index],SceneInputFailure::unknown_held_work,0x40f0b9);
        s.pressed_work_at_3b0e354[index]=~previous&directions;
        s.held_work_at_3b0e3a8[index]=directions;
        const auto held=need(raw->read_u32(8),SceneInputFailure::unknown_controller_word,0x40f0d2);
        s.held_work_at_3b0e3a8[index]=held|directions;
        const auto pressed=need(raw->read_u32(0x10),SceneInputFailure::unknown_controller_word,0x40f0de);
        const auto prior_press=need(s.pressed_work_at_3b0e354[index],SceneInputFailure::unknown_pressed_work,0x40f0e1);
        const auto combined=pressed|prior_press;s.pressed_work_at_3b0e354[index]=combined;
        if(!(combined&0xf0)){
            const auto repeat_dirs=need(s.held_work_at_3b0e3a8[index],SceneInputFailure::unknown_held_work,0x40f0f8)&0xf0;
            if(repeat_dirs){const auto prior=need(s.repeat_at_3b0e3bc[index],SceneInputFailure::unknown_repeat,0x40f107);
                auto next=signed32(static_cast<std::uint32_t>(prior)+1U);s.repeat_at_3b0e3bc[index]=next;
                if(next>30){next=signed32(static_cast<std::uint32_t>(next)-6U);s.repeat_at_3b0e3bc[index]=next;
                    const auto edges=need(s.pressed_work_at_3b0e354[index],SceneInputFailure::unknown_pressed_work,0x40f125);s.pressed_work_at_3b0e354[index]=edges|repeat_dirs;}
            }else s.repeat_at_3b0e3bc[index]=0;
        }else s.repeat_at_3b0e3bc[index]=0;
        if(need(enabled,SceneInputFailure::unknown_control_enabled,0x40f135)==0){s.held_out_at_3b0e7e0[index]=0;s.pressed_out_at_3b0e3cc[index]=0;}
        else {const auto on=need(s.held_work_at_3b0e3a8[index],SceneInputFailure::unknown_held_work,0x40f14d);
            const auto press=need(s.pressed_work_at_3b0e354[index],SceneInputFailure::unknown_pressed_work,0x40f154);s.held_out_at_3b0e7e0[index]=on;s.pressed_out_at_3b0e3cc[index]=press;}
        result.completed=true;return result;
    }catch(Unavailable){return result;}
}
namespace {
// Exact clear body40fe4a..40fe96 /40feb4..40fed8 for one engine record.
void clear_engine_pad(GameLogicController& pad){
    using C=GameLogicController;
    for(std::size_t i=0;i<4;++i)pad.write_byte(C::old_offset+i,pad.read_byte(C::held_offset+i));
    pad.write_u32(0x08,0);pad.write_u32(0x0c,0xffffffffU);pad.write_u32(0x10,0);pad.write_u32(0x14,0);
    for(const std::size_t at:{0x1cU,0x1eU,0x1aU,0x18U,0x20U,0x22U})pad.write_u16(at,0);
}
}
SceneInputResult produce_scene_input(SceneInputProducerRuntime& runtime,X87Context& fp){
    auto& s=runtime.state;SceneInputResult result;
    const auto stop=[&](SceneInputFailure why,std::uint32_t at,std::uint32_t nested=0){result.failure=why;result.source_boundary=at;result.nested_source_boundary=nested;return result;};
    if(!s.stage_at_3b22dcc)return stop(SceneInputFailure::unknown_stage,0x40fdc0,0x4146b0);
    if(scene_input_hub_stage(*s.stage_at_3b22dcc)){s.masks.mask_at_909f94=2;s.masks.mask_at_909fa4=0x402;}
    else {s.masks.mask_at_909f94=0x402;s.masks.mask_at_909fa4=0x402;}
    if(!s.control_enabled_at_909fb0)return stop(SceneInputFailure::unknown_control_enabled,0x40fdef);
    if(*s.control_enabled_at_909fb0==1){
        constexpr std::array<std::uint32_t,2> enable_reads{{0x40fe0b,0x40fe2d}},pointer_loads{{0x40fe14,0x40fe36}},copies{{0x40fe24,0x40fe46}};
        for(std::size_t slot=0;slot<s.engine_pads.records.size();++slot){
            result.slot=slot;auto& pad=s.engine_pads.records[slot];
            if(slot<2){
                const auto enabled=s.controller_enabled?s.controller_enabled(slot):std::nullopt;
                if(!enabled)return stop(SceneInputFailure::unknown_controller_enabled,enable_reads[slot]);
                if(*enabled==1){
                    const auto& reference=s.raw_controllers[slot];
                    if(!reference)return stop(SceneInputFailure::unknown_controller_reference,pointer_loads[slot]);
                    if(!*reference)return stop(SceneInputFailure::null_controller,copies[slot]);
                    const auto& raw=**reference;
                    for(std::size_t at=0;at<GameLogicController::byte_count;++at)pad.write_byte(at,raw.read_byte(at));
                    result.copied_words+=GameLogicController::byte_count/4;continue;
                }
            }
            clear_engine_pad(pad);++result.cleared_records;
        }
    }else for(auto& pad:s.engine_pads.records){clear_engine_pad(pad);++result.cleared_records;}
    result.slot=0;
    if(!runtime.analog_owner)return stop(SceneInputFailure::missing_analog_owner,0x40fee6,0x40f170);
    result.analogs=runtime.analog_owner(fp);
    if(!*result.analogs)return stop(SceneInputFailure::analog_producer_failed,0x40fee6,result.analogs->source_boundary);
    for(std::uint32_t slot=0;slot<4;++slot){
        result.slot=slot;
        const auto keys=update_scene_directional_keys(slot,s.raw_controllers,s.directional,s.control_enabled_at_909fb0);
        if(!keys){result.directional_failure=keys.failure;return stop(SceneInputFailure::directional_failed,0x40fef1,keys.source_boundary);}
    }
    result.completed=true;return result;
}
} // namespace sadx
