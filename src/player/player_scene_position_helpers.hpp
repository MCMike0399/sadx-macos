#pragma once

#include "engine/task.hpp"
#include "player/player_coordinates.hpp"
#include "player/player_state.hpp"
#include <functional>
#include <stdexcept>

namespace sadx::scene {

struct Actor {
    NinjaVector position;
    PlayerOrientation orientation;
    std::uint8_t player_index;
};

struct FloorContact {
    NinjaVector direction;
    NinjaVector normal;
    std::int32_t pitch;
    std::int32_t roll;
};

struct PlatformRow;
using PlatformCallback =
    std::function<void(const TaskHandle &, Actor &, PlatformRow &, X87Context &)>;
struct PlatformRow {
    PlatformCallback callback;
    std::array<std::int32_t, 3> angle_delta;
    NinjaVector displacement;
};
struct PlatformWork {
    std::array<PlatformRow, 2> rows;
};

struct MotionConstants {
    float push_distance = 3.0f;
    float zero = 0.0f;
    double angle_units = 65536.0;
    double radians_to_turns = 0.15915497620314795;
};

enum class MotionFailure { CoordinateUnavailable };
class MotionError : public std::runtime_error {
  public:
    explicit MotionError(MotionFailure failure)
        : std::runtime_error("scene motion coordinate owner failed"), failure_(failure) {}
    MotionFailure failure() const { return failure_; }

  private:
    MotionFailure failure_;
};

// NOTSA: native composition interfaces for the shared game calculations.
// All state is borrowed. Callback mutations and stores before a failure remain observable.
void nudge_scene_normal(Actor &, MotionWork &, const NinjaVector &normal, X87Context &,
                        const MotionConstants & = {});
// A null work pointer is the source's observed absence of platform force work.
// The registered player handles are supplied by their native owner.
void move_scene_platform(Actor &, const PlayerMotion &, NinjaVector &center,
                         const NinjaVector &offset, PlatformWork *,
                         const std::array<TaskHandle, 8> &registered_players, X87Context &,
                         const MotionConstants & = {});
void rotate_scene_floor(Actor &, const PlayerMotion &, const std::uint16_t &player_flags,
                        const PlayerParameters &, const FloorContact &, const NinjaVector &gravity,
                        PlayerCoordinates &, X87Context &, const MotionConstants & = {});

} // namespace sadx::scene
