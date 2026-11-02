#pragma once
#include "player/model_motion_matrices.hpp"
namespace sadx::model_matrices::reference {
// NOTSA: source diagnostics stay outside the game algorithm. Function-entry
// boundaries identify undeclared key/object storage without claiming a fault PC.
struct FailureLocation {
    std::uint32_t source_boundary;
    const char *required_fact;
};
FailureLocation locate(Failure);
} // namespace sadx::model_matrices::reference
