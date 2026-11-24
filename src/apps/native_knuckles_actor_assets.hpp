#pragma once
#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/knuckles_actor_assets.hpp"

namespace sadx::native::app {
namespace {
// NOTSA: decode one imported Knuckles weld declaration without expanding buffers.
inline KnucklesJoinDeclaration knuckles_join(NSDictionary *row,
                                             const character_pack::CharacterPack &pack) {
    KnucklesJoinDeclaration result{};
    const auto identity = [&pack](id value, const char *label) {
        const auto id =
            static_cast<character_pack::ResourceId>(integer(value, 0, pack.record_count(), label));
        if (id)
            (void)pack.object(id);
        return id;
    };
    result.object = identity(row[@"object"], "Knuckles weld object");
    result.source_object = identity(row[@"source_object"], "Knuckles weld source object");
    result.destination_object =
        identity(row[@"destination_object"], "Knuckles weld destination object");
    result.vertex_count = static_cast<std::uint8_t>(
        integer(row[@"vertex_count"], 0, 255, "Knuckles weld vertices"));
    result.input_mode =
        static_cast<std::int8_t>(integer(row[@"input_mode"], -128, 127, "Knuckles weld mode"));
    result.source_depth = static_cast<std::uint8_t>(
        integer(row[@"source_node"], 0, 255, "Knuckles weld source depth"));
    result.destination_depth = static_cast<std::uint8_t>(
        integer(row[@"destination_node"], 0, 255, "Knuckles weld destination depth"));
    result.vertex_pairs = static_cast<character_pack::ResourceId>(
        integer(row[@"vertex_pairs"], 0, pack.record_count(), "Knuckles weld pairs"));
    return result;
}
} // namespace

// NOTSA: complete source recipe metadata binds to the SAME validated CharacterPack.
inline KnucklesActorAssets load_knuckles_actor_assets(const std::filesystem::path &directory,
                                                      const character_pack::CharacterPack &pack) {
    const auto metadata = read_metadata(directory / "characters/knuckles-actor.json");
    if (string(metadata[@"pack_schema"], "Knuckles pack schema") != "SDXACT03")
        throw std::runtime_error("invalid Knuckles pack schema");
    (void)integer(metadata[@"character"], 3, 3, "Knuckles character");
    (void)integer(metadata[@"texture_character"], 3, 3, "Knuckles texture character");
    const auto stand_declaration =
        integer(metadata[@"initial_animation_export_index"], 0, 89, "Knuckles Stand export");
    KnucklesActorAssets result{};
    const auto declarations = array(metadata[@"declarations"], 90, "Knuckles declarations");
    for (std::size_t index = 0; index < result.declarations.size(); ++index) {
        const auto row = dictionary(declarations[index], "Knuckles declaration");
        (void)integer(row[@"index"], index, index, "Knuckles declaration index");
        result.declarations[index] = gamma_declaration(row, pack);
    }
    auto table = std::make_shared<animation::Table>();
    const auto recipes = array(metadata[@"recipes"], 115, "Knuckles recipes");
    for (std::size_t index = 0; index < result.recipes.size(); ++index) {
        const auto row = dictionary(recipes[index], "Knuckles recipe");
        (void)integer(row[@"row"], index, index, "Knuckles recipe row");
        auto &recipe = result.recipes[index];
        recipe.resource = gamma_declaration(row, pack);
        recipe.object_count = static_cast<std::uint8_t>(
            integer(row[@"object_count"], 0, 255, "Knuckles object count"));
        recipe.mode =
            static_cast<std::int8_t>(integer(row[@"mode"], -128, 127, "Knuckles motion mode"));
        recipe.next =
            static_cast<std::uint16_t>(integer(row[@"next"], 0, 114, "Knuckles next motion"));
        recipe.transition = actor_scalar(actor_word(row[@"transition_bits"]));
        recipe.speed = actor_scalar(actor_word(row[@"speed_bits"]));
        if (index == 36) {
            if (string(row[@"declaration"], "constructed Knuckles action") != "constructed")
                throw std::runtime_error("Knuckles constructed recipe identity changed");
        } else if (recipe.resource.status == ActorResourceStatus::KnownNull) {
            if (string(row[@"declaration"], "known-null Knuckles declaration") != "known_null")
                throw std::runtime_error("Knuckles known-null recipe identity changed");
        } else {
            const auto declaration =
                integer(row[@"declaration"], 0, 89, "Knuckles recipe declaration");
            const auto &resource = result.declarations[declaration];
            if (recipe.resource.status != resource.status ||
                recipe.resource.action.identity != resource.action.identity ||
                recipe.resource.boundary != resource.boundary)
                throw std::runtime_error("Knuckles recipe declaration mismatch");
            if (index == 0 && declaration != stand_declaration)
                throw std::runtime_error("Knuckles request zero does not use the exported Stand");
        }
        animation::Record record{};
        record.action.status = recipe.resource.status;
        record.action.identity = recipe.resource.action.identity;
        record.action.unavailable_reason = recipe.resource.boundary;
        record.object_count = recipe.object_count;
        record.mode = static_cast<animation::Mode>(recipe.mode);
        record.next = recipe.next;
        record.transition_step = recipe.transition;
        record.frame_step = recipe.speed;
        table->records.emplace(static_cast<std::uint16_t>(index), record);
    }
    result.stand.identity =
        integer(metadata[@"initial_animation_action"], 1, pack.record_count(), "Knuckles Stand action");
    if (result.stand.identity != result.declarations[stand_declaration].action.identity)
        throw std::runtime_error("Knuckles Stand declaration mismatch");
    if (result.recipes[0].resource.action.identity != result.stand.identity)
        throw std::runtime_error("Knuckles request zero does not use Stand");
    // Derive the reached motion identity from the same pack the actor binds.
    const auto stand = pack.resolve(result.stand);
    const auto motion = pack.motion(stand.motion);
    if (motion.part_count != result.recipes[0].object_count)
        throw std::runtime_error("Knuckles Stand motion part count differs from request zero");
    result.animation_table = std::move(table);
    const auto joins = array(metadata[@"joins"], 30, "Knuckles weld declarations");
    for (std::size_t index = 0; index < result.joins.size(); ++index) {
        result.joins[index] =
            knuckles_join(dictionary(joins[index], "Knuckles weld declaration"), pack);
        const auto pairs = result.joins[index].vertex_pairs;
        if (!pairs)
            continue;
        const auto &record = pack.record(pairs);
        if (record.kind != character_pack::ResourceKind::Indices)
            throw std::runtime_error("Knuckles weld pair array kind mismatch");
        constexpr std::size_t kArrayHeaderBytes = 4, kIndexBytes = 2;
        const auto &bytes = record.payload;
        for (std::size_t offset = kArrayHeaderBytes; offset + kIndexBytes <= bytes.size();
             offset += kIndexBytes) {
            std::uint16_t value = 0;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            result.join_pairs[index].push_back(value);
        }
    }
    const auto equipment_objects =
        array(metadata[@"equipment_objects"], 16, "Knuckles equipment objects");
    for (std::size_t index = 0; index < result.equipment_objects.size(); ++index) {
        const auto row = dictionary(equipment_objects[index], "Knuckles equipment object");
        (void)integer(row[@"index"], 58 + index, 58 + index, "Knuckles equipment object row");
        const auto object = static_cast<character_pack::ResourceId>(
            integer(row[@"object"], 1, pack.record_count(), "Knuckles equipment object identity"));
        (void)pack.object(object);
        result.equipment_objects[index] = object;
    }
    const auto equipment_models =
        array(metadata[@"equipment_models"], 23, "Knuckles equipment models");
    for (std::size_t index = 0; index < result.equipment_models.size(); ++index) {
        const auto row = dictionary(equipment_models[index], "Knuckles equipment model");
        (void)integer(row[@"index"], index, index, "Knuckles equipment model row");
        const auto model = static_cast<character_pack::ResourceId>(
            integer(row[@"model"], 1, pack.record_count(), "Knuckles equipment model identity"));
        (void)pack.model(model);
        result.equipment_models[index] = model;
    }
    const auto collision = array(metadata[@"collision_words"], 60, "Knuckles collision records");
    std::array<std::uint32_t, 60> collision_words{};
    for (std::size_t index = 0; index < collision_words.size(); ++index)
        collision_words[index] = static_cast<std::uint32_t>(
            integer(collision[index], 0, UINT32_MAX, "Knuckles collision word"));
    result.collision_info = gamma_collision_info(collision_words);
    return result;
}
} // namespace sadx::native::app
