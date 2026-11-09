#include "player/gamma_target_effects.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value, const std::string &message) {
    if (!value)
        throw std::runtime_error(message);
}

struct Fake final : sadx::gamma_target_effects::Services {
    std::uint32_t mask = 1;
    std::uint32_t held_bits = 0;
    std::uint32_t pressed_bits = 0;
    bool sounds = false;
    bool fail_block = false;
    bool fail_aim = false;
    std::int32_t table = 0;
    std::vector<std::string> calls;
    std::uint8_t controller_slot() override { return 0; }
    std::uint32_t button_mask() override { return mask; }
    std::uint32_t held(std::uint8_t) override { return held_bits; }
    std::uint32_t pressed(std::uint8_t) override { return pressed_bits; }
    bool event_sounds_enabled() override { return sounds; }
    std::uintptr_t create_search_task() override {
        calls.push_back("search");
        return 11;
    }
    std::uintptr_t allocate_search_block() override {
        calls.push_back("block");
        return fail_block ? 0 : 22;
    }
    void attach_search_block(std::uintptr_t, std::uintptr_t) override { calls.push_back("attach"); }
    void destroy_task(std::uintptr_t task) override { calls.push_back(task ? "destroy" : "destroy-null"); }
    void play_lock_sound() override { calls.push_back("play"); }
    void stop_lock_sound() override { calls.push_back("stop"); }
    std::uintptr_t create_aim_task() override {
        calls.push_back("aim");
        return fail_aim ? 0 : 33;
    }
    void store_aim(std::uintptr_t, const sadx::gamma_target_effects::AimPose &pose) override {
        calls.push_back("store");
        stored = pose;
    }
    std::int32_t read_lock_table(std::int8_t mode) override {
        calls.push_back("table");
        seen_mode = mode;
        return table;
    }
    std::uintptr_t locked_subject(int index) override {
        return index >= 0 && index < static_cast<int>(subjects.size()) ? subjects[static_cast<std::size_t>(index)] : 0;
    }
    void release_locked_target(std::uintptr_t subject) override {
        calls.push_back("release-" + std::to_string(subject));
    }
    std::vector<std::uintptr_t> subjects;
    sadx::gamma_target_effects::AimPose stored{};
    std::int8_t seen_mode = 0;
};

void ready_scope_stays_zero_until_the_button_is_held() {
    sadx::X87Context arithmetic(0x007f);
    Fake services;
    sadx::gamma_target_effects::LaserTarget target;
    target.scope = 4;
    target.distance = 9;
    target.lock_timer = 3;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.mode == 1 && target.scope == 0 && target.distance == 0 && target.lock_timer == 0,
            "mode 0 clears the scope, distance and lock timer, then the release clamps it");
    target.mode = 1;
    target.scope = 5;
    services.held_bits = 1;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.scope == 6.0f, "a held masked button advances the scope");
    target.scope = 19;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.scope == 19.0f, "the scope stops at 19");
    services.held_bits = 0;
    target.scope = 0;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.scope == 0.0f, "a released button cannot drive the scope below zero");
}

void an_unlisted_mode_only_moves_the_scope() {
    sadx::X87Context arithmetic(0x007f);
    Fake services;
    services.held_bits = 1;
    sadx::gamma_target_effects::LaserTarget target;
    target.mode = 7;
    target.scope = 2;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.mode == 7 && target.scope == 3.0f, "modes above 6 skip the switch");
    target.mode = -1;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.mode == -1 && target.scope == 4.0f, "a negative mode byte is above the switch");
}

void pressed_input_asks_the_lock_table_before_acquiring() {
    sadx::X87Context arithmetic(0x007f);
    Fake services;
    services.pressed_bits = 1;
    services.table = 0;
    sadx::gamma_target_effects::LaserTarget target;
    target.mode = 1;
    target.scope = 1;
    sadx::gamma_target_effects::update(target, 4, services, arithmetic);
    require(target.mode == 1 && services.seen_mode == 4 && services.calls == std::vector<std::string>{"table"},
            "a zero table read leaves ready mode");
    services.calls.clear();
    services.table = 1;
    sadx::gamma_target_effects::update(target, 4, services, arithmetic);
    require(target.mode == 2, "a nonzero table read requests acquisition");
}

void acquisition_keeps_source_order_including_a_failed_block() {
    sadx::X87Context arithmetic(0x007f);
    Fake services;
    services.sounds = true;
    sadx::gamma_target_effects::LaserTarget target;
    target.mode = 2;
    target.scope = 1;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(services.calls == std::vector<std::string>{"search", "block", "attach", "play"},
            "search, block, attach, then the event sound");
    require(target.mode == 3 && target.laser_timer == 180 && target.search == 11, "a linked search locks");
    services.calls.clear();
    services.fail_block = true;
    target.mode = 2;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(services.calls == std::vector<std::string>{"search", "block", "destroy", "play"},
            "a failed block destroys the search and still plays the event sound");
    require(target.mode == 2 && target.search == 11, "the failed search id is stored and not cleared");
}

void empty_release_finishes_without_a_callee() {
    sadx::X87Context arithmetic(0x007f);
    Fake services;
    sadx::gamma_target_effects::LaserTarget target;
    target.mode = 4;
    target.gun_mode = 9;
    target.scope = 1;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.mode == 6 && target.gun_mode == 0 && services.calls.empty(),
            "no locks finishes the laser without a release call");
    services.subjects = {0, 5};
    target.mode = 4;
    target.gun_mode = 9;
    target.lock_count = 2;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(target.mode == 6 && target.gun_mode == 0 &&
                services.calls == std::vector<std::string>{"release-5"},
            "a null auxiliary is skipped and the next subject is released");
}

void clear_forgets_the_search_then_can_reacquire() {
    sadx::X87Context arithmetic(0x007f);
    Fake services;
    sadx::gamma_target_effects::LaserTarget target;
    target.mode = 5;
    target.search = 11;
    target.scope = 1;
    sadx::gamma_target_effects::update(target, 4, services, arithmetic);
    require(target.mode == 1 && target.search == 0 &&
                services.calls == std::vector<std::string>{"destroy", "stop"},
            "clear destroys, stores zero, returns to ready, and stops the sound");
    services.calls.clear();
    services.pressed_bits = 1;
    services.table = 7;
    target.mode = 6;
    target.search = 11;
    sadx::gamma_target_effects::update(target, 4, services, arithmetic);
    require(target.mode == 2 && target.search == 0 && services.seen_mode == 4 &&
                services.calls == std::vector<std::string>{"destroy", "stop", "table"},
            "a pressed lock after clear asks the table and can request acquisition");
}

void aim_stores_the_horizontal_length_before_pitch() {
    sadx::X87Context arithmetic(0x007f);
    Fake services;
    sadx::gamma_target_effects::LaserTarget target;
    target.mode = 3;
    target.scope = 1;
    target.position_x = 8;
    target.aim_x = 3;
    target.aim_y = 4;
    target.aim_z = 0;
    target.laser_timer = 1;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(services.calls == std::vector<std::string>{"aim", "store"}, "a created aim task is stored");
    require(services.stored.x == 8 && services.stored.y == 0 && services.stored.z == 0,
            "the aim position is the laser position");
    require(services.stored.yaw == 0 && services.stored.pitch == 9672,
            "yaw " + std::to_string(services.stored.yaw) + " pitch " +
                std::to_string(services.stored.pitch));
    require(target.mode == 4 && target.laser_timer == 0, "an expired timer releases without the table");
    services.calls.clear();
    target.mode = 3;
    target.laser_timer = 1;
    target.aim_x = 0;
    target.aim_z = 3;
    target.aim_y = 0;
    sadx::gamma_target_effects::update(target, 0, services, arithmetic);
    require(services.stored.yaw == -16384 && services.stored.pitch == 0,
            "yaw " + std::to_string(services.stored.yaw) + " pitch " +
                std::to_string(services.stored.pitch));
}
} // namespace

int main() {
    try {
        ready_scope_stays_zero_until_the_button_is_held();
        an_unlisted_mode_only_moves_the_scope();
        pressed_input_asks_the_lock_table_before_acquiring();
        acquisition_keeps_source_order_including_a_failed_block();
        empty_release_finishes_without_a_callee();
        clear_forgets_the_search_then_can_reacquire();
        aim_stores_the_horizontal_length_before_pitch();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Gamma target effects: init, scope clamp, acquisition order and aim angles passed\n";
    return 0;
}
