#pragma once
#include "cpu/fidelity_float.hpp"
#include "engine/task.hpp"
#include "player/player_initialization.hpp"
#include <optional>
#include <stdexcept>

namespace sadx::sonic {
// NOTSA: a helper captures the physics reference independently of its mutable
// containing record. Neither pointer owns or prolongs the source object's life.
// Each separately invoked helper takes a new borrow at its own entry.
template <class Record> struct PlayerBorrow {
    Record *record = nullptr;
    PlayerPhysicsState *physics = nullptr;
    explicit operator bool() const noexcept { return record != nullptr; }
};
// MD_SONIC. Names retain source modes; unknown bodies remain explicit.
enum class Mode : std::uint8_t {
    Initialize,
    Stand,
    Walk,
    WalkPath,
    Spin,
    SpinDash,
    LightSpeedDash,
    SpinPath,
    Jump,
    Spring,
    Launch,
    Parachute,
    Fall,
    Brake,
    SpinAttack,
    Push,
    Damage,
    Back,
    Crash,
    Slide,
    PlaceOn,
    Pick,
    Put,
    Hold,
    Touch,
    HoldWalk,
    Throw,
    HoldBrake,
    HoldFall,
    HoldJump,
    HoldSpring,
    HoldLaunch,
    Pull,
    Through,
    Shake,
    EscapeShake,
    HangWait,
    HangGo,
    LadderWait,
    LadderGo,
    PanelJump,
    PanelWait,
    Capture,
    Ice,
    LightSpeedAttack,
    Cart,
    SandWalk,
    SandSpinDash,
    SandFall,
    SandJump,
    SandCrash,
    SnowWait,
    SnowSlide,
    SnowRotateLeft,
    SnowRotateRight,
    IceWait,
    IceSlip,
    IceRotateLeft,
    IceRotateRight,
    IceJump,
    IceClimb,
    IceFly,
    IceSlide,
    IceFastSlide,
    IceBrake,
    IceFall,
    IceAirJump,
    IceSpecial,
    IceCrash,
    Pinball,
    PinballFall,
    PetStart,
    PetLoop,
    PetEnd,
    Whistle,
    SuperStand,
    SuperWalk,
    SuperPath,
    SuperFall,
    SuperLaunch,
    SuperSpring,
    SuperSpinAttack,
    SuperJump,
    Pose,
    Event,
    Sleep,
    Kill,
    Edit
};
enum class Predicate {
    Event,
    RollRelease,
    Fall,
    Jump,
    Hold,
    Charge,
    StagePickup,
    StageAction,
    JumpPress,
    JumpClassify,
    ClearTasks,
    Idle,
    TurnBrake,
    SpinAnimation,
    JumpAnimation,
    WalkAnimation,
    WallForm,
    SkidEnd,
    ChargeTick
};
enum class Word {
    Equipment,
    Flags,
    JumpTimer,
    IdleTimer,
    Timer,
    ReleaseTimer,
    ReleasePhase,
    RequestedAnimation,
    CurrentAnimation,
    PreviousAnimation
};
enum class Button { Roll, Jump, Hold, StageAction, StagePickup };
enum class Operation {
    SetMotion,
    EventEffect,
    UpgradeRelease,
    PlaySound,
    PlayAttachedSound,
    StopSound,
    EventCamera,
    DropHeld,
    SpecialJump,
    SpecialJumpAnimation,
    FindCollision,
    PlayVoice,
    WallTurn,
    NotifyDamage,
    StageQuery,
    StageApply,
    IdleAnimation,
    Bounce,
    WalkAnimation,
    SpinEffect,
    ReleaseEffect,
    ReleaseDestructor,
    GroundAcceleration,
    AirAcceleration,
    Gravity,
    Speed,
    Position,
    GroundHeading,
    Coordinate,
    FloorAngles,
    Projection,
    Initialize,
    HoldQuery,
    WallForm,
    InputQuery
};
// NOTSA: the actual jump table groups several modes into the same source body.
enum class Body { Initialize, Stand, Walk, Ground, Air, GravityAir, Other, CommonTail };
Body body(std::uint8_t mode);
// NOTSA: native missing compositions fail by semantic operation, never by success defaults.
class Unavailable : public std::runtime_error {
  public:
    explicit Unavailable(const char *operation) : std::runtime_error(operation) {}
};
struct Input {
    bool active = false;
    bool angle_written = false;
    std::int32_t angle = 0;
};
struct Reply {
    std::optional<std::uint32_t> value{};
    std::shared_ptr<const PlayerTaskState> task_work{};
};
} // namespace sadx::sonic
