#pragma once
#include "cpu/crt_asin.hpp"
namespace sadx {
// Source6452a4 binary64 argument entry and645290 retained-ST0 entry. The
// source special classifier imports NaN encodings as ext80 without FLD64
// quieting. Error policy, saved CW and partial result remain explicit.
CrtAsinResult crt_sqrt(double input,const CrtAsinRuntime&,X87Context&);
CrtAsinResult crt_sqrt_extended(X87Value input,const CrtAsinRuntime&,X87Context&);
} // namespace sadx
