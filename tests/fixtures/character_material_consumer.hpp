#pragma once
#include "runtime/model_device.hpp"
#include "runtime/model_draw_context.hpp"
#include "runtime/object_draw_context.hpp"
#include <fstream>

namespace character_material_test {
inline std::uint32_t word(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned byte = 0; byte < 4; ++byte)
        value |= std::uint32_t(bytes.at(offset + byte)) << (byte * 8);
    return value;
}
inline void put(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned byte = 0; byte < 4; ++byte)
        bytes.at(offset + byte) = static_cast<std::uint8_t>(value >> (byte * 8));
}
inline void append(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
    const auto offset = bytes.size();
    bytes.resize(offset + 4);
    put(bytes, offset, value);
}
inline void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
// Reject malformed declarations at the importer boundary, before scene publication.
inline void declarations(const std::filesystem::path &directory,
                         const sadx::character_pack::CharacterPack &pack) {
    using namespace sadx;
    const auto metadata = native::app::read_metadata(directory / "characters/gamma-actor.json");
    const auto original = native::app::gamma_materials(metadata, pack);
    require(original.size() == 5, "all five real declarations decode");
    for (unsigned fault = 0; fault < 7; ++fault) {
        NSMutableDictionary *changed = [metadata mutableCopy];
        NSMutableArray *rows = [changed[@"materials"] mutableCopy];
        NSMutableDictionary *row = [rows[0] mutableCopy];
        switch (fault) {
        case 0: [changed removeObjectForKey:@"materials"]; break;
        case 1: [rows removeLastObject]; changed[@"materials"] = rows; break;
        case 2: row[@"table_row"] = @5; break;
        case 3: row[@"material_index"] = @2; break;
        case 4: row[@"materials"] = @0; break;
        case 5: row[@"materials"] = @(pack.resolve(native::app::load_gamma_actor_assets(directory, pack).stand).object); break;
        case 6: row[@"record_index"] = @UINT32_MAX; break;
        }
        if (fault >= 2) { rows[0] = row; changed[@"materials"] = rows; }
        bool rejected = false;
        try { (void)native::app::gamma_materials(changed, pack); }
        catch (const std::runtime_error &) { rejected = true; }
        require(rejected, "malformed Gamma material declaration must fail explicitly");
    }
}
// All five words, including exponent signed zero and NaN payloads, stay intact
// through typed construction. The whole imported graph remains valid.
inline void unusual_bits(const std::filesystem::path &directory) {
    using namespace sadx;
    std::ifstream stream(directory / "characters/sonic-selection.actions", std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(stream), {}};
    const std::array<std::uint32_t, 5> patterns{0x80000000, 0x7fa12345, 0x7fc54321, 0xffffffff, 0};
    const auto count = word(bytes, 8);
    std::size_t offset = 16 + word(bytes, 12) * 16;
    for (std::size_t id = 1; id <= count; ++id) {
        const auto kind = word(bytes, offset), size = word(bytes, offset + 4);
        offset += 8;
        if (kind == static_cast<std::uint32_t>(character_pack::ResourceKind::Materials))
            for (std::size_t field = 0; field < (size - 4) / 4; ++field)
                put(bytes, offset + 4 + field * 4, patterns[field % patterns.size()]);
        offset += size;
    }
    const auto pack = character_pack::CharacterPack::decode(bytes);
    const character_pack::MaterialState materials(pack);
    for (std::size_t id = 1; id <= count; ++id) {
        const auto &record = pack.record(static_cast<character_pack::ResourceId>(id));
        if (record.kind != character_pack::ResourceKind::Materials) continue;
        for (std::size_t item = 0; item < word(record.payload, 0); ++item) {
            const auto &material = materials.material(static_cast<character_pack::ResourceId>(id), item);
            require(material.words == patterns, "all unusual material words preserve bits");
            require(material.diffuse() == patterns[0] && material.specular() == patterns[1] &&
                    material.texture() == patterns[3] && material.attributes() == patterns[4],
                    "semantic accessors preserve raw words");
            const auto exponent = material.power();
            std::uint32_t exponent_bits;
            std::memcpy(&exponent_bits, &exponent, sizeof(exponent_bits));
            require(exponent_bits == patterns[2], "exponent NaN payload preserved without arithmetic");
        }
    }
}
// A controlled graph fixture routes two model identities through the already
// supported Question strip. Material/geometry payloads are original and unchanged;
// only model/mesh descriptors are rewired. This is consumer proof, not Gamma pixels.
inline void render(sadx::gamma_selection::Context &gamma,
                   const std::filesystem::path &directory,
                   const sadx::character_pack::CharacterPack &pack) {
    using namespace sadx;
    namespace cp = character_pack;
    const auto &binding = gamma.players.gamma_assets->materials.back();
    const auto &live = gamma.scene.materials.material(binding.materials, binding.record_index);
    require(gamma.players.gamma.headlight_material == &live,
            "headlight must retain the canonical renderer record address");
    constexpr cp::ResourceId question = 2943; // Existing model-binding fixture.
    const auto question_model = pack.model(question);
    std::array<cp::ResourceId, 2> models{};
    for (std::size_t id = 1, count = 0; id <= pack.record_count() && count < models.size(); ++id)
        if (pack.record(id).kind == cp::ResourceKind::Model && id != question)
            models[count++] = static_cast<cp::ResourceId>(id);
    const auto mesh_id = pack.model(models[0]).meshsets;
    auto model_bytes = pack.record(question).payload;
    put(model_bytes, 12, mesh_id);
    put(model_bytes, 16, binding.materials);
    const auto material_count = word(pack.record(binding.materials).payload, 0);
    put(model_bytes, 20, (material_count << 16) | question_model.mesh_count);
    auto mesh_bytes = pack.record(question_model.meshsets).payload;
    const auto type_count = word(mesh_bytes, 4);
    put(mesh_bytes, 4, (type_count & ~std::uint32_t{16383}) |
                      static_cast<std::uint32_t>(binding.record_index));
    std::ifstream stream(directory / "characters/sonic-selection.actions", std::ios::binary);
    std::vector<std::uint8_t> source{std::istreambuf_iterator<char>(stream), {}};
    const auto prefix = 16 + word(source, 12) * 16;
    std::vector<std::uint8_t> bytes(source.begin(), source.begin() + prefix);
    for (std::size_t id = 1; id <= pack.record_count(); ++id) {
        const auto &record = pack.record(id);
        const auto &payload = id == models[0] || id == models[1] ? model_bytes :
                              id == mesh_id ? mesh_bytes : record.payload;
        append(bytes, static_cast<std::uint32_t>(record.kind));
        append(bytes, static_cast<std::uint32_t>(payload.size()));
        bytes.insert(bytes.end(), payload.begin(), payload.end());
    }
    const auto fixture = cp::CharacterPack::decode(bytes);
    lighting::State lighting({});
    auto hook = bootstrap::MaterialCallback::None;
    const native::ModelDrawEnvironment environment{fixture, gamma.scene, hook, lighting};
    basic_model::Context renderer{gamma.rendering, environment};
    object_draw::Context traversal{gamma.rendering, environment};
    polybuff::Context device{gamma.rendering};
    gamma.rendering.output().select_texture_list(*gamma.character_textures.tables[6].entries[0].textures);
    gamma.rendering.begin_game_frame();
    const auto root = pack.resolve(gamma.players.gamma_assets->stand).object;
    auto &object = gamma.scene.objects.object(root);
    const auto old_object = object;
    object.evaluation_flags = static_cast<std::uint32_t>(object_draw::ObjectFlags::UnitTransform) |
                              static_cast<std::uint32_t>(object_draw::ObjectFlags::Break);
    object.sibling = 0;
    std::uint8_t fog_flags = 0, fog_active = 0, simple = 0;
    float depth = 0;
    std::int32_t ticks = 0, palette = 0;
    object_draw::State state{fog_flags, fog_active, depth, ticks, simple, palette};
    auto &commands = device.frame().commands;
    require(commands.empty(), "consumer witness starts with an empty frame");
    for (const auto model : models) {
        object.model = model;
        object_draw::draw_objects(root, state, traversal);
        const auto &draw = std::get<graphics::Draw>(commands.back());
        const auto color = live.diffuse();
        require(draw.material.diffuse == std::array<float, 4>{
                    float((color >> 16) & 255), float((color >> 8) & 255),
                    float(color & 255), float(color >> 24)},
                "actual recorded draw must observe the Gamma-mutated canonical material");
        require(draw.material.specular == std::array<float, 4>{17, 34, 51, 68} &&
                    draw.material.power == 13,
                "Gamma must preserve the other material words consumed by the renderer");
        require(renderer.state().landtable_color == color && renderer.state().render_flags == 0,
                "real material parser and renderer tail execute in order");
    }
    require(commands.size() == 2, "both model users draw through the same material owner");
    // The object->model link is live on every traversal. Switch to the unmodified
    // Question model and observe its different material in a real third draw.
    object.model = question;
    object_draw::draw_objects(root, state, traversal);
    require(commands.size() == 3 &&
            std::get<graphics::Draw>(commands.back()).material.diffuse !=
                std::get<graphics::Draw>(commands.front()).material.diffuse,
            "changed live object model must change actual renderer output");
    require(std::get<graphics::Draw>(commands.front()).material.diffuse[3] == 255,
            "earlier draw retains its Gamma material snapshot");
    object = old_object;
}
} // namespace character_material_test
