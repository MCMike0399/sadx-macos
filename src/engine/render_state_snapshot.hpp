#pragma once
#include "engine/ninja_queue_native.hpp"
#include "cpu/fidelity_float.hpp"

namespace sadx::render_snapshot {
enum class ObjectCallback { None, Gamma };
struct State {
    // GLOBAL: SONIC 0x3b2d3c4 _nj_control_3d_backup size=4
    std::uint32_t control = 0;
    // GLOBAL: SONIC 0x3b2d3c8 _backup_nj_constant_attr_and_ size=4
    std::uint32_t mask = 0;
    // GLOBAL: SONIC 0x3b2d3c0 _backup_nj_constant_attr_or_ size=4
    std::uint32_t attributes = 0;
    // GLOBAL: SONIC 0x3b18220 _nj_constant_material_temp size=16
    std::array<float, 4> material = {};
    // GLOBAL: SONIC 0x3d08844 NinjaObjectCallback size=4
    ObjectCallback object_callback = ObjectCallback::None;
};
struct View {
    State &saved;
    std::uint32_t &control, &mask, &attributes;
};
// FUNCTION: SONIC 0x4395c0 SaveControl3D
void save_control(View);
// FUNCTION: SONIC 0x4395d0 LoadControl3D
void load_control(View);
// FUNCTION: SONIC 0x439520 SaveConstantAttr
void save_attributes(View);
// FUNCTION: SONIC 0x439540 LoadConstantAttr
void load_attributes(View);
// FUNCTION: SONIC 0x439590 OffConstantAttr
void clear_attributes(View, std::uint32_t mask, std::uint32_t attributes);
// FUNCTION: SONIC 0x77ff60 njControl3D
void set_control(View, std::uint32_t);
// FUNCTION: SONIC 0x4128a0 SetMaterial
void set_material(State &, const std::array<float, 4> &, queue::State &,
                  const queue::Services &, fidelity::Context &);
// FUNCTION: SONIC 0x7802d0 njSetObjectCallback
void set_object_callback(State &, ObjectCallback);
} // namespace sadx::render_snapshot
