#pragma once
#include "runtime/native_display.hpp"

namespace sadx::object_draw {
// NOTSA: traversal and model drawing borrow the same live scene and display owners.
struct Context {
    native::Display &display;
    const native::ModelDrawEnvironment &environment;
    auto &matrices() { return *display.coordinates_; }
    auto &arithmetic() { return display.arithmetic_; }
    const auto *queued_list() const { return display.queued_texture_list_; }
};
} // namespace sadx::object_draw
