#pragma once
#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/miles_actor_assets.hpp"

namespace sadx::native::app {
namespace {
// NOTSA: decode one imported Miles weld declaration without expanding buffers.
inline MilesJoinDeclaration miles_join(NSDictionary *row, const character_pack::CharacterPack &pack) {
    MilesJoinDeclaration result{};
    const auto identity = [&pack](id value, const char *label) {
        const auto id =
            static_cast<character_pack::ResourceId>(integer(value, 0, pack.record_count(), label));
        if (id)
            (void)pack.object(id);
        return id;
    };
    result.object = identity(row[@"object"], "Miles weld object");
    result.source_object = identity(row[@"source_object"], "Miles weld source object");
    result.destination_object =
        identity(row[@"destination_object"], "Miles weld destination object");
    result.vertex_count =
        static_cast<std::uint8_t>(integer(row[@"vertex_count"], 0, 255, "Miles weld vertices"));
    result.input_mode =
        static_cast<std::int8_t>(integer(row[@"input_mode"], -128, 127, "Miles weld mode"));
    result.source_depth = static_cast<std::uint8_t>(
        integer(row[@"source_node"], 0, 255, "Miles weld source depth"));
    result.destination_depth = static_cast<std::uint8_t>(
        integer(row[@"destination_node"], 0, 255, "Miles weld destination depth"));
    result.vertex_pairs = static_cast<character_pack::ResourceId>(
        integer(row[@"vertex_pairs"], 0, pack.record_count(), "Miles weld pairs"));
    return result;
}
} // namespace

// NOTSA: complete source recipe metadata binds to the SAME validated CharacterPack.
inline MilesActorAssets load_miles_actor_assets(const std::filesystem::path &directory,
                                                const character_pack::CharacterPack &pack) {
    const auto metadata = read_metadata(directory / "characters/miles-actor.json");
    if (string(metadata[@"pack_schema"], "Miles pack schema") != "SDXACT03")
        throw std::runtime_error("invalid Miles pack schema");
    (void)integer(metadata[@"character"], 2, 2, "Miles character");
    (void)integer(metadata[@"texture_character"], 2, 2, "Miles texture character");
    const auto stand_declaration =
        integer(metadata[@"initial_animation_export_index"], 0, 113, "Miles Stand export");
    MilesActorAssets result{};
    const auto declarations = array(metadata[@"declarations"], 114, "Miles declarations");
    for (std::size_t index = 0; index < result.declarations.size(); ++index) {
        const auto row = dictionary(declarations[index], "Miles declaration");
        (void)integer(row[@"index"], index, index, "Miles declaration index");
        result.declarations[index] = gamma_declaration(row, pack);
    }
    auto table = std::make_shared<animation::Table>();
    const auto recipes = array(metadata[@"recipes"], 136, "Miles recipes");
    for (std::size_t index = 0; index < result.recipes.size(); ++index) {
        const auto row = dictionary(recipes[index], "Miles recipe");
        (void)integer(row[@"row"], index, index, "Miles recipe row");
        auto &recipe = result.recipes[index];
        recipe.resource = gamma_declaration(row, pack);
        recipe.object_count = static_cast<std::uint8_t>(
            integer(row[@"object_count"], 0, 255, "Miles object count"));
        recipe.mode = static_cast<std::int8_t>(integer(row[@"mode"], -128, 127, "Miles motion mode"));
        recipe.next = static_cast<std::uint16_t>(integer(row[@"next"], 0, 135, "Miles next motion"));
        recipe.transition = actor_scalar(actor_word(row[@"transition_bits"]));
        recipe.speed = actor_scalar(actor_word(row[@"speed_bits"]));
        if (index == 89) {
            if (string(row[@"declaration"], "constructed Miles action") != "constructed")
                throw std::runtime_error("Miles constructed recipe identity changed");
        } else if (recipe.resource.status == ActorResourceStatus::KnownNull) {
            if (string(row[@"declaration"], "known-null Miles declaration") != "known_null")
                throw std::runtime_error("Miles known-null recipe identity changed");
        } else {
            const auto declaration = integer(row[@"declaration"], 0, 113, "Miles recipe declaration");
            const auto &resource = result.declarations[declaration];
            if (recipe.resource.status != resource.status ||
                recipe.resource.action.identity != resource.action.identity ||
                recipe.resource.boundary != resource.boundary)
                throw std::runtime_error("Miles recipe declaration mismatch");
            if (index == 0 && declaration != stand_declaration)
                throw std::runtime_error("Miles request zero does not use the exported Stand");
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
        integer(metadata[@"initial_animation_action"], 1, pack.record_count(), "Miles Stand action");
    if (result.stand.identity != result.declarations[stand_declaration].action.identity)
        throw std::runtime_error("Miles Stand declaration mismatch");
    if (result.recipes[0].resource.action.identity != result.stand.identity)
        throw std::runtime_error("Miles request zero does not use Stand");
    // Derive the reached motion identity from the same pack the actor binds.
    const auto stand = pack.resolve(result.stand);
    const auto motion = pack.motion(stand.motion);
    if (motion.part_count != result.recipes[0].object_count)
        throw std::runtime_error("Miles Stand motion part count differs from request zero");
    result.animation_table = std::move(table);
    const auto joins = array(metadata[@"joins"], 30, "Miles weld declarations");
    for (std::size_t index = 0; index < result.joins.size(); ++index) {
        result.joins[index] = miles_join(dictionary(joins[index], "Miles weld declaration"), pack);
        const auto pairs = result.joins[index].vertex_pairs;
        if (!pairs)
            continue;
        const auto &record = pack.record(pairs);
        if (record.kind != character_pack::ResourceKind::Indices)
            throw std::runtime_error("Miles weld pair array kind mismatch");
        constexpr std::size_t kArrayHeaderBytes = 4, kIndexBytes = 2;
        const auto &bytes = record.payload;
        for (std::size_t offset = kArrayHeaderBytes; offset + kIndexBytes <= bytes.size();
             offset += kIndexBytes) {
            std::uint16_t value = 0;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            result.join_pairs[index].push_back(value);
        }
    }
    constexpr std::array<unsigned, 6> kTailObjectRows = {28, 31, 48, 51, 62, 63};
    constexpr std::array<unsigned, 8> kSiblingObjectRows = {32, 34, 52, 54, 60, 61, 64, 65};
    constexpr std::array<unsigned, 3> kTailModelRows = {0, 1, 14};
    const auto tail_objects = array(metadata[@"tail_objects"], 6, "Miles tail objects");
    for (std::size_t index = 0; index < result.tail_objects.size(); ++index) {
        const auto row = dictionary(tail_objects[index], "Miles tail object");
        (void)integer(row[@"index"], kTailObjectRows[index], kTailObjectRows[index],
                      "Miles tail object row");
        const auto object = static_cast<character_pack::ResourceId>(
            integer(row[@"object"], 1, pack.record_count(), "Miles tail object identity"));
        (void)pack.object(object);
        result.tail_objects[index] = object;
    }
    const auto sibling_objects = array(metadata[@"sibling_objects"], 8, "Miles sibling objects");
    for (std::size_t index = 0; index < result.sibling_objects.size(); ++index) {
        const auto row = dictionary(sibling_objects[index], "Miles sibling object");
        (void)integer(row[@"index"], kSiblingObjectRows[index], kSiblingObjectRows[index],
                      "Miles sibling object row");
        const auto object = static_cast<character_pack::ResourceId>(
            integer(row[@"object"], 1, pack.record_count(), "Miles sibling object identity"));
        (void)pack.object(object);
        result.sibling_objects[index] = object;
    }
    const auto tail_models = array(metadata[@"tail_models"], 3, "Miles tail models");
    for (std::size_t index = 0; index < result.tail_models.size(); ++index) {
        const auto row = dictionary(tail_models[index], "Miles tail model");
        (void)integer(row[@"index"], kTailModelRows[index], kTailModelRows[index],
                      "Miles tail model row");
        const auto model = static_cast<character_pack::ResourceId>(
            integer(row[@"model"], 1, pack.record_count(), "Miles tail model identity"));
        (void)pack.model(model);
        result.tail_models[index] = model;
    }
    const auto collision = array(metadata[@"collision_words"], 60, "Miles collision records");
    std::array<std::uint32_t, 60> collision_words{};
    for (std::size_t index = 0; index < collision_words.size(); ++index)
        collision_words[index] = static_cast<std::uint32_t>(
            integer(collision[index], 0, UINT32_MAX, "Miles collision word"));
    result.collision_info = gamma_collision_info(collision_words);
    return result;
}
} // namespace sadx::native::app
