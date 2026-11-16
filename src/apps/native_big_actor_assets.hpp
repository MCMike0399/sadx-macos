#pragma once
#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/big_actor_assets.hpp"

namespace sadx::native::app {
namespace {
// NOTSA: decode one imported Big weld declaration without expanding buffers.
inline BigJoinDeclaration big_join(NSDictionary *row, const character_pack::CharacterPack &pack) {
    BigJoinDeclaration result{};
    const auto identity = [&pack](id value, const char *label) {
        const auto id = static_cast<character_pack::ResourceId>(integer(value, 0, pack.record_count(), label));
        if (id)
            (void)pack.object(id);
        return id;
    };
    result.object = identity(row[@"object"], "Big weld object");
    result.source_object = identity(row[@"source_object"], "Big weld source object");
    result.destination_object = identity(row[@"destination_object"], "Big weld destination object");
    result.vertex_count = static_cast<std::uint8_t>(integer(row[@"vertex_count"], 0, 255, "Big weld vertices"));
    result.input_mode = static_cast<std::int8_t>(integer(row[@"input_mode"], -128, 127, "Big weld mode"));
    result.source_depth = static_cast<std::uint8_t>(integer(row[@"source_node"], 0, 255, "Big weld source depth"));
    result.destination_depth =
        static_cast<std::uint8_t>(integer(row[@"destination_node"], 0, 255, "Big weld destination depth"));
    result.vertex_pairs = static_cast<character_pack::ResourceId>(
        integer(row[@"vertex_pairs"], 0, pack.record_count(), "Big weld pairs"));
    return result;
}
} // namespace

// NOTSA: complete source recipe metadata binds to the SAME validated CharacterPack.
inline BigActorAssets load_big_actor_assets(const std::filesystem::path &directory,
                                            const character_pack::CharacterPack &pack) {
    const auto metadata = read_metadata(directory / "characters/big-actor.json");
    if (string(metadata[@"pack_schema"], "Big pack schema") != "SDXACT03")
        throw std::runtime_error("invalid Big pack schema");
    (void)integer(metadata[@"character"], 7, 7, "Big character");
    (void)integer(metadata[@"texture_character"], 7, 7, "Big texture character");
    (void)integer(metadata[@"initial_animation_export_index"], 23, 23, "Big Stand export");
    BigActorAssets result{};
    const auto declarations = array(metadata[@"declarations"], 90, "Big declarations");
    for (std::size_t index = 0; index < result.declarations.size(); ++index) {
        const auto row = dictionary(declarations[index], "Big declaration");
        (void)integer(row[@"index"], index, index, "Big declaration index");
        result.declarations[index] = gamma_declaration(row, pack);
    }
    result.stand.identity = integer(metadata[@"initial_animation_action"], 1,
                                    pack.record_count(), "Big Stand action");
    if (result.stand.identity != result.declarations[23].action.identity)
        throw std::runtime_error("Big Stand declaration mismatch");
    const auto stand = pack.resolve(result.stand);
    const auto motion = pack.motion(stand.motion);
    if (motion.part_count != 73 || motion.frame_count != 32 || motion.channels != 3 ||
        motion.interpolation != 2)
        throw std::runtime_error("Big Stand motion metadata mismatch");
    auto table = std::make_shared<animation::Table>();
    const auto recipes = array(metadata[@"recipes"], 105, "Big recipes");
    for (std::size_t index = 0; index < result.recipes.size(); ++index) {
        const auto row = dictionary(recipes[index], "Big recipe");
        (void)integer(row[@"row"], index, index, "Big recipe row");
        auto &recipe = result.recipes[index];
        recipe.resource = gamma_declaration(row, pack);
        recipe.object_count = static_cast<std::uint8_t>(integer(row[@"object_count"], 0, 255,
                                                                 "Big object count"));
        recipe.mode = static_cast<std::int8_t>(integer(row[@"mode"], -128, 127, "Big motion mode"));
        recipe.next = static_cast<std::uint16_t>(integer(row[@"next"], 0, 104, "Big next motion"));
        recipe.transition = actor_scalar(actor_word(row[@"transition_bits"]));
        recipe.speed = actor_scalar(actor_word(row[@"speed_bits"]));
        if (index == 96) {
            if (string(row[@"declaration"], "constructed Big action") != "constructed")
                throw std::runtime_error("Big constructed recipe identity changed");
        } else if (recipe.resource.status == ActorResourceStatus::KnownNull) {
            if (string(row[@"declaration"], "known-null Big declaration") != "known_null")
                throw std::runtime_error("Big known-null recipe identity changed");
        } else {
            const auto declaration = integer(row[@"declaration"], 0, 89, "Big recipe declaration");
            const auto &resource = result.declarations[declaration];
            if (recipe.resource.status != resource.status ||
                recipe.resource.action.identity != resource.action.identity ||
                recipe.resource.boundary != resource.boundary)
                throw std::runtime_error("Big recipe declaration mismatch");
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
    if (result.recipes[0].resource.action.identity != result.stand.identity)
        throw std::runtime_error("Big request zero does not use Stand");
    result.animation_table = std::move(table);
    const auto joins = array(metadata[@"joins"], 17, "Big weld declarations");
    for (std::size_t index = 0; index < result.joins.size(); ++index) {
        result.joins[index] = big_join(dictionary(joins[index], "Big weld declaration"), pack);
        const auto pairs = result.joins[index].vertex_pairs;
        if (!pairs)
            continue;
        const auto &record = pack.record(pairs);
        if (record.kind != character_pack::ResourceKind::Indices)
            throw std::runtime_error("Big weld pair array kind mismatch");
        constexpr std::size_t kArrayHeaderBytes = 4, kIndexBytes = 2;
        const auto &bytes = record.payload;
        for (std::size_t offset = kArrayHeaderBytes; offset + kIndexBytes <= bytes.size();
             offset += kIndexBytes) {
            std::uint16_t value = 0;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            result.join_pairs[index].push_back(value);
        }
    }
    const auto equipment_objects = array(metadata[@"equipment_objects"], 6, "Big equipment objects");
    for (std::size_t index = 0; index < result.equipment_objects.size(); ++index) {
        const auto row = dictionary(equipment_objects[index], "Big equipment object");
        (void)integer(row[@"index"], 31 + index, 31 + index, "Big equipment object row");
        const auto object = static_cast<character_pack::ResourceId>(
            integer(row[@"object"], 1, pack.record_count(), "Big equipment object identity"));
        (void)pack.object(object);
        result.equipment_objects[index] = object;
    }
    const auto equipment_models = array(metadata[@"equipment_models"], 2, "Big equipment models");
    for (std::size_t index = 0; index < result.equipment_models.size(); ++index) {
        const auto row = dictionary(equipment_models[index], "Big equipment model");
        (void)integer(row[@"index"], 2 + index, 2 + index, "Big equipment model row");
        const auto model = static_cast<character_pack::ResourceId>(
            integer(row[@"model"], 1, pack.record_count(), "Big equipment model identity"));
        (void)pack.model(model);
        result.equipment_models[index] = model;
    }
    const auto collision = array(metadata[@"collision_words"], 36, "Big collision records");
    std::array<std::uint32_t, 36> collision_words{};
    for (std::size_t index = 0; index < collision_words.size(); ++index)
        collision_words[index] = static_cast<std::uint32_t>(
            integer(collision[index], 0, UINT32_MAX, "Big collision word"));
    result.collision_info = gamma_collision_info(collision_words);
    return result;
}
} // namespace sadx::native::app
