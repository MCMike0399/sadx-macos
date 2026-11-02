#pragma once
#include "engine/character_pack.hpp"
#include "cpu/fidelity_float.hpp"
#include "engine/ninja_matrix.hpp"
#include "engine/task.hpp"

namespace sadx::gamma_display {
using ObjectId = character_pack::ResourceId;
struct Context;
enum class Axis { X, Y, Z };

// NOTSA: per-consumption transport to canonical task/player/scene/display owners.
void capture(Context &, const TaskHandle &);
std::uint16_t equipment(Context &);
void capture_extra(Context &);
ObjectId object_row(Context &, unsigned);
ObjectId sibling(Context &, ObjectId);
ObjectId child(Context &, ObjectId);
void store_sibling(Context &, ObjectId, ObjectId);
void store_weld_source(Context &, ObjectId);
void select_palette(Context &, std::int32_t);
std::uint16_t requested_animation(Context &);
std::uint16_t current_animation(Context &);
std::int16_t animation_mode(Context &);
std::uint16_t status(Context &);
float motion_height(Context &);
std::int32_t angle(Context &, Axis);
float position(Context &, Axis);
fidelity::ExactFloat exact(Context &, float);
void select_body_textures(Context &);
void save_control(Context &);
void set_control(Context &, std::uint32_t);
void set_white_material(Context &);
void save_attributes(Context &);
void clear_attributes(Context &, std::uint32_t, std::uint32_t);
void push_current(Context &);
void push_unit(Context &);
void rotate(Context &, Axis, std::uint16_t);
NinjaVector transform_vector(Context &, const NinjaVector &);
void pop(Context &);
void translate(Context &, const NinjaVector &);
void capture_collision_info(Context &);
void store_collision_center(Context &, unsigned record, Axis, float);
void store_walk_phase(Context &, std::uint32_t);
bool has_event_action(Context &);
// NOTSA: reached source branches still awaiting recovery, never skipped.
[[noreturn]] void walking_sway(Context &);
[[noreturn]] void event_model(Context &);
[[noreturn]] void hidden_tail(Context &);
void set_object_callback(Context &);
[[noreturn]] void draw_player_action(Context &, std::uint16_t animation);

// STUB: SONIC 0x47fd50 E102Display prefix through first action draw; later attachments and tail unresolved
void display(Context &, const TaskHandle &);
} // namespace sadx::gamma_display
