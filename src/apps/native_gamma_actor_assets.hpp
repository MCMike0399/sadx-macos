#pragma once
#include "apps/native_assets.hpp"
#include "runtime/gamma_actor_assets.hpp"

namespace sadx::native::app {
// NOTSA: exact word decoding at the imported-content boundary, without FP arithmetic.
inline float actor_scalar(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}
inline std::uint32_t actor_word(id value) {
    const auto text = string(value, "actor floating word");
    if ((text.size() < 3 || text.size() > 10) || text.substr(0, 2) != "0x")
        throw std::runtime_error("invalid imported actor floating word");
    std::uint32_t result = 0;
    for (std::size_t index = 2; index < text.size(); ++index) {
        const auto digit = text[index];
        if (digit >= '0' && digit <= '9') result = (result << 4) | unsigned(digit - '0');
        else if (digit >= 'a' && digit <= 'f') result = (result << 4) | unsigned(digit - 'a' + 10);
        else throw std::runtime_error("invalid imported actor floating digit");
    }
    return result;
}
inline GammaActionDeclaration gamma_declaration(NSDictionary *row,
                                                 const character_pack::CharacterPack &pack) {
    GammaActionDeclaration result{};
    const auto status = string(row[@"status"], "Gamma action status");
    id action = row[@"action"];
    id boundary = row[@"resource_boundary"];
    if (status == "loaded") {
        result.status = ActorResourceStatus::Loaded;
        result.action.identity = integer(action, 1, pack.record_count(), "Gamma action resource");
        (void)pack.resolve(result.action);
        if (boundary != NSNull.null) throw std::runtime_error("loaded Gamma action has boundary");
    } else if (status == "known_null") {
        result.status = ActorResourceStatus::KnownNull;
        if (action != NSNull.null || boundary != NSNull.null)
            throw std::runtime_error("invalid known-null Gamma declaration");
    } else if (status == "unavailable") {
        result.status = ActorResourceStatus::Unavailable;
        if (action != NSNull.null) throw std::runtime_error("unavailable Gamma action has payload");
        const auto resource = dictionary(boundary, "Gamma resource boundary");
        result.boundary = string(resource[@"component"], "Gamma component") + ":" +
            string(resource[@"source_address"], "Gamma resource identity") + ":" +
            string(resource[@"reason"], "Gamma resource reason");
    } else throw std::runtime_error("unknown Gamma declaration status");
    return result;
}
// NOTSA: complete source recipe metadata binds to the SAME validated CharacterPack.
inline GammaActorAssets load_gamma_actor_assets(const std::filesystem::path &directory,
                                                const character_pack::CharacterPack &pack) {
    const auto metadata = read_metadata(directory / "characters/gamma-actor.json");
    if (string(metadata[@"pack_schema"], "Gamma pack schema") != "SDXACT03")
        throw std::runtime_error("invalid Gamma pack schema");
    (void)integer(metadata[@"character"], 6, 6, "Gamma character");
    (void)integer(metadata[@"texture_character"], 6, 6, "Gamma texture character");
    (void)integer(metadata[@"initial_animation_export_index"], 0, 0, "Gamma Stand export");
    GammaActorAssets result{};
    const auto sonic = read_metadata(directory / "characters/sonic-actor.json");
    const auto parameters = array(sonic[@"player_parameter_words"], 8, "player parameters");
    for (std::size_t index = 0; index < result.parameters.size(); ++index) {
        const auto words = array(parameters[index], 33, "player parameter row");
        std::array<std::uint8_t, 132> bytes{};
        for (std::size_t item = 0; item < 33; ++item) {
            const auto word = static_cast<std::uint32_t>(integer(words[item], 0, UINT32_MAX,
                                                                 "player parameter word"));
            for (unsigned byte = 0; byte < 4; ++byte)
                bytes[item * 4 + byte] = static_cast<std::uint8_t>(word >> (byte * 8));
        }
        const auto decoded = decode_player_parameters(bytes.data(), bytes.size());
        if (!decoded) throw std::runtime_error("invalid imported player parameter record");
        result.parameters[index] = *decoded.parameters;
    }
    const auto declarations = array(metadata[@"declarations"], 78, "Gamma declarations");
    for (std::size_t index = 0; index < result.declarations.size(); ++index) {
        const auto row = dictionary(declarations[index], "Gamma declaration");
        (void)integer(row[@"index"], index, index, "Gamma declaration index");
        result.declarations[index] = gamma_declaration(row, pack);
        if ((index == 61) != (result.declarations[index].status == ActorResourceStatus::KnownNull))
            throw std::runtime_error("Gamma declaration null identity changed");
    }
    result.stand.identity = integer(metadata[@"initial_animation_action"], 1,
                                    pack.record_count(), "Gamma Stand action");
    if (result.stand.identity != result.declarations[0].action.identity)
        throw std::runtime_error("Gamma Stand declaration mismatch");
    const auto stand = pack.resolve(result.stand);
    const auto motion = pack.motion(stand.motion);
    if (motion.part_count != 77 || motion.frame_count != 16 || motion.channels != 3 ||
        motion.interpolation != 2)
        throw std::runtime_error("Gamma Stand motion metadata mismatch");
    auto table = std::make_shared<PlayerAnimationTable>();
    const auto recipes = array(metadata[@"recipes"], 91, "Gamma recipes");
    for (std::size_t index = 0; index < result.recipes.size(); ++index) {
        const auto row = dictionary(recipes[index], "Gamma recipe");
        (void)integer(row[@"row"], index, index, "Gamma recipe row");
        auto &recipe = result.recipes[index];
        recipe.resource = gamma_declaration(row, pack);
        recipe.object_count = static_cast<std::uint8_t>(integer(row[@"object_count"], 0, 255,
                                                                 "Gamma object count"));
        recipe.mode = static_cast<std::int8_t>(integer(row[@"mode"], -128, 127, "Gamma motion mode"));
        recipe.next = static_cast<std::uint16_t>(integer(row[@"next"], 0, 90, "Gamma next motion"));
        recipe.transition = actor_scalar(actor_word(row[@"transition_bits"]));
        recipe.speed = actor_scalar(actor_word(row[@"speed_bits"]));
        if (index == 72) {
            if (string(row[@"declaration"], "constructed Gamma action") != "constructed")
                throw std::runtime_error("Gamma constructed recipe identity changed");
        } else {
            const auto declaration = integer(row[@"declaration"], 0, 77, "Gamma recipe declaration");
            const auto &resource = result.declarations[declaration];
            if (recipe.resource.status != resource.status ||
                recipe.resource.action.identity != resource.action.identity ||
                recipe.resource.boundary != resource.boundary)
                throw std::runtime_error("Gamma recipe declaration mismatch");
        }
        PlayerAnimationRecord record{};
        // Legacy table adapter: unavailable payload remains unresolved, never known NULL.
        if (recipe.resource.status == ActorResourceStatus::Loaded)
            record.action = AnimationResourceReference{recipe.resource.action.identity};
        else if (recipe.resource.status == ActorResourceStatus::KnownNull)
            record.action = AnimationResourceReference{};
        record.object_count = recipe.object_count;
        record.mode = recipe.mode;
        record.next = recipe.next;
        record.transition_step = recipe.transition;
        record.frame_step = recipe.speed;
        table->records.emplace(static_cast<std::uint16_t>(index), record);
    }
    if (result.recipes[0].resource.action.identity != result.stand.identity)
        throw std::runtime_error("Gamma request zero does not use Stand");
    result.animation_table = std::move(table);
    const auto collision = array(metadata[@"collision_words"], 36, "Gamma collision records");
    for (std::size_t index = 0; index < result.collision_words.size(); ++index)
        result.collision_words[index] = static_cast<std::uint32_t>(
            integer(collision[index], 0, UINT32_MAX, "Gamma collision word"));
    const auto colors = array(metadata[@"line_color_words"], 2, "Gamma line colors");
    for (std::size_t index = 0; index < result.line_colors.size(); ++index)
        result.line_colors[index] = static_cast<std::uint32_t>(
            integer(colors[index], 0, UINT32_MAX, "Gamma line color"));
    (void)integer(metadata[@"line_point_count"], 2, 2, "Gamma line point count");
    return result;
}
} // namespace sadx::native::app
