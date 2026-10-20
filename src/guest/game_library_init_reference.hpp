#pragma once
#include <optional>
#include "engine/game_logic.hpp"
#include "cpu/x87_math.hpp"
#include <array>
#include <cstdint>
#include <functional>
namespace sadx {
// Reference compatibility for Sonic library initialization. The four entry
// strings are company, application, system root and backup root; the original
// New.h NES_LIB_PARAM declaration is shifted relative to this actual layout.
// Configuration availability, original resource pointers and call diagnostics
// stay here; library::initialize_libraries owns the shared startup policy.
struct GameLibraryInitState {
    std::array<std::optional<std::uint32_t>,9> words_at_389d864; // config +0x28..+0x48
    std::optional<std::uint32_t> word_at_38a5dc4,word_at_38a5dc8,word_at_38a5dcc; // config +0x10, +0x50, +0x54
    std::optional<float> scale_at_3d0f800,aspect_at_3d0f9c0;
};
struct GameLibraryCall {std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,4> arguments={};};
struct GameLibraryOwnerResult {GameLogicOwnerResult owner;std::optional<std::uint32_t> eax;};
struct GameLibraryInitRuntime {
    GameLibraryInitState& state;
    std::uint32_t config=0x892940;
    // Reads a config word: its words have several owners (892950 WinMain, 89295c the renderer...).
    std::function<std::optional<std::uint32_t>(std::uint32_t address)> read;
    // The device viewport's Width/Height (3d12788, 3d1278c), read after 791610 filled them.
    std::function<std::optional<std::uint32_t>(std::uint32_t address)> read_viewport;
    X87Context& fp;
    std::function<GameLibraryOwnerResult(const GameLibraryCall&)> invoke;
};
enum class GameLibraryInitFailure {none,unknown_state,missing_owner,owner_failed,cancelled,fp_failed};
struct GameLibraryInitResult {
    GameLibraryInitFailure failure=GameLibraryInitFailure::none;
    std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    std::optional<GameLibraryCall> pending_call;
    std::uint32_t tries_left=0; // EBX after the 788000 loop (0 = every try failed)
    explicit operator bool()const noexcept{return failure==GameLibraryInitFailure::none;}
};
GameLibraryInitResult run_library_init_781070(GameLibraryInitRuntime&);
} // namespace sadx
