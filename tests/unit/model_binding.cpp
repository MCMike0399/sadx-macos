#include "game/library_initial_state.hpp"
#include "runtime/model_device.hpp"
#include "runtime/model_draw_context.hpp"
#include <cassert>
#include <cstring>

namespace {
sadx::D3dxMatrix words(const sadx::graphics::Matrix &matrix) {
    sadx::D3dxMatrix value;
    std::memcpy(value.data(), matrix.data(), sizeof(value));
    return value;
}
} // namespace
namespace sadx::native {
struct DisplayModelProbe {
    static void verify_startup(Display &display, polybuff::Context &context) {
        const auto &resources = display.polygon_resources_;
        assert(resources.base.value == &resources.workspace);
        assert(resources.buffers.vertices);
        constexpr std::size_t kScratchVertices = 0x6000;
        assert(resources.buffers.vertices->bytes.size() ==
               kScratchVertices * polybuff::kScratchVertexBytes);
        for (const auto &record : resources.buffers.records) {
            assert(record.resource && record.resource->references == 1);
            assert(record.render_arguments.words.data && record.lock_count == 0);
        }
        for (const auto word : resources.scratch.words)
            assert(word == 0);
        assert(&context.counts() == &display.graphics_library_.polygon_buffer_counts);
        const auto configured = context.counts().back();
        assert(configured == 32000);
        assert(configured !=
               static_cast<std::uint32_t>(library_initial_state().buffer_counts.back()));
        context.counts().back() = 17;
        assert(polybuff::configured_count(context, polybuff::kBufferCount - 1) == 17);
        context.counts().back() = configured;
        assert(display.graphics_library_.scale == 1 && display.graphics_library_.aspect == 0.75f);
    }
    static void verify_null_texture(Display &display, basic_model::Context &context,
                                    texture_lifetime::Memory *memory,
                                    const std::shared_ptr<graphics::Texture> &image) {
        const auto references = memory->references;
        const auto surface = memory->surface;
        basic_model::set_null_texture(context);
        assert(display.cached_texture_global_index_ == -1);
        assert(display.active_texture_memory_ == nullptr);
        assert(context.device().texture == nullptr);
        assert(memory->references == references && memory->surface == surface);
        assert(surface->texture == image);
        // The original guard does not touch a stale active owner or separately bound image.
        display.active_texture_memory_ = memory;
        context.device().texture = image;
        basic_model::set_null_texture(context);
        assert(display.active_texture_memory_ == memory && context.device().texture == image);
        assert(memory->references == references && memory->surface == surface);
    }
    static void verify_matrix_owners(Display &display, basic_model::Context &context) {
        auto owners = context.owners();
        auto &current = display.coordinates_->current_matrix();
        assert(&owners.current_matrix == &current);
        assert(&owners.inverse_view == &display.view_.inverse);
        assert(&owners.reset_world == &display.view_.depth.base_transform);
        assert(&owners.constant_material == &display.material_argb_);
        assert(&owners.control == &display.control_3d_);
        assert(&owners.attribute_mask == &display.constant_attribute_mask_);
        assert(&owners.attributes == &display.constant_attributes_);
        const auto saved_current = current;
        const auto saved_view = display.view_.inverse;
        current = {2, 0, 0, 0, 0, 4, 0, 0, 0, 0, 8, 0, 0, 0, 0, 1};
        display.view_.inverse = words({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 5, 7, 11, 1});
        basic_model::prepare_matrices(owners, context);
        assert(owners.state.current_no_view ==
               words({2, 0, 0, 0, 0, 4, 0, 0, 0, 0, 8, 0, 5, 7, 11, 1}));
        assert(owners.state.inverse_current ==
               words({0.5f, 0, 0, 0, 0, 0.25f, 0, 0, 0, 0, 0.125f, 0, 0, 0, 0, 1}));
        basic_model::project_to_world_space(owners, context);
        assert(owners.state.world == owners.state.current_no_view);
        assert(owners.state.extracted == words(graphics::kIdentity));
        current = saved_current;
        display.view_.inverse = saved_view;
    }
};
} // namespace sadx::native

int main(int argc, char **argv) {
    assert(argc == 3);
    using namespace sadx;
    namespace pb = polybuff;
    const auto pack = character_pack::CharacterPack::load(argv[1]);
    character_pack::SceneState scene(pack);
    lighting::State lighting({});
    auto hook = bootstrap::MaterialCallback::CorrectMaterial;
    const native::ModelDrawEnvironment environment{pack, scene, hook, lighting};
    frame::ModeState mode{};
    native::Display display(argv[2], mode);
    pb::Context device{display};
    native::DisplayModelProbe::verify_startup(display, device);
    basic_model::Context model{display, environment};
    native::DisplayModelProbe::verify_matrix_owners(display, model);

    // The callback retains its identity, while Power is read from the live palette later.
    const auto callback = basic_model::snapshot_material_callback(model);
    assert(callback);
    hook = bootstrap::MaterialCallback::None;
    lighting.active_palette.upper_specular_power = 61;
    graphics::Material raw;
    raw.diffuse = {255, 255, 0, 255};
    raw.ambient = raw.diffuse;
    raw.power = 11;
    basic_model::set_device_material(model, raw);
    callback(model);
    assert(device.device().material.power == 61);
    assert((device.device().material.diffuse == std::array<float, 4>{1, 1, 0, 255}));
    assert(basic_model::snapshot_material_callback(model) == nullptr);
    hook = bootstrap::MaterialCallback::CorrectMaterial;
    lighting.active_palette.upper_specular_power = 37;

    auto &buffer = native::normal_uv_buffer(display);
    assert(&buffer == &device.buffer(8));
    assert(buffer.resource && buffer.vertex_format == 0x152);
    const auto configured = library_initial_state().configuration.buffer_counts.back();
    assert(buffer.capacity_bytes ==
           static_cast<std::uint32_t>(configured) * pb::kNormalUvVertexBytes);
    display.begin_game_frame();
    assert(buffer.remaining_bytes == buffer.capacity_bytes - buffer.stride);
    std::array<texture_lifetime::Memory *, 8> texture_memories{};
    auto image = std::make_shared<graphics::Texture>();
    image->width = image->height = 1;
    image->bgra = {255, 255, 255, 255};
    const auto pixels = std::make_shared<const std::vector<std::uint8_t>>(image->bgra);
    for (std::size_t index = 0; index < texture_memories.size(); ++index)
        texture_memories[index] =
            display.create_argb_texture(static_cast<std::uint32_t>(index), pixels, image);
    texture_lifetime::List textures;
    textures.entries.assign(texture_memories.begin(), texture_memories.end());
    display.output().select_texture_list(textures);
    constexpr character_pack::ResourceId kQuestionModel = 2943;
    const auto model_resource = pack.model(kQuestionModel);
    const auto &before = scene.geometry.vector_words(model_resource.points);
    const auto first_coordinate = before.front();
    basic_model::draw_model(kQuestionModel, pack, model.owners(), model);
    assert(device.frame().commands.size() == 1);
    const auto &draw = std::get<graphics::Draw>(device.frame().commands.back());
    assert(draw.has_normals && draw.has_diffuse && draw.texture_coordinates == 1);
    assert(draw.primitive_type == pb::kTriangleStripPrimitive && draw.primitive_count > 0);
    assert(draw.texture == image && draw.material.power == 37);
    assert((draw.material.diffuse == std::array<float, 4>{1, 1, 0, 255}));
    assert(draw.stage0_states.at(24) == 2 && draw.stage0_states.at(11) == 65536);
    assert(device.device().stage0_states.at(24) == 0 && device.device().stage0_states.at(11) == 0);
    assert(model.state().render_flags == 0 && buffer.lock_count == 0);
    assert(device.stream().resource == nullptr);

    // A later draw sees live vector edits, while the earlier frame owns its old geometry.
    const auto original_x = draw.vertices.front().position[0];
    constexpr float kChangedX = 12345.0f;
    std::uint32_t changed_bits;
    std::memcpy(&changed_bits, &kChangedX, sizeof(changed_bits));
    auto &live_points = scene.geometry.vector_words(model_resource.points);
    for (std::size_t component = 0; component < live_points.size(); component += 3)
        live_points[component] = changed_bits;
    basic_model::draw_model(kQuestionModel, pack, model.owners(), model);
    assert(device.frame().commands.size() == 2);
    assert(std::get<graphics::Draw>(device.frame().commands[0]).vertices.front().position[0] ==
           original_x);
    assert(std::get<graphics::Draw>(device.frame().commands[1]).vertices.front().position[0] ==
           kChangedX);
    std::uint32_t serialized;
    std::memcpy(&serialized, pack.record(model_resource.points).payload.data() + 4,
                sizeof(serialized));
    assert(serialized == first_coordinate);
    native::DisplayModelProbe::verify_null_texture(display, model, texture_memories.back(), image);
}
