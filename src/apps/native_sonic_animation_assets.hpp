#pragma once
#include "apps/native_assets.hpp"
#include "runtime/sonic_animation_assets.hpp"
#include "apps/native_sonic_model_assets.hpp"

namespace sadx::native::app {
// NOTSA: complete declarations, bounded payloads. The canonical initializer supplies all
// scalar recipes and source publication order; no second recipe/table owner is decoded here.
inline std::unique_ptr<SonicAnimationAssets>
load_sonic_animation_assets(const std::filesystem::path &directory,
                            const character_pack::CharacterPack &pack) {
    const auto metadata = read_metadata(directory / "characters/sonic-animation.json");
    if (string(metadata[@"pack_schema"], "Sonic animation schema") != "SDXACT03")
        throw std::runtime_error("unsupported Sonic animation pack schema");
    (void)integer(metadata[@"initial_animation_export_index"], 1, 1, "Sonic Stand export");
    const auto rows = array(metadata[@"declarations"], 149, "Sonic action declarations");
    SonicAnimationAssets::Declarations declarations{};
    for (std::size_t index = 0; index < declarations.size(); ++index) {
        const auto row = dictionary(rows[index], "Sonic action declaration");
        (void)integer(row[@"index"], index, index, "Sonic export ordinal");
        auto &declaration = declarations[index];
        declaration.identity = static_cast<std::uint16_t>(
            integer(row[@"declaration_identity"], 0, index + 1, "Sonic declaration identity"));
        auto &action = declaration.action;
        const auto status = string(row[@"status"], "Sonic action status");
        if (status == "loaded") {
            action.status = animation::ResourceStatus::Loaded;
            action.identity =
                integer(row[@"action"], 1, pack.record_count(), "Sonic action resource");
            if (row[@"resource_boundary"] != NSNull.null)
                throw std::runtime_error("loaded Sonic action has boundary");
        } else {
            if (row[@"action"] != NSNull.null)
                throw std::runtime_error("unloaded Sonic action has payload");
            if (status == "known_null") {
                action.status = animation::ResourceStatus::KnownNull;
                if (row[@"resource_boundary"] != NSNull.null)
                    throw std::runtime_error("null Sonic action has boundary");
            } else if (status == "unavailable") {
                action.status = animation::ResourceStatus::Unavailable;
                const auto boundary =
                    dictionary(row[@"resource_boundary"], "Sonic action boundary");
                for (NSString *key in @[@"component", @"source_address", @"reason"]) {
                    const auto value = string(boundary[key], "Sonic missing resource");
                    if (value.empty())
                        throw std::runtime_error("empty Sonic resource boundary");
                    action.unavailable_reason += value + ":";
                }
            } else {
                throw std::runtime_error("unknown Sonic action declaration status");
            }
        }
    }
    const auto initial = integer(metadata[@"initial_animation_action"], 1, pack.record_count(),
                                 "Sonic Stand identity");
    const auto actor = read_metadata(directory / "characters/sonic-actor.json");
    if (declarations[1].action.status != animation::ResourceStatus::Loaded ||
        declarations[1].action.identity != static_cast<std::size_t>(initial) ||
        integer(actor[@"initial_animation_export_index"], 1, 1, "Sonic actor Stand export") != 1 ||
        integer(actor[@"initial_animation_action"], initial, initial,
                "Sonic actor Stand identity") != initial)
        throw std::runtime_error("Sonic animation Stand differs from actor import");
    auto result = std::make_unique<SonicAnimationAssets>(pack, std::move(declarations));
    result->install_model(load_sonic_model_assets(actor, pack));
    return result;
}
} // namespace sadx::native::app
