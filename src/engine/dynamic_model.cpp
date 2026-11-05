#include "engine/dynamic_model.hpp"

namespace sadx::basic_model {
namespace {
// NOTSA: the imported mesh record is immutable and independent of process layouts.
std::uint32_t field(const character_pack::ResourceRecord &record, std::size_t offset,
                    std::size_t width) {
    std::uint32_t result = 0;
    for (std::size_t index = 0; index < width; ++index)
        result |= std::uint32_t(record.payload.at(offset + index)) << (index * 8);
    return result;
}
} // namespace
void return_only_primitive() {}

void draw_dynamic_model(character_pack::ResourceId id, const character_pack::CharacterPack &pack,
                        const character_pack::MaterialState &materials, Owners owners,
                        Context &context) {
    project_to_world_space(owners, context);
    set_world_transform(owners, context);
    const auto callback = snapshot_material_callback(context);
    const auto model = pack.model(id);
    constexpr std::uint32_t kMaterialMask = 16383;
    auto previous_material = kMaterialMask;
    const bool ignore_colors = (owners.state.render_flags &
                                static_cast<std::uint32_t>(RenderFlag::IgnoreVertexColors)) != 0;
    for (std::uint32_t index = 0; index < model.mesh_count; ++index) {
        const auto &record = pack.record(model.meshsets);
        const auto offset = 4 + std::size_t(index) * 28;
        const auto mesh_type = field(record, offset, 2);
        const auto material_index = mesh_type & kMaterialMask;
        if (material_index != previous_material) {
            const auto &entry = materials.material(model.materials, material_index);
            parse_material({entry.diffuse(), entry.specular(), entry.power(), entry.texture(),
                            entry.attributes()},
                           owners, context);
            previous_material = material_index;
            if (callback)
                callback(context);
        }
        const bool colors = field(record, offset + 16, 4) != 0;
        const bool uv = field(record, offset + 20, 4) != 0;
        const auto layout = static_cast<VertexLayout>(std::uint32_t(model.normals != 0) * 4 +
                                                      std::uint32_t(colors && !ignore_colors) * 2 +
                                                      std::uint32_t(uv));
        const auto primitive = static_cast<Primitive>(mesh_type >> 14);
        if (primitive == Primitive::Polygon)
            return_only_primitive();
        else
            draw_dynamic_mesh(context, layout, primitive, {pack, model.meshsets, index},
                              model.points, model.normals);
    }
    reset_world_transform(owners, context);
}
} // namespace sadx::basic_model
