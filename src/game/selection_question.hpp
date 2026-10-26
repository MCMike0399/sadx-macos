#pragma once
#include "math/ninja_math.hpp"
#include <array>
#include <cstdint>

namespace sadx::selection_question {
// NOTSA: borrows the actual primary-work pose; no duplicate actor state.
struct Pose {
    std::array<std::int32_t, 3> &rotation;
    const NinjaVector &position;
};
// NOTSA: binds existing source question resources from the one combined pack.
struct Assets {
    const std::uint32_t &object, &menu_texture;
};
struct Calls {
    virtual ~Calls() = default;
    virtual void push_current_matrix() = 0;
    virtual void translate_current(const NinjaVector &) = 0;
    virtual void rotate_z(std::uint16_t angle) = 0;
    virtual void rotate_x(std::uint16_t angle) = 0;
    virtual void rotate_y(std::uint16_t angle) = 0;
    virtual void enable_specular() = 0;
    virtual void select_menu_texture(std::uint32_t) = 0;
    virtual void draw_object(std::uint32_t object) = 0;
    virtual void pop_matrix(std::uint32_t count) = 0;
};
// This source exec updates and draws directly. It installs no other callbacks.
// FUNCTION: SONIC 0x5125a0 CharSel_Qmark_Main
void execute(Pose, const std::uint32_t &logic_ticks, Assets, Calls &);
} // namespace sadx::selection_question
