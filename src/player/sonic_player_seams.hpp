#pragma once
#include "player/sonic_player.hpp"
namespace sadx::sonic {
struct Context;
namespace calls {
// NOTSA: composition seam to the maintained sound-effects owner.
void play_sound(Context &, std::uint32_t sound);
// STUB: SONIC 0x423f50 FUN_00423f50 attached sound composition remains open
void play_attached_sound(Context &, std::uint32_t sound);
// STUB: SONIC 0x424210 FUN_00424210 sound stop composition remains open
void stop_sound(Context &, std::uint32_t sound);
// STUB: SONIC 0x45a870 FUN_0045a870 event effect composition remains open
void event_effect(Context &);
// STUB: SONIC 0x492710 FUN_00492710 upgrade-release predicate remains open
std::uint32_t upgrade_release(Context &);
// STUB: SONIC 0x437d20 FUN_00437d20 event camera composition remains open
void event_camera(Context &);
// STUB: SONIC 0x473b00 FUN_00473b00 held-task drop remains open
void drop_held(Context &);
// STUB: SONIC 0x4b8470 FUN_004b8470 special jump remains open
void special_jump(Context &);
// STUB: SONIC 0x495c80 FUN_00495c80 special jump animation remains open
void special_jump_animation(Context &);
// STUB: SONIC 0x41c0c0 FUN_0041c0c0 collision query composition remains open
std::shared_ptr<const PlayerTaskState> find_collision(Context &, std::int32_t yaw);
// STUB: SONIC 0x423be0 FUN_00423be0 stage voice composition remains open
void play_voice(Context &);
// STUB: SONIC 0x43c060 FUN_0043c060 wall-turn predicate remains open
std::uint32_t wall_turn(Context &);
// STUB: SONIC 0x4bcc10 FUN_004bcc10 damage notification remains open
void notify_damage(Context &, std::uint8_t player_index);
// STUB: SONIC 0x491550 FUN_00491550 stage query remains open
std::uint32_t stage_query(Context &);
// STUB: SONIC 0x442720 FUN_00442720 stage action remains open
void stage_apply(Context &, std::uint8_t player_index, std::uint32_t value);
// STUB: SONIC 0x491660 FUN_00491660 idle animation remains open
void idle_animation(Context &);
// STUB: SONIC 0x440790 FUN_00440790 bounce composition remains open
void bounce(Context &);
// STUB: SONIC 0x495cd0 FUN_00495cd0 walking animation remains open
void walk_animation(Context &);
} // namespace calls
} // namespace sadx::sonic
