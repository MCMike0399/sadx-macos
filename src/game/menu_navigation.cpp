#include "game/menu_navigation.hpp"
namespace sadx::menu_navigation {
void set_character_next(title_menu::MenuMode next, State state) {
    if (!state.character_task)
        throw Error(Boundary::MissingCharacterTask, "character selection has no canonical task");
    const auto character = std::static_pointer_cast<title_menu::CharacterSelectWork>(
        state.character_task->auxiliary_native_work);
    if (!character)
        throw Error(Boundary::MissingCharacterWork, "character selection has no canonical work");
    character->presentation.next = next;
}
void prepare_character_select(title_menu::MenuMode next, State state, Calls &calls) {
    set_character_next(next, state);
    if (!state.sequence_task)
        throw Error(Boundary::MissingSequenceTask, "menu sequence has no canonical task");
    const auto sequence = std::static_pointer_cast<title_menu::SequenceWork>(
        state.sequence_task->auxiliary_native_work);
    if (!sequence)
        throw Error(Boundary::MissingSequenceWork, "menu sequence has no canonical work");
    // The source captures sequence work before music shutdown, which can replace its owner.
    calls.stop_menu_music();
    sequence->waiting = 1;
    sequence->waiting_mode = title_menu::MenuMode::CharacterSelect;
}
} // namespace sadx::menu_navigation
