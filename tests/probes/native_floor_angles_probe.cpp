// Function-local comparison adapter. Runtime policy and source constants are
// caller-selected; no original gameplay/startup state is invented here.
#include "player/floor_angles.hpp"

#include <cstddef>
#include <cstring>

namespace {
float number(std::uint32_t word) { float value; std::memcpy(&value, &word, 4); return value; }
std::uint64_t bits(double value) { std::uint64_t word; std::memcpy(&word, &value, 8); return word; }
}

extern "C" const char* sadx_native_floor_angles_compiler() {
#if defined(__clang__) && defined(__aarch64__)
#define SADX_FLOOR_STRINGIZE_RAW(value) #value
#define SADX_FLOOR_STRINGIZE(value) SADX_FLOOR_STRINGIZE_RAW(value)
    return "Clang " __clang_version__ "; target arm64; __cplusplus="
        SADX_FLOOR_STRINGIZE(__cplusplus) "; native integer SoftFloat/Bochs";
#undef SADX_FLOOR_STRINGIZE
#undef SADX_FLOOR_STRINGIZE_RAW
#else
    return "Unsupported compiler/target profile";
#endif
}

extern "C" int sadx_native_floor_angles(const std::uint8_t* constant_bytes, std::size_t constant_size,
                                         const std::uint32_t* words, std::size_t word_count,
                                         std::uint16_t control_word,
                                         std::uint32_t* output, std::size_t output_count) {
    // Cases: mode(0=floor,1=CRTasin), x,y,z, output sentinels x,z,
    // matherr-policy-present, exact matherr-control word.
    // Output: angle x,z, CRTfailure,boundary,flags,CW,returnedvaluepresent,
    // X87failure+1,pendingF64low/high,ext80signExp/significandlow/high,pendingpresent.
    if (!constant_bytes || constant_size != 36 || !words || word_count != 8 ||
        !output || output_count != 14) return 1;
    std::memset(output, 0, 14 * sizeof(*output));
    output[0] = words[4]; output[1] = words[5]; output[5] = control_word;
    try {
        sadx::FloorAngleConstants constants;
        std::memcpy(&constants.upper_cut, constant_bytes, 4);
        std::memcpy(&constants.lower_cut, constant_bytes + 4, 4);
        std::memcpy(&constants.axis_epsilon, constant_bytes + 8, 4);
        std::memcpy(&constants.angle_units, constant_bytes + 12, 8);
        std::memcpy(&constants.positive_factor, constant_bytes + 20, 8);
        std::memcpy(&constants.negative_factor, constant_bytes + 28, 8);
        sadx::CrtAsinRuntime runtime;
        if (words[6]) runtime.matherr_control = words[7];
        sadx::X87Context fp(control_word);
        try {
            if (words[0] == 0) {
                const auto result = sadx::floor_normal_angles(
                    {number(words[1]), number(words[2]), number(words[3])}, constants, runtime, fp);
                output[2] = static_cast<std::uint32_t>(result.failure);
                output[3] = result.source_boundary;
                output[6] = result.angles.has_value();
                if (result.angles) {
                    std::memcpy(output, &result.angles->inclination, 4);
                    std::memcpy(output + 1, &result.angles->roll, 4);
                }
            } else if (words[0] == 1) {
                const auto result = sadx::crt_asin(fp.from_float(number(words[3])), runtime, fp);
                output[2] = static_cast<std::uint32_t>(result.failure);
                output[3] = result.source_boundary;
                output[6] = result.value.has_value();
                if (result.value) {
                    output[10] = result.value->sign_exponent();
                    output[11] = static_cast<std::uint32_t>(result.value->significand());
                    output[12] = static_cast<std::uint32_t>(result.value->significand() >> 32U);
                }
                if (result.pending_stored_result) {
                    const auto stored = bits(*result.pending_stored_result);
                    output[8] = static_cast<std::uint32_t>(stored);
                    output[9] = static_cast<std::uint32_t>(stored >> 32U);
                    output[13] = 1;
                }
            } else return 2;
        } catch (const sadx::X87Error& error) {
            output[7] = static_cast<std::uint32_t>(error.failure()) + 1;
        }
        output[4] = fp.exception_flags(); output[5] = fp.control_word();
        return 0;
    } catch (const sadx::X87Error& error) {
        output[7] = static_cast<std::uint32_t>(error.failure()) + 1;
        return 3;
    } catch (...) { return 4; }
}
