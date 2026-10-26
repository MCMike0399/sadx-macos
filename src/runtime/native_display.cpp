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

Display::Display(const std::filesystem::path &directory, frame::ModeState &mode,
                 std::uint32_t renderer_control_flags)
    : frame_mode_(mode), sine_(ninja_sine_table()), matrices_(initial_matrices()),
      screen_transform_(words(runtime::engine_code_tables().unit_matrix)),
      renderer_control_flags_(renderer_control_flags) {
    auto fog_runtime = this->fog_runtime();
    fog::initialize(fog_runtime);
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
                                           arithmetic_, sse_, sse_reciprocal_by_division,
                                           texture_state(), texture_selection_state(),
                                           queued_texture_list_, queued_texture_index_);
    auto owners = queue_owners();
    queue::initialize(owners, queue_services(), arithmetic_);
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
void Display::set_perspective(std::int32_t angle, std::uint32_t &pause_state) {
    screen::State state{horizontal_scale_, vertical_scale_, reset_pending_, pause_state,
                        animation_queue_state_, screen_descriptor_, view_.sprite_scale,
                        projection_center_x_, projection_center_y_, view_.camera.angle,
                        perspective_copy_, saved_fov_.value, view_material_};
    auto services = screen_services();
    screen::set_perspective(angle, state, services);
}
void Display::set_view_angle(std::int32_t angle, std::uint32_t &pause_state) {
    screen::CameraAngleState state{camera_type_, interpolate_camera_angle_,
                                   previous_camera_angle_, saved_fov_.value};
    screen::set_view_angle(angle, state, [this, &pause_state](std::int32_t value) {
        set_perspective(value, pause_state);
    });
}
void Display::set_lighting(bool enabled) {
    // [platform:graphics] Direct3D's LIGHTING state has a native shader equivalent.
    device_state_.render_states[137] = enabled;
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
    device_state_.render_states[23] = static_cast<std::uint32_t>(table[mode & 7]);
}
void Display::set_depth_write(bool enabled) { device_state_.render_states[14] = enabled; }
void Display::restore_depth_comparison() { set_depth_comparison(default_depth_comparison_); }
void Display::release_all_textures() {
    auto state = texture_state();
    auto services = texture_services();
    texture_lifetime::release_scene_textures(state, services, queue_state_.values.execution_mode,
                                             registered_texture_list_count_);
}
void Display::submit_sprite_quad(const sprite::Quad &quad) {
    sprite::QuadServices services{
        [this](float depth) {
            return ninja_projected_depth(depth, view_.depth, sse_, arithmetic_,
                                         sse_reciprocal_by_division);
        },
        [this](const std::array<sprite::ScreenVertex, 4> &vertices) {
            output_->snapshot_bound_texture();
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
sprite::ImmediateServices Display::sprite_services() {
    return {
        [this](const sprite::TextureList &textures) {
            output_->select_direct_texture_list(textures);
        },
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
            output_->set_texture_number(static_cast<std::uint32_t>(index));
        },
        [this](const sprite::Quad &quad) { submit_sprite_quad(quad); },
        [this] {
            device_state_.stage0_states[16] = 2;
            device_state_.stage0_states[17] = 2;
            device_state_.stage0_states[18] = 2;
        }};
}
void Display::draw_sprite(const sprite::Sprite &value, std::int32_t frame, float priority,
                          std::uint8_t attributes) {
    auto services = sprite_services();
    sprite::draw_sprite(&value, frame, priority, attributes, material_argb_, services, arithmetic_);
}
void Display::draw_screen_sprite(const sprite::ScreenSprite &value, std::int32_t frame, float priority,
                                 std::uint8_t attributes) {
    auto services = sprite_services();
    const auto view = sprite::screen_view(value);
    sprite::draw_screen_sprite(&view, frame, priority, attributes, material_argb_, services, arithmetic_);
}
LogoFrame Display::frame_state() {
    return {horizontal_scale_, vertical_scale_, frame_mode_.pending_logic_ticks,
            fog_.special_flags, fog_.active, queued_depth_};
}
queue::Owners Display::queue_owners() {
    return {queue_storage_, queue_state_, reset_material_, frame_mode_.pending_logic_ticks};
}
queue::Services Display::queue_services() {
    return {
        [this](std::uint32_t mode) {
            set_depth_comparison(mode);
            return true;
        },
        [this](bool enabled) {
            set_depth_write(enabled);
            return true;
        },
        [this](bool enabled) {
            device_state_.render_states[27] = enabled;
            return true;
        },
        [this](const std::array<float, 4> &color) { set_constant_material(color); },
        [this](std::uint32_t side, std::uint32_t mode) {
            output_->set_blending(side, mode);
            return true;
        }};
}
screen::Services Display::screen_services() {
    return {
        [this] { set_blank_queue_mode(); },
        [this] {
            auto owners = queue_owners();
            queue::reset(owners, queue_services(), arithmetic_);
        },
        [this] { output_->set_background(4278190080U, 4278190080U, 4278190080U); },
        [this](std::uint32_t side, std::uint32_t mode) {
            queue::set_blend(side, mode, queue_state_, queue_services());
        },
        [this] {
            auto owners = queue_owners();
            queue::reset_material(owners, queue_services(), arithmetic_);
        },
        [this](float x, float y, float width, float height, float depth, std::uint32_t color) {
            pvr::draw_box_fill_2d(x, y, width, height, depth, color, frame_mode_.pending_logic_ticks,
                                fog_.special_flags, fog_.active, queued_depth_, *output_, arithmetic_);
        },
        [this](std::int32_t angle) {
            view::set_screen_distance(angle, static_cast<float>(view_.depth.viewport.width),
                                      view_, sine_, arithmetic_);
        },
        [this](std::uint32_t control) { control_3d_ = control; },
        [this](std::uint32_t mask, std::uint32_t attributes) {
            constant_attribute_mask_ = mask;
            constant_attributes_ = attributes;
        },
        [this](const std::array<float, 4> &color) {
            queue::set_material(color, queue_state_, queue_services(), arithmetic_);
        },
        [this](float near_distance, float far_distance) { set_clip_range(near_distance, far_distance); },
        [this](std::uint32_t mode) { set_depth_comparison(mode); },
        [this](bool enabled) { set_depth_write(enabled); }
    };
}
void Display::begin_game_frame() {
    screen::begin_frame(reset_pending_);
    auto owners = queue_owners();
    queue::reset(owners, queue_services(), arithmetic_);
    set_depth_comparison(3);
}
void Display::end_game_frame() {
    auto services = screen_services();
    screen::end_frame(reset_pending_, services);
}
void Display::setup_screen(std::uint32_t &pause_state) {
    screen::State state{horizontal_scale_, vertical_scale_, reset_pending_, pause_state,
                        animation_queue_state_, screen_descriptor_, view_.sprite_scale,
                        projection_center_x_, projection_center_y_, view_.camera.angle,
                        perspective_copy_, saved_fov_.value, view_material_};
    auto services = screen_services();
    screen::setup(state, services, arithmetic_);
}
void Display::set_blank_queue_mode() { queue_state_.values.execution_mode = 2; }
void Display::set_depth_compare_mode(std::uint32_t mode) {
    default_depth_comparison_ = mode;
    queue_state_.values.depth_comparison = static_cast<std::int32_t>(mode);
}
void Display::clear_texture_binding() {
    auto state = texture_state();
    auto services = texture_services();
    texture_lifetime::clear_temporary_info(state, services);
}
texture_lifetime::State Display::texture_state() {
    return {texture_pool_, cached_texture_global_index_, active_texture_memory_,
            current_texture_list_, temporary_texture_info_};
}
texture_selection::State Display::texture_selection_state() {
    return {current_texture_list_, requested_texture_memory_, cached_texture_global_index_,
            active_texture_memory_, texture_palettes_};
}
texture_lifetime::Services Display::texture_services() {
    return {[this] {
        output_->unbind_texture();
    }};
}
void Display::release_texture_list(sprite::TextureList &list) {
    release_texture_list(&list);
}
void Display::release_texture_list(sprite::TextureList *list) {
    auto state = texture_state();
    auto services = texture_services();
    texture_lifetime::release_texture_list(list, state, services);
}
void Display::release_movie_texture_list(sprite::TextureList &list) {
    auto state = texture_state();
    auto services = texture_services();
    texture_lifetime::release_list(&list, state, services);
}
texture_lifetime::Memory *Display::create_argb_texture(
    std::uint32_t global_index, std::shared_ptr<const std::vector<std::uint8_t>> pixels,
    std::shared_ptr<const graphics::Texture> texture) {
    if (!pixels || !texture)
        throw std::runtime_error("native image texture requires owned image data");
    auto state = texture_state();
    auto prepared = texture_lifetime::prepare_argb_image(
        global_index, pixels->data(), texture->width, texture->height);
    auto *memory =
        texture_lifetime::acquire_prepared(std::move(prepared), std::move(texture), state, pixels);
    if (!memory)
        throw std::runtime_error("native texture pool is full");
    return memory;
}
fog::Runtime Display::fog_runtime() {
    // [platform:graphics] The native shader implements W-fog; this is the native
    // device capability, not a claim about the uncaptured Windows GPU/registry.
    constexpr std::uint32_t kNativeRasterCaps = 1U << 20;
    return {fog_, arithmetic_,
            [this](fog::RenderState state, std::uint32_t value) {
                device_state_.render_states[static_cast<std::uint32_t>(state)] = value;
            },
            kNativeRasterCaps, renderer_control_flags_};
}
void Display::enable_fog() {
    auto runtime = fog_runtime();
    fog::set_enabled(runtime, true, frame_mode_.pending_logic_ticks);
}
void Display::apply_fog_emulation_setting(std::int32_t value) {
    // NOTSA: WinMain modifies only bit0 of the shared SpecialFlags byte.
    fog_.special_flags = static_cast<std::uint8_t>(
        value <= 0 ? (fog_.special_flags & 254U) : (fog_.special_flags | 1U));
}
void Display::disable_fog() {
    auto runtime = fog_runtime();
    fog::set_enabled(runtime, false, frame_mode_.pending_logic_ticks);
}
void Display::reset_material() {
    auto owners = queue_owners();
    queue::reset_material(owners, queue_services(), arithmetic_);
}
void Display::set_alpha_test_enabled(bool enabled) {
    // [platform:graphics] Native shader alpha testing implements this device state.
    device_state_.render_states[29] = enabled;
}
} // namespace sadx::native
