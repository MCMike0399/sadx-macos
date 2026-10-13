#pragma once
#include "cpu/x87_math.hpp"
namespace sadx {
// Actual648bd5: finite FLD64, special encodings constructed then FLD80.
// Preserves signaling NaNs until the math body selects its arithmetic path.
X87Value load_crt_binary64_argument(double,X87Context&);
} // namespace sadx
