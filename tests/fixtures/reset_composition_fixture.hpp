#pragma once
#include "guest/game_reset_reference.hpp"
#include <memory>
namespace sadx {
// NOTSA: NativeGame's actual layout is compiled without the Sonic fixture macro.
// The forced fixture translation unit can borrow only the real reset/editor owners.
struct ResetCompositionFixture {
    ResetCompositionFixture();
    ~ResetCompositionFixture();
    GameResetState &reset();
    control::EditorView editor();
  private:
    struct Impl;
    std::unique_ptr<Impl> implementation_;
};
}
