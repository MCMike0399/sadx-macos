#pragma once
#include "game/title_menu.hpp"
#include <stdexcept>

namespace sadx::menu_navigation {
// NOTSA: borrows canonical menu work owners, allowing the music callback to replace either owner.
struct State {
    const TaskHandle &character_task;
    const TaskHandle &sequence_task;
};
enum class Boundary {
    MissingCharacterTask,
    MissingCharacterWork,
    MissingSequenceTask,
    MissingSequenceWork
};
class Error : public std::runtime_error {
  public:
    Error(Boundary boundary, const char *message)
        : std::runtime_error(message), boundary_(boundary) {}
    Boundary boundary() const { return boundary_; }

  private:
    Boundary boundary_;
};
struct Calls {
    virtual ~Calls() = default;
    // NOTSA: existing native menu-music shutdown composition implements the source callee.
    virtual void stop_menu_music() = 0;
};
// FUNCTION: SONIC 0x512540 SetCharacterSelectNextMode
void set_character_next(title_menu::MenuMode, State);
// FUNCTION: SONIC 0x505e60 CharSelAdvaModeProcedure
void prepare_character_select(title_menu::MenuMode, State, Calls &);
} // namespace sadx::menu_navigation
