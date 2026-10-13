#pragma once

#include "ninja/ninja_math.hpp"
#include <optional>

namespace sadx {

// Native owner of playerwk +0x64 contact work. This is distinct from the
// task's CCL collision-info resource. A missing field remains unobserved;
// scene entrypoints resolve whole-resource nullness/identity separately.
struct PlayerContactWork {
    std::optional<NinjaVector> direction_at_1c;
    std::optional<float> radius_at_0 = {};
    std::optional<NinjaVector> center_at_4 = {};
    std::optional<NinjaVector> displacement_at_10 = {};
    std::optional<std::int32_t> angle_x_at_34 = {};
    std::optional<std::int32_t> angle_z_at_38 = {};
    std::optional<std::uint16_t> contact_count_at_28 = {};
    std::optional<std::uint16_t> selected_contact_at_2a = {};
    std::optional<NinjaVector> vector_at_3c = {},vector_at_48 = {},vector_at_54 = {},vector_at_60 = {},vector_at_6c = {},vector_at_78 = {};
};

} // namespace sadx
