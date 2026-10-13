#pragma once
#include "cpu/x87_math.hpp"
#include <functional>
#include <optional>
namespace sadx {
enum class CrtFabsFailure { none, unknown_nan_policy, missing_nan_handler, nan_handler_failed, unavailable_result };
struct CrtFabsResult {
    CrtFabsFailure failure=CrtFabsFailure::none;
    std::optional<X87Value> value;
    std::uint32_t source_boundary=0;
    std::optional<double> pending_stored_argument;
    explicit operator bool()const noexcept{return failure==CrtFabsFailure::none;}
};
struct CrtFabsRuntime {
    std::optional<std::uint32_t> nan_policy; // source0x2bcc6f0, consumed only for NaN
    std::optional<std::uint32_t> errno_value; // source0x3c837d4 store, owned mutable fact
    // Policy0 calls actual64aef3 with error1/function21/input/result/zero/savedCW.
    // The handler owns that complete lower CRT policy, including control state.
    std::function<CrtFabsResult(double stored_argument,std::uint16_t saved_control,X87Context&)> nan_handler;
};
CrtFabsResult crt_fabs(double,CrtFabsRuntime&,X87Context&);
} // namespace sadx
