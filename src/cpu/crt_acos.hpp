#pragma once

#include "cpu/crt_asin.hpp"

namespace sadx {
// Actual source 0x6450f0/0x64510d with shared CRT runtime/error ownership.
// Finite math, FLDPI/FLDZ endpoints and policy addresses differ from asin.
CrtAsinResult crt_acos(X87Value,const CrtAsinRuntime&,X87Context&);
} // namespace sadx
