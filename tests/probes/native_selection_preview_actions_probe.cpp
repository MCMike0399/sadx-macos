#include "game/selection_preview.hpp"
#include "player/player_task.hpp"

#include <algorithm>
#include <vector>

// Reference-only fixture nodes. Production defines its canonical node separately.
namespace sadx { struct PlayerEventActionNode { PlayerEventActionNode *next = nullptr; unsigned id = 0; }; }
namespace {
namespace p = sadx::selection_preview;
struct Stop { unsigned reason; };
struct Fixture final : p::ActionCalls {
    const unsigned *in;
    sadx::TaskScheduler scheduler;
    p::State state;
    p::Work work;
    p::ActionState actions;
    std::uint8_t selected;
    std::array<sadx::TaskHandle, 7> tasks;
    std::array<std::shared_ptr<sadx::PlayerTaskState>, 7> primaries;
    std::array<std::array<sadx::PlayerEventActionNode, 4>, 6> nodes;
    std::array<sadx::regular_textures::List, 6> lists{};
    std::array<unsigned, 7> modes{}, flags{};
    std::int32_t counter;
    std::vector<unsigned> trace;
    unsigned calls = 0, failure = 0;
    explicit Fixture(const unsigned *input) : in(input), selected(input[1]), counter(input[15]) {
        state.task = input[5] ? nullptr : scheduler.append_task(0, nullptr).task;
        if (state.task && !input[4])
            state.task->auxiliary_native_work = std::make_shared<p::Work>();
        work.selected_motion = input[2];
        for (unsigned n = 0; n < 7; ++n) {
            tasks[n] = scheduler.append_task(0, nullptr).task;
            primaries[n] = std::make_shared<sadx::PlayerTaskState>();
            tasks[n]->primary_state = input[6] & (1U << n) ? nullptr : primaries[n];
            work.actors[n] = input[3] & (1U << n) ? nullptr : tasks[n];
            work.motion_counts[n] = static_cast<std::int8_t>(input[16 + n]);
            if (n < 6) {
                for (unsigned k = 0; k < 4; ++k) {
                    nodes[n][k].id = n * 8 + k + 1;
                    nodes[n][k].next = k + 1 < input[32 + n] ? &nodes[n][k + 1] : nullptr;
                }
                actions.tails[n] = nullptr;
            }
        }
    }
    unsigned slot(const sadx::TaskHandle &task) {
        for (unsigned n = 0; n < 7; ++n)
            if (task == tasks[n]) return n;
        throw Stop{9};
    }
    void event(std::initializer_list<unsigned> values) {
        trace.insert(trace.end(), values);
        if (++calls == in[13]) throw Stop{5};
    }
    std::shared_ptr<sadx::PlayerTaskState> primary(const sadx::TaskHandle &task) override {
        return task->primary_state;
    }
    void set_input(std::uint8_t player, std::int8_t mode) override {
        event({1, player, static_cast<unsigned>(mode), 0});
        if (!(in[14] && player == 1)) {
            modes[player] = static_cast<std::uint8_t>(mode);
            flags[player] |= 16;
        }
    }
    std::int32_t &miles_counter() override {
        if (in[14]) throw Stop{6};
        return counter;
    }
    sadx::ActionHandle action(std::size_t actor, std::int32_t index) override {
        if (index < 0 || index > 7) throw Stop{9};
        return {index < static_cast<int>(in[24 + actor]) ? actor * 16 + unsigned(index) + 1 : 0};
    }
    sadx::regular_textures::List &textures(std::size_t actor) override { return lists[actor]; }
    std::int32_t link_frames(std::size_t actor, std::int32_t index) override {
        return static_cast<std::int32_t>(actor * 10 + index + 3);
    }
    void set_action(const sadx::TaskHandle &task, sadx::ActionHandle action,
                    sadx::regular_textures::List &, float speed, bool loop, std::int32_t link) override {
        const auto n = slot(task);
        event({2, n, static_cast<unsigned>(action.identity), static_cast<unsigned>(link),
               speed == 0.5f ? 2U : 1U, loop ? 1U : 0U});
        if (in[9] & 1) work.selected_motion = in[10];
        if (in[9] & 2) selected = in[11];
        if (in[9] & 4) work.motion_counts[n] = in[12];
        if (in[9] & 8 && n < 6) work.actors[n + 1].reset();
        if (in[9] & 16) task->primary_state.reset();
    }
    sadx::PlayerEventActionNode *action_head(const std::shared_ptr<sadx::PlayerTaskState> &primary) override {
        for (unsigned n = 0; n < 6; ++n) {
            if (primary != primaries[n]) continue;
            if (in[8] & (1U << n)) throw Stop{3};
            if (in[7] & (1U << n)) throw Stop{4};
            return &nodes[n][0];
        }
        throw Stop{2};
    }
    sadx::PlayerEventActionNode *next_action(sadx::PlayerEventActionNode *node) override {
        return node->next;
    }
    void run(unsigned *out) {
        try {
            if (in[0] == 1) p::select_motion(state, static_cast<int>(in[48]));
            else p::update_actions(work, actions, selected, *this);
        } catch (const Stop &stop) { failure = stop.reason; }
        catch (const p::Error &error) { failure = error.boundary() == p::Boundary::Task ? 7 : 8; }
        out[0] = failure;
        out[1] = selected;
        out[2] = work.selected_motion;
        out[3] = static_cast<unsigned>(counter);
        out[4] = state.task && state.task->auxiliary_native_work ?
            std::static_pointer_cast<p::Work>(state.task->auxiliary_native_work)->selected_motion : 0;
        for (unsigned n = 0; n < 7; ++n) {
            out[8 + n] = static_cast<std::uint8_t>(work.motion_counts[n]);
            out[16 + n] = modes[n]; out[24 + n] = flags[n];
            if (n < 6) out[32 + n] = actions.tails[n] ? actions.tails[n]->id : 0;
        }
        out[40] = trace.size();
        std::copy(trace.begin(), trace.end(), out + 64);
    }
};
}
extern "C" void sadx_native_selection_preview_actions_probe(const unsigned *in, unsigned *out) {
    std::fill(out, out + 1024, 0U);
    Fixture fixture(in);
    fixture.run(out);
}
