#include "input/input_mod_pad.hpp"
#include <cstring>

namespace sadx {
namespace {
std::int16_t low16(std::int32_t v){std::int16_t s;const auto u=static_cast<std::uint16_t>(static_cast<std::uint32_t>(v));std::memcpy(&s,&u,2);return s;}
std::int16_t trigger_pressure(std::int16_t axis,const InputModPadConstants& k,SseContext& sse){ // cvtdq2ps, divss 32767, mulss 255, cvttss2si
    return low16(sse.truncate_int32(sse.multiply(sse.divide(sse.from_int32(axis),k.axes.maximum),k.trigger_scale)));
}
std::uint16_t as_u16(std::int16_t v){return static_cast<std::uint16_t>(v);}
struct Axes{std::int16_t x,y;Sse32 magnitude;};
std::optional<Axes> convert(std::int16_t x,std::int16_t y,std::int16_t deadzone,bool radial,const InputModPadConstants& k,SseContext& sse){
    InputModAxesOutput out;convert_input_mod_261_axes({x,y},deadzone,radial,k.axes,sse,out);
    if(!out.x||!out.y||!out.magnitude)return std::nullopt;return Axes{*out.x,*out.y,*out.magnitude};
}
}
void input_mod_connect(InputModDreamPad& pad){pad.record.write_u32(4,0x3f0fff);pad.connected=true;}
InputModResult input_mod_poll(InputModDreamPad& pad,const InputModKeyboardPad& kb,const InputModGamepadState& gp,bool demo,const InputModPadConstants& k,SseContext& sse){
    InputModResult r;try{auto& rec=pad.record;const bool allow=pad.settings.allow_keyboard;
    if(!pad.connected&&!allow)return r;
    if(pad.connected&&(!allow||(kb.lx==0&&kb.ly==0))){
        const auto a=convert(gp.axes[0],gp.axes[1],pad.settings.deadzone_l,pad.settings.radial_l,k,sse);if(!a){r.failure=InputModFailure::axes_incomplete;r.source_boundary=0x10004117;return r;}
        rec.write_u16(0x1c,as_u16(a->x));rec.write_u16(0x1e,as_u16(a->y));pad.normalized_l=a->magnitude;
    }else{pad.normalized_l=kb.normalized_l;rec.write_u16(0x1c,as_u16(kb.lx));rec.write_u16(0x1e,as_u16(kb.ly));}
    if(!pad.connected||(allow&&(kb.rx!=0||kb.ry!=0))){pad.normalized_r=kb.normalized_r;rec.write_u16(0x20,as_u16(kb.rx));rec.write_u16(0x22,as_u16(kb.ry));}
    else{const auto a=convert(gp.axes[2],gp.axes[3],pad.settings.deadzone_r,pad.settings.radial_r,k,sse);if(!a){r.failure=InputModFailure::axes_incomplete;r.source_boundary=0x1000416d;return r;}
        rec.write_u16(0x20,as_u16(a->x));rec.write_u16(0x22,as_u16(a->y));pad.normalized_r=a->magnitude;}
    std::uint16_t left=kb.ltrigger,right=kb.rtrigger;
    if(pad.connected&&(!allow||kb.ltrigger==0))left=as_u16(trigger_pressure(gp.axes[4],k,sse));
    rec.write_u16(0x1a,left);
    if(pad.connected&&(!allow||kb.rtrigger==0))right=as_u16(trigger_pressure(gp.axes[5],k,sse));
    rec.write_u16(0x18,right);
    const std::uint16_t threshold=pad.settings.trigger_threshold;
    std::uint32_t buttons=(threshold<right?0x10000U:0U)|(threshold<left?0x20000U:0U);
    if(pad.connected){
        static constexpr std::array<std::pair<unsigned,std::uint32_t>,12> map{{{0,0x4},{1,0x2},{2,0x400},{3,0x200},{6,0x8},{9,0x1},{4,0x800},{10,0x100},{11,0x10},{12,0x20},{13,0x40},{14,0x80}}};
        for(const auto& [button,mask]:map)if(button<gp.buttons.size()&&gp.buttons[button])buttons|=mask;
        pad.e_held=gp.buttons[7];pad.half_press=gp.buttons[8];
    }
    if(allow)buttons|=kb.held;
    if(pad.half_press){
        const auto halve=[&](std::size_t at){const auto v=*rec.read_u16(at);std::int16_t s;std::memcpy(&s,&v,2);rec.write_u16(at,as_u16(static_cast<std::int16_t>(s/2)));};
        halve(0x1c);pad.normalized_l=sse.multiply(*pad.normalized_l,k.half_press_scale);halve(0x1e);pad.normalized_r=sse.multiply(*pad.normalized_r,k.half_press_scale);halve(0x20);halve(0x22);
    }
    if(demo){buttons&=8;rec.write_u32(0x1c,0);rec.write_u32(0x20,0);rec.write_u32(0x18,0);pad.e_held=false;pad.half_press=false;}
    const auto old=rec.read_u32(0x2c);if(!old){r.failure=InputModFailure::unknown_old_buttons;r.source_boundary=0x100043d3;return r;}
    const auto changed=*old^buttons;
    rec.write_u32(0x08,buttons);rec.write_u32(0x0c,~buttons);rec.write_u32(0x14,changed&*old);rec.write_u32(0x10,changed&buttons);rec.write_u32(0x2c,buttons);
    r.polled=true;return r;
    }catch(const SseError&){r.failure=InputModFailure::fp_failed;return r;}
}
namespace {
InputModResult publish_one(std::size_t i,const InputModDreamPad& pad,const std::array<std::shared_ptr<GameLogicController>,4>& raw,const InputModRawRecords& raw_input){
    InputModResult r;
    if(!raw_input[i]){r.failure=InputModFailure::null_raw_controller;r.source_boundary=0x100048f4;return r;}
    for(std::size_t b=0;b<GameLogicController::byte_count;++b)raw_input[i]->write_byte(b,pad.record.read_byte(b));
    if(i<4){if(!raw[i]){r.failure=InputModFailure::null_raw_controller;r.source_boundary=0x10004919;return r;}
        for(std::size_t b=0;b<GameLogicController::byte_count;++b)raw[i]->write_byte(b,pad.record.read_byte(b));}
    r.polled=true;return r;
}
}
InputModResult input_mod_publish(const std::array<InputModDreamPad,8>& pads,const std::array<std::shared_ptr<GameLogicController>,4>& raw,const InputModRawRecords& raw_input){
    for(std::size_t i=0;i<pads.size();++i){const auto r=publish_one(i,pads[i],raw,raw_input);if(!r)return r;}
    InputModResult r;r.polled=true;return r;
}
void input_mod_init_raw_controllers(const InputModRawRecords& raw_input,GameLogicControllers& pointers){
    for(std::size_t i=0;i<pointers.size();++i)pointers[i]=raw_input[i];
}
InputModTableFailure input_mod_enable_controller(std::uint8_t index,InputModControllerTable& table,InputModKeyboardPad* overflow){
    if(index>1){table.enabled_at_1000e73c[0]=1;table.enabled_at_1000e73c[1]=1;}
    if(index>8){for(std::uint8_t i=0;i<8;++i)input_mod_enable_controller(i,table,overflow);return InputModTableFailure::none;}
    if(index==8){if(!overflow)return InputModTableFailure::overflow_target_missing;overflow->normalized_r.bits=(overflow->normalized_r.bits&0xffffff00U)|1U;return InputModTableFailure::none;}
    table.enabled_at_1000e73c[index]=1;return InputModTableFailure::none;
}
InputModTableFailure input_mod_disable_controller(std::uint8_t index,InputModControllerTable& table){
    if(index>1){table.enabled_at_1000e73c[0]=0;table.enabled_at_1000e73c[1]=0;}
    if(index>8){for(std::uint8_t i=0;i<8;++i)input_mod_disable_controller(i,table);return InputModTableFailure::none;}
    if(index==8)return InputModTableFailure::invalid_parameter_abort;
    table.enabled_at_1000e73c[index]=0;return InputModTableFailure::none;
}
InputModResult input_mod_on_input_tail(const GameLogicControllers& pointers,InputModFrameState& s){
    InputModResult r;const auto stop=[&](InputModFailure f,std::uint32_t at){r.failure=f;r.source_boundary=at;return r;};
    if(!pointers[0])return stop(InputModFailure::unknown_raw_controller,0x10006516);
    if(!*pointers[0])return stop(InputModFailure::null_raw_controller,0x10006520);
    const auto& pad=**pointers[0];const auto held=pad.read_u32(0x08);if(!held)return stop(InputModFailure::unknown_fact,0x10006520);
    if(*held==0x606){const auto released=pad.read_u32(0x14);if(!released)return stop(InputModFailure::unknown_fact,0x10006529);
        if(*released==8){if(!s.game_mode_at_3abdc7c||!*s.game_mode_at_3abdc7c)return stop(InputModFailure::unknown_fact,0x1000652f);
            if(**s.game_mode_at_3abdc7c!=1&&**s.game_mode_at_3abdc7c!=8){s.soft_reset_byte=1;r.soft_reset=true;}}}
    if(!s.demo_playing_at_3b2a2e4||!*s.demo_playing_at_3b2a2e4)return stop(InputModFailure::unknown_fact,0x10006559);
    bool start_demo=false;
    if(**s.demo_playing_at_3b2a2e4!=0){if(!s.demo_enabled)return stop(InputModFailure::unknown_fact,0x10006568);
        if(*s.demo_enabled!=0){if(!s.demo_cutscene_at_3b2a2e8)return stop(InputModFailure::unknown_fact,0x1000656d);start_demo=*s.demo_cutscene_at_3b2a2e8<0;}}
    if(start_demo&&!s.demo_at_1000e708)s.demo_at_1000e708=true;
    if(s.demo_at_1000e708)return stop(InputModFailure::demo_unported,0x10006595);
    r.polled=true;return r;
}
InputModKeyboardPad input_mod_keyboard_pad(const input::TickInput& k){
    InputModKeyboardPad pad;pad.held=k.held;pad.lx=k.left.x;pad.ly=k.left.y;pad.rx=k.right.x;pad.ry=k.right.y;
    std::uint32_t bits;std::memcpy(&bits,&k.left.magnitude,4);pad.normalized_l={bits};std::memcpy(&bits,&k.right.magnitude,4);pad.normalized_r={bits};
    pad.ltrigger=(k.held&0x20000)?255:0;pad.rtrigger=(k.held&0x10000)?255:0;return pad;
}
InputModResult input_mod_on_input(InputModFrameOwners& owners,const InputModKeyboardPad& kb,const std::array<InputModGamepadState,8>& gamepads,InputModFrameState& state,const InputModPadConstants& k,SseContext& sse){
    // poll_controllers polls pad i and copies it before moving to pad i+1.
    for(std::size_t i=0;i<owners.pads.size();++i){
        const auto polled=input_mod_poll(owners.pads[i],kb,gamepads[i],state.demo_at_1000e708,k,sse);if(!polled)return polled;
        const auto copied=publish_one(i,owners.pads[i],owners.controllers_raw_at_3b0e9c8,owners.raw_input_at_1000e748);if(!copied)return copied;
    }
    return input_mod_on_input_tail(owners.controller_pointers_at_3b0e77c,state);
}
bool input_mod_vibration_stop(InputModRumbleTasks& tasks,std::array<InputModDreamPad,8>& pads,const InputModVibrationStopOwners& owners){
    for(auto& task:tasks.tasks_at_1000ec60)if(task!=0){if(!owners.free_task||!owners.free_task(task))return false;task=0;}
    for(std::size_t i=0;i<pads.size();++i){auto& pad=pads[i];if(pad.rumble_flags==0)continue;
        pad.rumble_word=0;pad.rumble_flags=static_cast<std::uint8_t>(pad.rumble_flags&0xfc);
        if(!owners.stop_rumble||!owners.stop_rumble(i))return false;}
    return true;
}
} // namespace sadx
