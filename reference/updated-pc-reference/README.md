# Pinned updated Windows reference for native SADX reconstruction

This folder contains an isolated copy of the supplied game inputs with the selected updated mod suite. It is a **static prepared reference**, not an observed running game, proof the mods loaded, a verified Windows installation, or the native Mac deliverable. No PE code was executed and the original `Sonic Adventure DX` tree was not written.

Canonical definition: `manifest.json`, SHA-256 **795918d0e40d30437a0fde905d40917cac861ef46b3e86dff0cffdac901f8cd8**. `manifest.sha256` protects that exact definition; `game-inventory.json` pins all **6,272** reference game files. Raw configs have separate hashes and `effective-config.json` records parsed typed settings and source-bound interpretation. The game is at `game/`, intended launch is `sonic.exe` from that working directory with **no arguments**. No selected profile/settings facts are inferred from ambient environment.

`copy-scope.json` records **6,229** selected source files, **1,393,992,524** source bytes before added reference mod/config files. It was captured after the preparation command had started; the final immutable manifest was written after preparation. The storage check reported **608,000,000 KiB** available on the local Data volume. Independent copied bytes were used, with no symlinks/hardlinks back to the original. Only original sonic.exe/sonicDX.ini root files were selected; original data directories were copied, excluding original mods and old CHRMODELS.dll loader wrapper. Alternate executables, hack launcher and manager were excluded. Any supplied save/data files in selected directories were retained byte-for-byte; fresh category save requirements remain runtime fixture work.

## Exact release and binary identities

Release API metadata was queried live on 2026-10-11 and retained in `releases/`; no stale browser `/latest` result was used. Unpacked inventories retain every file size/hash and PE machine, and archives remain intact under `archives/`.

| Component | Current selected release / source revision | Archive SHA-256 | Selected DLL SHA-256 |
| --- | --- | --- | --- |
| Loader | [676](https://github.com/X-Hax/sadx-mod-loader/releases/tag/676), eb4cdcca9637d611a98b910fcfed6ce18580451c | 59d2178af2403832c30c6845acc3783054e840b6ac490c9777b342aac7d23d88 | 3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba |
| Input | [2.6.1](https://github.com/michael-fadely/sadx-input-mod/releases/tag/v2.6.1), 68e2a91bec4a55967674c1420ecd9abf89bcd0ce | 1dc58bf3cbd4384275a598c10aa2fe469ac37417c220c885eb76c952f6346156 | c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5 |
| Speedrun | [2.0](https://github.com/skewbmaster/sadx-speedrun-mod/releases/tag/2.0), 1833a11fccb93b96c15eb044d4c8b628039e6f8a | aa9339380db004c4857c154c39c4d259b89c0a2fad85281bb3bce8f2eaaa6958 | f3fc68a3058fb726142547018da860b170f137248e54b82712b52527e7b74802 |

The loader archive hash matches the digest published by GitHub. The other upstream assets supplied no digest, so their retained/downloaded SHA-256 and immutable asset IDs/timestamps pin actual observed bytes. Speedrun 2.0's release asset was updated on 2025-02-20; the version tag alone would not establish asset identity. Static source evidence does not prove that release DLLs were built from the recorded commit; runtime behavior/readbacks remain required.

The supplied/copied `sonic.exe` remains SHA-256 **6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74**, **55,549,952 bytes**. Latest loader bytes were independently verified at `game/system/CHRMODELS.dll` and equal the unpacked archive's `SADXModLoader.dll`, **1,031,680 bytes**. Original character-data `CHRMODELS_orig.dll` remains **7,647,232 bytes**, SHA-256 **76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5**, and its actual PE export table includes **___SONIC_OBJECTS**. The old original-game CHRMODELS.dll is the prior loader hash e40fac49e18dafdbbb107c62a6e13a16d75fc546b839ad5abcfcb245ccafe990, **993,280 bytes**. Neither original file was replaced.

`bootstrap-static.json` contains PE identities/imports/selected exports and the actual source bytes at VA **0x789e50**, **83ec285733**, which match loader 676's 2004-US signature check. sonic.exe's import table does not statically import CHRMODELS; dynamic bootstrap execution remains unobserved. File names, signature checks and export presence support the planned installation layout but do not establish runtime loading.

## Selected owners and configuration

Loader profile index 0 resolves `game/mods/.modloader/profiles/updated-speedrun.json` through `Profiles.json`. Enabled mod order is **sadx-input-mod**, then **sadx-speedrun-mod**. Core loader package is retained under mods/.modloader, with its DLL also independently copied to system/CHRMODELS.dll for bootstrap.

* Loader **Controller.EnabledInputMod=false** disables its built-in newer SDL/input replacement. External Input 2.6.1 is the sole selected input owner. Loader **XInputFix=false** avoids a second input replacement.
* Loader **KeepCamSettings=false**; Speedrun **OtherSettings.CCEF=true** owns the camera OR patches at **0x434870** and **0x438330**.
* Speedrun **Skipma=true** owns held-Start in-game skipping and held-A-or-Start movie skipping. Eligibility still comes from the surrounding engine state.
* Speedrun 2.0 applies **EHCF unconditionally**, NOP3 at **0x533939** and NOP2 at **0x440ef5**. No separate EHCF DLL is enabled.
* Speedrun **FrameLimit=63FPS** is explicit. The source uses **std::stoi(frameLimitOption)**: the decimal prefix is consumed as **63**, despite the `FPS` suffix. It then applies its revised precise limiter and disabled lag-accumulation policy. The ordinary standalone Frame Limit 2.0 is a 60-FPS default and was **not included**. Neither old Good 63 FPS nor a separate limiter DLL is loaded by the intended configuration.
* **FreeCam=false** preserves selected/default camera mode rather than forcing free camera at startup. **QuickSaveSettings.Enabled=false** and **ForceLoad=false** disable automatic save replacement. The release configschema names the latter `ForceReload`, but actual source reads `ForceLoad`; the direct frozen config uses the actual source key. Chao stat viewer and Gamma timer setup are unconditional in Speedrun 2.0 and are included in the expected modded reference.
* Input config explicitly selects keyboard player **0**, mouse disabled, debug false, original VK key defaults (arrows; J/L/I/M camera; X/Z/A/S actions; Return Start; Q/W camera triggers; extended defaults also recorded). All eight controller slots explicitly use deadzones **7849/8689**, **RadialL=true**, **RadialR=false**, trigger threshold **30**, rumble factor **1**, MegaRumble=false, minimum rumble time=0. Controller mapping database is absent, not inferred from a future attached device.

`sonicDX.ini` is retained exactly: framerate=1, fogemulation=1, sound3d=1, screensize=2, cliplevel=0, sevoice=1, bgm=1, screen=0, mousemode=0, bgmv=30, voicev=70. This defines the current reference audio choice; no voice files were deleted or altered.

## Loader defaults and unavoidable effects audit

The latest archive's **Patches.json** marks many patches checked by default. Those catalog defaults are **not used implicitly** by this profile. All **18** catalog patch names are explicitly **false**, with EnabledGamePatches empty:

`HRTFSound`, `KeepCamSettings`, `FixVertexColorRendering`, `MaterialColorFix`, `NodeLimit`, `FOVFix`, `SkyChaseResolutionFix`, `Chaos2CrashFix`, `ChunkSpecularFix`, `E102NGonFix`, `ChaoPanelFix`, `PixelOffSetFix`, `LightFix`, `KillGBIX`, `DisableCDCheck`, `ExtendedSaveSupport`, `CrashGuard`, `XInputFix`.

These choices deliberately exclude unrelated optional gameplay changes: **Sky Chase resolution fix** changes Tornado/reticle movement and enemy fire rate; **Chaos2CrashFix** changes restart crash behavior; **FOVFix** changes camera projection; **KeepCamSettings** overlaps the requested Speedrun owner. Graphics/model/light/node-limit changes, broader missing-file CrashGuard behavior, extra save support, CD-check bypass and HRTF changes are also disabled. This is a chosen updated required-mod profile, not a claim to reproduce every manager default or to be the only correct profile.

Graphics are explicitly **640x480**, backend **0/DirectX8**, **Vsync=false** to avoid silently imposing 60-Hz presentation on 63-FPS reference cadence, AA/AF=0, auto-mipmapping/forced filtering/HUD scaling=false, border image disabled, resizable window=false, pause-on-inactive=true. All graphics keys read by the pinned loader config are explicitly recorded. Background/FMVs use explicit fill modes 2/1. Core audio uses BASS music/SFX=false, SEVolume=100; existing copied sound directories prevent the source's absent-directory fallback in the intended data layout. Debug console/screen/file/crash logging are false; logging must not silently become part of the frozen reference. Application languages are explicitly text/voice **1**.

The latest loader still performs **unconditional** effects despite optional patches being disabled. Source files are pinned under `sources/loader`:

* DllMain checks the original-version bytes, installs CreateFile redirection and bootstrap hook at **0x401ae1/0x401ae2**; LoadChrmodels dynamically opens preserved character data and installs mod initialization/game-loop hooks at **0x402513** and **0x40c045**.
* PatchWindow overrides window creation at **0x789e50**, bypasses original resolution handling at **0x40297a**, and always enables dynamic rendering buffers. Window/render hooks affect presentation/resource/timing behavior even with fixed resolution and extra graphics options disabled.
* InitMods unconditionally changes depth-buffer pointer **0x38a5db8 -> 0x38a5d94**, file-select branch **0x50473e**, language call **0x402614**, E-key camera hook **0x437547**, sound-list loading and texture replacement hooks.
* Angle interpolation hooks initialize unconditionally, but `interpolation::enabled` begins false and routes to the original until helper users enable it. This is a source-bound condition, not a runtime measurement. Texture/file/model lookup, animation/resource registries and helper callbacks must be considered when comparing source/native behavior.
* Code/event/input/control dispatch installs jumps at **0x426063**, **0x40fdb3**, **0x42f1c5**, **0x42f1e9**, **0x40ff00**. GVR initialization and removal of Tails Adventure's gray filter at **0x87cba8/0x87cbac** are unconditional. No test-spawn arguments are provided; useTestSpawn begins false.

Neither `Codes.dat` nor `Patches.dat` exists at either legacy mods/ or current mods/.modloader/ path. The isolated copy did not inherit the original game's executable code files. Catalog text alone does not execute codes. No unlimited lives, trainers, custom physics or automatic save reloader is enabled. The full binary code path/callee closure is not proven equivalent by this source audit; exact loaded-memory trace comparison must include all effective loader effects. The target is **modded updated PC**, not unmodified vanilla PC.

## Static verification and next runtime evidence

Run `python3 _reference/sadx/updated-pc-reference/verify_profile.py` from the repository root. It checks every game/source/archive/config digest, no unexpected files/code binaries, exact source sonic identity, bootstrap character-data identities, active profile resolution, selected external mods/owners, parsed configs and source signature. It produced **static-reference-verified-not-executed**, 6,272 files, cadence 63. `expected-hooks.json` records source-bound expected patch prefixes and target module ownership for required camera/crash/skip/frame/input hooks; **observed_live=false** throughout.

Required proof before claiming the updated mods are active or this is a valid precision reference:

1. Run the exact reference on **native x86/x64 Windows hardware** with required x86 runtimes and real DirectX8/audio device availability. Record process/thread identity, OS/build/CPU, launch args, working directory and rendering/device settings. Wine, CrossOver, Unicorn function probes and Windows ARM x86 translation are not equivalent evidence for this requested native-PC baseline.
2. Enumerate actual loaded module paths and hash bytes, including CHRMODELS.dll, original character-data DLL, both mod DLLs and their actual SDL2 dependency. Capture effective profile/config paths and successful Init completion. Presence/extraction/metadata/version labels do not establish this.
3. Read back required patch bytes and jump destinations **after initialization**, verify destinations lie in the expected loaded modules, and account for unavoidable loader hooks and any other mutation. Record the effective live 63.0f scalar and frameMultiplier/extra-tick policy.
4. Capture x87 control/status, precision/rounding, MXCSR, per-thread floating state, and boundary register/memory results. Raw original-function probes without these updated mods loaded do not establish the user-selected modded baseline.
5. Record synchronized simulation-tick traces with raw input, pressed/held/released, analog direction/magnitude, position/velocity/action/collision, RNG/state, tick counts, IGT counters and QPC/presentation cadence. Use exact hashed category fixture saves and cover keyboard ramp/diagonals, transitions, deaths, camera persistence, Egg Hornet skip/crash conditions and held-start Skipma.
6. Test the actual Xbox and Switch 2 models/transports on both intended hosts. Original Input 2.6.1's SDL may not recognize new Switch 2 hardware; no mapping/database/driver substitution is silently made in this profile. Native AppKit/GameController preprocessing and original SDL deadzones/ramp/FPU behavior require comparison. A future controller-path change must create a new pinned reference profile.

Latest-version availability is verified; **community approval of this exact version/config combination is not**. No messages were sent to moderators. Leaderboard acceptance, full gameplay parity and the native Mac deliverable remain separate unverified requirements.
