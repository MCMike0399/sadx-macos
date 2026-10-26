#include "engine/material_correction.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx::material_correction {
struct Context {
    X87Context fp;
    graphics::Material material;
    const std::uint32_t *input;
    std::uint32_t *output;
    float power;
};
graphics::Material get_material(Context &context) {
    context.output[4 + context.output[3]++] = 1;
    if (context.input[3] & 1U)
        throw 1;
    std::memcpy(&context.power, context.input + 2, sizeof(float));
    return context.material;
}
void set_material(Context &context, const graphics::Material &material) {
    context.output[4 + context.output[3]++] = 2;
    if (context.input[3] & 2U)
        throw 2;
    std::memcpy(context.output + 6, &material, sizeof(material));
}
fidelity::ExactFloat exact(Context &context, float value) { return {context.fp, value}; }
float specular_power(Context &context) { return context.power; }
} // namespace sadx::material_correction

extern "C" int sadx_native_material_correction(const std::uint32_t *input, std::uint32_t *output) {
    if (!input || !output)
        return -1;
    static_assert(sizeof(sadx::graphics::Material) == 17 * sizeof(float));
    using namespace sadx::material_correction;
    Context context{sadx::X87Context(static_cast<std::uint16_t>(input[0])), {}, input, output, 0};
    std::memcpy(&context.material, input + 6, sizeof(context.material));
    std::memcpy(&context.power, input + 1, sizeof(float));
    try {
        correct_material(context);
        output[0] = 1;
    } catch (int boundary) {
        output[1] = static_cast<std::uint32_t>(boundary);
    }
    output[2] = context.fp.exception_flags();
    return 0;
}
