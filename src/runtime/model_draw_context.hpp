#pragma once
#include "runtime/native_display.hpp"

namespace sadx::basic_model {
// NOTSA: a call borrows all inputs; source render scratch stays owned by Display.
struct Context {
    native::Display &display;
    const native::ModelDrawEnvironment &environment;
    auto &device() { return display.device_state_; }
    auto &state() { return display.model_state_; }
    Owners owners() {
        return {display.model_state_,
                display.control_3d_,
                display.constant_attribute_mask_,
                display.constant_attributes_,
                display.material_argb_,
                display.coordinates_->current_matrix(),
                display.view_.inverse,
                display.view_.depth.base_transform};
    }
};
} // namespace sadx::basic_model
namespace sadx::material_correction {
// NOTSA: late Power reads borrow the live palette, never the asset exponent or a cached float.
struct Context {
    native::Display &display;
    const lighting::State &lighting;
    auto &device() { return display.device_state_; }
};
} // namespace sadx::material_correction
