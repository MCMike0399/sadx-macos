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
    coordinates_ = std::make_unique<NinjaMatrixPrimitives>(
        sine_, runtime::engine_code_tables().unit_matrix, matrices_);
    view::initialize_camera(view_.camera);
    set_camera();
    output_ = std::make_unique<LogoRenderer>(directory, device_state_, frame_, view_.depth,
                                           arithmetic_, sse_, sse_reciprocal_by_division);
}
view::Services Display::view_services() {
    return {
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
}
void Display::set_camera() {
    auto services = view_services();
    if (!view::set_camera(view_, services, arithmetic_, sse_))
        throw std::runtime_error("native camera update failed");
}
void Display::set_clip_range(float near_distance, float far_distance) {
    view::ClipServices services{
        [this] { return device_state_.projection; },
        [this](const NinjaMatrix &matrix) {
            device_state_.projection = matrix;
        }};
    view::set_clip_range(near_distance, far_distance, view_, services, arithmetic_);
}
void Display::set_depth_comparison(std::uint32_t mode) {
    const auto table = runtime::engine_code_tables().depth_comparisons;
    if (mode >= table.size())
        throw std::runtime_error("native depth comparison is out of range");
    device_state_.render_states[23] = static_cast<std::uint32_t>(table[mode]);
}
void Display::set_depth_write(bool enabled) { device_state_.render_states[14] = enabled; }
void Display::restore_depth_comparison() { set_depth_comparison(default_depth_comparison_); }
void Display::release_all_textures() {
    output_->release_logo_archive();
    sprite_textures_ = nullptr;
    device_state_.texture.reset();
}
void Display::submit_sprite_quad(const sprite::Quad &quad) {
    sprite::QuadServices services{
        [this](float depth) {
            return ninja_projected_depth(depth, view_.depth, sse_, arithmetic_,
                                         sse_reciprocal_by_division);
        },
        [this](const std::array<sprite::ScreenVertex, 4> &vertices) {
            auto draw = device_state_;
            draw.primitive_type = 5;
            draw.primitive_count = 2;
            draw.indices.clear();
            draw.vertices.clear();
            for (const auto &source : vertices) {
                graphics::Vertex vertex;
                vertex.position = {source.x, source.y, source.depth, source.reciprocal_w};
                vertex.diffuse = {float((source.color >> 16) & 255) / 255,
                                  float((source.color >> 8) & 255) / 255,
                                  float(source.color & 255) / 255, float(source.color >> 24) / 255};
                vertex.uv[0] = {source.u, source.v};
                draw.vertices.push_back(vertex);
            }
            frame_.commands.emplace_back(std::move(draw));
        }};
    sprite::draw_quad(quad, hud_color_, sprite_vertices_, services, arithmetic_);
}
void Display::draw_sprite(const sprite::Sprite &value, std::int32_t frame, float priority,
                          std::uint8_t attributes) {
    sprite::ImmediateServices services{
        [this](const sprite::TextureList &textures) { sprite_textures_ = &textures; },
        [this] { coordinates_->push_unit_frame(); },
        [this](std::int32_t angle) {
            coordinates_->rotate_z(coordinates_->current_matrix(), angle, arithmetic_);
        },
        [this](const std::array<float, 3> &input) {
            NinjaVector output{};
            const auto result = coordinates_->calculate_vector(
                coordinates_->current_matrix(), {input[0], input[1], input[2]}, output, arithmetic_);
            if (!result)
                throw std::runtime_error("native sprite vector transformation failed");
            return std::array<float, 3>{output.x, output.y, output.z};
        },
        [this] { coordinates_->end_ex_frame(); },
        [this](bool alpha) { sprite::begin_quad(alpha, device_state_); },
        [this](std::uint32_t color) { hud_color_ = color; },
        [this](std::int16_t index) {
            if (!sprite_textures_ || index < 0 ||
                static_cast<std::size_t>(index) >= sprite_textures_->size())
                throw std::runtime_error("native sprite texture is unavailable");
            device_state_.texture = sprite_textures_->at(static_cast<std::size_t>(index));
        },
        [this](const sprite::Quad &quad) { submit_sprite_quad(quad); },
        [this] {
            device_state_.stage0_states[16] = 2;
            device_state_.stage0_states[17] = 2;
            device_state_.stage0_states[18] = 2;
        }};
    sprite::draw_sprite(&value, frame, priority, attributes, material_argb_, services, arithmetic_);
}
LogoFrame Display::frame_state() {
    return {horizontal_scale_, vertical_scale_, display_disabled_,
            fog_emulation_, fog_enabled_, queued_depth_};
}
} // namespace sadx::native
