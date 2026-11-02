#pragma once
#include "apps/native_assets.hpp"
#include "runtime/sonic_model_assets.hpp"
#include <cstring>

namespace sadx::native::app {
// NOTSA: decode only declarations consumed by the recovered model prefix.
// The existing combined pack already contains this closure; no asset is appended.
inline std::unique_ptr<SonicModelAssets>
load_sonic_model_assets(NSDictionary *actor, const character_pack::CharacterPack &pack) {
    if (string(actor[@"pack_schema"], "Sonic actor schema") != "SDXACT03")
        throw std::runtime_error("unsupported Sonic actor schema");
    SonicModelDeclarations result;
    const auto objects = array(actor[@"objects"], 3, "Sonic object declarations");
    for (std::size_t index = 0; index < 2; ++index) {
        const auto row = dictionary(objects[index], "Sonic object declaration");
        (void)integer(row[@"index"], index + 5, index + 5, "Sonic object slot");
        const auto identity = static_cast<character_pack::ResourceId>(
            integer(row[@"object"], 1, pack.record_count(), "Sonic object identity"));
        (index == 0 ? result.attach : result.root) = identity;
    }
    result.join_target = static_cast<character_pack::ResourceId>(
        integer(dictionary(objects[2], "Sonic join target")[@"object"], 1, pack.record_count(),
                "Sonic join target identity"));
    (void)integer(dictionary(objects[2], "Sonic join target")[@"index"], 22, 22,
                  "Sonic join target slot");
    const auto direct = array(actor[@"direct_ahead_objects"], 3, "Sonic direct-ahead objects");
    for (std::size_t index = 0; index < result.direct_ahead.size(); ++index)
        result.direct_ahead[index] = static_cast<character_pack::ResourceId>(
            integer(direct[index], 1, pack.record_count(), "Sonic direct-ahead object identity"));
    id pair_rows = actor[@"vertex_pairs"];
    if (![pair_rows isKindOfClass:NSArray.class])
        throw std::runtime_error("invalid Sonic vertex-pair declarations");
    for (std::size_t index = 0; index < [pair_rows count]; ++index) {
        const auto row = dictionary(pair_rows[index], "Sonic vertex-pair declaration");
        (void)integer(row[@"id"], index + 1, index + 1, "Sonic vertex-pair identity");
        id pairs = row[@"values"];
        if (![pairs isKindOfClass:NSArray.class] || [pairs count] > 255)
            throw std::runtime_error("invalid Sonic vertex-pair count");
        std::vector<std::uint16_t> values;
        for (id pair in pairs) {
            const auto words = array(pair, 2, "Sonic vertex pair");
            for (id word in words)
                values.push_back(
                    static_cast<std::uint16_t>(integer(word, 0, UINT16_MAX, "Sonic vertex index")));
        }
        result.pairs.push_back(std::move(values));
    }
    const auto joins = dictionary(actor[@"joins"], "Sonic join declarations");
    const auto decode_joins = [&](NSString *name, auto &target_rows) {
        const auto rows = array(joins[name], target_rows.size(), "Sonic join rows");
        for (std::size_t index = 0; index < target_rows.size(); ++index) {
            const auto row = dictionary(rows[index], "Sonic join row");
            auto &target = target_rows[index];
            auto object = [&](NSString *key) {
                return static_cast<character_pack::ResourceId>(
                    integer(row[key], 0, pack.record_count(), "Sonic join object"));
            };
            target.object = object(@"object");
            target.source = object(@"source_object");
            target.destination = object(@"destination_object");
            target.count = integer(row[@"vertex_count"], 0, UINT8_MAX, "Sonic join count");
            target.mode = integer(row[@"input_mode"], INT8_MIN, INT8_MAX, "Sonic join mode");
            target.source_depth = integer(row[@"source_node"], 0, UINT8_MAX, "Sonic source node");
            target.destination_depth =
                integer(row[@"destination_node"], 0, UINT8_MAX, "Sonic destination node");
            (void)integer(row[@"original_vertices"], 0, 0, "Sonic original allocation");
            target.pairs = integer(row[@"vertex_pairs"], 0, result.pairs.size(), "Sonic pair row");
        }
    };
    decode_joins(@"sonic", result.joins);
    decode_joins(@"metal", result.metal_joins);
    const auto words = array(actor[@"collision_words"], 24, "Sonic collision records");
    for (std::size_t index = 0; index < 2; ++index) {
        std::array<std::uint32_t, 12> record{};
        for (std::size_t column = 0; column < record.size(); ++column)
            record[column] = static_cast<std::uint32_t>(
                integer(words[index * 12 + column], 0, UINT32_MAX, "Sonic collision word"));
        auto scalar = [&](std::size_t column) {
            float value;
            std::memcpy(&value, &record[column], sizeof(value));
            return value;
        };
        collision_component::Info info{};
        info.kind = static_cast<std::uint8_t>(record[0]);
        info.form =
            static_cast<collision_component::Form>(static_cast<std::uint8_t>(record[0] >> 8));
        info.push = static_cast<std::uint8_t>(record[0] >> 16);
        info.damage = static_cast<std::uint8_t>(record[0] >> 24);
        info.attributes = record[1];
        info.center = {scalar(2), scalar(3), scalar(4)};
        info.a = scalar(5);
        info.b = scalar(6);
        info.c = scalar(7);
        info.d = scalar(8);
        for (std::size_t axis = 0; axis < info.angles.size(); ++axis)
            std::memcpy(&info.angles[axis], &record[9 + axis], sizeof(info.angles[axis]));
        result.collision.push_back(info);
    }
    return std::make_unique<SonicModelAssets>(pack, std::move(result));
}
} // namespace sadx::native::app
