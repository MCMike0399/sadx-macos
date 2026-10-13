#pragma once
#include <optional>
#include "engine/game_logic.hpp"
#include "engine/ninja_memory.hpp"
#include "cpu/source_bytes.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
namespace sadx {
// WinMain 4025b0, its settings span 4025b0..402c29 (up to CoInitialize), from
// exact instructions, as the pinned profile runs it: the mod loader WriteCalls
// 402614 to SetLanguage (VoiceLanguage/TextLanguage from its settings) and
// WriteJumps 40297a to 402a90 (its own resolution replaces the screensize
// block); its HRTF (402773) and CD-check (402621) patches are off.
// Win32 calls (named by their IAT slot) and unported callees are owners.
struct GameWinMainState {
    std::optional<std::uint32_t> voice_language_at_3b0ef38,text_language_at_3b0f0e8,clip_level_at_3abdcf0,bgm_volume_at_909f28,screen_at_892950;
    std::optional<std::uint32_t> sound3d_at_3b0ef28,mouse_mode_at_3b0eae0; // 40ff20, 40e8a0
    std::array<std::optional<std::uint16_t>,13> pad_buttons_at_389d634;  // rep stosd x6 + stosw, then word stores
    std::array<std::optional<std::int32_t>,17> pad_bits_at_8929d8;       // .data table indexed 1..0x11 from 8929d4
    // Members of the CRT static object at 3ab918c (constructed before WinMain; not ported).
    std::optional<std::uint32_t> media_at_3ab918c,media_at_3ab9190;
    std::optional<std::uint32_t> message_text_at_8929b0,message_caption_at_8929c4; // .data string pointers for that MessageBoxA
};
struct GameWinMainBorrowed {
    std::optional<std::int32_t>& render_factor_at_89295c;   // GameFrameRendererState
    std::optional<std::uint8_t>& special_flags_at_909eb4;   // NinjaQueueConsumerState
    std::optional<std::uint32_t>& sevoice_at_38f6ec0;       // ResetLeafState
    std::optional<std::uint32_t>& voice_volume_at_3abdfac;  // ResetLeafState
    std::optional<std::uint32_t>& bgm_at_3a7cf3c;           // ResetLeafState
    std::optional<std::uint32_t>& voice_copy_at_3abdf90;    // ResetLeafState (40ccf0)
};
// GetPrivateProfileIntA/StringA on ".\sonicDX.ini" (the file argument is always that).
struct WinMainProfileRead {std::uint32_t source_call;std::string section,key;std::int32_t fallback;std::string string_fallback;std::uint32_t size;};
struct WinMainCall {std::uint32_t source_call,callee;std::array<std::optional<std::uint32_t>,4> arguments={};};
struct WinMainOwnerResult {GameLogicOwnerResult owner;std::optional<std::uint32_t> eax;};
// Settings the mod loader applies (its profile values).
struct WinMainLoaderSettings {std::int32_t voice_language=1,text_language=1;};
struct GameWinMainRuntime {
    GameWinMainState& state;
    GameWinMainBorrowed borrowed;
    WinMainLoaderSettings loader;
    std::function<std::optional<std::int32_t>(const WinMainProfileRead&)> profile_int;
    std::function<std::optional<std::string>(const WinMainProfileRead&)> profile_string;
    // 7db040 LoadLibraryA("imm32.dll") (EAX = module), 7db044 GetProcAddress + its call (ImmDisableIME(0)),
    // 401a20, 7db1e0 MessageBoxA, 7db1dc ShowCursor(0) (EAX = display count), 40e8b0.
    // 40ff20, 40ccf0 and 40e8a0 are ported here.
    std::function<WinMainOwnerResult(const WinMainCall&)> invoke;
};
enum class GameWinMainFailure {none,unknown_state,missing_owner,owner_failed,cancelled,profile_unavailable};
struct GameWinMainResult {
    GameWinMainFailure failure=GameWinMainFailure::none;
    std::uint32_t source_boundary=0;
    std::optional<std::uint32_t> required_fact;
    std::optional<WinMainCall> pending_call;
    std::optional<std::uint32_t> imm32_module; // [esp+0x10]: FreeLibrary'd at shutdown (402e13)
    explicit operator bool()const noexcept{return failure==GameWinMainFailure::none;}
};
GameWinMainResult run_winmain_settings(GameWinMainRuntime&);
// 401a20 (WinMain's CD-drive scan) and 401990 (its disc check), from exact
// instructions: 3a8af78[0] = 0; GetLogicalDriveStringsA(0, NULL) sizes an
// njMalloc buffer, a second call fills it; for each drive string, a CD-ROM
// (GetDriveTypeA == 5) whose "%sSoundData\SE\V_AMY_J_BANK06.dat" FindFirstFileA
// succeeds is lstrcpyA'd into 3a8af78; the buffer is njFree'd.
struct CdDriveState {SourceBytes<0x3a8af78,0x104> root_at_3a8af78;};
struct CdDriveHost {
    // GetLogicalDriveStringsA(size, buffer): the return value and the bytes Win32 writes (none when size is too small).
    std::function<std::optional<std::pair<std::uint32_t,std::vector<std::uint8_t>>>(std::uint32_t size)> logical_drive_strings;
    std::function<std::optional<std::uint32_t>(const std::string&)> drive_type;    // GetDriveTypeA
    std::function<std::optional<bool>(const std::string&)> file_exists;            // FindFirstFileA != INVALID_HANDLE_VALUE (then FindClose)
};
enum class CdDriveFailure {none,unknown_state,host_unavailable,buffer_overflow,memory_failed};
struct CdDriveResult {
    CdDriveFailure failure=CdDriveFailure::none;std::uint32_t source_boundary=0;std::optional<std::uint32_t> required_fact;
    struct Query {char kind;std::string text;}; // 'D' GetDriveTypeA, 'F' FindFirstFileA
    std::vector<Query> queries;                 // in source order
    explicit operator bool()const noexcept{return failure==CdDriveFailure::none;}
};
CdDriveResult run_cd_drive_scan_401a20(CdDriveState&,NinjaMemoryState&,GuestHeap&,const CdDriveHost&);
} // namespace sadx
