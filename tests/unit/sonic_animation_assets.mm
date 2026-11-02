#include "apps/native_sonic_animation_assets.hpp"
#include "apps/native_gamma_actor_assets.hpp"
#include "player/animation/sonic_animation_content.hpp"
#include "runtime/native_sonic_context.hpp"
#include <fstream>
#include <iostream>

namespace {
using namespace sadx;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
unsigned bits(float value) {
    unsigned result;
    std::memcpy(&result, &value, 4);
    return result;
}
void auxiliary(TaskScheduler &, const TaskHandle &) {
    throw std::runtime_error("stored auxiliary callback must remain unexecuted");
}
std::uint32_t word(const std::vector<std::uint8_t> &bytes, std::size_t at) {
    return std::uint32_t(bytes.at(at)) | (std::uint32_t(bytes.at(at + 1)) << 8) |
           (std::uint32_t(bytes.at(at + 2)) << 16) | (std::uint32_t(bytes.at(at + 3)) << 24);
}
void loader_negatives(const std::filesystem::path &directory,
                      const character_pack::CharacterPack &pack) {
    const auto output = std::filesystem::path("build/sonic-animation-native/invalid/characters");
    std::filesystem::create_directories(output);
    std::filesystem::copy_file(directory / "characters/sonic-actor.json", output / "sonic-actor.json",
                               std::filesystem::copy_options::overwrite_existing);
    const auto original = native::app::read_metadata(directory / "characters/sonic-animation.json");
    for (unsigned fault = 0; fault < 12; ++fault) {
        // JSON round trip gives independent mutable containers for each negative.
        NSData *data = [NSJSONSerialization dataWithJSONObject:original options:0 error:nil];
        NSMutableDictionary *metadata = [NSJSONSerialization JSONObjectWithData:data
            options:NSJSONReadingMutableContainers error:nil];
        NSMutableArray *rows = metadata[@"declarations"];
        NSMutableDictionary *stand = rows[1];
        switch (fault) {
        case 0: [metadata removeObjectForKey:@"declarations"]; break;
        case 1: [rows removeLastObject]; break;
        case 2: stand[@"index"] = @2; break;
        case 3: stand[@"status"] = @"unknown"; break;
        case 4: stand[@"action"] = NSNull.null; break;
        case 5: stand[@"action"] = @1.5; break;
        case 6: stand[@"resource_boundary"] = @{}; break;
        case 7: stand[@"declaration_identity"] = @0; break;
        case 8: metadata[@"initial_animation_action"] = @1; break;
        case 9: rows[0][@"resource_boundary"] = NSNull.null; break;
        case 10: stand[@"status"] = @"known_null"; break;
        case 11: [stand removeObjectForKey:@"action"]; break;
        }
        data = [NSJSONSerialization dataWithJSONObject:metadata options:0 error:nil];
        require([data writeToFile:[NSString stringWithUTF8String:
                    (output / "sonic-animation.json").c_str()] atomically:YES], "write negative metadata");
        bool rejected = false;
        try { (void)native::app::load_sonic_animation_assets(output.parent_path().parent_path(), pack); }
        catch (const std::exception &) { rejected = true; }
        require(rejected, "malformed/missing/null declaration rejected");
    }
}
std::array<player_initialization::PlayerHandle, 2> composition(const std::filesystem::path &directory,
                 const character_pack::CharacterPack &pack,
                 const native::SonicAnimationAssets &assets) {
    native::PlayerSystem players;
    TaskScheduler scheduler;
    auto arena = std::make_shared<TaskWorkArena>();
    require(bool(scheduler.initialize_source_pool(arena, std::shared_ptr<void>{})), "source pool");
    X87Context fp(127);
    std::int16_t stage = 0;
    std::uint8_t alternate = 0;
    PlayerEventFlags events{};
    // Existing importer owns the common parameter table. Only that input is borrowed;
    // Gamma's animation table is never supplied to Sonic.
    players.gamma_assets = std::make_unique<native::GammaActorAssets>(
        native::app::load_gamma_actor_assets(directory, pack));
    std::array<player_initialization::PlayerHandle, 2> records;
    for (unsigned index = 0; index < records.size(); ++index) {
        auto task = scheduler.create_element(7, 3, nullptr).task;
        player_initialization::establish_zeroed_primary(scheduler, task);
        player_initialization::establish_zeroed_motion(scheduler, task);
        auto pose = player_initialization::bind_primary_view(task);
        pose.player_index = index;
        sonic::Context context{players, scheduler, task, fp};
        context.stage = &stage;
        context.alternate_model = &alternate;
        context.parameters = &players.gamma_assets->parameters;
        context.events = &events;
        context.auxiliary = auxiliary;
        context.animation_table = &assets.table();
        bool editor_stop = false;
        try { sonic::run(context); }
        catch (const sonic::Unavailable &error) {
            editor_stop = std::string(error.what()) == "sonic_check_edit_mode_editor";
        }
        require(editor_stop && !players.registry.players[index], "real editor boundary stays missing");
        // Controlled witness inputs only; production editor composition remains Root-owned.
        context.read_editor = [] { return std::int16_t{0}; };
        context.read_forced = [] { return std::int16_t{0}; };
        bool model_stop = false;
        try { sonic::run(context); }
        catch (const sonic::Unavailable &error) {
            model_stop = std::string(error.what()) == "sonic_model_graph";
        }
        records[index] = players.registry.players[index];
        require(model_stop && records[index] &&
                    records[index]->stage == player_initialization::Stage::Complete,
                "real Sonic initializer reaches existing model boundary");
        require(records[index]->physics->animation_tail.table_at_140 == assets.table(),
                "actual Sonic bind consumes canonical imported table");
    }
    require(records[0] != records[1] && records[0]->physics != records[1]->physics,
            "two distinct canonical players");
    animation::Runtime runtime{arena.get(), assets.resources()};
    auto &player = *records[0]->physics;
    animation::View view{player, records[0]->allocation};
    player.current_animation_at_126 = 0;
    player.requested_animation_at_124 = 0;
    player.animation_mode_at_120 = 3;
    player.animation_tail.frame_at_130 = 2.25f;
    const auto result = animation::update(view, runtime, fp);
    require(bool(result) && player.animation_tail.frame_at_130 == 2.75f,
            "real imported Stand advances exact half frame");
    require(records[1]->physics->animation_tail.frame_at_130 == 0,
            "other player's animation state remains isolated");
    player.current_animation_at_126 = 1;
    player.requested_animation_at_124 = 1;
    const auto missing = animation::update(view, runtime, fp);
    require(missing.failure == animation::Failure::UnresolvedResource &&
                !missing.unavailable_resource_reason.empty(), "unavailable payload fails at consumption");
    return records;
}
}
int main(int argc, char **argv) {
    @autoreleasepool {
        try {
            require(argc == 3, "requires imported directory and observation output");
            const std::filesystem::path directory(argv[1]);
            const auto pack = character_pack::CharacterPack::load(
                directory / "characters/sonic-selection.actions");
            auto assets = native::app::load_sonic_animation_assets(directory, pack);
            auto table = std::dynamic_pointer_cast<const animation::Table>(assets->table());
            require(table && table->records.size() == 147, "147 rows, independent of payload availability");
            unsigned loaded = 0, nulls = 0, unavailable = 0;
            for (const auto &declaration : assets->declarations()) {
                loaded += declaration.action.status == animation::ResourceStatus::Loaded;
                nulls += declaration.action.status == animation::ResourceStatus::KnownNull;
                unavailable += declaration.action.status == animation::ResourceStatus::Unavailable;
            }
            require(loaded == 4 && nulls == 3 && unavailable == 142, "bounded original declarations");
            std::ofstream output(argv[2]);
            for (unsigned index = 0; index < 147; ++index) {
                const auto &row = table->records.at(index);
                const auto &recipe = animation::kSonicRecipes[index];
                const auto action = recipe.action_export
                    ? assets->declarations()[recipe.action_index].action : animation::ActionResource{};
                require(row.action.status == action.status && row.action.identity == action.identity &&
                            row.action.unavailable_reason == action.unavailable_reason,
                        "canonical row/export mapping including aliases and nulls");
                output << index << '\t' << int(row.action.status) << '\t' << row.action.identity << '\t'
                       << unsigned(row.object_count) << '\t' << int(row.mode) << '\t' << row.next << '\t'
                       << bits(row.transition_step) << '\t' << bits(row.frame_step) << '\t'
                       << row.action.unavailable_reason << '\n';
            }
            require(assets->preview()[0].identity == assets->declarations()[5].action.identity &&
                        assets->preview()[1].identity == assets->declarations()[100].action.identity,
                    "actual preview aliases preserved");
            for (const auto action : assets->preview()) {
                const auto &bound = assets->resources().actions.at(action.identity);
                const auto source = pack.resolve(action);
                require(bound.object.id == source.object && bound.motion.id == source.motion,
                        "preview payload uses unchanged pack identity");
                const auto motion = pack.motion(source.motion);
                const auto &bound_motion = assets->resources().motions.at(source.motion);
                require(bound_motion.frame_count == motion.frame_count &&
                            bound_motion.type == motion.channels &&
                            bound_motion.input_function == motion.interpolation &&
                            bound_motion.data.id == motion.tracks,
                        "exact motion identity/metadata");
            }
            for (unsigned fault = 0; fault < 7; ++fault) {
                auto declarations = assets->declarations();
                switch (fault) {
                case 0: declarations[1].action.identity = pack.record_count() + 1; break;
                case 1: declarations[1].action.identity = 0; break;
                case 2: declarations[1].identity = 0; break;
                case 3: declarations[0].action.unavailable_reason.clear(); break;
                case 4: declarations[1].action.identity = 2; break; // wrong resource kind
                case 5: declarations[1].action.status = animation::ResourceStatus::KnownNull; break;
                case 6: declarations = {}; break; // unfilled declarations are unavailable, never guessed nulls
                }
                bool rejected = false;
                try { native::SonicAnimationAssets invalid(pack, std::move(declarations)); }
                catch (const std::exception &) { rejected = true; }
                require(rejected, "invalid declaration must fail before publication");
            }
            for (const auto &entry : assets->resources().motions) {
                const auto motion = pack.motion(entry.first);
                const auto &tracks = pack.record(motion.tracks).payload;
                const auto &nodes = assets->resources().node_arrays.at(motion.tracks);
                require(nodes.size() == motion.part_count, "full reached node extent");
                for (std::size_t index = 0; index < nodes.size(); ++index) {
                    const auto &node = nodes[index];
                    require(!node.keys[2].id && !node.counts[2] && !node.raw_scale_pointer_bits,
                            "MDATA2 has no scale declaration");
                    for (unsigned channel = 0; channel < 2; ++channel) {
                        const auto key = word(tracks, 4 + index * 16 + channel * 4);
                        require(node.keys[channel].id.value_or(0) == key &&
                                    node.counts[channel] == word(tracks, 12 + index * 16 + channel * 4),
                                "node declarations exact");
                        if (!key) continue;
                        const auto &backing = assets->resources().key_arrays.at(key);
                        require(*backing.bytes == pack.record(key).payload && backing.offset == 4 &&
                                    backing.byte_extent + 4 == backing.bytes->size(),
                                "unchanged position/rotation keys and extent");
                    }
                }
            }
            loader_negatives(directory, pack);
            const auto borrowers = composition(directory, pack, *assets);
            const auto keys = assets->resources().key_arrays.begin()->second;
            std::weak_ptr<const animation::TableStorage> lifetime = table;
            assets.reset();
            table.reset();
            const auto retained = std::dynamic_pointer_cast<const animation::Table>(
                *borrowers[0]->physics->animation_tail.table_at_140);
            require(!lifetime.expired() && retained ==
                        *borrowers[1]->physics->animation_tail.table_at_140 &&
                        retained->records.at(0).action.status == animation::ResourceStatus::Loaded &&
                        keys.bytes && keys.byte_extent > 0,
                    "table and retained key backing outlive asset wrapper");
            std::cerr << "149 declarations, 147 rows, imported preview closure, seven invalid declarations, "
                         "two real Sonic binds and Stand playback pass\n";
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
