#include "game/menu_navigation.hpp"
#include <cassert>
namespace n = sadx::menu_navigation;
namespace t = sadx::title_menu;
struct Calls final : n::Calls {
    sadx::TaskHandle &sequence;
    sadx::TaskHandle replacement;
    std::shared_ptr<t::SequenceWork> old_work;
    Calls(sadx::TaskHandle &task, sadx::TaskHandle other, std::shared_ptr<t::SequenceWork> work)
        : sequence(task), replacement(std::move(other)), old_work(std::move(work)) {}
    void stop_menu_music() override {
        assert(old_work->waiting == 0);
        sequence = replacement;
    }
};
int main() {
    sadx::TaskScheduler scheduler;
    auto character = scheduler.append_task(3, nullptr, {}).task;
    auto sequence = scheduler.append_task(3, nullptr, {}).task;
    auto other = scheduler.append_task(3, nullptr, {}).task;
    auto cw = std::make_shared<t::CharacterSelectWork>();
    auto sw = std::make_shared<t::SequenceWork>();
    auto osw = std::make_shared<t::SequenceWork>();
    character->auxiliary_native_work = cw;
    sequence->auxiliary_native_work = sw;
    other->auxiliary_native_work = osw;
    Calls calls(sequence, other, sw);
    n::prepare_character_select(t::MenuMode::TrialActSelect, {character, sequence}, calls);
    assert(cw->presentation.next == t::MenuMode::TrialActSelect);
    assert(sw->waiting == 1 && sw->waiting_mode == t::MenuMode::CharacterSelect);
    assert(sequence == other && osw->waiting == 0 && osw->waiting_mode == t::MenuMode::None);
    sequence.reset();
    bool missing_sequence = false;
    try {
        n::prepare_character_select(t::MenuMode::Explanation, {character, sequence}, calls);
    } catch (const n::Error &e) {
        missing_sequence = e.boundary() == n::Boundary::MissingSequenceTask;
    }
    assert(missing_sequence && cw->presentation.next == t::MenuMode::Explanation);
    character->auxiliary_native_work.reset();
    bool missing_work = false;
    try {
        n::set_character_next(t::MenuMode::None, {character, sequence});
    } catch (const n::Error &e) {
        missing_work = e.boundary() == n::Boundary::MissingCharacterWork;
    }
    assert(missing_work);
}
