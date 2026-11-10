#include "player/player_underwater.hpp"

#include <cstdint>
#include <cstring>

namespace {
enum Input : unsigned {
    Status = 0,
    Slot,
    PosX,
    PosY,
    PosZ,
    RotX,
    RotY,
    RotZ,
    MotionAccX,
    MotionAccY,
    MotionAccZ,
    MotionAccel,
    EffX,
    EffY,
    EffZ,
    AccX,
    AccY,
    AccZ,
    NoControl,
    Breath,
    Island,
    EyesHeight,
    ShadowTopY,
    PriorAttrs,
    HistoryIndex,
    ConvX,
    ConvY,
    ConvZ,
    CreateResult,
    ControlWord,
    UnderwaterWord,
    PoseX,
    PoseY,
    PoseZ,
    PoseRotX,
    PoseRotY,
    PoseRotZ,
    Words
};
enum Output : unsigned {
    OutStatus = 0,
    OutNoControl,
    OutBreath,
    OutIsland,
    OutHistoryIndex,
    OutPosX,
    OutPosY,
    OutPosZ,
    OutRotX,
    OutRotY,
    OutRotZ,
    OutUnderwaterWord,
    OutConvInX,
    OutConvInY,
    OutConvInZ,
    OutFlags,
    OutStoredSlot,
    OutMotionAccX,
    OutMotionAccY,
    OutMotionAccZ,
    OutMotionAccel,
    OutEffX,
    OutEffY,
    OutEffZ,
    OutAccX,
    OutAccY,
    OutAccZ,
    OutPrevPosX,
    OutNextPosX,
    OutPrevRotX,
    OutNextRotX,
    OutWords
};
enum Flag : std::uint32_t { Converted = 1, Created = 2, Stored = 4 };
constexpr std::uint32_t kSentinel = 0xeeeeeeeeu;

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

struct Adapter final : sadx::player_underwater::Services {
    const std::uint32_t *in;
    sadx::NinjaVector pose_position;
    std::array<std::int32_t, 3> pose_rotation;
    sadx::NinjaVector convert_input{0, 0, 0};
    std::uint32_t flags = 0;
    std::uint32_t stored_slot = 0;
    explicit Adapter(const std::uint32_t *input)
        : in(input),
          pose_position{float_at(input[PoseX]), float_at(input[PoseY]), float_at(input[PoseZ])},
          pose_rotation{static_cast<std::int32_t>(input[PoseRotX]),
                        static_cast<std::int32_t>(input[PoseRotY]),
                        static_cast<std::int32_t>(input[PoseRotZ])} {}
    sadx::player_underwater::Pose player_pose(std::uint8_t) override {
        return {pose_position, pose_rotation};
    }
    void convert_to_global(sadx::NinjaVector &vector) override {
        convert_input = vector;
        flags |= Converted;
        vector = {float_at(in[ConvX]), float_at(in[ConvY]), float_at(in[ConvZ])};
    }
    std::uintptr_t create_breath_task() override {
        flags |= Created;
        return in[CreateResult] != 0 ? std::uintptr_t{0x1000} : std::uintptr_t{0};
    }
    void store_breath_slot(std::uintptr_t, std::uint8_t slot) override {
        flags |= Stored;
        stored_slot = slot;
    }
};
} // namespace

extern "C" int sadx_native_player_underwater(const std::uint32_t *input, std::uint32_t *output) {
    try {
        using namespace sadx;
        Adapter services(input);
        player_underwater::State state;
        const auto slot = static_cast<std::uint8_t>(input[Slot]);
        for (auto &position : state.positions) {
            const float sentinel = float_at(kSentinel);
            position = {sentinel, sentinel, sentinel};
        }
        for (auto &rotation : state.rotations)
            rotation = {static_cast<std::int32_t>(kSentinel), static_cast<std::int32_t>(kSentinel),
                        static_cast<std::int32_t>(kSentinel)};
        state.history_index[slot] = static_cast<std::uint8_t>(input[HistoryIndex]);
        state.underwater_word = static_cast<std::int32_t>(input[UnderwaterWord]);

        std::uint16_t status = static_cast<std::uint16_t>(input[Status]);
        NinjaVector task_position{float_at(input[PosX]), float_at(input[PosY]),
                                  float_at(input[PosZ])};
        NinjaVector motion_acceleration{float_at(input[MotionAccX]), float_at(input[MotionAccY]),
                                        float_at(input[MotionAccZ])};
        float motion_accel = float_at(input[MotionAccel]);
        NinjaVector effect{float_at(input[EffX]), float_at(input[EffY]), float_at(input[EffZ])};
        NinjaVector acceleration{float_at(input[AccX]), float_at(input[AccY]),
                                 float_at(input[AccZ])};
        std::int16_t no_control = static_cast<std::int16_t>(input[NoControl]);
        std::int16_t breath = static_cast<std::int16_t>(input[Breath]);
        std::uint16_t island = static_cast<std::uint16_t>(input[Island]);
        const float top_y = float_at(input[ShadowTopY]);
        const std::uint32_t prior = input[PriorAttrs];
        player_underwater::PlayerView view{status,
                                           slot,
                                           task_position,
                                           motion_acceleration,
                                           motion_accel,
                                           effect,
                                           acceleration,
                                           no_control,
                                           breath,
                                           island,
                                           float_at(input[EyesHeight]),
                                           top_y,
                                           prior};
        fidelity::Context arithmetic(static_cast<std::uint16_t>(input[ControlWord]));
        player_underwater::run(state, view, services, arithmetic);

        const unsigned entry = slot + static_cast<unsigned>(state.history_index[slot]) * 8u;
        std::memset(output, 0, OutWords * sizeof(std::uint32_t));
        output[OutStatus] = status;
        output[OutNoControl] = static_cast<std::uint16_t>(no_control);
        output[OutBreath] = static_cast<std::uint16_t>(breath);
        output[OutIsland] = island;
        output[OutHistoryIndex] = state.history_index[slot];
        output[OutPosX] = bits_of(state.positions[entry].x);
        output[OutPosY] = bits_of(state.positions[entry].y);
        output[OutPosZ] = bits_of(state.positions[entry].z);
        output[OutRotX] = static_cast<std::uint32_t>(state.rotations[entry][0]);
        output[OutRotY] = static_cast<std::uint32_t>(state.rotations[entry][1]);
        output[OutRotZ] = static_cast<std::uint32_t>(state.rotations[entry][2]);
        output[OutUnderwaterWord] = static_cast<std::uint32_t>(state.underwater_word);
        output[OutConvInX] = bits_of(services.convert_input.x);
        output[OutConvInY] = bits_of(services.convert_input.y);
        output[OutConvInZ] = bits_of(services.convert_input.z);
        output[OutFlags] = services.flags;
        output[OutStoredSlot] = services.stored_slot;
        output[OutMotionAccX] = bits_of(motion_acceleration.x);
        output[OutMotionAccY] = bits_of(motion_acceleration.y);
        output[OutMotionAccZ] = bits_of(motion_acceleration.z);
        output[OutMotionAccel] = bits_of(motion_accel);
        output[OutEffX] = bits_of(effect.x);
        output[OutEffY] = bits_of(effect.y);
        output[OutEffZ] = bits_of(effect.z);
        output[OutAccX] = bits_of(acceleration.x);
        output[OutAccY] = bits_of(acceleration.y);
        output[OutAccZ] = bits_of(acceleration.z);
        output[OutPrevPosX] = entry > 0 ? bits_of(state.positions[entry - 1].x) : kSentinel;
        output[OutNextPosX] =
            entry + 1 < state.positions.size() ? bits_of(state.positions[entry + 1].x) : kSentinel;
        output[OutPrevRotX] =
            entry > 0 ? static_cast<std::uint32_t>(state.rotations[entry - 1][0]) : kSentinel;
        output[OutNextRotX] = entry + 1 < state.rotations.size()
                                  ? static_cast<std::uint32_t>(state.rotations[entry + 1][0])
                                  : kSentinel;
        return 1;
    } catch (...) {
        return 0;
    }
}
