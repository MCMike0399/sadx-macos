#include "engine/model_matrices.hpp"

namespace sadx::model_matrices {
bool get(const State &state, std::uint32_t index, NinjaMatrix &destination) {
    // Original quirk: unsigned comparison; a rejected index preserves the
    // caller's retained destination, which several player callers still use.
    if (index >= state.count)
        return false;
    destination = state.matrices.at(index);
    return true;
}
} // namespace sadx::model_matrices
