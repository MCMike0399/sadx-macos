#include "game/menu_dialog_control.hpp"
#include <cassert>
namespace c = sadx::menu_dialog_control;
namespace d = sadx::menu_dialog;
struct Definitions final : c::Definitions {
    d::Parameters actual{};
    const d::Parameters &parameters(std::int32_t id) override {
        if (id != 4)
            throw d::Error(d::Boundary::DialogIndex, "missing imported definition");
        return actual;
    }
};
struct Calls final : d::Calls {
    unsigned sounds = 0;
    void play_sound(std::int32_t) override { ++sounds; }
};
int main() {
    sadx::TaskScheduler scheduler;
    auto task = scheduler.append_task(3, nullptr, {}).task;
    auto work = std::make_shared<d::Work>();
    task->auxiliary_native_work = work;
    d::Owner owner{task};
    Definitions defs;
    defs.actual.choice_count = 3;
    defs.actual.sounds = std::make_shared<sadx::title_menu::DialogSounds>(
        sadx::title_menu::DialogSounds{25, 0, 0, 0, 0});
    Calls calls;
    c::open_advanced(4, 1, {}, defs, owner, calls);
    assert(work->phase == 1 && work->cursor == 1 && calls.sounds == 1);
    assert(!c::close(owner));
    work->fade = 1;
    work->animation_angle = 123456;
    c::open_advanced_quick(4, 2, {}, defs, owner);
    assert(work->phase == 2 && work->fade == 1 && work->animation_angle == 0 && work->cursor == 2);
    assert(calls.sounds == 1);
    c::reset_idle_frames(-32768, owner);
    assert(c::idle_frames(owner) == -32768);
    assert(c::close(owner) && work->phase == 3 && work->opening_frames == 4);
    work->opening_frames = 7;
    assert(c::close(owner) && work->opening_frames == 7);
    bool missing = false;
    try {
        c::open_advanced_quick(1, 0, {}, defs, owner);
    } catch (const d::Error &e) {
        missing = e.boundary() == d::Boundary::DialogIndex;
    }
    assert(missing);
}
