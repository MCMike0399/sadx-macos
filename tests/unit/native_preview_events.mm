#include "apps/native_character_selection_assets.hpp"
#include "apps/native_gamma_actor_assets.hpp"
#include "runtime/native_character_resources.hpp"
#include "runtime/native_event_actions_context.hpp"
#include "game/game_bootstrap.hpp"
#include <cassert>
#include <iostream>

namespace {
void unavailable_auxiliary(sadx::TaskScheduler &, const sadx::TaskHandle &) {
    throw std::runtime_error("actor auxiliary execution is outside this queue witness");
}
}

int main(int argc, char **argv) {
    @autoreleasepool {
        using namespace sadx;
        assert(argc == 2);
        const std::filesystem::path directory(argv[1]);
        const auto imported = native::app::load_assets(directory);
        const auto pack = character_pack::CharacterPack::load(
            directory / "characters/sonic-selection.actions");
        const auto assets = native::app::load_character_selection_assets(directory, pack);
        const auto actor_assets = native::app::load_gamma_actor_assets(directory, pack);
        native::CharacterResources characters(imported.character_textures);
        characters.initialize();
        const auto textures = native::preview_texture_bindings(assets, characters);
        const std::array<std::pair<std::size_t, std::size_t>, 6> identities{
            {{0, 0}, {2, 0}, {3, 1}, {5, 0}, {7, 1}, {6, 0}}};
        for (std::size_t slot = 0; slot < identities.size(); ++slot) {
            const auto [character, binding] = identities[slot];
            assert(textures[slot] == &characters.lists()[character][binding]);
        }
        auto inconsistent = assets;
        inconsistent.sequences[0].character = 6;
        bool rejected = false;
        try { (void)native::preview_texture_bindings(inconsistent, characters); }
        catch (const selection_preview::Error &) { rejected = true; }
        assert(rejected);

        const auto fields = native::event_primary_fields();
        PlayerTaskState unknown{};
        fields.store_secondary_mode(unknown, 49);
        assert(unknown.secondary_mode_at_1 == 49 && !unknown.source_counter_at_c);
        rejected = false;
        try { (void)fields.counter(unknown); }
        catch (const event_actions::Error &error) {
            rejected = error.boundary == event_actions::Boundary::RegisteredCounter;
        }
        assert(rejected);

        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        assert(scheduler.initialize_source_pool(arena, {}));
        player_initialization::Registry players;
        std::int32_t card = 0;
        std::uint32_t timer = 0, movie = 0;
        std::int16_t rings = 0;
        std::uint16_t mode = 0;
        bootstrap::State bootstrap({card, timer, rings, movie, mode});
        X87Context arithmetic(127);
        player_initialization::NativeAccess access(actor_assets.parameters, bootstrap.event_work,
                                                   unavailable_auxiliary);
        selection_preview::Work preview;
        std::array<TaskWorkHandle, 6> initial_events;
        for (std::size_t slot = 0; slot < initial_events.size(); ++slot) {
            const auto task = scheduler.create_element(7, 1, nullptr).task;
            assert(task);
            player_initialization::establish_zeroed_primary(scheduler, task);
            player_initialization::establish_zeroed_motion(scheduler, task);
            auto primary = player_initialization::bind_primary_view(task);
            primary.player_index = static_cast<std::uint8_t>(slot);
            primary.character = static_cast<std::uint8_t>(assets.characters[slot]);
            player_initialization::PlayerHandle player;
            player_initialization::initialize(static_cast<std::int32_t>(slot), task,
                player_initialization::bind_existing_motion(task), scheduler, players, access,
                arithmetic, player);
            assert(player && player->stage == player_initialization::Stage::Complete);
            initial_events[slot] = event_actions::event_work(*arena, task->primary_work);
            assert(initial_events[slot] && initial_events[slot]->requested_size() == 124);
            fields.counter(*task->primary_state) = static_cast<std::int32_t>(0x20001000 + slot);
            preview.actors[slot] = task;
        }
        event_actions::State events;
        selection_preview::ActionState tails;
        native::EventActionsContext context(*arena, events, players, pack, fields);
        native::PreviewActionCalls calls(context, assets, textures);
        const std::uint8_t selected = 0;
        selection_preview::update_actions(preview, tails, selected, calls);
        assert(events.node_identities.size() == 6);
        for (std::size_t slot = 0; slot < initial_events.size(); ++slot) {
            const auto &task = preview.actors[slot];
            assert(event_actions::event_work(*arena, task->primary_work) == initial_events[slot]);
            auto *node = event_actions::action_head(*arena, task->primary_work);
            assert(node && node == tails.tails[slot] && node->textures == textures[slot]);
            const auto action = pack.resolve(assets.sequences[slot].steps[0].action);
            assert(node->action.object == action.object && node->action.motion == action.motion);
            assert(node->mode == 1 && node->link_frames == 0);
            assert(node->speed == (slot == 1 ? 0.5f : 0.25f));
            assert(task->primary_state->secondary_mode_at_1 == (slot == 1 ? 49 : 12));
            assert((task->primary_state->orientation.flags & 4096) != 0);
            const auto expected_counter = (slot == 1 ? 0x1000U : 0x20001000U) + slot;
            assert(static_cast<std::uint32_t>(fields.counter(*task->primary_state)) == expected_counter);
            event_actions::clear_action(context.core, task);
            assert(!event_actions::live(*node) && tails.tails[slot] == node);
        }
        std::cout << "Imported preview bindings and real PInitialize event allocations feed six queues; retained tails survive cleanup\n";
    }
}
