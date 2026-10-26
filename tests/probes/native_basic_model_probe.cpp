#include "basic_model_backend.hpp"

extern "C" int sadx_native_basic_model(const std::uint32_t *input, std::uint32_t *output,
                                       const std::uint8_t *pack_bytes, std::size_t pack_size) {
    namespace bm = sadx::basic_model;
    if (!input || !output)
        return -1;
    std::fill(output, output + 8192, 0);
    bm::State state;
    state.render_flags = input[1];
    state.constant_color = input[5];
    state.vertex_color = input[6];
    state.landtable_color = input[7];
    state.cull_mode = input[8];
    std::array<float, 4> constant;
    for (std::size_t i = 0; i < constant.size(); ++i)
        constant[i] = bm::probe_float(input[9 + i]);
    sadx::graphics::Matrix current;
    std::memcpy(current.data(), input + 32, sizeof(current));
    sadx::D3dxMatrix inverse_view, reset;
    for (std::size_t i = 0; i < current.size(); ++i) {
        inverse_view[i] = input[48 + i];
        reset[i] = input[64 + i];
        state.inverse_current[i] = input[80 + i];
        state.current_no_view[i] = input[96 + i];
        state.world[i] = input[112 + i];
        state.extracted[i] = input[128 + i];
    }
    bm::Owners owners{state, input[2], input[3], input[4], constant, current, inverse_view, reset};
    bm::Context context(state, input);
    if (input[26])
        context.callback = bm::first_material_callback;
    std::uint32_t boundary = 0;
    try {
        switch (input[0]) {
        case 0:
            bm::reset_render_flags(state, context);
            break;
        case 1:
            bm::parse_material(
                {input[13], input[14], bm::probe_float(input[15]), input[16], input[17]}, owners,
                context);
            break;
        case 2:
            bm::copy_constant_material(owners, context);
            break;
        case 3:
            bm::prepare_matrices(owners, context);
            break;
        case 4:
            bm::project_to_world_space(owners, context);
            break;
        case 5:
            bm::set_world_transform(owners, context);
            break;
        case 6:
            bm::reset_world_transform(owners, context);
            break;
        case 7:
        case 8: {
            if (!pack_bytes)
                return -2;
            const auto pack = sadx::character_pack::CharacterPack::decode(
                std::vector<std::uint8_t>(pack_bytes, pack_bytes + pack_size));
            if (input[0] == 7)
                bm::draw_model(3, pack, owners, context);
            else
                bm::draw_direct_model(3, pack, owners, context);
            break;
        }
        case 9:
            bm::set_texture_shading(context, static_cast<bm::TextureShading>(input[29]));
            break;
        default:
            return -3;
        }
    } catch (const bm::Unavailable &failure) {
        boundary = static_cast<std::uint32_t>(failure.boundary) + 1;
    } catch (...) {
        return -4;
    }
    output[0] = state.render_flags;
    output[1] = state.constant_color;
    output[2] = state.vertex_color;
    output[3] = state.landtable_color;
    output[4] = state.cull_mode;
    output[5] = context.arithmetic.exception_flags();
    output[6] = boundary;
    output[7] = context.events.size();
    std::copy(state.current_no_view.begin(), state.current_no_view.end(), output + 8);
    std::copy(state.inverse_current.begin(), state.inverse_current.end(), output + 24);
    std::copy(state.world.begin(), state.world.end(), output + 40);
    std::copy(state.extracted.begin(), state.extracted.end(), output + 56);
    if (context.events.size() > 8192 - 72)
        return -5;
    std::copy(context.events.begin(), context.events.end(), output + 72);
    return 0;
}
extern "C" const char *sadx_native_basic_model_compiler() { return __VERSION__; }
