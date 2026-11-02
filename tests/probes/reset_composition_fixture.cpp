#include "reset_composition_fixture.hpp"
#include "guest/native_game_reference.hpp"
namespace sadx {
struct ResetCompositionFixture::Impl {
    NativeGame game{NativeGameConfig{}};
};
ResetCompositionFixture::ResetCompositionFixture() : implementation_(std::make_unique<Impl>()) {}
ResetCompositionFixture::~ResetCompositionFixture() = default;
GameResetState &ResetCompositionFixture::reset() { return *implementation_->game.reset_; }
control::EditorView ResetCompositionFixture::editor() { return implementation_->game.sonic_globals_.editor; }
}
