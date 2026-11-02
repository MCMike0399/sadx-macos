#include "reset_final_parent.hpp"
#include "player/sonic_player.hpp"
#if __has_include("guest/reset_control_reference.hpp")
#include "guest/reset_control_reference.hpp"
#define RESET_CURRENT_BINDING 1
#endif
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
std::uint16_t editor_input(const char *text) {
    unsigned value = 0;
    const std::string input(text);
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() || value > 65535)
        throw std::invalid_argument("expected uint16 editor word");
    return static_cast<std::uint16_t>(value);
}

struct Fixture {
    sadx::TaskScheduler scheduler;
    std::shared_ptr<sadx::TaskWorkArena> arena = std::make_shared<sadx::TaskWorkArena>();
    sadx::TaskHandle object = scheduler.append_task(0, sadx::TaskScheduler::source_no_op).task;
    std::shared_ptr<sadx::PlayerTaskState> task = std::make_shared<sadx::PlayerTaskState>();
    std::shared_ptr<sadx::PlayerMotionState> world = std::make_shared<sadx::PlayerMotionState>();
    std::shared_ptr<sadx::PlayerStartupPlayer> player =
        std::make_shared<sadx::PlayerStartupPlayer>();
    sadx::PlayerPositionCollisionScene collision;
    sadx::SonicPlayerGlobals globals;
    sadx::PlayerStartupRegistry players;
    Fixture() {
        object->primary_work = arena->allocate(64).work;
        object->primary_state = task;
        player->player_work = arena->allocate(512).work;
        player->player = std::make_shared<sadx::PlayerPhysicsState>();
        player->parameters = sadx::PlayerParameters{};
        task->action_at_0 = 86;
        task->task_id_at_2 = 2;
        task->control_timer_at_6 = 0;
        task->orientation.flags = 0;
        collision.task_collision = sadx::PositionResourceRef{};
        globals.stage_at_3b22dcc = 0;
        player->player->source_flags_at_6 = 0;
        player->player->equipment_at_4 = 0;
        player->player->timer_at_7e = 0;
        player->player->prior_attributes = 0;
    }
};

void observation(const sadx::SonicPlayerResult &result, const Fixture &fixture,
                 sadx::X87Context &fp, unsigned external_calls) {
    const auto details = unsigned(bool(result.acceleration)) | unsigned(bool(result.air)) << 1 |
        unsigned(bool(result.projection)) << 2 | unsigned(bool(result.movement)) << 3 |
        unsigned(bool(result.position)) << 4 | unsigned(bool(result.coordinate)) << 5 |
        unsigned(bool(result.floor_angles)) << 6 | unsigned(bool(result.input)) << 7 |
        unsigned(bool(result.startup)) << 8 | unsigned(bool(result.work_failure)) << 9 |
        unsigned(bool(result.hold)) << 10 | unsigned(bool(result.normal)) << 11 |
        unsigned(bool(result.animation)) << 12 | unsigned(bool(result.task_creation)) << 13 |
        unsigned(bool(result.task_work_return)) << 14;
    if (details != 0)
        throw std::runtime_error("unexpected nested Sonic result before editor stop");
    std::cout << '[' << unsigned(result.failure) << ',' << result.source_boundary << ','
              << result.callee_address << ',' << result.nested_source_boundary << ','
              << result.completed << ',' << bool(result.value) << ',' << result.value.value_or(0)
              << ',' << bool(result.dispatched_action) << ','
              << unsigned(result.dispatched_action.value_or(0)) << ','
              << bool(result.dispatch_target) << ',' << result.dispatch_target.value_or(0) << ','
              << details << ',' << bool(fixture.task->action_at_0) << ','
              << unsigned(fixture.task->action_at_0.value_or(0)) << ','
              << fixture.task->orientation.flags << ','
              << bool(fixture.globals.current_task_at_3abdf60) << ','
              << (fixture.globals.current.primary == fixture.object->primary_work) << ','
              << bool(fixture.globals.current_player_at_3abdf64) << ','
              << (fixture.globals.current.player == fixture.player->player_work) << ','
              << bool(fixture.globals.range_at_3c55d45) << ','
              << unsigned(fixture.globals.range_at_3c55d45.value_or(0)) << ','
              << fp.control_word() << ',' << fp.exception_flags() << ',' << external_calls << ']';
}

void observe(const char *constants_path, std::uint16_t initial) {
    using namespace sadx;
    std::ifstream file(constants_path, std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
    const auto constants = decode_sonic_player_constants(bytes.data(), bytes.size());
    if (!constants)
        throw std::runtime_error("missing frozen Sonic constants");
    Fixture fixture;
    SonicPlayerFrame frame{fixture.object, fixture.task, fixture.world, fixture.player,
                            &fixture.collision};
    X87Context fp(0x7f);
    unsigned external_calls = 0;
    SonicPlayerRuntime runtime{fixture.globals, fixture.scheduler, fixture.players,
                               nullptr, nullptr, *constants, {}};
    runtime.external = [&](const auto &, auto &, auto &) {
        ++external_calls;
        throw std::runtime_error("unexpected external Sonic call before editor stop");
        return SonicPlayerResult{};
    };
#ifdef RESET_CURRENT_BINDING
    reference::EditorFact editor(initial);
    auto borrowed = editor.borrow();
    const auto initial_signed = borrowed.signed_status();
    fixture.globals.editor = borrowed;
    editor.reset();
#else
    std::optional<std::uint16_t> editor(initial);
    const auto initial_signed = static_cast<std::int16_t>(*editor);
    fixture.globals.forced_at_3b29d08 = &editor;
    editor.reset();
#endif
    fixture.globals.forced_at_3b29d0c.reset();
    const auto first = run_sonic_player(frame, runtime, fp);
    std::cout << "{\"case\":\"reserved-editor-read\",\"input\":[" << initial
              << ",1],\"initial_signed\":" << initial_signed << ",\"observations\":[";
    observation(first, fixture, fp, external_calls);
    fixture.globals.forced_at_3b29d0c = 1;
    const auto second = run_sonic_player(frame, runtime, fp);
    std::cout << ',';
    observation(second, fixture, fp, external_calls);
    std::cout << "]}\n";
}
} // namespace

int main(int argc, char **argv) {
    if (argc != 5)
        return 64;
    try {
        unsigned parent_fd = 0, parent_pid = 0;
        const std::string fd_text(argv[3]), pid_text(argv[4]);
        const auto fd = std::from_chars(fd_text.data(), fd_text.data() + fd_text.size(), parent_fd);
        const auto pid = std::from_chars(
            pid_text.data(), pid_text.data() + pid_text.size(), parent_pid);
        if (fd.ec != std::errc{} || fd.ptr != fd_text.data() + fd_text.size() ||
            pid.ec != std::errc{} || pid.ptr != pid_text.data() + pid_text.size())
            throw std::invalid_argument("invalid reserved parent identity");
        sadx::test::retain_reset_parent(parent_fd, parent_pid);
        observe(argv[1], editor_input(argv[2]));
    } catch (const std::exception &failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }
}
