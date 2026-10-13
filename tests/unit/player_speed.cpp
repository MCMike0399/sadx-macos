#include "player/player_speed.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool equal(sadx::NinjaVector a, sadx::NinjaVector b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

sadx::PlayerParameters parameters() {
    // Explicit test seed, not a claim about any character's game parameters.
    sadx::PlayerParameters p{};
    p.jog_speed = 1; p.run_speed = 2; p.run_break = -0.25F;
    p.lim_h_spd = 20; p.lim_v_spd = 20;
    return p;
}

sadx::PlayerPhysicsState player() {
    return {{{0,0,0},{0,0,0}},1,{0,0,0},sadx::NinjaVector{0,1,0},0,0};
}

sadx::PlayerSpeedCallees explicit_test_callees(std::vector<std::string>& calls, bool active) {
    sadx::PlayerSpeedCallees c;
    c.input_gate = [&,active](const auto&, auto&) {
        calls.push_back("input");
        return sadx::PlayerInputGateResult{sadx::PlayerSpeedFailure::none,active};
    };
    c.task_to_world = [&](const auto&, auto&, auto&) {
        calls.push_back("task-transform");
        return sadx::PlayerSpeedFailure::none;
    };
    c.floor_to_world = [&](const auto&, auto&, auto, auto&) {
        calls.push_back("floor-transform");
        return sadx::PlayerSpeedFailure::none;
    };
    return c;
}

void test_complete_local_sequence() {
    sadx::X87Context fp(0x037f);
    const sadx::PlayerOrientation o{0,{12,-345,67}};
    sadx::MotionWork world{{91,92,93},{81,82,83}};
    auto p = parameters(); p.lim_h_spd = 2; p.lim_v_spd = 1;
    auto state = player(); state.motion = {{0.25F,0.5F,-0.5F},{1,2,3}};
    state.effect = {1,-1,10}; state.dotp = 0;
    std::vector<std::string> calls;
    auto c = explicit_test_callees(calls,false);
    c.task_to_world = [&](const auto& received, auto& v, auto&) {
        require(received.angles == o.angles, "raw task orientation must reach transform");
        calls.push_back("task-transform"); v = {v.z,v.x,v.y};
        return sadx::PlayerSpeedFailure::none;
    };
    require(static_cast<bool>(sadx::get_player_speed(o,world,state,p,c,fp)), "complete local sequence");
    require(equal(state.motion.speed,{1.25F,2.5F,2.5F}), "player acceleration stores");
    require(equal(world.speed,{2,0.25F,2}), "transform then effect then axis limits");
    require(equal(world.acceleration,{81,82,83}) && equal(state.motion.acceleration,{0.25F,0.5F,-0.5F}),
            "both acceleration owners must remain unchanged");
    require(calls == std::vector<std::string>({"input","task-transform","input"}),
            "X gate and repeated cleanup gate must preserve source call order");
}

void test_stopping_and_special_attributes() {
    const sadx::PlayerOrientation o{0,{0,0,0}};
    for (const auto cw : {0x007f,0x027f,0x037f}) {
        sadx::X87Context fp(static_cast<std::uint16_t>(cw));
        auto p = parameters();
        for (float sign : {-1.0F,1.0F}) {
            auto state = player(); state.motion.speed = {sign,0,sign};
            state.motion.acceleration = {-2*sign,0,-0.2F*sign};
            sadx::MotionWork world{{91,92,93},{81,82,83}};
            std::vector<std::string> calls; auto c = explicit_test_callees(calls,false);
            require(static_cast<bool>(sadx::get_player_speed(o,world,state,p,c,fp)), "cross-zero case");
            require(state.motion.speed.x == 0 && state.motion.speed.z == 0, "X sign crossing and Z threshold crossing stop");
            require(calls == std::vector<std::string>({"task-transform","input"}), "sign crossing skips first input gate");
            state = player(); state.motion.speed = {0.5F*sign,0,0.5F*sign};
            calls.clear(); c = explicit_test_callees(calls,true);
            require(static_cast<bool>(sadx::get_player_speed(o,world,state,p,c,fp)), "active input small speed");
            require(state.motion.speed.x == 0.5F*sign && state.motion.speed.z == 0,
                    "input preserves X while unchanged small Z still stops");
        }
        for (const auto shadow : {0x400000U,0x400002U,2U}) {
            auto state = player(); state.shadow_attributes_top = shadow;
            state.motion.speed.x = shadow == 2 ? 2 : 1;
            sadx::MotionWork world{{91,92,93},{81,82,83}};
            std::vector<std::string> calls; auto c = explicit_test_callees(calls,true);
            require(static_cast<bool>(sadx::get_player_speed(o,world,state,p,c,fp)), "shadow attribute braking");
            require(state.motion.speed.x == (shadow == 2 ? 1.75F : 0.75F),
                    "shadow400000 branch priority versus bit2 mean threshold");
        }
    }
    sadx::X87Context fp(0x037f); auto p = parameters(); auto state = player();
    state.motion.acceleration = {0.02F,-0.02F,0.01F};
    sadx::MotionWork world{{91,92,93},{81,82,83}};
    std::vector<std::string> calls; auto c = explicit_test_callees(calls,false);
    require(static_cast<bool>(sadx::get_player_speed(o,world,state,p,c,fp)), "small cleanup boundary");
    require(equal(world.speed,{0.02F,-0.02F,0}), "cleanup uses strict open plus/minus0.02 bounds");
    require(state.motion.speed.z == 0.01F, "cleanup does not rewrite player-relative velocity");
}

void test_required_callees_and_partial_stores() {
    const sadx::PlayerOrientation o{0,{12,-345,67}};
    sadx::X87Context fp(0x037f); const auto p = parameters();
    sadx::MotionWork world{{91,92,93},{81,82,83}};
    auto state = player(); state.motion = {{0.25F,0.5F,1},{1,2,3}};
    auto failed = sadx::get_player_speed(o,world,state,p,{},fp);
    require(failed.failure == sadx::PlayerSpeedFailure::missing_input_gate && failed.source_boundary == 0x443fa9,
            "missing X input boundary must be explicit");
    require(equal(state.motion.speed,{1.25F,2,3}) && equal(world.speed,{91,92,93}),
            "failed X gate preserves already stored X and untouched later fields");
    std::vector<std::string> calls; auto c = explicit_test_callees(calls,false);
    state = player(); state.motion.acceleration = {1,2,3}; c.task_to_world = {};
    failed = sadx::get_player_speed(o,world,state,p,c,fp);
    require(failed.failure == sadx::PlayerSpeedFailure::missing_task_to_world_transform &&
            failed.source_boundary == 0x444165 && failed.temporary_vector &&
            equal(*failed.temporary_vector,{1,2,3}), "missing transform retains known temporary vector");
    require(equal(state.motion.speed,{1,2,3}) && equal(world.speed,{91,92,93}),
            "transform failure preserves preceding player stores without fake world speed");
    state = player(); state.attributes = 0x4000; state.motion.acceleration = {1,2,3};
    c = explicit_test_callees(calls,false); c.floor_to_world = {};
    failed = sadx::get_player_speed(o,world,state,p,c,fp);
    require(failed.failure == sadx::PlayerSpeedFailure::missing_floor_to_world_transform,
            "floor path must not fall back to task rotation");
    c.floor_to_world = [](const auto&, auto& v, auto, auto&) {
        v.x = 123; return sadx::PlayerSpeedFailure::floor_to_world_transform_unavailable;
    };
    state = player(); state.attributes = 0x4000; state.motion.acceleration = {1,2,3};
    failed = sadx::get_player_speed(o,world,state,p,c,fp);
    require(failed.failure == sadx::PlayerSpeedFailure::floor_to_world_transform_unavailable &&
            failed.temporary_vector->x == 123 && equal(world.speed,{91,92,93}),
            "failed transform reports partial local facts and preserves actual world owner");
    state = player(); state.motion.acceleration = {1,2,3};
    c = explicit_test_callees(calls,false); c.input_gate = {};
    failed = sadx::get_player_speed(o,world,state,p,c,fp);
    require(failed.failure == sadx::PlayerSpeedFailure::missing_input_gate && failed.source_boundary == 0x43c4c5 &&
            equal(world.speed,{1,2,3}), "cleanup boundary failure retains earlier world stores");
    state = player(); state.motion.speed.x = 1;
    c.input_gate = [](const auto&,auto&) { return sadx::PlayerInputGateResult{sadx::PlayerSpeedFailure::none,{}}; };
    failed = sadx::get_player_speed(o,world,state,p,c,fp);
    require(failed.failure == sadx::PlayerSpeedFailure::input_gate_unavailable,
            "unknown gate value must not silently become false");
}

void test_floor_arguments_and_fcom_nan() {
    const sadx::PlayerOrientation o{0,{1,-123456,3}};
    sadx::X87Context fp(0x037f); auto p = parameters(); auto state = player();
    state.attributes = 0x4000; state.floor_normal = {0.1F,0.2F,0.3F};
    sadx::MotionWork world{{91,92,93},{81,82,83}};
    std::vector<std::string> calls; auto c = explicit_test_callees(calls,true);
    c.floor_to_world = [&](const auto& normal,auto&,auto yaw,auto&) {
        require(state.floor_normal && equal(normal,*state.floor_normal) && yaw == o.angles[1], "floor transform must receive source normal and raw yaw");
        calls.push_back("floor-transform"); return sadx::PlayerSpeedFailure::none;
    };
    require(static_cast<bool>(sadx::get_player_speed(o,world,state,p,c,fp)), "floor transform selection");
    require(calls == std::vector<std::string>({"floor-transform","input"}), "floor-only call selection");
    state=player();state.attributes=0x4000;state.floor_normal.reset();
    state.motion.acceleration={1,2,3};world.speed={91,92,93};calls.clear();
    const auto missing=sadx::get_player_speed(o,world,state,p,c,fp);
    require(missing.failure==sadx::PlayerSpeedFailure::unavailable_floor_normal &&
        missing.temporary_vector && state.motion.speed.x==1 && equal(world.speed,{91,92,93}) && calls.empty(),
        "missing shared floor fact retains local speed stores without a fabricated zero-floor transform");
    state = player(); state.motion.speed.x = std::numeric_limits<float>::quiet_NaN();
    c = explicit_test_callees(calls,true); fp.clear_exceptions();
    require(static_cast<bool>(sadx::get_player_speed(o,world,state,p,c,fp)) &&
            std::isnan(state.motion.speed.x) && std::isnan(world.speed.x), "quiet NaN remains unordered rather than zero" );
    require((fp.exception_flags() & 1U) != 0, "source FCOMs must raise invalid even for quiet NaN");
}

} // namespace

int main() {
    try {
        test_complete_local_sequence();
        test_stopping_and_special_attributes();
        test_required_callees_and_partial_stores();
        test_floor_arguments_and_fcom_nan();
        std::cout << "PGetSpeed local body: source order, stop/brake cases, mandatory boundaries, partial stores and FCOM NaNs passed.\n"
                     "Input/rotation test doubles do not prove original callees or functioning gameplay.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
