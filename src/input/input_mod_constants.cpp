#include "input/input_mod_constants.hpp"
#include <cstring>
namespace sadx {
namespace {
// NOTSA: preserve a binary32 literal as the exact arithmetic model's input word.
Sse32 sse_literal(float value) {
    Sse32 result{};
    std::memcpy(&result.bits, &value, sizeof(value));
    return result;
}
} // namespace
InputModPadConstants input_mod_261_constants() {
    return {{sse_literal(1.1920929e-7F), sse_literal(32767.0F), sse_literal(127.0F),
             sse_literal(128.0F), sse_literal(1.0F)},
            sse_literal(255.0F),
            sse_literal(0.5F)};
}
} // namespace sadx
