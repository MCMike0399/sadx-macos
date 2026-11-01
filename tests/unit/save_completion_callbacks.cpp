#include "game/file_select.hpp"
#include "game/save_completion.hpp"
#include <cassert>
#include <functional>

namespace {
using namespace sadx;
using namespace sadx::file_select;
struct Stop {};
struct Fixture final : SaveCompletionCalls {
    title_menu::FileSelectWork work{};
    std::uint8_t blocked = 0, ok = 0, fade = 9, ready = 8, exec = 7, alert = 6, card = 5;
    std::uint16_t backup = 4;
    std::int32_t phase = 3, disabled = 0;
    TaskScheduler scheduler;
    TaskHandle warning, replacement;
    SaveCompletionState state{blocked, ok, fade, phase, ready, exec, alert, card,
                              backup, disabled, warning};
    std::function<void(int)> observe = [](int) {};
    int selection = 1;
    Fixture() {
        scheduler.set_default_field30(std::shared_ptr<void>{});
        warning = scheduler.append_task(0, nullptr).task;
        replacement = scheduler.append_task(0, nullptr).task;
        set_control_stage(work, ControlStage::WaitForNewSave);
        work.wait_frames = 5;
        work.presentation.next = title_menu::MenuMode::CharacterSelect;
    }
    void request_dialog(std::int32_t index) override { assert(index == 17); observe(1); }
    TaskHandle create_save_warning() override { observe(2); return replacement; }
    std::int8_t dialog_selection() override { observe(3); return selection; }
    void close_save_warning(TaskHandle task) override { assert(task == warning); observe(4); }
    void transition_menu(title_menu::MenuMode mode) override {
        assert(mode == work.presentation.next);
        observe(5);
    }
    void prepare_new_save() override { observe(6); }
    void apply_save() override { observe(7); }
    void cleared() const { assert(phase == 0 && ready == 0 && exec == 0 && alert == 0); }
    bool update() { return update_save_completion(work, state, *this); }
};
}
int main() {
    // Every throwing boundary must leave the source prefix published, never roll it back.
    for (int stop : {1, 2}) {
        Fixture f;
        auto old = f.warning;
        f.observe = [&](int call) {
            f.cleared();
            assert(f.blocked == 1 && f.fade == 0 && f.work.wait_frames == 0);
            assert(control_stage(f.work) == ControlStage::WaitForNewSave && f.warning == old);
            if (call == stop) throw Stop{};
        };
        try { f.update(); assert(false); } catch (const Stop &) {}
        assert(f.warning == old);
    }
    {
        Fixture f;
        assert(f.update());
        assert(f.warning == f.replacement && f.work.wait_frames == 1);
        assert(control_stage(f.work) == ControlStage::CompleteNewSave);
    }
    // Success reads the selected-file owner after the transition callback.
    {
        Fixture f;
        set_control_stage(f.work, ControlStage::CompleteNewSave);
        f.ok = 1;
        f.observe = [&](int call) {
            assert(call == 5 && f.blocked == 0 && f.ok == 1 && f.backup == 4);
            f.card = 211;
        };
        assert(f.update());
        assert(f.backup == 211 && f.fade == 0 && f.ok == 0);
        assert(control_stage(f.work) == ControlStage::Leaving);
    }
    for (int stop : {3, 4, 5, 6, 7}) {
        Fixture f;
        set_control_stage(f.work, ControlStage::CompleteNewSave);
        int last = 2;
        f.observe = [&](int call) {
            assert(call == ++last);
            if (call == 3) {
                assert(f.phase == 3 && f.fade == 9);
                f.warning = f.replacement; // Close must consume the live owner.
            } else {
                f.cleared();
                assert(f.ok == 0 && f.blocked == 0);
                if (call <= 5) {
                    assert(f.fade == 1 && f.disabled == 0);
                    assert(control_stage(f.work) == ControlStage::CompleteNewSave);
                } else {
                    assert(f.fade == 0 && f.disabled == 1);
                    assert(control_stage(f.work) == ControlStage::Leaving);
                }
            }
            if (call == 4)
                f.work.presentation.next = title_menu::MenuMode::Title;
            if (call == stop) throw Stop{};
        };
        try { f.update(); assert(false); } catch (const Stop &) {}
        assert(last == stop);
    }
    {
        Fixture f;
        set_control_stage(f.work, ControlStage::CompleteNewSave);
        f.selection = 0;
        f.observe = [&](int call) { assert(call == 3 || call == 4); };
        assert(f.update());
        assert(control_stage(f.work) == ControlStage::Initialize && f.fade == 1);
    }
}
