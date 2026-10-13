#pragma once

#include "cpu/x87_transcendental.hpp"
#include <functional>
#include <optional>

namespace sadx {

enum class CrtAsinFailure { none, unknown_matherr_policy, missing_matherr_handler, matherr_unavailable };
struct CrtMathErrorResult { CrtAsinFailure failure; std::optional<double> stored_result; };
enum class CrtMathFunction : std::uint32_t { sqrt = 5, acos = 13, asin = 14 };
using CrtMathErrorHandler = std::function<CrtMathErrorResult(
    CrtMathFunction function, unsigned error_code, double argument, double stored_result,
    std::uint16_t saved_control_word, X87Context&)>;
struct CrtAsinRuntime {
    // Exact runtime word at 0x3c837a8: nonzero skips the CRT error dispatcher.
    // Zero-filled PE memory does not establish the word after initialization.
    std::optional<std::uint32_t> matherr_control;
    CrtMathErrorHandler matherr_handler;
};
struct CrtAsinResult {
    CrtAsinFailure failure;
    std::optional<X87Value> value;
    std::uint32_t source_boundary;
    // Preserved partial computation at an unresolved dispatcher boundary.
    std::optional<double> pending_stored_result;
};

// Recovered CRT asin local body 0x645020/0x64503d with explicit Bochs P5/P6
// FPATAN model. Error dispatch and actual hardware behavior remain separate.
// A missing boundary leaves the context at the source call-site control word;
// no invented rollback/default matherr or hidden host asin is performed.
CrtAsinResult crt_asin(X87Value input, const CrtAsinRuntime&, X87Context&);
CrtAsinResult crt_asin_binary64(double input,const CrtAsinRuntime&,X87Context&);

} // namespace sadx
