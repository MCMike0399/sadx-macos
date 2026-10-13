#pragma once

// System headers are loaded outside the private vendor namespace. Every
// upstream translation unit is wrapped in the same namespace by build.sh.
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

namespace sadx::bochs_fpatan {
#include "config.h"
#include "source/fpu/fpu_trans.h"
} // namespace sadx::bochs_fpatan
