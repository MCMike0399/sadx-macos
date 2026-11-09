#include "cpu/x87_transcendental.hpp"
#include "player/gamma_target_effects.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
enum Input : unsigned {
    Mode = 0, Gun, LockCount, LockCombo, ShotCombo, LaserTimer, LockTimer,
    PositionX, PositionY, PositionZ, AimX, AimY, AimZ, Distance, Scope, SearchPresent,
    TaskMode, ScopeSlot, LockSlot, Mask, Held0, Pressed0 = Held0 + 8,
    Event = Pressed0 + 8, CreateSearch, AllocateBlock, CreateAim, Table, ControlWord,
    Subject0, Words = Subject0 + 4
};
enum Output : unsigned {
    OutMode = 0, OutGun, OutLock, OutLockCombo, OutShot, OutLaser, OutLockTimer,
    OutPositionX, OutPositionY, OutPositionZ, OutAimX, OutAimY, OutAimZ, OutDistance,
    OutScope, OutSearch, OutCalls, OutCall0, OutAttachPlayer = OutCall0 + 12,
    OutAttachExtra, OutAimXPose, OutAimYPose, OutAimZPose, OutYaw, OutPitch, OutAimExtra,
    OutReleaseCount, OutRelease0, OutWords = OutRelease0 + 4
};
enum Call : std::uint32_t {
    Search = 1, Block, Attach, Destroy, Play, Stop, Aim, Store, TableRead, Release
};

struct Adapter final : sadx::gamma_target_effects::Services {
    const std::uint32_t *in;
    std::vector<std::uint32_t> calls;
    bool attach_player = false, attach_extra = false, aim_extra = false;
    sadx::gamma_target_effects::AimPose pose{};
    std::vector<std::uint32_t> releases;
    std::uint8_t controller_slot() override { return static_cast<std::uint8_t>(in[ScopeSlot]); }
    std::uint32_t button_mask() override { return in[Mask]; }
    std::uint32_t held(std::uint8_t slot) override {
        return slot < 8 ? in[Held0 + slot] : 0;
    }
    std::uint32_t pressed(std::uint8_t slot) override {
        return slot < 8 ? in[Pressed0 + slot] : 0;
    }
    bool event_sounds_enabled() override { return in[Event] != 0; }
    std::uintptr_t create_search_task() override {
        calls.push_back(Search);
        return in[CreateSearch] ? 11 : 0;
    }
    std::uintptr_t allocate_search_block() override {
        calls.push_back(Block);
        return in[AllocateBlock] ? 22 : 0;
    }
    void attach_search_block(std::uintptr_t, std::uintptr_t) override {
        attach_player = true;
        attach_extra = true;
    }
    void destroy_task(std::uintptr_t) override { calls.push_back(Destroy); }
    void play_lock_sound() override { calls.push_back(Play); }
    void stop_lock_sound() override { calls.push_back(Stop); }
    std::uintptr_t create_aim_task() override {
        calls.push_back(Aim);
        return in[CreateAim] ? 33 : 0;
    }
    void store_aim(std::uintptr_t, const sadx::gamma_target_effects::AimPose &stored) override {
        pose = stored;
        aim_extra = true;
    }
    std::int32_t read_lock_table(std::int8_t) override {
        calls.push_back(TableRead);
        return static_cast<std::int32_t>(in[Table]);
    }
    std::uintptr_t locked_subject(int index) override {
        if (index < 0 || index >= 4)
            return 0;
        return in[Subject0 + static_cast<unsigned>(index)];
    }
    void release_locked_target(std::uintptr_t subject) override {
        calls.push_back(Release);
        releases.push_back(static_cast<std::uint32_t>(subject));
    }
    explicit Adapter(const std::uint32_t *input) : in(input) {}
};

std::int8_t byte_at(std::uint32_t word) { return static_cast<std::int8_t>(word); }
std::int16_t word_at(std::uint32_t word) { return static_cast<std::int16_t>(word); }
float float_at(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
std::uint32_t bits_of(float value) {
    std::uint32_t word;
    std::memcpy(&word, &value, sizeof(word));
    return word;
}
} // namespace

extern "C" int sadx_gamma_target_fpatan(std::uint64_t y_sig, std::uint16_t y_exp,
                                        std::uint64_t x_sig, std::uint16_t x_exp,
                                        std::uint64_t *out_sig, std::uint16_t *out_exp) {
    try {
        sadx::X87Context arithmetic(0x007f);
        const auto y = arithmetic.from_canonical_ext80(y_exp, y_sig);
        const auto x = arithmetic.from_canonical_ext80(x_exp, x_sig);
        const auto result = sadx::X87Transcendental::fpatan_bochs_p5_p6(arithmetic, y, x);
        *out_sig = result.significand();
        *out_exp = result.sign_exponent();
        return 1;
    } catch (...) {
        return 0;
    }
}

extern "C" int sadx_native_gamma_target_effects(const std::uint32_t *input, std::uint32_t *output) {
    try {
        Adapter services(input);
        sadx::gamma_target_effects::LaserTarget target;
        target.mode = byte_at(input[Mode]);
        target.gun_mode = byte_at(input[Gun]);
        target.lock_count = byte_at(input[LockCount]);
        target.lock_combo = word_at(input[LockCombo]);
        target.shot_combo = word_at(input[ShotCombo]);
        target.laser_timer = word_at(input[LaserTimer]);
        target.lock_timer = word_at(input[LockTimer]);
        target.position_x = float_at(input[PositionX]);
        target.position_y = float_at(input[PositionY]);
        target.position_z = float_at(input[PositionZ]);
        target.aim_x = float_at(input[AimX]);
        target.aim_y = float_at(input[AimY]);
        target.aim_z = float_at(input[AimZ]);
        target.distance = float_at(input[Distance]);
        target.scope = float_at(input[Scope]);
        target.search = input[SearchPresent] ? 11 : 0;
        sadx::X87Context arithmetic(static_cast<std::uint16_t>(input[ControlWord]));
        sadx::gamma_target_effects::update(target, byte_at(input[TaskMode]), services, arithmetic);
        std::memset(output, 0, OutWords * sizeof(std::uint32_t));
        output[OutMode] = static_cast<std::uint8_t>(target.mode);
        output[OutGun] = static_cast<std::uint8_t>(target.gun_mode);
        output[OutLock] = static_cast<std::uint8_t>(target.lock_count);
        output[OutLockCombo] = static_cast<std::uint16_t>(target.lock_combo);
        output[OutShot] = static_cast<std::uint16_t>(target.shot_combo);
        output[OutLaser] = static_cast<std::uint16_t>(target.laser_timer);
        output[OutLockTimer] = static_cast<std::uint16_t>(target.lock_timer);
        output[OutPositionX] = bits_of(target.position_x);
        output[OutPositionY] = bits_of(target.position_y);
        output[OutPositionZ] = bits_of(target.position_z);
        output[OutAimX] = bits_of(target.aim_x);
        output[OutAimY] = bits_of(target.aim_y);
        output[OutAimZ] = bits_of(target.aim_z);
        output[OutDistance] = bits_of(target.distance);
        output[OutScope] = bits_of(target.scope);
        output[OutSearch] = target.search ? 1 : 0;
        output[OutCalls] = static_cast<std::uint32_t>(services.calls.size());
        for (unsigned index = 0; index < services.calls.size() && index < 12; ++index)
            output[OutCall0 + index] = services.calls[index];
        output[OutAttachPlayer] = services.attach_player ? 1 : 0;
        output[OutAttachExtra] = services.attach_extra ? 1 : 0;
        output[OutAimXPose] = bits_of(services.pose.x);
        output[OutAimYPose] = bits_of(services.pose.y);
        output[OutAimZPose] = bits_of(services.pose.z);
        output[OutYaw] = static_cast<std::uint32_t>(services.pose.yaw);
        output[OutPitch] = static_cast<std::uint32_t>(services.pose.pitch);
        output[OutAimExtra] = services.aim_extra ? 1 : 0;
        output[OutReleaseCount] = static_cast<std::uint32_t>(services.releases.size());
        for (unsigned index = 0; index < services.releases.size() && index < 4; ++index)
            output[OutRelease0 + index] = services.releases[index];
        return 1;
    } catch (...) {
        return 0;
    }
}
