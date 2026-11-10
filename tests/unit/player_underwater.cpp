#include "player/player_underwater.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}

struct Fake final : sadx::player_underwater::Services {
    sadx::NinjaVector pose_position{400, 0, -20};
    std::array<std::int32_t, 3> pose_rotation{0, 16384, 0};
    sadx::NinjaVector converted{0, 8, 0};
    sadx::NinjaVector convert_input{0, 0, 0};
    bool create_succeeds = true;
    int converts = 0, creates = 0;
    std::uint8_t stored_slot = 0xff;
    sadx::player_underwater::Pose player_pose(std::uint8_t) override {
        return {pose_position, pose_rotation};
    }
    void convert_to_global(sadx::NinjaVector &vector) override {
        convert_input = vector;
        ++converts;
        vector = converted;
    }
    std::uintptr_t create_breath_task() override {
        ++creates;
        return create_succeeds ? std::uintptr_t{7} : std::uintptr_t{0};
    }
    void store_breath_slot(std::uintptr_t, std::uint8_t slot) override { stored_slot = slot; }
};

struct Fixture {
    sadx::player_underwater::State state;
    std::uint16_t status = 0xffff;
    std::uint8_t slot = 5;
    sadx::NinjaVector position{400, 0, -20};
    sadx::NinjaVector motion_acceleration{1, 2, 3};
    float motion_accel = 4;
    sadx::NinjaVector effect{5, 6, 7};
    sadx::NinjaVector acceleration{8, 9, 10};
    std::int16_t no_control = 5;
    std::int16_t breath = 77;
    std::uint16_t island = 3;
    float top_y = 0;
    std::uint32_t prior = 0;
    Fake services;
    sadx::fidelity::Context arithmetic{0x007f};
    void run() {
        sadx::player_underwater::PlayerView view{
            status,       slot,   position,     motion_acceleration,
            motion_accel, effect, acceleration, no_control,
            breath,       island, 8.0f,         top_y,
            prior};
        sadx::player_underwater::run(state, view, services, arithmetic);
    }
};
} // namespace

int main() {
    using namespace sadx;
    { // Bit-clear path: timers reset, pose recorded, no conversion.
        Fixture f;
        f.state.underwater_word = 9;
        f.run();
        require(f.status == 0xefc7, "status mask");
        require(f.no_control == 4, "no-control decrement");
        require(f.breath == 0, "breath cleared out of water");
        require(f.island == 0, "island cleared");
        require(f.state.underwater_word == 0, "underwater word cleared");
        require(f.state.history_index[5] == 1, "history index incremented first");
        const auto &recorded = f.state.positions[13];
        require(recorded.x == 400 && recorded.y == 0 && recorded.z == -20, "pose recorded");
        require(f.state.rotations[13][1] == 16384, "rotation recorded");
        require(f.motion_acceleration.x == 0 && f.motion_accel == 0 && f.effect.y == 0 &&
                    f.acceleration.z == 0,
                "vectors zeroed");
        require(f.services.converts == 0 && f.services.creates == 0, "no water work");
    }
    { // No-control clamps at zero instead of going negative.
        Fixture f;
        f.no_control = -3;
        f.run();
        require(f.no_control == 0, "no-control clamped");
    }
    { // Eyes above the water surface: breath resets.
        Fixture f;
        f.prior = 0x400000;
        f.top_y = 4;
        f.run();
        require(f.services.converts == 1, "converted once");
        require(f.services.convert_input.y == 8, "eye vector uses eyes height");
        require(f.breath == 0, "breath cleared above surface");
    }
    { // Eyes at the surface count as underwater: breath increments.
        Fixture f;
        f.prior = 0x2;
        f.top_y = 8;
        f.breath = 41;
        f.run();
        require(f.breath == 42, "breath increments at surface");
        require(f.services.creates == 0, "no task before the limit");
    }
    { // Reaching the limit spawns the task and stores the slot byte.
        Fixture f;
        f.prior = 0x400002;
        f.top_y = 12;
        f.breath = 1079;
        f.state.underwater_word = 9;
        f.run();
        require(f.breath == 1080, "breath reaches limit");
        require(f.services.creates == 1, "breath task created");
        require(f.services.stored_slot == 5, "slot byte stored");
        require(f.state.underwater_word == 0, "word cleared even on spawn");
    }
    { // A failed spawn stores nothing.
        Fixture f;
        f.prior = 0x400002;
        f.top_y = 12;
        f.breath = 1079;
        f.services.create_succeeds = false;
        f.run();
        require(f.services.stored_slot == 0xff, "no slot store on failed spawn");
    }
    { // The history index wraps as a byte.
        Fixture f;
        f.slot = 7;
        f.state.history_index[7] = 255;
        f.run();
        require(f.state.history_index[7] == 0, "index wraps");
        require(f.state.positions[7].x == 400, "wrapped entry recorded");
    }
    std::cout << "player underwater: pose history, timers, breath limit and surface test\n";
    return 0;
}
