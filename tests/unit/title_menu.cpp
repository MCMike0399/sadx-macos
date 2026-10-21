#include "game/title_menu.hpp"
#include "engine/frame_loop_native.hpp"
#include "cpu/x87_math.hpp"
#include <array>
#include <cassert>
#include <iostream>

struct TextureCalls : sadx::title_menu::MenuTextureCalls {
    sadx::title_menu::MenuTextureState &state;
    std::uint32_t count = 0;
    explicit TextureCalls(sadx::title_menu::MenuTextureState &value) : state(value) {}
    void load_menu_texture(sadx::title_menu::MenuTextureId id) override {
        assert((state.loaded[id.value / 32] & (1U << (id.value % 32))) != 0);
        assert(state.ready == 0);
        ++count;
    }
};
namespace {
struct PutiTaskCalls : sadx::title_menu::PutiCalls, sadx::TaskCallbackContext {
    std::shared_ptr<sadx::TaskWorkArena> arena;
    sadx::title_menu::SequenceWork sequence;
    unsigned allocations = 0, installations = 0;
    explicit PutiTaskCalls(std::shared_ptr<sadx::TaskWorkArena> work_arena)
        : arena(std::move(work_arena)) {}
    std::shared_ptr<sadx::title_menu::PutiWork>
    allocate_puti_work(const sadx::TaskHandle &task) override {
        ++allocations;
        const auto allocated = arena->allocate(16);
        assert(allocated);
        task->auxiliary_work = allocated.work;
        return std::make_shared<sadx::title_menu::PutiWork>();
    }
    void install_puti_callbacks(const sadx::TaskHandle &task) override {
        ++installations;
        task->destructor = sadx::TaskScheduler::source_no_op;
        task->display = sadx::TaskScheduler::source_no_op;
    }
};
void execute_puti(sadx::TaskScheduler &scheduler, const sadx::TaskHandle &task) {
    const auto calls = std::dynamic_pointer_cast<PutiTaskCalls>(scheduler.callback_context(execute_puti));
    assert(calls);
    assert(sadx::title_menu::update_puti(task, calls->sequence, *calls) ==
           sadx::title_menu::MenuCallbackResult::Complete);
}
void repeated_puti_tasks() {
    sadx::TaskScheduler scheduler;
    auto arena = std::make_shared<sadx::TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, {}));
    auto calls = std::make_shared<PutiTaskCalls>(arena);
    assert(scheduler.register_callback_context(execute_puti, calls));
    const auto first = scheduler.create_element(0, 4, execute_puti).task;
    assert(first && scheduler.run_index(4));
    const auto retained_first_work =
        std::static_pointer_cast<sadx::title_menu::PutiWork>(first->auxiliary_native_work);
    assert(retained_first_work && first->auxiliary_work);
    retained_first_work->phase = 0;
    retained_first_work->waiting_frames = 15;

    const auto second = scheduler.create_element(0, 4, execute_puti).task;
    assert(second && !second->auxiliary_native_work);
    assert(scheduler.run_index(4));
    const auto second_work =
        std::static_pointer_cast<sadx::title_menu::PutiWork>(second->auxiliary_native_work);
    assert(second_work && second_work != retained_first_work && second->auxiliary_work);
    assert(calls->allocations == 2 && calls->installations == 2);
    assert(second_work->phase == 1 && second_work->waiting_frames == 0);
    assert(second->destructor && second->display);
    assert(retained_first_work->waiting_frames == 16);

    assert(scheduler.run_index(4));
    assert(first->auxiliary_native_work == retained_first_work);
    assert(second->auxiliary_native_work == second_work);
    assert(calls->allocations == 2 && calls->installations == 2);
    assert(retained_first_work->waiting_frames == 17);
}
struct TitleInputCalls final : sadx::title_menu::TitleInputCalls {
    sadx::title_menu::TitleWork *current = nullptr;
    std::uint32_t music_requests = 0;
    void calculate_ticks() override {}
    void request_menu_music(std::int32_t index) override {
        assert(index == 0 && current && current->music_started == 1);
        ++music_requests;
    }
    std::int32_t key_pressed(std::int32_t) override { return 0; }
    std::uint32_t pressed_buttons() override { return 0; }
    void transition_menu(sadx::title_menu::MenuMode) override { assert(false); }
    void play_title_confirm_sound() override { assert(false); }
    std::int32_t audio_ready() override { return 1; }
    std::uint32_t music_state() override { return 0; }
};
void repeated_title_music_initialization() {
    sadx::TaskScheduler scheduler;
    auto arena = std::make_shared<sadx::TaskWorkArena>();
    assert(scheduler.initialize_source_pool(arena, {}));
    const auto first = scheduler.create_element(0, 3, nullptr).task;
    const auto second = scheduler.create_element(0, 3, nullptr).task;
    assert(first && second);
    auto a = std::make_shared<sadx::title_menu::TitleWork>();
    auto b = std::make_shared<sadx::title_menu::TitleWork>();
    first->auxiliary_native_work = a;
    second->auxiliary_native_work = b;
    std::uint32_t loop = 1, ticks = 1, before = 7, after = 8, exit = 0, wait = 0;
    std::int32_t pending = 9, saving = 0, ready = 1;
    std::uint8_t skip = 1;
    const bool enabled = true;
    sadx::title_menu::TitleInputState state{loop, ticks, pending, skip, saving, after,
                                          before, exit, ready, enabled, wait};
    sadx::title_menu::SequenceWork sequence;
    TitleInputCalls calls;
    for (const auto &task : {first, second}) {
        const auto work = std::static_pointer_cast<sadx::title_menu::TitleWork>(task->auxiliary_native_work);
        calls.current = work.get();
        sadx::title_menu::update_title_input(*work, sequence, state, calls);
        assert(work->submode == sadx::title_menu::TitleSubmode::StayMain);
        assert(work->music_started == 0 && pending == 0 && skip == 0 && saving == 1);
        assert(before == 0 && after == 0);
        sadx::title_menu::update_title_input(*work, sequence, state, calls);
        sadx::title_menu::update_title_input(*work, sequence, state, calls);
    }
    assert(calls.music_requests == 2 && a->music_started == 1 && b->music_started == 1);
    assert(a->timer == 2 && b->timer == 2);
}

} // namespace

int main() {
    repeated_puti_tasks();
    repeated_title_music_initialization();
    std::array<sadx::ActionHandle, 9> slots{};
    sadx::title_menu::ActionBindings bindings{slots[0], slots[1], slots[2], slots[3], slots[4],
                                             slots[5], slots[6], slots[7], slots[8]};
    const sadx::title_menu::CharacterActions actions{{100}, {101}, {40}, {41}, {4}, {29}, {30}};
    sadx::title_menu::prepare_title_assets(bindings, actions);
    const std::array<std::uint64_t, 9> expected{100, 101, 40, 41, 4, 4, 29, 30, 30};
    for (std::size_t i = 0; i < slots.size(); ++i) {
        assert(slots[i].identity == expected[i]);
    }
    sadx::title_menu::MenuTextureState textures{{123,456},0};
    TextureCalls calls(textures);
    const std::vector<sadx::title_menu::MenuTextureId> ids{{0},{5},{29}};
    sadx::title_menu::initialize_menu_data(textures, ids, calls);
    assert(textures.loaded[0] == ((1U << 0) | (1U << 5) | (1U << 29)));
    assert(textures.loaded[1] == 0);
    assert(textures.ready == 1 && calls.count == 3);
    sadx::title_menu::DialogWork dialog;
    sadx::title_menu::DialogWork canonical_dialog;
    dialog.choice_ids.fill(7);
    canonical_dialog.choice_ids.fill(8);
    sadx::title_menu::initialize_dialog_work(dialog, canonical_dialog, 0);
    assert(dialog.blink_step == 1 && dialog.fade == 1.0f);
    assert(dialog.choice_ids[0] == 7 && canonical_dialog.choice_ids[0] == -1);
    assert(!dialog.animation_angle);
    sadx::X87Context arithmetic{127};
    sadx::title_menu::update_dialog_animation(dialog.animation_value, dialog.animation_angle,
                                               dialog.fade, 1, arithmetic);
    assert(!dialog.animation_angle && dialog.animation_value == 255 && dialog.fade == 1.0f);
    sadx::title_menu::PutiWork puti;
    assert(!puti.exit_frames);
    sadx::frame::ModeState mode{2, 3, 17};
    std::uint32_t loop_count = 11;
    std::int32_t elapsed = 40, starting = 1, demo = 0;
    sadx::frame_loop::TickState tick{mode, loop_count, elapsed, starting, demo};
    assert(sadx::frame_loop::calculate_ticks(tick) == sadx::frame_loop::TickFailure::None);
    assert(mode.pending_logic_ticks == 6 && loop_count == 6);
    demo = 1;
    assert(sadx::frame_loop::calculate_ticks(tick) == sadx::frame_loop::TickFailure::None);
    assert(mode.pending_logic_ticks == 63 && loop_count == 63);
    mode.frame_multiplier = 0;
    assert(sadx::frame_loop::calculate_ticks(tick) == sadx::frame_loop::TickFailure::DivideByZero);
    assert(mode.pending_logic_ticks == 63 && loop_count == 63);
    std::cout << "title action bindings: nine original destination slots match\n";
}
