#pragma once
#include "engine/object_draw.hpp"

// Reference-only compatibility for the byte-preserved 2026-10-21 probe/unit.
// Product translation units must never include this header.
namespace sadx::object_draw {
struct Calls {
    virtual ~Calls() = default;
    virtual const Object &object(ObjectId) = 0;
    virtual void push_matrix_ex() = 0;
    virtual void translate_current(const std::array<float, 3> &) = 0;
    virtual void rotate_current(RotationOrder, const std::array<std::int32_t, 3> &) = 0;
    virtual void scale_current(const std::array<float, 3> &) = 0;
    virtual void pop_matrix_ex() = 0;
    virtual bool texture_list_not_ready() = 0;
    virtual void draw_late_object(ObjectId, std::uint32_t) = 0;
    virtual void draw_model(ModelId) = 0;
};
std::int32_t draw_simple_model(ModelId model, Calls &calls);
void draw_objects(ObjectId root, State state, Calls &calls);
void draw(ObjectId root, State state, Calls &calls);
} // namespace sadx::object_draw
