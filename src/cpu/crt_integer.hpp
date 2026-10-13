#pragma once

#include "cpu/x87_math.hpp"
#include <cstdint>

namespace sadx {

// Recovered MSVC conversion at 0x644340. The routine performs current-RC
// FISTP64, then uses the binary32 sign/difference stores and integer carry
// correction. It is not a host cast or a direct truncate instruction.
// The returned word preserves EDX:EAX, including the masked indefinite value.
std::uint64_t crt_ftol(X87Value value, X87Context& context);
std::uint32_t crt_ftol_low32(X87Value value, X87Context& context);

} // namespace sadx
