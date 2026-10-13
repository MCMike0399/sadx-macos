#pragma once
#include "cpu/x87_math.hpp"
#include "cpu/sse_math.hpp"
#include <cstddef>
#include <functional>
#include <optional>
namespace sadx {
enum class CrtFloorFailure { none, unknown_cpu_policy, missing_sse_context,
    unknown_fallback_control_word, missing_error_handler, error_handler_failed, unavailable_error_result };
enum class CrtFloorErrorRoute { sse_nan, x87_nan, x87_precision };
struct CrtFloorErrorResult { CrtFloorFailure failure; std::optional<X87Value> value; };
using CrtFloorErrorHandler=std::function<CrtFloorErrorResult(CrtFloorErrorRoute,
    double input,double pending,std::uint16_t saved_control_word,X87Context&,SseContext*)>;
struct CrtFloorRuntime {
    std::optional<std::uint32_t> cpu_sse2; // actual runtime global0x3d0f400
    SseContext* sse=nullptr; // consulted only when cpu_sse2 is known nonzero
    std::optional<std::uint16_t> fallback_control_word; // runtime global0x2bcc034
    CrtFloorErrorHandler error_handler;
};
struct CrtFloorConstants {
    std::uint64_t one,shift_bias,negative_one_mask,negative_zero,exponent_mask;
};
std::optional<CrtFloorConstants> decode_crt_floor_constants(const std::uint8_t*,std::size_t);
struct CrtFloorResult {
    CrtFloorFailure failure=CrtFloorFailure::none;
    std::optional<X87Value> value;
    std::uint32_t source_boundary=0;
    std::optional<double> pending_stored_result;
    bool used_sse=false;
    explicit operator bool() const noexcept { return failure==CrtFloorFailure::none; }
};
// Actual binary64 argument at0x6449f0: SSE integer-mask fast route or explicit
// 0x64917a x87 fallback, with source stores and control-state boundaries.
CrtFloorResult crt_floor(double,const CrtFloorConstants&,const CrtFloorRuntime&,X87Context&);
} // namespace sadx
