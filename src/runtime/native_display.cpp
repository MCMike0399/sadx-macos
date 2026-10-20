#include "runtime/native_display.hpp"
#include "math/ninja_sine_table.hpp"
#include "runtime/engine_code_tables.hpp"
#include <cstring>
#include <stdexcept>

namespace sadx::native {
namespace {
// NOTSA: lossless word conversion at the maintained D3DX implementation boundary.
D3dxMatrix words(const graphics::Matrix &matrix) {
    D3dxMatrix result;
    std::memcpy(result.data(), matrix.data(), sizeof(matrix));
    return result;
}
// NOTSA: native device commands use floats rather than the D3DX compatibility words.
graphics::Matrix floats(const D3dxMatrix &matrix) {
    graphics::Matrix result;
    std::memcpy(result.data(), matrix.data(), sizeof(matrix));
    return result;
}
// NOTSA: InitNinjaSystem's matrix storage and njInitMatrix's first-slot orientation.
NinjaMatrixStack initial_matrices() {
    std::vector<NinjaMatrix> storage(128);
    storage.front() = runtime::engine_code_tables().unit_matrix;
    storage.front()[0] = storage.front()[5] = -1;
    return NinjaMatrixStack(std::move(storage), 1, 0);
}
} // namespace

Display::Display(const std::filesystem::path &directory)
    : sine_(ninja_sine_table()), matrices_(initial_matrices()),
      screen_transform_(words(runtime::engine_code_tables().unit_matrix)) {
    // [platform:graphics] Native 640x480 device creation exposes the same initial
    // viewport as the pinned Direct3D profile. Arbitrary viewports remain unsupported.
    device_state_.viewport = {0, 0, 640, 480, 0, 1};
    pvr::set_default_render_state(device_state_);
    pvr::set_default_texture_stage_state(device_state_);
    view_.depth.viewport = {0, 0, 640, 480, 0, 1065353216U};
    view_.depth.base_transform = screen_transform_;
    view::Services services{
        matrices_, sine_, screen_transform_,
        [this](bool multiply, std::uint32_t kind, const D3dxMatrix &value) {
            if (kind != 2 && kind != 3)
                throw std::runtime_error("unsupported native camera transform");
            auto &target = kind == 2 ? device_state_.view : device_state_.projection;
            target = multiply
                         ? floats(d3dx_matrix_multiply_774310(value, words(target), sse_))
                         : floats(value);
        },
        sse_reciprocal_by_division};
    view::initialize_camera(view_.camera);
    if (!view::set_camera(view_, services, arithmetic_, sse_))
        throw std::runtime_error("native camera initialization failed");
    output_ = std::make_unique<LogoRenderer>(directory, device_state_, view_.depth, arithmetic_,
                                           sse_, sse_reciprocal_by_division);
}
LogoFrame Display::frame_state() {
    return {horizontal_scale_, vertical_scale_, display_disabled_,
            fog_emulation_, fog_enabled_, queued_depth_};
}
} // namespace sadx::native
