#include "game/menu_navigation.hpp"
#include <algorithm>
namespace n = sadx::menu_navigation;
namespace t = sadx::title_menu;
namespace {
struct Calls final : n::Calls {
    sadx::TaskHandle &character;
    sadx::TaskHandle &sequence;
    sadx::TaskHandle other_character;
    sadx::TaskHandle other_sequence;
    const std::uint32_t *input;
    std::uint32_t *out;
    Calls(sadx::TaskHandle &c, sadx::TaskHandle &s,
          sadx::TaskHandle oc, sadx::TaskHandle os,
          const std::uint32_t *i, std::uint32_t *o)
        : character(c), sequence(s), other_character(std::move(oc)), other_sequence(std::move(os)),
          input(i), out(o) {}
    void stop_menu_music() override {
        out[16] = 1;
        const auto cw = std::static_pointer_cast<t::CharacterSelectWork>(character->auxiliary_native_work);
        const auto sw = std::static_pointer_cast<t::SequenceWork>(sequence->auxiliary_native_work);
        out[17] = static_cast<std::uint32_t>(cw->presentation.next);
        out[18] = sw->waiting;
        out[19] = static_cast<std::uint32_t>(sw->waiting_mode);
        if (input[6]) sequence = other_sequence;
        if (input[7]) character = other_character;
    }
};
}
extern "C" void sadx_native_menu_navigation_probe(const std::uint32_t *i, std::uint32_t *o) {
    std::fill_n(o, 32, 0);
    auto original_character = std::make_shared<t::CharacterSelectWork>();
    auto original_sequence = std::make_shared<t::SequenceWork>();
    auto other_character = std::make_shared<t::CharacterSelectWork>();
    auto other_sequence = std::make_shared<t::SequenceWork>();
    original_character->presentation.next = static_cast<t::MenuMode>(i[2]);
    original_sequence->waiting = static_cast<std::uint8_t>(i[3]);
    original_sequence->waiting_mode = static_cast<t::MenuMode>(i[4]);
    other_character->presentation.next = static_cast<t::MenuMode>(i[8]);
    other_sequence->waiting = static_cast<std::uint8_t>(i[9]);
    other_sequence->waiting_mode = static_cast<t::MenuMode>(i[10]);
    sadx::TaskScheduler scheduler;
    auto character = scheduler.append_task(3, nullptr, {}).task;
    auto sequence = scheduler.append_task(3, nullptr, {}).task;
    auto other_character_task = scheduler.append_task(3, nullptr, {}).task;
    auto other_sequence_task = scheduler.append_task(3, nullptr, {}).task;
    character->auxiliary_native_work = original_character;
    sequence->auxiliary_native_work = original_sequence;
    other_character_task->auxiliary_native_work = other_character;
    other_sequence_task->auxiliary_native_work = other_sequence;
    Calls calls(character, sequence, other_character_task, other_sequence_task, i, o);
    if (i[0]) n::prepare_character_select(static_cast<t::MenuMode>(i[1]), {character, sequence}, calls);
    else n::set_character_next(static_cast<t::MenuMode>(i[1]), {character, sequence});
    o[0] = static_cast<std::uint32_t>(original_character->presentation.next);
    o[1] = original_sequence->waiting;
    o[2] = static_cast<std::uint32_t>(original_sequence->waiting_mode);
    o[3] = static_cast<std::uint32_t>(other_character->presentation.next);
    o[4] = other_sequence->waiting;
    o[5] = static_cast<std::uint32_t>(other_sequence->waiting_mode);
    o[6] = character == other_character_task;
    o[7] = sequence == other_sequence_task;
}
