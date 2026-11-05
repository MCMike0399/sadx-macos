#include "engine/vector_normalize.hpp"
#include <cassert>
#include <cstring>

int main() {
    sadx::SseContext fp(0x1f80);
    std::array<float, 3> vector{1, 2, 3}, output{};
    sadx::normalize_vector(output, vector, fp);
    fp.set_mxcsr(0x1f80);
    sadx::normalize_vector(vector, vector, fp);
    assert(std::memcmp(vector.data(), output.data(), 12) == 0);
    // An unmasked invalid square must leave the destination untouched.
    const std::uint32_t signaling_nan = 0x7f800001;
    std::memcpy(&vector[0], &signaling_nan, 4);
    output = {7, 8, 9};
    fp.set_mxcsr(0x1f00);
    bool trapped = false;
    try { sadx::normalize_vector(output, vector, fp); }
    catch (const sadx::SseError &error) {
        trapped = error.failure() == sadx::SseFailure::unmasked_exception && error.exceptions() == 1;
    }
    assert(trapped && (output == std::array<float, 3>{7, 8, 9}));
    assert(fp.mxcsr() == 0x1f01);
}
