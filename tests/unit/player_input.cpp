#include "player/player_input.hpp"

#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
float value(std::uint32_t bits) { float result; std::memcpy(&result,&bits,4); return result; }
std::uint32_t word(float value) { std::uint32_t result; std::memcpy(&result,&value,4); return result; }
sadx::PlayerInputRuntime explicit_runtime(float magnitude) {
    sadx::PlayerInputRuntime runtime;
    runtime.stage=1; runtime.act=0; runtime.current_character=0;
    runtime.normalized[3]=sadx::NormalizedPlayerInput{-123456,magnitude};
    runtime.no_control_timer[3]=0;
    return runtime;
}
}

int main() {
    using namespace sadx;
    PlayerInputTask task{0,3,0,123456,{}};
    X87Context fp(0x037f);
    std::int32_t angle=777;
    float magnitude=value(0xdeadbeef);
    auto runtime=explicit_runtime(0.25F);
    auto query=get_player_input(task,runtime,fp,&angle,&magnitude,{});
    require(query && *query.active && angle==-123456 && word(magnitude)==word(0.25F),
            "normal input must copy exact normalized angle/magnitude");
    require(query.outputs_written==3,"ordinary query records both actual source output stores");
    runtime.normalized[3]->angle.reset();
    query=get_player_input(task,runtime,fp,nullptr,&magnitude,{});
    require(query && *query.active,"known magnitude must support null-angle gameplay gate without inventing direction");
    angle=777; magnitude=value(0xdeadbeef);
    query=get_player_input(task,runtime,fp,&angle,&magnitude,{});
    require(query.failure==PlayerInputFailure::unavailable_normalized_angle && query.active==true &&
                angle==777 && word(magnitude)==0xdeadbeef,
            "requested unknown angle must stop before magnitude store, preserving known gate activity");
    for (const auto bits : {0U,0x80000000U}) {
        runtime=explicit_runtime(value(bits)); angle=777; magnitude=value(0xdeadbeef);
        query=get_player_input(task,runtime,fp,&angle,&magnitude,{});
        require(query && !*query.active && angle==777 && word(magnitude)==0xdeadbeef,
                "both zero signs must be inactive with outputs untouched");
    }
    for (const auto bits : {0xbf800000U,1U,0x80000001U,0x7f800000U,0xff800000U,
                            0x7fc12345U,0xffc54321U,0x7f812345U}) {
        fp.clear_exceptions(); runtime=explicit_runtime(value(bits));
        query=get_player_input(task,runtime,fp,&angle,&magnitude,{});
        require(query && *query.active && word(magnitude)==bits,
                "negative/subnormal/nonfinite active magnitudes must retain source bits");
        if ((bits & 0x7f800000U)==0x7f800000U && (bits & 0x007fffffU)!=0)
            require((fp.exception_flags() & 1)!=0,"FCOMP must signal invalid for quiet/signaling NaNs");
        if ((bits & 0x7f800000U)==0 && (bits & 0x007fffffU)!=0)
            require((fp.exception_flags() & 2)!=0,"FLD subnormal must preserve denormal exception");
    }
    fp.clear_exceptions(); runtime=explicit_runtime(1);
    runtime.no_control_timer[3]=65535; angle=777; magnitude=value(0xdeadbeef);
    query=get_player_input(task,runtime,fp,&angle,&magnitude,{});
    require(query && !*query.active && angle==777 && word(magnitude)==0xdeadbeef,
            "nonzero nocontimer must block normal input without output stores");
    task.flags=0x2000; runtime.no_control_timer[3].reset();
    query=get_player_input(task,runtime,fp,&angle,nullptr,{});
    require(query && *query.active && angle==-123456,"flag0x2000 must bypass timer dependency");

    task.flags=0x4000; task.player_index=255; task.event_move_mode=255;
    PlayerInputRuntime unknown_runtime;
    angle=777; magnitude=value(0xdeadbeef); fp.clear_exceptions();
    query=get_player_input(task,unknown_runtime,fp,&angle,&magnitude,{});
    require(query && *query.active && angle==777 && word(magnitude)==0xdeadbeef && fp.exception_flags()==0,
            "event-mode branch must ignore stage/pad/index and leave outputs/FPU intact");
    require(query.outputs_written==0,"active event branch cannot claim caller scratch was initialized");
    task.event_move_mode=0;
    query=get_player_input(task,unknown_runtime,fp,&angle,&magnitude,{});
    require(query && !*query.active,"event move byte0 must return inactive");
    task.event_move_mode.reset();
    require(get_player_input(task,unknown_runtime,fp,nullptr,nullptr,{}).failure==PlayerInputFailure::unavailable_event_move_mode,
            "missing event state must not become false");

    task={0x100,3,0,123456,{}};
    for (const auto act : {2,3}) {
        runtime=explicit_runtime(-0.0F); runtime.stage=9; runtime.act=static_cast<std::int16_t>(act);
        query=get_player_input(task,runtime,fp,&angle,&magnitude,{});
        require(query && *query.active && angle==123456 && word(magnitude)==0,
                "Sonic ball in source stage9/act2-or3 must return yaw/positive zero");
    }
    runtime.act=-1;
    require(!*get_player_input(task,runtime,fp,nullptr,nullptr,{}).active,
            "signed negative act must not be normalized to a valid stage combination");
    runtime.act=2; task.character_id=1;
    require(!*get_player_input(task,runtime,fp,nullptr,nullptr,{}).active,
            "character byte must gate the zero-magnitude Sonic fallback");

    // Forced stage path doesn't consume normalized slots/index/timer.
    task={0x2000,255,0,0,{}}; runtime=PlayerInputRuntime{};
    runtime.stage=2; runtime.act=2; runtime.current_character=0;
    magnitude=value(0xdeadbeef);
    query=get_player_input(task,runtime,fp,nullptr,&magnitude,{});
    require(query && *query.active && word(magnitude)==0x3f800000,
            "forced null-angle query must be complete without an invented angle provider");
    angle=777; magnitude=value(0xdeadbeef);
    query=get_player_input(task,runtime,fp,&angle,&magnitude,{});
    require(query.failure==PlayerInputFailure::missing_forced_angle_conversion && query.active==true &&
                angle==777 && word(magnitude)==0xdeadbeef,
            "unresolved forced-angle math must retain known activity and source output order");
    int calls=0;
    const ForcedInputAngleConversion explicit_test_conversion=[&](X87Context& actual) {
        require(&actual==&fp,"selected x87 owner must reach required angle boundary");
        ++calls; return ForcedInputAngleResult{PlayerInputFailure::none,16384};
    };
    query=get_player_input(task,runtime,fp,&angle,&magnitude,explicit_test_conversion);
    require(query.failure==PlayerInputFailure::unavailable_camera_yaw && calls==1,
            "forced conversion must occur before camera-yaw lookup, not use a zero fallback");
    runtime.camera_yaw=std::numeric_limits<std::int32_t>::min();
    query=get_player_input(task,runtime,fp,&angle,&magnitude,explicit_test_conversion);
    require(query && *query.active && static_cast<std::uint32_t>(angle)==0x7fffc000 && word(magnitude)==0x3f800000,
            "camera NEG/SUB must wrap as source int32 instructions");

    task={0,3,0,0,{}}; runtime=explicit_runtime(1); runtime.stage.reset();
    require(get_player_input(task,runtime,fp,nullptr,nullptr,{}).failure==PlayerInputFailure::unavailable_stage,
            "unknown required stage must be surfaced");
    runtime.stage=1; runtime.normalized[3].reset();
    require(get_player_input(task,runtime,fp,nullptr,nullptr,{}).failure==PlayerInputFailure::unavailable_normalized_input,
            "unknown input slot must not become zero magnitude");
    runtime.normalized[3]=NormalizedPlayerInput{12,1}; runtime.no_control_timer[3].reset();
    require(get_player_input(task,runtime,fp,nullptr,nullptr,{}).failure==PlayerInputFailure::unavailable_no_control_timer,
            "unknown nocontimer must not become enabled input");
    X87Context unmasked_invalid(0x037e); runtime.no_control_timer[3]=0;
    runtime.normalized[3]=NormalizedPlayerInput{12,value(0x7fc12345)};
    bool invalid_thrown=false;
    try { get_player_input(task,runtime,unmasked_invalid,nullptr,nullptr,{}); }
    catch (const X87Error& error) { invalid_thrown=error.failure()==X87Failure::unmasked_exception; }
    require(invalid_thrown,"unmasked signaling compare failure must not be silently masked");
    std::cout<<"player input query passed\n";
}
