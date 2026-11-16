#pragma once
#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/amy_actor_assets.hpp"

namespace sadx::native::app {
namespace {
// NOTSA: decode one imported Amy weld declaration without expanding buffers.
inline AmyJoinDeclaration amy_join(NSDictionary *row, const character_pack::CharacterPack &pack) {
    AmyJoinDeclaration result{};
    const auto identity = [&pack](id value, const char *label) {
        const auto id =
            static_cast<character_pack::ResourceId>(integer(value, 0, pack.record_count(), label));
        if (id)
            (void)pack.object(id);
        return id;
    };
    result.object = identity(row[@"object"], "Amy weld object");
    result.source_object = identity(row[@"source_object"], "Amy weld source object");
    result.destination_object = identity(row[@"destination_object"], "Amy weld destination object");
    result.vertex_count =
        static_cast<std::uint8_t>(integer(row[@"vertex_count"], 0, 255, "Amy weld vertices"));
    result.input_mode =
        static_cast<std::int8_t>(integer(row[@"input_mode"], -128, 127, "Amy weld mode"));
    result.source_depth =
        static_cast<std::uint8_t>(integer(row[@"source_node"], 0, 255, "Amy weld source depth"));
    result.destination_depth = static_cast<std::uint8_t>(
        integer(row[@"destination_node"], 0, 255, "Amy weld destination depth"));
    result.vertex_pairs = static_cast<character_pack::ResourceId>(
        integer(row[@"vertex_pairs"], 0, pack.record_count(), "Amy weld pairs"));
    return result;
}
} // namespace

// NOTSA: complete source recipe metadata binds to the SAME validated CharacterPack.
inline AmyActorAssets load_amy_actor_assets(const std::filesystem::path &directory,
                                            const character_pack::CharacterPack &pack) {
    const auto metadata = read_metadata(directory / "characters/amy-actor.json");
    if (string(metadata[@"pack_schema"], "Amy pack schema") != "SDXACT03")
        throw std::runtime_error("invalid Amy pack schema");
    (void)integer(metadata[@"character"], 5, 5, "Amy character");
    (void)integer(metadata[@"texture_character"], 5, 5, "Amy texture character");
    const auto stand_declaration =
        integer(metadata[@"initial_animation_export_index"], 0, 79, "Amy Stand export");
    AmyActorAssets result{};
    const auto declarations = array(metadata[@"declarations"], 80, "Amy declarations");
    for (std::size_t index = 0; index < result.declarations.size(); ++index) {
        const auto row = dictionary(declarations[index], "Amy declaration");
        (void)integer(row[@"index"], index, index, "Amy declaration index");
        result.declarations[index] = gamma_declaration(row, pack);
    }
    auto table = std::make_shared<animation::Table>();
    const auto recipes = array(metadata[@"recipes"], 102, "Amy recipes");
    for (std::size_t index = 0; index < result.recipes.size(); ++index) {
        const auto row = dictionary(recipes[index], "Amy recipe");
        (void)integer(row[@"row"], index, index, "Amy recipe row");
        auto &recipe = result.recipes[index];
        recipe.resource = gamma_declaration(row, pack);
        recipe.object_count =
            static_cast<std::uint8_t>(integer(row[@"object_count"], 0, 255, "Amy object count"));
        recipe.mode = static_cast<std::int8_t>(integer(row[@"mode"], -128, 127, "Amy motion mode"));
        recipe.next = static_cast<std::uint16_t>(integer(row[@"next"], 0, 101, "Amy next motion"));
        recipe.transition = actor_scalar(actor_word(row[@"transition_bits"]));
        recipe.speed = actor_scalar(actor_word(row[@"speed_bits"]));
        if (index == 76) {
            if (string(row[@"declaration"], "constructed Amy action") != "constructed")
                throw std::runtime_error("Amy constructed recipe identity changed");
        } else if (recipe.resource.status == ActorResourceStatus::KnownNull) {
            if (string(row[@"declaration"], "known-null Amy declaration") != "known_null")
                throw std::runtime_error("Amy known-null recipe identity changed");
        } else {
            const auto declaration = integer(row[@"declaration"], 0, 79, "Amy recipe declaration");
            const auto &resource = result.declarations[declaration];
            if (recipe.resource.status != resource.status ||
                recipe.resource.action.identity != resource.action.identity ||
                recipe.resource.boundary != resource.boundary)
                throw std::runtime_error("Amy recipe declaration mismatch");
            if (index == 0 && declaration != stand_declaration)
                throw std::runtime_error("Amy request zero does not use the exported Stand");
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
        integer(metadata[@"initial_animation_action"], 1, pack.record_count(), "Amy Stand action");
    if (result.stand.identity != result.declarations[stand_declaration].action.identity)
        throw std::runtime_error("Amy Stand declaration mismatch");
    if (result.recipes[0].resource.action.identity != result.stand.identity)
        throw std::runtime_error("Amy request zero does not use Stand");
    // Derive the reached motion identity from the same pack the actor binds.
    const auto stand = pack.resolve(result.stand);
    const auto motion = pack.motion(stand.motion);
    if (motion.part_count != result.recipes[0].object_count)
        throw std::runtime_error("Amy Stand motion part count differs from request zero");
    result.animation_table = std::move(table);
    const auto joins = array(metadata[@"joins"], 18, "Amy weld declarations");
    for (std::size_t index = 0; index < result.joins.size(); ++index) {
        result.joins[index] = amy_join(dictionary(joins[index], "Amy weld declaration"), pack);
        const auto pairs = result.joins[index].vertex_pairs;
        if (!pairs)
            continue;
        const auto &record = pack.record(pairs);
        if (record.kind != character_pack::ResourceKind::Indices)
            throw std::runtime_error("Amy weld pair array kind mismatch");
        constexpr std::size_t kArrayHeaderBytes = 4, kIndexBytes = 2;
        const auto &bytes = record.payload;
        for (std::size_t offset = kArrayHeaderBytes; offset + kIndexBytes <= bytes.size();
             offset += kIndexBytes) {
            std::uint16_t value = 0;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            result.join_pairs[index].push_back(value);
        }
    }
    const auto tail_objects = array(metadata[@"tail_objects"], 3, "Amy tail objects");
    for (std::size_t index = 0; index < result.tail_objects.size(); ++index) {
        const auto row = dictionary(tail_objects[index], "Amy tail object");
        (void)integer(row[@"index"], 36 + index, 36 + index, "Amy tail object row");
        const auto object = static_cast<character_pack::ResourceId>(
            integer(row[@"object"], 1, pack.record_count(), "Amy tail object identity"));
        (void)pack.object(object);
        result.tail_objects[index] = object;
    }
    const auto collision = array(metadata[@"collision_words"], 36, "Amy collision records");
    std::array<std::uint32_t, 36> collision_words{};
    for (std::size_t index = 0; index < collision_words.size(); ++index)
        collision_words[index] = static_cast<std::uint32_t>(
            integer(collision[index], 0, UINT32_MAX, "Amy collision word"));
    result.collision_info = gamma_collision_info(collision_words);
    return result;
}
} // namespace sadx::native::app
