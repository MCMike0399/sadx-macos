#pragma once
#include "player/player_heading.hpp"
#include "player/sonic_player.hpp"
#include "player/sonic_player_sites.hpp"

namespace sadx::sonic::detail {
using fidelity::ExactFloat;
// NOTSA: native/reference transports keep the byte-sized source mode owner.
template <class C> void set_mode(C &context, Mode mode) {
    context.mode(static_cast<std::uint8_t>(mode));
}

constexpr std::uint16_t kGrounded = 3; // Status_Ground | Status_OnColli
constexpr std::uint16_t kHurt = 4;
constexpr std::uint16_t kFollowFloor = 32; // Status_Unknown3
constexpr std::uint16_t kBall = 256;
constexpr std::uint16_t kLightDash = 512;
constexpr std::uint16_t kAttack = 1024;
constexpr std::uint16_t kHolding = 2048;
constexpr std::uint16_t kNextAction = 4096;
constexpr std::uint16_t kOnPath = 8192;
constexpr std::uint16_t kDisableControl = 16384;
constexpr std::uint16_t kInvincible = 32768;
constexpr std::uint16_t kSpinFlags = kBall | kAttack;
constexpr std::uint16_t kClearHurt = static_cast<std::uint16_t>(~kHurt);
constexpr std::uint16_t kClearInvincible = static_cast<std::uint16_t>(~kInvincible);
constexpr std::uint16_t kClearPath = static_cast<std::uint16_t>(~kOnPath);
constexpr std::uint16_t kClearSpin = static_cast<std::uint16_t>(~kSpinFlags);
constexpr std::uint16_t kClearOnCollision = 65533;
constexpr std::uint16_t kClearDisableControl = static_cast<std::uint16_t>(~kDisableControl);
constexpr std::uint16_t kClearLightDash = static_cast<std::uint16_t>(~kLightDash);
constexpr std::uint16_t kClearBall = static_cast<std::uint16_t>(~kBall);
constexpr std::uint16_t kEventRetainedFlags = 56063; // source clears ball/attack/on-path
constexpr std::uint32_t kDamageAttribute = 65536;
constexpr std::uint32_t kProjectedInput = 131072;
constexpr std::uint32_t kAlternateFloor = 16384;
constexpr std::uint8_t kDamageClassRetained = 240;
constexpr std::uint32_t kCollisionEnabled = 16;
constexpr std::uint32_t kCarryable = 512;
constexpr std::uint32_t kTouchable = 1024;
constexpr std::uint32_t kSpecialHold = 2048;
constexpr std::uint32_t kTouchOrSpecial = kTouchable | kSpecialHold;
constexpr std::uint32_t kNoJumpTarget = 0xffffffff;

constexpr float kGroundInputThreshold = 0.9f;
constexpr float kTinyForwardSpeed = 0.01f;
constexpr float kTinyAirSpeed = 0.001f;
constexpr float kBounceSpeedMultiplier = -0.3f;
constexpr float kSlideSlopeThreshold = 0.77f;
constexpr float kJumpActorTimerLimit = 30.0f;
constexpr float kPickupHeight = 1.499f;
constexpr float kMaximumCharge = 10.0f;
constexpr float kChargeIncrement = 0.4f;
// NOTSA: unordered comparisons deliberately follow the source parity branches.
inline bool greater_or_unordered(X87Comparison value) {
    return value == X87Comparison::greater || value == X87Comparison::unordered;
}
inline bool less_or_unordered(X87Comparison value) {
    return value == X87Comparison::less || value == X87Comparison::unordered;
}
template <class C> typename C::Result dispatch(C &);
template <class C> auto ground_input(C &);
template <class C> auto copy_local_speed(C &);
template <class C> auto finalize_local_speed(C &, NinjaVector &);
template <class C> void inverse_floor(C &, NinjaVector &);
template <class C> typename C::Result predicate(C &, Predicate, std::optional<std::uint32_t> = {});

// NOTSA: the source asks these predicates in order and returns at the first transition.
// FUNCTION: SONIC 0x4b8320 ResetJumpPanels
template <class C> void clear_tasks(C &c) {
    for (unsigned index = 0; index < 10; ++index) {
        auto task = c.reset_task(index);
        if (task)
            c.clear_task_mode(task, index);
    }
}

// NOTSA: stop at the first source transition.
template <class C> auto common_checks(C &c, bool roll, bool hold, bool charge) {
    for (auto test : {Predicate::Event, Predicate::RollRelease, Predicate::Fall, Predicate::Jump,
                      Predicate::Hold, Predicate::Charge}) {
        if ((test == Predicate::RollRelease && !roll) || (test == Predicate::Hold && !hold) ||
            (test == Predicate::Charge && !charge))
            continue;
        auto result = predicate(c, test);
        c.require_value(result, c.predicate_site(test));
        if (*result.value)
            return result;
    }
    return c.done(0);
}

// NOTSA: complete recovered transition chain for this source mode.
template <class C> auto check_stand(C &c) {
    auto result = common_checks(c, true, true, true);
    if (*result.value)
        return c.done();
    result = predicate(c, Predicate::StagePickup);
    auto p = c.require_player(Site::StandPlayer);
    if (*result.value == 1) {
        set_mode(c, Mode::PetStart);
        c.word(p, Word::RequestedAnimation, 130);
    } else {
        // Packed source request: current mode, next mode, animation number.
        result = predicate(c, Predicate::StageAction, (133U << 16) | (74U << 8) | 1U);
        if (*result.value == 1)
            return c.done();
    }
    if (c.input(Site::StandInput).active) {
        set_mode(c, Mode::Walk);
        c.word(p, Word::RequestedAnimation, 9);
        return c.done();
    }
    const auto slope = ExactFloat(c.fp(), c.contact_direction(p, Site::StandContact).y)
                           .compare(kSlideSlopeThreshold);
    if (slope != X87Comparison::less)
        return c.done();
    const auto length = ExactFloat::retained(c.fp(), rotation_vector_length(c.speed(p), c.fp()));
    if (length.compare(c.parameters(p, Site::StandParameters).slide_speed) !=
        X87Comparison::greater)
        return c.done();
    set_mode(c, Mode::Slide);
    c.word(p, Word::RequestedAnimation, 31);
    return c.done();
}
// NOTSA: complete recovered transition chain for this source mode.
template <class C> auto check_walk(C &c) {
    auto &flags = c.orientation().flags;
    for (auto test : {Predicate::Event, Predicate::RollRelease}) {
        auto result = predicate(c, test);
        if (*result.value)
            return c.done();
    }
    for (auto test : {Predicate::Fall, Predicate::Jump}) {
        auto result = predicate(c, test);
        if (*result.value) {
            flags &= kClearPath;
            return c.done();
        }
    }
    auto result = predicate(c, Predicate::Hold);
    if (*result.value)
        return c.done();
    for (auto test : {Predicate::Charge, Predicate::Idle}) {
        result = predicate(c, test);
        if (*result.value) {
            flags &= kClearPath;
            return c.done();
        }
    }
    auto p = c.require_player(Site::WalkPlayer);
    auto own = c.collision(Site::WalkCollision);
    if ((c.collision_flags(own, Site::WalkCollision) & 16) && !(flags & 4)) {
        const auto info_index = c.collision_index(own, Site::WalkCollisionIndex);
        auto hit = c.required_hit(own, Site::WalkHit, Site::WalkHitRequired);
        auto table = c.infos(hit, Site::WalkHitRequired);
        auto record = c.info(table, info_index, Site::WalkHitAttributes);
        if (!(c.info_attributes(record, Site::WalkHitAttributes) & kOnPath) &&
            (c.collision_flags(own, Site::WalkCollision) & 4)) {
            auto wall = c.wall_turn(Site::WalkWallTurn);
            c.require_value(wall, Site::WalkWallResult, Operation::WallTurn);
            if (*wall.value == 4) {
                set_mode(c, Mode::Push);
                c.word(p, Word::RequestedAnimation, 21);
                flags &= kClearPath;
            }
        }
    }
    result = predicate(c, Predicate::TurnBrake);
    if (!*result.value)
        return c.done();
    auto speed =
        ExactFloat(c.fp(), c.speed(p).x).compare(c.parameters(p, Site::WalkParameters).jog_speed);
    if (less_or_unordered(speed))
        return c.done();
    set_mode(c, Mode::Brake);
    c.word(p, Word::RequestedAnimation, 20);
    const bool alternate = c.alternate(Site::WalkAlternate) != 0;
    c.play_sound(alternate ? 776U : 18U, alternate ? Site::WalkBrakeAlt : Site::WalkBrakeSound);
    flags &= kClearPath;
    return c.done();
}
// NOTSA: complete recovered transition chain for this source mode.
template <class C> auto check_walk_path(C &c) {
    auto &flags = c.orientation().flags;
    flags &= kClearSpin;
    auto result = predicate(c, Predicate::Event);
    if (*result.value)
        return c.done();
    for (auto test : {Predicate::RollRelease, Predicate::Fall, Predicate::Jump}) {
        result = predicate(c, test);
        if (*result.value) {
            flags &= kClearPath;
            return c.done();
        }
    }
    auto p = c.require_player(Site::PathPlayer);
    const auto &parameters = c.parameters(p, Site::PathParameters);
    if (ExactFloat(c.fp(), c.speed(p).x).compare(parameters.jog_speed) == X87Comparison::less) {
        set_mode(c, Mode::Walk);
        c.word(p, Word::RequestedAnimation, 9);
        flags &= kClearPath;
        return c.done();
    }
    result = predicate(c, Predicate::Idle);
    if (*result.value)
        flags &= kClearPath;
    return c.done();
}
// NOTSA: complete recovered transition chain for this source mode.
template <class C> auto check_spin(C &c) {
    auto &flags = c.orientation().flags;
    auto result = predicate(c, Predicate::Event);
    if (!*result.value) {
        result = predicate(c, Predicate::Fall);
        if (!*result.value) {
            predicate(c, Predicate::ChargeTick);
            return c.done();
        }
        predicate(c, Predicate::SpinAnimation);
        auto p = c.require_player(Site::SpinFallPlayer);
        c.charge(p, 5.0f);
        flags |= kSpinFlags;
    }
    c.stop_sound(767, Site::SpinStopCharge);
    c.stop_sound(763, Site::SpinStopUpgrade);
    return c.done();
}
// NOTSA: complete recovered transition chain for this source mode.
template <class C> auto check_spin_dash(C &c) {
    auto &flags = c.orientation().flags;
    auto result = predicate(c, Predicate::Event);
    if (*result.value)
        return c.done();
    result = predicate(c, Predicate::Fall);
    if (*result.value) {
        predicate(c, Predicate::SpinAnimation);
        auto p = c.require_player(Site::DashFallPlayer);
        c.charge(p, 5.0f);
        flags = static_cast<std::uint16_t>((flags & kClearPath) | kSpinFlags);
        return c.done();
    }
    result = predicate(c, Predicate::Jump);
    if (*result.value) {
        flags &= kClearPath;
        return c.done();
    }
    result = predicate(c, Predicate::SkidEnd);
    if (!*result.value) {
        if (!c.pad(false, Button::Roll, Site::DashMask, Site::DashPad))
            return c.done();
        auto p = c.require_player(Site::DashPlayer);
        set_mode(c, Mode::Walk);
        c.word(p, Word::RequestedAnimation, 29);
    }
    flags &= kClearSpin;
    return c.done();
}
// STUB: SONIC 0x496f50 Sonic_Act1 prefix only; DamageSuffix and RemainingChecks are unrecovered
template <class C> auto action_checks(C &c) {
    c.require_task(Site::ActionTask);
    const auto stage = c.stage(Site::Stage);
    if (stage >= 39 && stage <= 42) {
        auto value = c.stage_query(Site::ActionStageQuery);
        c.require_value(value, Site::ActionStageApply, Operation::StageApply);
        c.stage_apply(c.index(), *value.value, Site::ActionStageApply);
    }
    auto player = c.player(Site::ActionPlayer);
    auto &flags = c.orientation().flags;
    if (player && (c.word(player, Word::Flags, Site::ActionFlags) & kInvincible)) {
        flags |= kInvincible;
        c.control_timer(0);
    } else {
        const auto timer = c.control_timer(Site::ActionControlTimer);
        if (timer) {
            flags &= kClearHurt;
            c.control_timer(static_cast<std::uint16_t>(timer - 1));
        } else {
            const auto id = c.task_id(Site::DamageTaskId);
            const bool damage =
                id != 2 && ((flags & 4) || (player && (c.attributes(player) & kDamageAttribute)));
            if (damage) {
                c.control_timer(120);
                c.notify_damage(c.index(), Site::DamageNotify);
                c.unrecovered(Site::DamageSuffix, Operation::NotifyDamage);
            }
            flags &= static_cast<std::uint16_t>(~kInvincible);
        }
    }
    const auto mode = c.mode(Site::ActionMode);
    const auto index =
        static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(mode)) - 1);
    if (index > 86)
        return c.done(index);
    if (mode == static_cast<std::uint8_t>(Mode::Stand))
        return check_stand(c);
    if (mode == static_cast<std::uint8_t>(Mode::Walk))
        return check_walk(c);
    if (mode == static_cast<std::uint8_t>(Mode::WalkPath))
        return check_walk_path(c);
    if (mode == static_cast<std::uint8_t>(Mode::Spin))
        return check_spin(c);
    if (mode == static_cast<std::uint8_t>(Mode::SpinDash))
        return check_spin_dash(c);
    c.unrecovered(Site::RemainingChecks, Operation::StageApply);
    return c.done();
}
// NOTSA: animation requests share the original small effect-construction pattern.
template <class C> auto request_animation(C &c, Predicate test) {
    const bool alternate = c.alternate(c.animation_site(test, 0)) != 0;
    const bool walk = test == Predicate::WalkAnimation;
    auto p = c.require_player(c.animation_site(test, alternate ? 1 : 2));
    const std::uint16_t requested = walk ? 11 : test == Predicate::JumpAnimation ? 16 : 14;
    bool create = false;
    if (alternate) {
        const auto previous = c.word(p, Word::RequestedAnimation, c.animation_site(test, 1));
        create = walk ? previous == 146 : previous != requested;
    }
    if (create) {
        const auto site = c.animation_site(test, 3);
        const auto callback = c.callback(Operation::SpinEffect, site);
        auto result = c.scheduler().create_element(2, 3, callback);
        c.check_creation(result, site);
        if (result.task)
            c.effect_counter(result.task,
                             walk              ? 5
                             : requested == 16 ? 20
                                               : 10,
                             c.animation_site(test, 4));
    }
    c.word(p, Word::RequestedAnimation, requested);
    return c.done();
}

// FUNCTION: SONIC 0x495c30 FUN_00495c30
template <class C> auto request_spin_animation(C &c) {
    return request_animation(c, Predicate::SpinAnimation);
}
// FUNCTION: SONIC 0x495b50 FUN_00495b50
template <class C> auto request_jump_animation(C &c) {
    return request_animation(c, Predicate::JumpAnimation);
}
// FUNCTION: SONIC 0x495ba0 FUN_00495ba0
template <class C> auto request_walk_animation(C &c) {
    return request_animation(c, Predicate::WalkAnimation);
}
// NOTSA: charge cleanup is shared by timeout and the upgraded-release transition.
template <class C, class P> auto charge_cleanup(C &c, const P &p, bool effect) {
    auto &flags = c.orientation().flags;
    const auto speed = ExactFloat(c.fp(), c.speed(p).x);
    c.word(p, Word::Timer, 0);
    const auto comparison = speed.compare(0.0f);
    if (greater_or_unordered(comparison))
        set_mode(c, Mode::Walk);
    else {
        set_mode(c, Mode::Stand);
        c.word(p, Word::IdleTimer, 0);
    }
    c.word(p, Word::RequestedAnimation, 29);
    flags &= kClearSpin;
    c.stop_sound(767, effect ? Site::EnhancedStopCharge : Site::TimeoutStopCharge);
    c.stop_sound(763, effect ? Site::EnhancedStopUpgrade : Site::TimeoutStopUpgrade);
    if (effect && c.alternate(Site::EnhancedAlternate) == 0)
        c.play_sound(1227, Site::EnhancedSound);
    return c.done(1);
}

// FUNCTION: SONIC 0x495080 FUN_00495080
template <class C> auto charge_tick(C &c) {
    auto p = c.require_player(Site::ChargePlayer);
    const auto timer = static_cast<std::int16_t>(c.word(p, Word::Timer, Site::ChargePlayer));
    bool enhanced = false;
    if (timer >= 120)
        enhanced = (c.word(p, Word::Equipment, Site::ChargeUpgradeFirst) & 3) == 3;
    if (!enhanced && timer >= 210)
        enhanced = (c.word(p, Word::Equipment, Site::ChargeUpgradeSecond) & 2) != 0;
    if (enhanced) {
        c.orientation().flags |= kLightDash;
        const auto callback = c.callback(Operation::ReleaseEffect, Site::ReleaseEffect);
        const auto destructor = c.callback(Operation::ReleaseDestructor, Site::ReleaseEffect);
        auto result = c.scheduler().create_element(8, 0, callback);
        c.check_creation(result, Site::ReleaseEffect);
        if (result.task) {
            c.effect_primary(result.task, Site::ReleasePrimary);
            c.effect_player(result.task, p, Site::ReleasePlayer);
            auto &parameters = c.parameters(p, Site::ReleaseParameters);
            c.effect_speed(result.task, parameters.max_x_spd, Site::ReleaseSpeed);
            parameters.max_x_spd = 0.5f;
            result.task->destructor = destructor;
        }
        return charge_cleanup(c, p, true);
    }
    if (timer >= 300)
        return charge_cleanup(c, p, false);
    if (!c.pad(true, Button::Roll, Site::ChargeMask, Site::ChargePad)) {
        set_mode(c, Mode::SpinDash);
        const auto charge = c.charge(p, Site::ReleaseCharge);
        c.word(p, Word::RequestedAnimation, 15);
        c.speed(p).x = charge;
        c.play_sound(768, Site::ReleaseSound);
        c.stop_sound(767, Site::ReleaseStopCharge);
        c.stop_sound(763, Site::ReleaseStopUpgrade);
        c.word(p, Word::Timer, 0);
        c.charge(p, 0.0f);
        if (!(c.orientation().flags & kBall))
            c.stop_sound(763, Site::ReleaseStopWithoutSpin);
        return c.done(1);
    }
    const auto charge = c.charge(p, Site::ChargeValue);
    if (ExactFloat(c.fp(), charge).compare(kMaximumCharge) == X87Comparison::less)
        c.charge(p,
                 (ExactFloat(c.fp(), c.charge(p, Site::ChargeValue)) + kChargeIncrement).rounded());
    const auto equipment = c.word(p, Word::Equipment, Site::ChargeSoundEquipment);
    if (equipment & 2) {
        if (c.orientation().flags & kBall)
            c.play_attached_sound(763, Site::ChargeUpgradeSound);
        else
            c.stop_sound(763, Site::ChargeUpgradeStop);
    } else
        c.play_attached_sound(767, Site::ChargeSound);
    return c.done(0);
}

// NOTSA: source leaf predicates share this typed selector; no address dispatch.
// FUNCTION: SONIC 0x43bfe0 FUN_0043bfe0
template <class C> auto wall_form(C &c) {
    const auto normal = c.normal(Site::WallNormal);
    const auto z = -ExactFloat(c.fp(), normal.z);
    const auto x = -ExactFloat(c.fp(), normal.x);
    auto angle = ExactFloat::arctangent(z, x);
    angle = angle.multiply_double(c.angle_units());
    angle = angle.multiply_double(c.positive_factor());
    const auto distance = angle_distance(
        c.orientation().angles[1],
        static_cast<std::int32_t>(static_cast<std::uint32_t>(angle.truncated_integer())));
    auto result = c.done(distance > 24576 ? 1 : distance > 13312 ? 2 : distance > 7168 ? 3 : 4);
    c.attach_normal(result);
    return result;
}
// FUNCTION: SONIC 0x4930d0 FUN_004930d0
template <class C> auto skid_end(C &c) {
    auto p = c.require_player(Site::SkidPlayer);
    const auto &parameters = c.parameters(p, Site::SkidParameters);
    if (ExactFloat(c.fp(), c.speed(p).x).compare(parameters.run_speed) != X87Comparison::less)
        return c.done(0);
    const auto comparison = ExactFloat(c.fp(), c.speed(p).x).compare(0.0f);
    if (greater_or_unordered(comparison))
        set_mode(c, Mode::Walk);
    else {
        set_mode(c, Mode::Stand);
        c.word(p, Word::IdleTimer, 0);
    }
    c.word(p, Word::RequestedAnimation, 29);
    return c.done(1);
}
// FUNCTION: SONIC 0x4b83f0 FUN_004b83f0
template <class C> auto classify_jump(C &c) {
    auto own = c.collision(Site::JumpCollision);
    const auto hit_present = c.has_hit(own, Site::JumpHit);
    auto actor = c.empty_actor();
    if (hit_present && c.index() == 0) {
        actor = c.hit_actor(own, Site::JumpHit, Site::JumpHitTask, Site::JumpHitWork);
        if (c.actor_id(actor, Site::JumpActorId) != 8)
            actor = c.empty_actor();
    }
    const auto reset_return = [&](std::uint32_t value) {
        if (c.index() == 0)
            clear_tasks(c);
        return c.done(value);
    };
    if (!actor)
        return reset_return(0);
    if (c.actor_timer(actor, Site::JumpActorTimer) == 0)
        return reset_return(kNoJumpTarget);
    if (c.collision_index(own, Site::JumpInfoIndex) != 1)
        return reset_return(0);
    const auto counter = c.actor_counter(actor, Site::JumpActorCounter);
    const auto comparison = ExactFloat::integer(c.fp(), counter).compare(kJumpActorTimerLimit);
    if (less_or_unordered(comparison))
        return c.done(1);
    return reset_return(0);
}
// FUNCTION: SONIC 0x43bf40 JumpAllowed
template <class C> auto jump_pressed(C &c) {
    auto &flags = c.orientation().flags;
    if (flags & kDisableControl)
        return c.done(0);
    if (!c.pad(false, Button::Jump, Site::JumpMask, Site::JumpPad))
        return c.done(0);
    auto result = predicate(c, Predicate::JumpClassify);
    return c.done(static_cast<std::int32_t>(*result.value) > 0 ? 2 : 1);
}
// STUB: SONIC 0x495fa0 FUN_00495fa0 prefix only; EventRemaining is unrecovered
template <class C> auto event_action(C &c) {
    auto &flags = c.orientation().flags;
    if (c.event_mode(Site::EventMode)) {
        c.event_effect(Site::EventEffect);
        return c.done(0);
    }
    if (c.event_link(Site::EventList)) {
        flags &= kEventRetainedFlags;
        set_mode(c, Mode::Event);
        return c.done(1);
    }
    if (!(flags & kNextAction))
        return c.done(0);
    const auto submode = static_cast<std::int8_t>(c.submode(Site::EventSubmode));
    if (submode <= 0 || submode > 50)
        return c.done(0);
    c.unrecovered(Site::EventRemaining, Operation::EventEffect);
    return c.done();
}
// FUNCTION: SONIC 0x494f70 FUN_00494f70
template <class C> auto fall(C &c) {
    auto &flags = c.orientation().flags;
    if (flags & kGrounded)
        return c.done(0);
    if (flags & kBall) {
        set_mode(c, Mode::Jump);
        return c.done(1);
    }
    if (flags & kHolding) {
        auto held = c.hold(Site::FallHold);
        if (*held.value & kCarryable) {
            set_mode(c, Mode::HoldFall);
            auto p = c.require_player(Site::FallHeldPlayer);
            c.word(p, Word::RequestedAnimation, 57);
            return c.done(1);
        }
        c.drop_held(Site::FallDrop);
    }
    set_mode(c, Mode::Fall);
    auto p = c.require_player(Site::FallPlayer);
    c.word(p, Word::RequestedAnimation, 18);
    return c.done(1);
}
// FUNCTION: SONIC 0x492f90 FUN_00492f90
template <class C> auto release_roll(C &c) {
    auto &flags = c.orientation().flags;
    if (c.pad(true, Button::Roll, Site::RollMask, Site::RollPad) || !(flags & kLightDash))
        return c.done(0);
    auto p = c.require_player(Site::RollPlayer);
    bool upgraded = false;
    if (c.word(p, Word::Equipment, Site::RollPlayer) & 4) {
        auto result = c.upgrade_release(Site::RollUpgrade);
        c.require_value(result, Site::RollUpgradeResult, Operation::UpgradeRelease);
        upgraded = *result.value != 0;
    }
    flags &= kClearLightDash;
    if (upgraded) {
        set_mode(c, Mode::LightSpeedAttack);
        c.word(p, Word::RequestedAnimation, 15);
        c.speed(p).y = 8;
        flags |= kSpinFlags;
        c.play_sound(764, Site::RollEnhancedSound);
        c.word(p, Word::Timer, 0);
        const auto stage = static_cast<std::int16_t>(c.stage(Site::RollStage));
        if (stage < 15 || stage > 25)
            c.event_camera(Site::RollCamera);
        auto own = c.collision(Site::RollCollision);
        auto info = c.infos(own, Site::RollInfo);
        auto first = c.info(info, 0, Site::RollDamage);
        c.damage(first, static_cast<std::uint8_t>(c.damage(first, Site::RollDamage) | 3));
        c.damage(first, static_cast<std::uint8_t>(c.damage(first, Site::RollDamage) | 12));
        return c.done(1);
    }
    set_mode(c, Mode::LightSpeedDash);
    c.word(p, Word::RequestedAnimation, 64);
    c.word(p, Word::ReleaseTimer, 10);
    c.word(p, Word::ReleasePhase, 0);
    c.speed(p).x = 8;
    flags = static_cast<std::uint16_t>((flags & kClearBall) | kAttack);
    c.play_sound(764, Site::RollSound);
    if (c.alternate(Site::RollAlternate) == 0) {
        c.stop_sound(1227, Site::RollStopSound);
        c.play_sound(1228, Site::RollStartSound);
    }
    return c.done(1);
}
// FUNCTION: SONIC 0x496ee0 FUN_00496ee0
template <class C> auto start_charge(C &c) {
    auto &flags = c.orientation().flags;
    if (!c.pad(false, Button::Roll, Site::ChargePressMask, Site::ChargePressPad))
        return c.done(0);
    auto p = c.require_player(Site::ChargePressPlayer);
    set_mode(c, Mode::Spin);
    predicate(c, Predicate::SpinAnimation);
    flags |= kSpinFlags;
    const auto comparison = ExactFloat(c.fp(), c.speed(p).x).compare(2.0f);
    c.word(p, Word::Timer, 0);
    c.charge(p, comparison == X87Comparison::greater ? c.speed(p).x : 2.0f);
    c.play_attached_sound(767, Site::ChargePressSound);
    return c.done(1);
}
// FUNCTION: SONIC 0x493120 FUN_00493120
template <class C> auto hold_action(C &c) {
    auto &flags = c.orientation().flags;
    if (!c.pad(false, Button::Hold, Site::HoldMask, Site::HoldPad))
        return c.done(0);
    auto held = c.hold(Site::HoldQuery);
    if (!*held.value)
        return held;
    auto form = predicate(c, Predicate::WallForm);
    c.require_value(form, Site::HoldFormResult, Operation::WallForm);
    const auto type = static_cast<std::int32_t>(*form.value);
    if (type <= 0 || type > 4)
        return c.done(0);
    auto p = c.require_player(Site::HoldPlayer);
    if (*held.value & kTouchable) {
        set_mode(c, Mode::Touch);
        c.word(p, Word::RequestedAnimation, 53);
        auto own = c.collision(Site::HoldCollision);
        c.held_task(p, c.hit_task(own, Site::HoldHit, Site::HoldHitTask, Site::HoldTask));
        flags |= kHolding;
        return c.done(1);
    }
    if (*held.value & kCarryable) {
        set_mode(c, Mode::Pick);
        const auto &parameters = c.parameters(p, Site::HoldParameters);
        const auto comparison = ExactFloat(c.fp(), c.speed(p).x).compare(parameters.jog_speed);
        c.word(p, Word::RequestedAnimation, 33);
        if (!less_or_unordered(comparison))
            c.speed(p).x = parameters.jog_speed;
        return c.done(1);
    }
    if (*held.value & kSpecialHold) {
        set_mode(c, Mode::Touch);
        c.word(p, Word::RequestedAnimation, 53);
        return c.done(1);
    }
    return c.done(0);
}
// FUNCTION: SONIC 0x495e60 FUN_00495e60
template <class C> auto jump(C &c) {
    auto &flags = c.orientation().flags;
    auto first = predicate(c, Predicate::JumpPress);
    if (*first.value != 2) {
        auto second = predicate(c, Predicate::JumpPress);
        if (!*second.value)
            return second;
    }
    auto p = c.require_player(Site::JumpPlayer);
    if (*first.value == 2) {
        if (flags & kHolding)
            c.drop_held(Site::JumpSpecialDrop);
        c.special_jump(Site::JumpSpecial);
        set_mode(c, Mode::PanelJump);
        c.special_jump_animation(Site::JumpSpecialAnimation);
        flags &= kClearOnCollision;
    } else {
        bool carried = false;
        if (flags & kHolding) {
            auto held = c.hold(Site::JumpHold);
            if (*held.value & kCarryable) {
                set_mode(c, Mode::HoldJump);
                c.speed(p).y = c.parameters(p, Site::JumpHeldParameters).jmp_y_spd;
                c.word(p, Word::RequestedAnimation, 55);
                flags &= kClearOnCollision;
                carried = true;
            } else
                c.drop_held(Site::JumpDrop);
        }
        if (!carried) {
            set_mode(c, Mode::Jump);
            c.speed(p).y = c.parameters(p, Site::JumpParameters).jmp_y_spd;
            auto moving = c.moving_task(p, Site::JumpMoving);
            if (moving && moving->force_work)
                c.registered_force(c.index(), Site::RegisteredMotion, Site::RegisteredMotionNull) =
                    c.moving_force(moving, Site::JumpForce);
            predicate(c, Predicate::SpinAnimation);
            c.charge(p, 5.0f);
            flags = static_cast<std::uint16_t>((flags & kClearOnCollision) | kSpinFlags);
        }
    }
    c.word(p, Word::JumpTimer, 0);
    const bool alternate = c.alternate(Site::JumpAlternate) != 0;
    c.play_sound(alternate ? 775U : 17U, alternate ? Site::JumpAltSound : Site::JumpSound);
    return c.done(1);
}
// FUNCTION: SONIC 0x442680 FUN_00442680
template <class C> auto stage_pickup(C &c) {
    const auto stage = c.stage(Site::Stage);
    if (stage < 39 || stage > 42)
        return c.done(0);
    if (!c.pad(true, Button::StagePickup, Site::PickupMask, Site::PickupPad))
        return c.done(0);
    auto held = c.hold(Site::PickupHold);
    if (!*held.value)
        return held;
    auto form = predicate(c, Predicate::WallForm);
    const auto type = static_cast<std::int32_t>(*form.value);
    if (type <= 0 || type > 4 || !(*held.value & kCarryable))
        return c.done(0);
    auto found = c.find_collision(c.orientation().angles[1], Site::PickupFind);
    const auto object = c.collision_return(found, Site::PickupFound);
    if (!object)
        return c.done(0);
    auto y = ExactFloat(c.fp(), c.position(object, Site::PickupOtherPosition).y);
    auto delta = y - c.position(Site::PickupPosition).y;
    if (delta.is_negative())
        delta = -delta;
    return c.done(delta.compare(kPickupHeight) == X87Comparison::greater ? 0 : 1);
}
// FUNCTION: SONIC 0x442570 FUN_00442570
template <class C> auto stage_action(C &c, std::optional<std::uint32_t> argument) {
    const auto stage = c.stage(Site::Stage);
    if (stage < 39 || stage > 42)
        return c.done(0);
    const auto current = c.mode(Site::StageActionMode);
    c.require_argument(argument, Site::StageActionMode);
    if (current != static_cast<std::uint8_t>(*argument))
        return c.done(0);
    if (!c.pad(false, Button::StageAction, Site::StageActionMask, Site::StageActionPad))
        return c.done(0);
    c.mode(static_cast<std::uint8_t>(*argument >> 8));
    auto p = c.require_player(Site::StageActionPlayer);
    c.word(p, Word::RequestedAnimation, static_cast<std::uint16_t>(*argument >> 16));
    const auto character = c.character(Site::StageCharacter);
    constexpr std::array<std::uint32_t, 8> kStageCharacterSounds{
        {775, 0, 776, 794, 0, 820, 0, 856}};
    if (character > 7 || character == 1 || character == 4)
        return c.done(1);
    if (character == 6)
        c.play_voice(Site::StageGammaSound);
    else
        c.play_sound(kStageCharacterSounds[character], c.character_sound_site(character));
    return c.done(1);
}
// FUNCTION: SONIC 0x494ff0 FUN_00494ff0
template <class C> auto idle(C &c) {
    auto &flags = c.orientation().flags;
    if (c.input(Site::IdleInput).active)
        return c.done(0);
    auto p = c.require_player(Site::IdlePlayer);
    if (ExactFloat(c.fp(), c.speed(p).x).compare(0.0f) != X87Comparison::equal)
        return c.done(0);
    if (flags & kHolding) {
        auto held = c.hold(Site::IdleHold);
        set_mode(c, (*held.value & kTouchOrSpecial) ? Mode::Touch : Mode::Hold);
        c.word(p, Word::RequestedAnimation, (*held.value & kTouchOrSpecial) ? 54 : 37);
        return c.done(1);
    }
    set_mode(c, Mode::Stand);
    c.word(p, Word::IdleTimer, 0);
    c.idle_animation(Site::IdleAnimation);
    return c.done(1);
}
// FUNCTION: SONIC 0x4429c0 PCheckBreak
template <class C> auto turn_brake(C &c) {
    auto &flags = c.orientation().flags;
    if (flags & kDisableControl) {
        if (c.event_mode(Site::TurnEvent) != 1)
            return c.done(0);
        auto p = c.registered_player(c.index(), Site::TurnPlayer);
        if (p) {
            auto &speed = c.registered_speed(p, Site::TurnPhysics);
            speed.z = 0;
            speed.y = 0;
            speed.x = 0;
        }
        c.event_mode(0);
        flags &= kClearDisableControl;
        return c.done(1);
    }
    if (flags & kOnPath)
        return c.done(0);
    const auto input = c.input(Site::TurnInput);
    if (!input.active)
        return c.input_reply(false);
    c.require_angle(input, Site::TurnAngle);
    return c.done(angle_distance(c.orientation().angles[1], input.angle) > 24576 ? 1 : 0);
}

template <class C>
typename C::Result predicate(C &c, Predicate test, std::optional<std::uint32_t> argument) {
    if (test == Predicate::ClearTasks) {
        clear_tasks(c);
        return c.done();
    }
    if (test == Predicate::SpinAnimation)
        return request_spin_animation(c);
    if (test == Predicate::JumpAnimation)
        return request_jump_animation(c);
    if (test == Predicate::WalkAnimation)
        return request_walk_animation(c);
    c.require_task(c.predicate_site(test));
    switch (test) {
    case Predicate::WallForm:
        return wall_form(c);
    case Predicate::SkidEnd:
        return skid_end(c);
    case Predicate::JumpClassify:
        return classify_jump(c);
    case Predicate::JumpPress:
        return jump_pressed(c);
    case Predicate::Event:
        return event_action(c);
    case Predicate::Fall:
        return fall(c);
    case Predicate::RollRelease:
        return release_roll(c);
    case Predicate::Charge:
        return start_charge(c);
    case Predicate::Hold:
        return hold_action(c);
    case Predicate::Jump:
        return jump(c);
    case Predicate::ChargeTick:
        return charge_tick(c);
    case Predicate::StagePickup:
        return stage_pickup(c);
    case Predicate::StageAction:
        return stage_action(c, argument);
    case Predicate::Idle:
        return idle(c);
    case Predicate::TurnBrake:
        return turn_brake(c);
    default:
        c.unrecovered(c.predicate_site(test), Operation::StageApply);
        return c.done();
    }
}
// NOTSA: full recovered pre-dispatch ordering, including publications before editor reads.
// STUB: SONIC 0x49a9b0 SonicTheHedgehog prefix only; model, action bodies and CommonTail remain
template <class C> auto run(C &c) {
    c.entry();
    auto &flags = c.orientation().flags;
    if ((flags & kFollowFloor) &&
        !greater_or_unordered(ExactFloat(c.fp(), c.world_speed().y).compare(0.0f))) {
        flags |= 2;
        auto own = c.collision(Site::EntryCollision);
        auto actor =
            c.collision_actor(own, Site::EntryCollision, Site::EntryActor, Site::EntryPitch);
        c.orientation().angles[0] = c.actor_angle(actor, 0, Site::EntryPitch);
        c.orientation().angles[2] = c.actor_angle(actor, 2, Site::EntryRoll);
    } else
        flags &= kClearOnCollision;
    // Both signed words are read even when the first already makes the result nonzero.
    const auto editor = c.editor(Site::Editor);
    const auto forced = c.forced(Site::Forced);
    if (static_cast<std::int32_t>(editor) | static_cast<std::int32_t>(forced))
        set_mode(c, Mode::Edit);
    else if (c.mode(Site::EntryMode) != 86)
        action_checks(c);
    if (c.has_collision(Site::EntryCollisionReference)) {
        auto own = c.collision(Site::EntryInfo);
        auto info = c.infos(own, Site::EntryInfo);
        auto p = c.require_player(Site::EntryPlayer);
        std::uint8_t outgoing = 0, incoming = 0;
        if (c.word(p, Word::Flags, Site::EntryPlayer) & kInvincible)
            outgoing = incoming = 3;
        else if (c.word(p, Word::Equipment, Site::EntryEquipment) & kInvincible)
            outgoing = incoming = 3;
        else {
            const auto mode = c.mode_unchecked();
            if (mode == static_cast<std::uint8_t>(Mode::LightSpeedAttack)) {
                outgoing = 2;
                incoming = 3;
            } else if (mode == static_cast<std::uint8_t>(Mode::LightSpeedDash)) {
                outgoing = 1;
                incoming = 3;
            } else if (flags & kBall)
                outgoing = incoming = 1;
            else if (mode == static_cast<std::uint8_t>(Mode::Cart))
                incoming = 1;
        }
        auto first = c.info(info, 0, Site::EntryDamage);
        c.damage(first, static_cast<std::uint8_t>(
                            (c.damage(first, Site::EntryDamage) & kDamageClassRetained) |
                            (incoming << 2) | outgoing));
        auto second = c.info(info, 1, Site::EntryAttributes);
        const auto attributes = c.info_attributes(second, Site::EntryAttributes);
        c.info_attributes(second, c.mode_unchecked() == 72 ? attributes & ~16U : attributes | 16U);
    }
    if (flags & kBall) {
        auto p = c.require_player(Site::EntryTimer);
        c.word(p, Word::Timer,
               static_cast<std::uint16_t>(c.word(p, Word::Timer, Site::EntryTimer) + 1));
    }
    const auto mode = static_cast<std::int8_t>(c.mode_unchecked());
    c.publish_super(mode >= 75 && mode <= 82 ? 1 : 0);
    return dispatch(c);
}

// NOTSA: movement stages retain source ordering and the position result across later calls.
template <class C>
std::uint32_t movement(C &c, bool ground, bool air, bool gravity, Site acceleration, Site speed,
                       Site position) {
    auto p = c.require_player(speed);
    c.require_motion(speed, Operation::Speed);
    auto borrowed_motion = c.capture_motion();
    auto &parameters = c.parameters(p, speed);
    if (ground)
        c.ground_acceleration(p, parameters, acceleration);
    if (air)
        c.air_acceleration(p, parameters, acceleration);
    if (gravity && !(c.orientation().flags & kDisableControl))
        c.gravity_acceleration(p, parameters, acceleration);
    c.calculate_speed(p, parameters, speed);
    return c.set_position(p, parameters, position);
}

// NOTSA: the source retains floor-transformed local speed before cleaning tiny components.
// FUNCTION: SONIC 0x43ee70 PResetPosition
template <class C> auto copy_local_speed(C &c) {
    auto borrowed_motion = c.capture_motion();
    c.require_speed_frame(Site::CopyWorld);
    auto p = c.require_player(Site::CopyPlayer);
    auto vector = c.world_speed();
    const bool ground = c.orientation().flags & kGrounded;
    if (ground && (c.attributes(p) & kAlternateFloor))
        inverse_floor(c, vector);
    else
        c.world_to_player(vector, Site::CopyTransform);
    return finalize_local_speed(c, vector);
}

template <class C> auto finalize_local_speed(C &c, NinjaVector &vector) {
    c.require_speed_frame(Site::FinalizeWorld);
    auto p = c.require_player(Site::FinalizePlayer);
    const bool ground = c.orientation().flags & kGrounded;
    if (ground)
        vector.y = 0;
    if (ground) {
        if (!(c.prior_attributes(p, Site::FinalizePlayer) & 12)) {
            auto threshold =
                ExactFloat(c.fp(), c.parameters(p, Site::FinalizeParameters).run_accel) *
                kGroundInputThreshold;
            auto x = ExactFloat(c.fp(), vector.x).compare(kTinyForwardSpeed);
            if (x == X87Comparison::less &&
                ExactFloat(c.fp(), vector.x).compare(-kTinyForwardSpeed) == X87Comparison::greater)
                vector.x = 0;
            auto z = ExactFloat(c.fp(), vector.z).compare(threshold);
            if (!greater_or_unordered(z) && !greater_or_unordered((-threshold).compare(vector.z)))
                vector.z = 0;
        }
    } else {
        for (float *value : {&vector.x, &vector.z}) {
            const auto upper = ExactFloat(c.fp(), *value).compare(kTinyAirSpeed);
            if (!greater_or_unordered(upper)) {
                const auto lower = ExactFloat(c.fp(), *value).compare(-kTinyAirSpeed);
                if (!less_or_unordered(lower))
                    *value = 0;
            }
        }
    }
    c.speed(p) = vector;
    for (auto &angle : c.orientation().angles)
        angle = static_cast<std::uint16_t>(angle);
    c.world_angle(0, static_cast<std::uint16_t>(c.world_angle(0, Site::FinalizePitch)));
    c.target_yaw() = static_cast<std::uint16_t>(c.target_yaw());
    c.world_angle(2, static_cast<std::uint16_t>(c.world_angle(2, Site::FinalizeRoll)));
    if (ground && !(c.attributes(p) & kAlternateFloor)) {
        const auto &normal = c.floor_normal(p, Site::FinalizeAlignment);
        const auto &gravity = c.world_gravity(Site::FinalizeAlignment);
        c.alignment(p,
                    (-ExactFloat::retained(c.fp(), inner_product_extended(gravity, normal, c.fp())))
                        .rounded());
    } else
        c.alignment(p, 1.0f);
    return c.done();
}

// FUNCTION: SONIC 0x4915f0 FUN_004915f0
template <class C> auto update_collision_center(C &c) {
    c.require_task(Site::CenterTask);
    c.require_collision_reference(Site::CenterTask);
    auto p = c.require_player(Site::CenterPlayer);
    const auto &parameters = c.parameters(p, Site::CenterPlayer);
    auto own = c.collision(Site::CenterCollision);
    auto table = c.infos(own, Site::CenterCollision);
    NinjaVector vector{0, parameters.center_height, 0};
    c.task_to_world(vector, Site::CenterTransform);
    auto first = c.info(table, 0, Site::CenterStore);
    c.center(first, vector);
    const auto position = c.position(Site::CenterPosition);
    auto &center = c.center(first);
    center.x = (ExactFloat(c.fp(), position.x) + center.x).rounded();
    center.y = (ExactFloat(c.fp(), position.y) + center.y).rounded();
    center.z = (ExactFloat(c.fp(), position.z) + center.z).rounded();
    return c.done();
}

// NOTSA: the recovered action bodies stop at the same still-unrecovered common tail.
template <class C> typename C::Result dispatch(C &c) {
    c.require_dispatch_frame(Site::DispatchMode);
    const auto mode = c.mode(Site::DispatchMode);
    c.dispatching(mode);
    const auto selected = body(mode);
    if (selected == Body::CommonTail) {
        c.set_motion(Site::CommonAnimation);
        c.unrecovered(Site::CommonTail, Operation::SetMotion);
    }
    if (mode == static_cast<std::uint8_t>(Mode::Initialize)) {
        c.character_id(0);
        c.initialize(Site::Initialize);
        auto p = c.player_unchecked();
        c.bind_animation(p, Site::InitializeAnimation);
        c.word(p, Word::RequestedAnimation, 0);
        const auto alternate = c.alternate(Site::InitializeAlternate);
        c.initialize_model(alternate ? Site::InitializeAlternateModel : Site::InitializeModel);
    }
    auto p = c.require_player(c.body_site(mode));
    const bool recovered = mode == static_cast<std::uint8_t>(Mode::Stand) ||
                           mode == static_cast<std::uint8_t>(Mode::Walk) ||
                           mode == static_cast<std::uint8_t>(Mode::Spring) ||
                           mode == static_cast<std::uint8_t>(Mode::Parachute) ||
                           mode == static_cast<std::uint8_t>(Mode::Hold) ||
                           mode == static_cast<std::uint8_t>(Mode::HoldSpring) ||
                           mode == static_cast<std::uint8_t>(Mode::PetLoop) ||
                           mode == static_cast<std::uint8_t>(Mode::PetEnd) ||
                           mode == static_cast<std::uint8_t>(Mode::Whistle) ||
                           mode == static_cast<std::uint8_t>(Mode::SuperStand) ||
                           mode == static_cast<std::uint8_t>(Mode::SuperSpring);
    if (!recovered)
        c.unrecovered(c.body_site(mode), Operation::SetMotion);
    c.require_motion(c.body_site(mode));
    if (selected == Body::Stand || selected == Body::Ground) {
        ground_input(c);
        movement(c, true, false, false,
                 mode == static_cast<std::uint8_t>(Mode::Stand) ? Site::StandAcceleration
                                                                : Site::GroundAcceleration,
                 mode == static_cast<std::uint8_t>(Mode::Stand) ? Site::StandSpeed
                                                                : Site::GroundSpeed,
                 mode == static_cast<std::uint8_t>(Mode::Stand) ? Site::StandPosition
                                                                : Site::GroundPosition);
        copy_local_speed(c);
        if (mode == static_cast<std::uint8_t>(Mode::Stand))
            c.idle_animation(Site::StandAnimation);
    } else if (selected == Body::Walk) {
        if (c.word(p, Word::PreviousAnimation, Site::WalkPreviousAnimation) == 30 &&
            c.word(p, Word::CurrentAnimation, Site::WalkCurrentAnimation) != 30) {
            const auto &parameters = c.parameters(p, Site::WalkJogParameters);
            c.speed(p).x = (ExactFloat(c.fp(), parameters.jog_speed) + c.speed(p).x).rounded();
        }
        const auto position = movement(c, true, false, false, Site::WalkAcceleration,
                                       Site::WalkSpeed, Site::WalkPosition);
        if (position == 2) {
            if (c.index() == 0 && c.alternate(Site::BounceAlternate) == 0)
                c.play_sound(1222, Site::BounceSound);
            set_mode(c, Mode::Crash);
            c.word(p, Word::RequestedAnimation, 27);
            c.bounce(Site::Bounce);
            c.speed(p).x = (ExactFloat(c.fp(), c.speed(p).x) * kBounceSpeedMultiplier).rounded();
            c.speed(p).z = (ExactFloat(c.fp(), c.speed(p).z) * kBounceSpeedMultiplier).rounded();
        } else {
            copy_local_speed(c);
            c.walk_animation(Site::WalkAnimation);
        }
    } else if (selected == Body::Air || selected == Body::GravityAir) {
        movement(c, false, selected == Body::Air, selected == Body::GravityAir,
                 selected == Body::GravityAir ? Site::AirGravity : Site::AirAcceleration,
                 Site::AirSpeed, Site::AirPosition);
        copy_local_speed(c);
    } else
        c.unrecovered(c.body_site(mode), Operation::SetMotion);
    c.set_motion(Site::CommonAnimation);
    c.unrecovered(Site::CommonTail, Operation::SetMotion);
    return c.done();
}

// FUNCTION: SONIC 0x43ed20 FUN_0043ed20
template <class C> void inverse_floor(C &c, NinjaVector &vector) {
    auto p = c.player_unchecked();
    auto &coordinates = c.coordinates();
    const auto angles = c.floor_angles(c.floor_normal(p, Site::InverseNormal), Site::InverseAngles);
    const auto input = vector;
    coordinates.begin_unit_frame();
    auto &matrix = coordinates.current_matrix();
    const auto yaw = c.orientation().angles[1];
    if (yaw)
        coordinates.rotate_y(matrix, static_cast<std::uint16_t>(yaw), c.fp());
    const auto pitch = 0U - static_cast<std::uint32_t>(angles.inclination);
    const auto roll = 0U - static_cast<std::uint32_t>(angles.roll);
    if (pitch)
        coordinates.rotate_x(matrix, pitch & 65535, c.fp());
    if (roll)
        coordinates.rotate_z(matrix, roll & 65535, c.fp());
    c.coordinate_result(coordinates.calculate_vector(matrix, input, vector, c.fp()),
                        Site::InverseVector);
    coordinates.end_frame();
}

// FUNCTION: SONIC 0x44bb60 PGetRotation
template <class C> auto ground_input(C &c) {
    auto borrowed_motion = c.capture_motion();
    c.require_task(Site::GroundTask);
    if (c.orientation().flags & kDisableControl)
        return c.done();
    auto p = c.require_player(Site::GroundPlayer);
    c.require_world(Site::GroundWorld);
    if (c.orientation().flags & kOnPath)
        c.reset_ground_direction(p, Site::GroundDirection);
    else {
        auto input = c.input(Site::GroundInput);
        if (!input.active) {
            if (c.orientation().flags & kGrounded) {
                const auto comparison = ExactFloat(c.fp(), c.speed(p).x).compare(2.0f);
                c.passive_rotation(p, comparison == X87Comparison::greater
                                          ? Site::GroundPassiveFast
                                          : Site::GroundPassiveSlow);
                return c.done();
            }
            c.target_yaw() = c.orientation().angles[1];
        } else if (c.attributes(p) & kProjectedInput) {
            const auto raw = c.raw_axes(Site::GroundAxes);
            auto x = ExactFloat::integer(c.fp(), static_cast<std::int32_t>(raw[0]) * 256);
            auto y = ExactFloat::integer(c.fp(), static_cast<std::int32_t>(raw[1]) * 256);
            auto angle = ExactFloat::arctangent(x, y);
            angle = angle.multiply_double(c.surface_angle_units());
            angle = angle.multiply_double(c.surface_positive_factor());
            const auto raw_angle = static_cast<std::uint32_t>(angle.truncated_integer());
            auto &coordinates = c.coordinates();
            coordinates.begin_unit_frame();
            auto &camera = coordinates.current_matrix();
            const auto angles = c.camera_angles(Site::GroundCamera);
            if (angles[1])
                coordinates.rotate_y(camera, static_cast<std::uint16_t>(angles[1]), c.fp());
            if (angles[0])
                coordinates.rotate_x(camera, static_cast<std::uint16_t>(angles[0]), c.fp());
            NinjaVector vector{0, 0, -1}, camera_direction, projected;
            c.coordinate_result(
                coordinates.calculate_vector(camera, vector, camera_direction, c.fp()),
                Site::GroundCameraVector);
            coordinates.end_frame();
            coordinates.begin_unit_frame();
            auto &floor = coordinates.current_matrix();
            const auto &normal = c.floor_normal(p, Site::GroundNormal);
            coordinates.rotate_axis(floor, normal, static_cast<std::int32_t>(raw_angle), c.axis(),
                                    c.fp());
            c.coordinate_result(
                coordinates.calculate_vector(floor, camera_direction, projected, c.fp()),
                Site::GroundFloorVector);
            coordinates.end_frame();
            vector = {1, 0, 0};
            c.coordinate_result(coordinates.task_to_world(&c.orientation(), vector, c.fp()),
                                Site::GroundTaskVector);
            const auto projected_angle =
                c.project_angle(vector, projected, normal, Site::GroundProjection);
            c.target_yaw() =
                static_cast<std::int32_t>((static_cast<std::uint32_t>(c.orientation().angles[1]) +
                                           static_cast<std::uint32_t>(projected_angle)) &
                                          65535);
            c.blended_rotation(p, c.target_yaw(), Site::GroundBlended);
        } else {
            c.require_angle(input, Site::GroundAngle);
            c.target_yaw() = input.angle;
        }
    }
    c.regular_rotation(p, c.target_yaw(), Site::GroundRegular);
    return c.done();
}
} // namespace sadx::sonic::detail
