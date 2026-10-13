# SADX speedrun mod / native integration findings

Investigation date: 2026-10-11. Scope: supplied `Sonic Adventure DX` game tree and local `sadx-mod-loader` source, public speedrun resources, live upstream metadata and source. Game files remained untouched. Native semantic components were added separately in `sadx-native/src/speedrun.{hpp,cpp}` and tested as C++17; they are not yet connected to the game loop.

## Target and local identity (observed)

The speedrun resource calls the compatible PC executable **2004 US**; 2003 is the overall SADX game release, not proof of this PC executable's edition. Resource: https://www.speedrun.com/sadx/resources/zqvoh . Current upstream loader README requires 2004 US, Windows 7+, x86 VC runtime; manager separately requires Windows .NET 8 x64 Desktop Runtime.

Complete hashes/sizes/PE machine/timestamps: `local-inventory.json` in this directory. Every inspected executable and mod DLL is PE machine 0x14c (i386).

| Local artifact | SHA-256 | Version evidence |
| --- | --- | --- |
| sonic.exe | 6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74 | 55,549,952 bytes; edition equivalence to downloaded resource not established |
| sadx.exe | 2120d260b2942675d7516f2e16fc7ae394ac2b804f496d3a637f670362748bc2 | bundled Read Me identifies SUPER Sonic Adventure DX hack 1.1.100704; this is not a clean speedrun launcher |
| mods/SADXModLoader.dll | e40fac49e18dafdbbb107c62a6e13a16d75fc546b839ad5abcfcb245ccafe990 | VS_FIXEDFILEINFO 3.7.0.0 at offset 953120; FileVersion `3.7`, `git: master/6edf5371+`, `Pre-Upgrade-661-g6edf5371+`; dirty build, not 2018 |
| sadx-input-mod.dll | c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5 | mod.ini 2.6.1 |
| sadx-frame-limit.dll | e59d7feb8bc0d5edc7908d2750836ba14e679ddf35f8f7f667f2521874d16224 | mod.ini 1.1; mod.version 02/02/2018 17:53:44; directory good-63-fps_e8b2696a |
| CCEF.dll | c521721df5d020715af74fd2e9649995b64c51bb91266fd3520f233f2a6f4e53 | no mod.ini version |
| EHCF.dll | 1ffd790d6a3df929e4f89332aade101afc0947a27b6b0ce0846e5f4ef3abcca4 | mod.ini 1.1 |

No Skipma or Speedrun Mod directories were present. No loader settings, input config.ini or gamecontrollerdb.txt were present in the inventory inspected. Presence of DLLs does not prove enabled mods. `sonicDX.ini`: framerate=1, fogemulation=1, sound3d=1, screensize=2, cliplevel=0, sevoice=1, bgm=1, screen=0, mousemode=0, bgmv=30, voicev=70. No game execution was performed.

Local loader source clean at eb4cdcca9637d611a98b910fcfed6ce18580451c (2026-04-08), version.h 3.7.0.0, API version 30. It is newer than the local game DLL. It builds Win32 and uses Windows LoadLibrary/GetProcAddress, MSVC naked x86 assembly, fixed pointers, x86 instruction patching and D3D8.

## Sources and current versions (live upstream metadata)

Browser-cached GitHub `/releases/latest` incorrectly showed loader 653. Live API was queried instead. Metadata retained as release-0.json through release-2.json.

* Loader current release **676**, published 2026-04-08, target commit eb4cdcca9637d611a98b910fcfed6ce18580451c; archive SHA-256 published upstream: 59d2178af2403832c30c6845acc3783054e840b6ac490c9777b342aac7d23d88. https://github.com/X-Hax/sadx-mod-loader/releases/tag/676
* Input current release **v2.6.1**, published 2022-09-13; matches local mod.ini but downloaded release bytes were not compared. https://github.com/michael-fadely/sadx-input-mod/releases/tag/v2.6.1
* Frame Limit current release **v2.0**, published 2024-05-18; notes persistent stutter fix and increased CPU usage. Current master commit 66701e7761af12d29025ba6224b4359ee013f589. It uses a fixed 60 target, not proof of equivalence to speedrun Good 63 FPS. https://github.com/michael-fadely/sadx-frame-limit/releases/tag/v2.0
* Speedrun Mod current release **2.0**, published 2024-11-28; asset replaced 2025-02-20. Tag resolved to 1833a11fccb93b96c15eb044d4c8b628039e6f8a. Includes configurable 60/63 FPS, Skipma, CCEF, EHCF, save reload, Chao viewer, Gamma timer. https://github.com/skewbmaster/sadx-speedrun-mod/releases/tag/2.0

Speedrun resource approved bundle remains **2018 loader**, Input 2.51, Speedrun Mod 1.3.1, camera fix, Egg Hornet crash fix, Good 63 FPS and Skipma test version. This is deliberate stability, not evidence current general loader defaults are speedrun safe. https://www.speedrun.com/sadx/resources/a5jc4

Retained source byte identities:

| File in this evidence folder | SHA-256 | Revision |
| --- | --- | --- |
| speedrun-mod.cpp | fa1f423b717472d310ffdd8385a1a0c05c586fe51a4d993a8915fab6b1cd1f5a | Speedrun tag 2.0 / 1833a11fccb93b96c15eb044d4c8b628039e6f8a |
| skipma.cpp | b73d96feb5f5733cfa0085979641d06aed4acdc8cfbf18cd5325c201371918f2 | same |
| speedrun-frame.cpp | d60451c7ec817daea7d73f1a9cd99db0abb6328e6580928706ff480b49afb567 | same |
| frame.cpp | 9d10d58b7bec78cbe716b3f4a4868d43e3f886ef6df554fa68299b7a6139bc94 | Frame Limit master / 66701e7761af12d29025ba6224b4359ee013f589 |

Specific resources: https://www.speedrun.com/sadx/resources/6s4lq (63 FPS), https://www.speedrun.com/sadx/resources/xgrpz (CCEF), https://www.speedrun.com/sadx/resources/yqzls (EHCF), https://www.speedrun.com/sadx/resources/7grb4 (Input), https://www.speedrun.com/sadx/resources/4qg58 (Skipma). Latest CCEF/EHCF/standalone Skipma source/release identity beyond these resources remains unknown; bundled local metadata is recorded above.

## Native equivalent semantics and binary anchors

The native source is an independent implementation of observed integer/mask semantics. Upstream unlicensed x86 source was used as corroborating behavioral evidence, not copied. Original instructions read from sonic.exe through PE32 section mapping (ImageBase 0x400000, file .text offset 0x1000, RVA 0x1000).

| Operation | Original VA / file offset | Original bytes | Semantics |
| --- | --- | --- | --- |
| Camera reset 1 | 0x434870 / 0x34870 | c7 05 a8 cb b2 03 04 00 00 80 | assign flags at 0x3b2cba8 to 0x80000004 |
| Camera reset 2 | 0x438330 / 0x38330 | c7 05 a8 cb b2 03 04 00 00 80 | same assignment |
| In-game skip check | 0x431517 / 0x31517 | 8b 0d 7c e7 b0 03 f6 41 10 08 74 27 | load controller pointer at 0x3b0e77c; test byte at +0x10 (PressedButtons), START 0x8 |
| Movie skip check | 0x5132b1 / 0x1132b1 | a1 7c e7 b0 03 f6 40 10 0c 74 2a | pressed A or START, mask 0xc |
| Frame limit | 0x7899e0 / 0x3899e0 | 83 ec 10 68 38 85 d0 03 ff 15 68 b0 7d 00 | original performance-frequency query routine entry |
| Tick calculation hook | 0x411d1c / 0x11d1c | 56 e8 be e3 36 00 8b 0d 9c 11 b1 03 | upstream replacement entry; full original owning routine not lifted here |
| Frame-rate float | 0x7dce58 / 0x3dce58 | 00 00 72 42 | IEEE754 60.5 |
| EHCF pointer read | 0x533939 / 0x133939 | 8b 40 10 | upstream NOP removes this dereference; owning game function unlifted |
| EHCF conditional branch | 0x440ef5 / 0x40ef5 | 75 05 | upstream NOP removes jne; owning game function unlifted |

CCEF changes opcode prefix c7 05 to 81 0d: OR existing flags with 0x80000004, preserving the caller's Free camera bit and all other existing bits. Native `reset_camera_flags` implements this exact integer behavior.

Skipma 2.0 tests **held** rather than pressed START inside in-game skip path, and held A|START for movies. The in-game hook unconditionally ORs START into PressedButtons before testing held START; native `in_game_skip_check` returns both the request and modified pressed mask. Surrounding original cutscene eligibility is not implemented by this component; callers must apply it at the owning skip check. SDK buttons are START=0x8, A=0x4 (Dreamcast masks), not SDL enum numbers.

Frame limiter interval is integer floor(FrameMultiplier * counterFrequency / chosenFPS). Disabled accumulation mode returns max(0, FrameMultiplier - 1) extra simulation ticks; this retains multiplier=2 for 30 FPS cutscenes and avoids lag recovery ticks. `frame_interval_ticks` and `extra_game_ticks` implement only these arithmetic policies. They do not implement deadlines, waiting, presentation, clock sampling, engine tick counting or whole-game IGT. Saturation/invalid values are rejected where original signed arithmetic would not yield reliable behavior. Frequency multiplication uses quotient/remainder to avoid allocating/overflowing an unnecessary intermediate.

No native EHCF patch was implemented: translating NOPs without the owning pointer/branch operation would invent semantics. Source and byte anchors are sufficient to prioritize lifting those functions next.

Validation: clang++ -std=c++17 -Wall -Wextra -Werror -pedantic; independent test executable prints `speedrun semantics passed`. Tests retain free-camera choice/unrelated flags; distinguish held vs pressed input and movie A vs in-game START; preserve original pressed-bit injection; verify 60/63 arithmetic, intentional extra ticks, zero multiplier, invalid modes and overflow boundaries. They do not prove game behavior, timing fidelity, renderer performance or native game-loop correctness.

Floating-point game behavior remains unproven: original x87 extended intermediates, rounding, FPU control state and native ARM64 float operations can differ. Whole-game verification must compare traces on original PC executable, including speedrun tricks, transitions, deaths and IGT counters. 63 FPS presentation does not mean dividing IGT by 63; community IGT represents simulation frames at a nominal 60 frames/second.

## Recommended integration

For the required native ARM64 endpoint, integrate the narrow speedrun semantics into the reconstructed owning camera/input/timing/cutscene operations. Add a source-level compatibility API with stable hooks and native dylib exports for rebuilt mods; retain mod.ini, data-file replacement and configurable input mapping where useful. Explicitly classify original PE DLLs as requiring a Windows x86 execution backend. A native loader cannot execute arbitrary existing PE/x86 code or honor original absolute addresses; a universal binary adapter would require CPU emulation plus Windows ABI/memory/layout virtualization and undercuts the native endpoint.

Keep an independent Windows-compatible reference build/installation for differential validation. A Wine/CrossOver solution is a useful reference/playable interim but does not satisfy native CPU or source-port requirements.

## Controllers and current SDL boundary

Live SDL latest API: **release-3.4.18**, 2026-10-02. Metadata retained in sdl-latest.json. https://github.com/libsdl-org/SDL/releases/tag/release-3.4.18

SDL3 supports native macOS gamepads, hotplug, standardized positional button mappings and runtime capability checks. https://wiki.libsdl.org/SDL3/CategoryGamepad . Preserve per-controller deadzones and radial behavior from Input Mod; translate platform button identifiers to original SDK masks at one input boundary. No physical controller was detected or tested in this scope.

SDL source identifies Switch 2 Pro VID/PID 057e:2069 and has its own HIDAPI backend (temporary source switch2.c). USB support is the primary initial verification route, requiring appropriate libusb build configuration. Wireless is not established: upstream issue https://github.com/libsdl-org/SDL/issues/15963 remains open, live state updated 2026-08-27, collecting Bluetooth LE implementations including macOS bridge projects. Do not claim Bluetooth Switch 2 support based on old Switch Pro support. Source: https://github.com/libsdl-org/SDL/blob/main/src/joystick/controller_list.h . Current source should be pinned again before integration, since downloaded switch2.c is release-3.4.8 rather than newest release.

Xbox controller model and wired/Bluetooth transport require actual device validation. SDL has added macOS libusb support for third-party Xbox controllers: https://discourse.libsdl.org/t/sdl-added-support-for-xbox-controllers-via-libusb-on-macos/68459 . No transport/model claim was validated locally.

## Rules and acceptance boundary

Live speedrun API `/games/sadx`: default-time=ingame, emulators-allowed=false, platforms list omits a separately observed native macOS category. Category rules were retained in categories.json; Sonic/Tails/etc require 0% initial progress and PC IGT; 130 Emblems requires fresh Chao and replaces Fusion editor with Speedrun Mod Stat Viewer 1.2.2+. These are explicit current category requirements, not proof a native port will be accepted.

Historical moderator source describes permitted specific fixes, bans texture/level/DC-conversion changes and BetterSADX, and explains IGT as simulation frame count: https://www.speedrun.com/sadx/forums/cuep6 . Current full general rules were not obtainable through `/rules` or unauthenticated HTML (403), so historical statements are identified as historical. Native port acceptance remains unverified and requires rules evidence/moderator determination before labeling leaderboard compliant. No messages were sent to moderators.

## Licensing observations

Frame Limit is MIT, copyright 2016 Michael Fadely (full notice retained as frame-LICENSE.md); Input Mod is MIT. Speedrun Mod API license=null, and repository listing showed no explicit license. SADX loader README prohibits commercial usage; this is not a permissive standard source license. Native speedrun code in this investigation was written independently from instruction semantics/arithmetic facts and does not copy upstream bodies, naked assembly, SDK declarations or copyrighted game bytes into runnable code. Full-game reconstruction/redistribution and copying third-party source require separate ownership/license evaluation; no claim of redistribution rights is made here.
