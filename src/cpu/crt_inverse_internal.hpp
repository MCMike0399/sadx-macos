#pragma once

#include "cpu/crt_asin.hpp"

namespace sadx {
// Shared CRT return/dispatch owner, with function identity and source policy
// boundaries kept distinct. Each math body owns its entry control/math order.
struct CrtInverseFrame {
    CrtMathFunction function;
    const CrtAsinRuntime& runtime;
    X87Context& fp;
    double classified;
    std::uint16_t saved_control;
    std::uint32_t ordinary_policy_boundary;
    std::uint32_t error_policy_boundary;
};
CrtAsinResult finish_crt_inverse(const CrtInverseFrame&,X87Value,
                               unsigned error_code,bool unconditional_error);
} // namespace sadx
