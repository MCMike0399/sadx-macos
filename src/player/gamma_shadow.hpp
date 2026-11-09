#pragma once

#include "cpu/fidelity_float.hpp"

#include <cstdint>

namespace sadx::gamma_shadow {
// NOTSA: the taskwk position and the dword at +0x18 that Shadow turns into a yaw.
struct Body {
    float x = 0;
    float y = 0;
    float z = 0;
    std::int32_t yaw = 0;
};

// The land query's height and the two normal words Shadow rotates by.
struct Ground {
    float height = 0;
    std::int32_t normal_z = 0;
    std::int32_t normal_x = 0;
};

// The land walk and the queued blob stay behind Services.
struct Services {
    virtual ~Services() = default;
    // The land walk is unrecovered once a table is loaded. An empty table
    // returns the image constant and zero normals.
    virtual Ground sample_ground(float x, float y, float z) = 0;
    // Nonzero pending logic ticks skip the blob.
    virtual bool missed_frames() = 0;
    virtual void prepare() = 0;
    virtual void push_matrix() = 0;
    virtual void translate(float x, float y, float z) = 0;
    virtual void rotate_z(std::uint16_t angle) = 0;
    virtual void rotate_x(std::uint16_t angle) = 0;
    virtual void rotate_y(std::uint16_t angle) = 0;
    virtual void scale(float x, float y, float z) = 0;
    virtual void draw(float clip) = 0;
    virtual void pop_matrix() = 0;
};

// Returns the ground height.
// FUNCTION: SONIC 0x49ee30 Shadow
fidelity::ExactFloat update(const Body &body, float scale, Services &services,
                            fidelity::Context &arithmetic);
} // namespace sadx::gamma_shadow
