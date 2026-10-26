#include "engine/basic_model.hpp"

#include <cstring>
#include <stdexcept>

namespace sadx::basic_model {
namespace {
// NOTSA: typed predicates name the distinct source flag namespaces.
template <typename Flag> bool has(std::uint32_t value, Flag flag) {
    return (value & static_cast<std::uint32_t>(flag)) != 0;
}
// NOTSA: count-prefixed imported records are immutable; decode only consumed words.
std::uint32_t word(const character_pack::ResourceRecord &record, std::size_t offset) {
    const auto &bytes = record.payload;
    if (offset > bytes.size() || bytes.size() - offset < 4)
        throw std::runtime_error("basic model: imported record word unavailable");
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset + 1]) << 8) |
           (std::uint32_t(bytes[offset + 2]) << 16) | (std::uint32_t(bytes[offset + 3]) << 24);
}
// NOTSA: preserve all raw exponent bits, including signed zero and NaN payloads.
Material material(const character_pack::CharacterPack &pack, character_pack::ResourceId resource,
                  std::uint32_t index) {
    const auto &record = pack.record(resource);
    if (record.kind != character_pack::ResourceKind::Materials || index >= word(record, 0))
        throw std::runtime_error("basic model: imported material unavailable");
    const auto offset = 4 + std::size_t(index) * 20;
    const auto power_bits = word(record, offset + 8);
    float power;
    std::memcpy(&power, &power_bits, sizeof(power));
    return {word(record, offset), word(record, offset + 4), power, word(record, offset + 12),
            word(record, offset + 16)};
}
// NOTSA: the original table's layout ignores only the color bit in no-color mode.
VertexLayout layout(bool normals, bool colors, bool uv, bool ignore_colors) {
    const auto index = std::uint32_t(normals) * 4 + std::uint32_t(colors && !ignore_colors) * 2 +
                       std::uint32_t(uv);
    return static_cast<VertexLayout>(index);
}
// Source table words independently proved from the pinned image; see packet receipts.
constexpr std::array<std::uint32_t, 4> kTextureAddressModes{1, 3, 2, 3};
constexpr std::array<std::uint32_t, 8> kBlendModes{1, 2, 3, 4, 5, 6, 7, 8};
// Distinct source data matrices. Equal identity bytes elsewhere do not alias these tables.
constexpr graphics::Matrix kEnvironmentMapTransform{-0.5f, 0, 0, 0, 0,    0.5f, 0, 0,
                                                    0,     0, 1, 0, 0.5f, 0.5f, 0, 1};
constexpr graphics::Matrix kResetTextureTransform{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
constexpr std::uint32_t kCameraSpaceNormalGenerator = 1U << 16;
constexpr std::uint32_t kTwoTextureCoordinates = 2;
constexpr float kColorByteReciprocal = 0x1.010102p-8f;
constexpr std::uint32_t kColorByteMask = 255;
constexpr std::uint32_t kMaterialIndexMask = 16383;
constexpr std::uint32_t kTextureIndexMask = 65535;
constexpr std::uint32_t kBlendIndexMask = 7;
constexpr std::uint32_t kCullNone = 1;
constexpr std::uint32_t kMaterialSourceVertexDiffuse = 1;
constexpr std::uint32_t kTemporaryMaterialFlags =
    static_cast<std::uint32_t>(RenderFlag::ConstantMaterial) |
    static_cast<std::uint32_t>(RenderFlag::OffsetMaterial);
constexpr std::uint32_t kTemporaryRenderFlags =
    kTemporaryMaterialFlags | static_cast<std::uint32_t>(RenderFlag::Environment);

// NOTSA: FILD byte channels are exact binary32 integers, without normalization.
std::array<float, 4> color_channels(std::uint32_t color) {
    return {float((color >> 16) & kColorByteMask), float((color >> 8) & kColorByteMask),
            float(color & kColorByteMask), float(color >> 24)};
}
// NOTSA: ordered source clamp keeps unordered values and signed zero unchanged.
fidelity::ExactFloat clamp_material_channel(Context &context, fidelity::ExactFloat value) {
    const auto one = exact(context, 1.0f);
    if (one.compare(value) == X87Comparison::less)
        return exact(context, 1.0f);
    if (value.compare(0.0f) == X87Comparison::less)
        return exact(context, 0.0f);
    return value;
}

} // namespace

void prepare_matrices(Owners owners, Context &context) {
    owners.state.current_no_view =
        multiply_matrices(context, owners.current_matrix, owners.inverse_view);
    invert_matrix(context, owners.state.inverse_current, owners.current_matrix);
}

void project_to_world_space(Owners owners, Context &context) {
    owners.state.world = multiply_matrices(context, owners.current_matrix, owners.inverse_view);
    owners.state.extracted =
        multiply_matrices(context, owners.current_matrix, owners.state.inverse_current);
}

void set_world_transform(Owners owners, Context &context) {
    set_world_transform(context, owners.state.world);
}

void reset_world_transform(Owners owners, Context &context) {
    set_world_transform(context, owners.reset_world);
}

void copy_constant_material(Owners owners, Context &context) {
    prepare_matrices(owners, context);
    set_render_state(context, RenderState::CullMode, kCullNone);
    set_render_state(context, RenderState::DiffuseMaterialSource, kMaterialSourceVertexDiffuse);
    set_render_state(context, RenderState::AmbientMaterialSource, kMaterialSourceVertexDiffuse);
    owners.state.cull_mode = kCullNone;
    if (has(owners.control, Control::ConstantMaterial)) {
        // The flag is published before the conversions. Publish the packed color
        // only after every conversion succeeds; _ftol's low byte wraps, not clamps.
        auto channel = exact(context, owners.constant_material[3]) * 255.0f;
        owners.state.render_flags |= static_cast<std::uint32_t>(RenderFlag::ConstantMaterial);
        auto packed = std::uint32_t(channel.truncated_integer()) & kColorByteMask;
        for (std::uint32_t index = 1; index < 4; ++index) {
            channel = exact(context, owners.constant_material[3 - index]) * 255.0f;
            packed |= (std::uint32_t(channel.truncated_integer()) & kColorByteMask) << (index * 8);
        }
        owners.state.constant_color = packed;
        return;
    }
    if (has(owners.control, Control::OffsetMaterial)) {
        // The source uses quiet comparisons, in B,G,R,A order, short circuiting
        // on any nonzero or unordered component. Preserve exceptional loads too.
        for (std::uint32_t index = 0; index < 4; ++index) {
            const auto zero = exact(context, 0.0f);
            const auto component = exact(context, owners.constant_material[3 - index]);
            if (component.quiet_compare(zero) != X87Comparison::equal) {
                owners.state.render_flags |= static_cast<std::uint32_t>(RenderFlag::OffsetMaterial);
                break;
            }
        }
    }
}

void reset_render_flags(State &state, Context &context) {
    if (has(state.render_flags, RenderFlag::Environment)) {
        reset_texture_transform(context);
        state.render_flags &= ~kTemporaryRenderFlags;
    } else {
        state.render_flags &= ~kTemporaryMaterialFlags;
    }
}

void set_texture_shading(Context &context, TextureShading mode) {
    enum class AlphaOperation : std::uint32_t {
        SelectTexture = 2,
        SelectDiffuse = 3,
        Modulate = 4
    };
    constexpr std::uint32_t kAlphaReference = 16;
    switch (mode) {
    case TextureShading::Modulate:
        set_render_state(context, RenderState::AlphaBlendEnable, 0);
        set_texture_state(context, TextureState::AlphaOperation,
                          static_cast<std::uint32_t>(AlphaOperation::SelectDiffuse));
        set_render_state(context, RenderState::AlphaReference, 0);
        break;
    case TextureShading::DecalAlpha:
        set_render_state(context, RenderState::AlphaBlendEnable, 1);
        set_texture_state(context, TextureState::AlphaOperation,
                          static_cast<std::uint32_t>(AlphaOperation::SelectTexture));
        set_render_state(context, RenderState::AlphaReference, 0);
        break;
    case TextureShading::ModulateAlpha:
        set_render_state(context, RenderState::AlphaBlendEnable, 1);
        set_texture_state(context, TextureState::AlphaOperation,
                          static_cast<std::uint32_t>(AlphaOperation::Modulate));
        set_render_state(context, RenderState::AlphaReference, kAlphaReference);
        break;
    default:
        break;
    }
}

std::uint32_t offset_material_color(const Material &material, Owners owners, Context &context) {
    const auto alpha = exact(context, float(material.diffuse >> 24)) * kColorByteReciprocal;
    const auto red =
        (exact(context, float((material.diffuse >> 16) & kColorByteMask)) * kColorByteReciprocal)
            .rounded();
    const auto green =
        (exact(context, float((material.diffuse >> 8) & kColorByteMask)) * kColorByteReciprocal)
            .rounded();
    const auto blue =
        (exact(context, float(material.diffuse & kColorByteMask)) * kColorByteReciprocal).rounded();
    // Alpha remains retained while the other normalized inputs spill. All four
    // clamped sums remain retained until the reversed B,G,R,A conversion sequence.
    const auto adjusted_alpha =
        clamp_material_channel(context, alpha + owners.constant_material[0]);
    const auto adjusted_red =
        clamp_material_channel(context, exact(context, owners.constant_material[1]) + red);
    const auto adjusted_green =
        clamp_material_channel(context, exact(context, owners.constant_material[2]) + green);
    const auto adjusted_blue =
        clamp_material_channel(context, exact(context, owners.constant_material[3]) + blue);
    auto packed = std::uint32_t((adjusted_blue * 255.0f).truncated_integer()) & kColorByteMask;
    packed |= (std::uint32_t((adjusted_green * 255.0f).truncated_integer()) & kColorByteMask) << 8;
    packed |= (std::uint32_t((adjusted_red * 255.0f).truncated_integer()) & kColorByteMask) << 16;
    packed |= (std::uint32_t((adjusted_alpha * 255.0f).truncated_integer()) & kColorByteMask) << 24;
    return packed;
}

void environment_map(Context &context) {
    set_texture_state(context, TextureState::TransformFlags, kTwoTextureCoordinates);
    set_texture_transform(context, kEnvironmentMapTransform);
    set_texture_state(context, TextureState::CoordinateIndex, kCameraSpaceNormalGenerator);
}

void reset_texture_transform(Context &context) {
    set_texture_state(context, TextureState::TransformFlags, 0);
    set_texture_transform(context, kResetTextureTransform);
    set_texture_state(context, TextureState::CoordinateIndex, 0);
}

void parse_material(const Material &material, Owners owners, Context &context) {
    const auto saved_flags = owners.state.render_flags;
    const auto saved_control = owners.control;
    auto color = material.diffuse;
    if (has(saved_flags, RenderFlag::ConstantMaterial))
        color = owners.state.constant_color;
    else if (has(saved_flags, RenderFlag::OffsetMaterial))
        color = offset_material_color(material, owners, context);
    owners.state.landtable_color = material.diffuse;
    owners.state.vertex_color = color;
    graphics::Material device_material;
    device_material.diffuse = color_channels(color);
    device_material.ambient = device_material.diffuse;
    device_material.specular = color_channels(material.specular);
    device_material.emissive = {0, 0, 0, 0};
    device_material.power = material.power;
    set_device_material(context, device_material);

    auto attributes = material.attributes;
    if (has(saved_control, Control::ConstantAttributes))
        attributes = (attributes & owners.attribute_mask) | owners.attributes;
    if (has(attributes, MaterialFlag::Texture)) {
        const auto v_index = ((attributes >> 15) & 1) | ((attributes >> 16) & 2);
        const auto u_index = ((attributes >> 16) & 1) | ((attributes >> 17) & 2);
        const auto source_blend = kBlendModes[attributes >> 29];
        const auto destination_blend = kBlendModes[(attributes >> 26) & kBlendIndexMask];
        set_texture_state(context, TextureState::AddressU, kTextureAddressModes[u_index]);
        set_texture_state(context, TextureState::AddressV, kTextureAddressModes[v_index]);
        set_render_state(context, RenderState::SourceBlend, source_blend);
        set_render_state(context, RenderState::DestinationBlend, destination_blend);
        set_texture_number(context, material.texture & kTextureIndexMask);
    } else {
        set_null_texture(context);
    }
    set_render_state(context, RenderState::Lighting,
                     has(attributes, MaterialFlag::IgnoreLight) ? 0 : 1);
    if (has(attributes, MaterialFlag::Environment)) {
        if (!has(saved_flags, RenderFlag::Environment)) {
            environment_map(context);
            owners.state.render_flags |= static_cast<std::uint32_t>(RenderFlag::Environment);
        }
    } else if (has(saved_flags, RenderFlag::Environment)) {
        reset_texture_transform(context);
        owners.state.render_flags &= ~static_cast<std::uint32_t>(RenderFlag::Environment);
    }
    set_texture_shading(context, has(attributes, MaterialFlag::UseAlpha)
                                     ? TextureShading::ModulateAlpha
                                     : TextureShading::Modulate);
}

void draw_direct_model(character_pack::ResourceId id, const character_pack::CharacterPack &pack,
                       Owners owners, Context &context) {
    const auto entry_flags = owners.state.render_flags;
    if (has(entry_flags, RenderFlag::ConstantMaterial) ||
        has(entry_flags, RenderFlag::OffsetMaterial)) {
        draw_alternate_model(context, id, pack, owners);
        return;
    }
    project_to_world_space(owners, context);
    set_world_transform(owners, context);
    const auto model = pack.model(id);
    const auto callback = snapshot_material_callback(context);
    auto previous_material = kMaterialIndexMask;
    const auto ignore_colors = has(entry_flags, RenderFlag::IgnoreVertexColors);
    for (std::uint32_t index = 0; index < model.mesh_count; ++index) {
        const auto &record = pack.record(model.meshsets);
        const auto offset = 4 + std::size_t(index) * 28;
        const auto mesh_type = word(record, offset);
        const auto material_index = mesh_type & kMaterialIndexMask;
        if (material_index != previous_material) {
            previous_material = material_index;
            parse_material(material(pack, model.materials, material_index), owners, context);
            if (callback)
                callback(context);
        }
        const Meshset mesh{pack, model.meshsets, index};
        if (word(record, offset + 24) != 0) {
            draw_buffered_mesh(context, mesh);
            continue;
        }
        const auto vertex_layout = layout(model.normals != 0, word(record, offset + 16) != 0,
                                          word(record, offset + 20) != 0, ignore_colors);
        const auto primitive = static_cast<Primitive>((mesh_type >> 14) & 3);
        if (vertex_layout == VertexLayout::PositionNormalUv && primitive == Primitive::Strip)
            draw_normal_uv_strip(context, mesh, model.points, model.normals);
        else
            unsupported_primitive(context, vertex_layout, primitive, mesh, model.points,
                                  model.normals);
    }
    reset_world_transform(owners, context);
    clear_stream_source(context);
    clear_indices(context);
}

void draw_model(character_pack::ResourceId id, const character_pack::CharacterPack &pack,
                Owners owners, Context &context) {
    copy_constant_material(owners, context);
    draw_direct_model(id, pack, owners, context);
    reset_render_flags(owners.state, context);
}
} // namespace sadx::basic_model
