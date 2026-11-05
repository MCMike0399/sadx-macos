// Independent numeric boundary for the animated-draw matrix wrappers.
#include "engine/ninja_matrix_composition.hpp"
#include <cstring>

extern "C" int draw_transform_probe(const std::uint8_t *sine_bytes,
                                      const std::uint8_t *unit_bytes,
                                      const std::uint32_t *input,
                                      std::uint32_t *output) {
    try {
        sadx::NinjaMath::SineTable sine;
        sadx::NinjaMatrix unit, matrix;
        std::array<std::int32_t, 3> angles;
        std::array<float, 3> position;
        std::memcpy(sine.data(), sine_bytes, sizeof(sine));
        std::memcpy(unit.data(), unit_bytes, sizeof(unit));
        std::memcpy(angles.data(), input + 1, sizeof(angles));
        std::memcpy(position.data(), input + 4, sizeof(position));
        std::memcpy(matrix.data(), input + 7, sizeof(matrix));
        sadx::NinjaMatrixStack stack({matrix}, 1, 0);
        sadx::NinjaMatrixPrimitives coordinates(sine, unit, stack);
        sadx::fidelity::Context fp(127);
        switch (input[0] % 3) {
        case 0:
            sadx::rotate_ninja_xyz(coordinates, stack.current(), angles, fp);
            break;
        case 1:
            sadx::rotate_ninja_zyx(coordinates, stack.current(), angles, fp);
            break;
        case 2:
            sadx::translate_ninja_matrix(stack.current(), position[0], position[1], position[2], fp);
            break;
        }
        std::memcpy(output, stack.current().data(), sizeof(matrix));
        output[16] = fp.exception_flags();
        return 0;
    } catch (...) {
        return 1;
    }
}
