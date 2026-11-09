#include "engine/collision_component.hpp"
#include "engine/game_frame_control.hpp"
#include "player/player_initialization.hpp"
#include "runtime/native_display.hpp"

#include <cstdint>
#include <vector>

extern "C" int sadx_native_collision_entry(const std::uint32_t *input, std::uint32_t *output) {
    using namespace sadx;
    try {
        auto arena = std::make_shared<TaskWorkArena>();
        TaskScheduler scheduler;
        if (!scheduler.initialize_source_pool(arena, {}))
            return 2;
        const auto created = scheduler.create_element(7, 3, nullptr);
        if (!created)
            return 2;
        const auto task = created.task;
        player_initialization::establish_zeroed_primary(scheduler, task);
        frame::ModeState timing;
        native::Display display{{}, timing};
        const std::vector<collision_component::Info> infos(1);
        auto &status = player_initialization::bind_primary_view(task).status;
        if (collision_component::initialize(*arena, task, status, infos, 1, 0, display.arithmetic()) !=
            collision_component::InitResult::Initialized)
            return 3;
        auto work = collision_component::component(*arena, task->primary_work);
        if (!work)
            return 3;
        work->id = static_cast<std::uint16_t>(input[0]);
        work->info_count = 0;
        work->hit_count = static_cast<std::int16_t>(input[6]);
        status = static_cast<std::uint16_t>(input[3]);
        if (input[2])
            task->exec = &TaskScheduler::destroy_callback;
        collision_component::EntryState state;
        state.special_stage = 0x11111111u;
        if (input[0] < 10)
            state.lists[input[0]].count = static_cast<std::uint16_t>(input[4]);
        const auto argument = input[5] ? work : std::shared_ptr<collision_component::Work>{};
        const auto result = collision_component::enter_list(state, argument, status,
                                                            static_cast<std::int16_t>(input[1]));
        const auto index = input[0] < 10 ? input[0] : 0;
        output[0] = static_cast<std::uint32_t>(result.route);
        output[1] = state.special_stage;
        output[2] = input[0] < 10 ? state.lists[index].count : 0;
        output[3] = status;
        output[4] = static_cast<std::uint16_t>(work->hit_count);
        output[5] = input[0] < 10 && !state.lists[index].slots.empty() && state.lists[index].slots.back().work
                        ? 1u
                        : 0u;
        return 0;
    } catch (...) {
        return 1;
    }
}
