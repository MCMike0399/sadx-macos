#pragma once

#include "cpu/x87_transcendental.hpp"
#include <functional>
#include <optional>

namespace sadx {

enum class CrtAtan2Failure {
    none, unknown_matherr_policy, missing_matherr_handler,
    matherr_unavailable, unmasked_return_exception
};
struct CrtAtan2MathErrorRequest {
    // Actual descriptor and signed-byte operation at +0xe, not an asin alias.
    std::uint32_t descriptor = 0x2bcc170;
    unsigned operation = 16;
    unsigned error_code;
    double argument_y;
    double argument_x;
    double stored_result;
    std::uint16_t saved_control_word;
    // Earlier source scratch write is distinct from the final (possibly
    // scaled) spill. A shared runtime handler must observe source write order.
    double scratch_result_at_3c83960;
    std::uint8_t argument_copy_flag_at_3c83968 = 0;
};
struct CrtAtan2MathErrorResult {
    CrtAtan2Failure failure;
    std::optional<double> stored_result;
};
using CrtAtan2MathErrorHandler = std::function<CrtAtan2MathErrorResult(
    const CrtAtan2MathErrorRequest&, X87Context&)>;
struct CrtAtan2Runtime {
    // Actual initialized value at 0x3c837a8 is required. Nonzero bypasses
    // the return dispatcher; file/BSS zeros do not establish runtime policy.
    std::optional<std::uint32_t> matherr_control;
    CrtAtan2MathErrorHandler matherr_handler;
};
struct CrtAtan2Result {
    CrtAtan2Failure failure;
    std::optional<X87Value> value;
    std::uint32_t source_boundary;
    std::optional<double> pending_stored_result;
    std::optional<unsigned> pending_error_code;
    // Source0x64b3ee writes this scratch global when policy is zero, even
    // when no dispatcher is needed. Caller owns that shared runtime state.
    std::optional<double> scratch_result_at_3c83960;
    // Source64b3de writes zero before the mandatory policy read. This selects
    // original stack-argument copying in the shared two-argument dispatcher.
    std::uint8_t argument_copy_flag_at_3c83968 = 0;
};
struct CrtAtan2Trace {
    // Optional observation of the actual arithmetic handoff at64b2ad.
    // It never replaces the selected FPATAN model or controls execution.
    std::optional<X87Value> result_after_fpatan;
};

// Actual cdecl645270: first binary64 argument is y, second is x. The body
// uses its own0x133f control word and explicit Bochs P5/P6 FPATAN model.
// Unknown/error boundaries preserve partial arithmetic and current CW.
CrtAtan2Result crt_atan2_binary64(double y, double x,
                                const CrtAtan2Runtime&, X87Context&,
                                CrtAtan2Trace* trace = nullptr);

} // namespace sadx
