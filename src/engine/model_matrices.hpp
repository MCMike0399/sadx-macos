#pragma once

#include "engine/ninja_matrix.hpp"

namespace sadx::model_matrices {
struct State {
    // GLOBAL: SONIC 0x3c58270 CurrentModelIndex size=4
    std::uint32_t count = 0;
    // GLOBAL: SONIC 0x3c58278 InstanceMatrices size=8192
    std::array<NinjaMatrix, 128> matrices = {};
    // GLOBAL: SONIC 0x3c5a278 LastModelIndex size=4
    std::uint32_t limit = 0;
};

// FUNCTION: SONIC 0x4b82d0 GetMMMatrix
bool get(const State &, std::uint32_t index, NinjaMatrix &destination);
} // namespace sadx::model_matrices
