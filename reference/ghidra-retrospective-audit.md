# Retrospective Ghidra validation

Requested by the user on 2026-10-11. This is an independent static cross-check of existing native code, not a full game or hardware-equivalence claim.

Target: `sonic.exe`, SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`. Provider: Ghidra 12.1.4, official sources with a locally built ARM64 decompiler. Executable bytes remain unchanged.

| Native module | Ghidra coverage | Result | Detailed evidence |
|---|---|---|---|
| Mesh registry | 12 function dossiers plus 2 storage-xref records | Initializer return corrected to `1`; pool/link/transform/strip/material/storage semantics checked | [Registry audit](player-mesh-registry-ghidra-audit.md) |
| Player startup and held-object query | 442750 / 417e80 / 43b9c0 retained bodies | Allocation/registration/copy/null/read order checked; newly added fields now receive source calloc/action facts; no new hold discrepancy | [Startup/hold audit](ghidra-startup-hold-audit.md) |
| Contact attributes | 43cb90 complete 335-byte body | No new discrepancy: attribute OR, shadow priority, character 5 weight, float store order and low-byte friction priority checked | [Attribute audit](contact-attributes-ghidra-audit.md) |
| Contact reaction | 43c580 complete 579-byte body | Local flags, normal copy, plane intersection, velocity stores and yaw adjustment agree; custom-register and unordered comparisons require instruction evidence | [Retained Ghidra Evidence](ghidra-session/contact-reaction.json) |
| PSetPosition / collision form | 44cdf0 / 41c1c0 complete 1,771 / 47-byte bodies | Local branches, fields, mandatory call targets and return selection checked; no new producing discrepancy | [Focused capture manifest](ghidra-retrospective-recovery/manifest.json) |
| GetAnalog / WriteAnalogs | 43bdc0 / 40f170 complete 372 / 468-byte bodies | Stage/event/input gates, signed axes, deadzone, camera/angle, magnitude and enabled stores checked; no new producing discrepancy | [Focused capture manifest](ghidra-retrospective-recovery/manifest.json) |
| PGetSpeed / task-to-world / floor-to-world | 443f50 / 43ec90 / 43edd0 complete 603 / 135 / 158-byte bodies | Speed gates/store/call order checked; floor input-copy order corrected | [Focused capture manifest](ghidra-retrospective-recovery/manifest.json) |
| Other completed native modules | Not included in this first batch | Unexamined by this retrospective audit | Earlier exact-instruction reports retain their own scope |

Each audit checks branches, constants, field widths/offsets, mutation order, call targets and return behavior. A disagreement is resolved against executable instructions before changing native code. Tests and original-instruction comparisons are rerun for actual producing changes.

Ghidra recovered types and custom-register parameters can be ambiguous. Static agreement does not establish actively loaded mods, Intel floating-point hardware behavior, input delivery, timing, or speedrun route equivalence. Numeric/status disagreements in existing reports remain visible.

## Contact-reaction cross-check

`player_reaction.cpp` agrees with Ghidra Evidence `ev_72c02dcf70737c03418f756b2a127ed88cc7030c2f127fc7878da7621c2da643` on: flags low bits 3, collision flags 0x10/4 and task flag4 gates; normal query 41bed0; raw normal stores at physics+44/+48/+4c before local normalization; floor normal at+50/+54/+58; plane intersection 7887d0 using the raw copy; velocity direction/sign selection; all prior velocity copies before mutation; angle call438410; yaw subtraction and threshold6000 flip8000; target yaw update. No producing discrepancy found in that local audit.

Ghidra renders the first speed test as `1 <= length`, which does not preserve the original unordered x87 condition by itself. Retained exact-instruction Evidence `ev_7179d69353bc9a8440c3a1c5ebf549eed584a5b57ddacf9c630410d9a44f2277` identifies the `JNP` at43c5aa. The native code preserves the actual status-mask condition. This is a decompiler interpretation limit, not a reason to rewrite the verified branch from pseudocode. Numeric/flag authority remains the instruction oracle and its explicit boundaries.

The complete-deliverable obligations are separately retained in [REA obligation Evidence](goal-obligations/ledger.json). All six remain open; partial module checks cannot close them.

## Position, input, speed and coordinates

The seven new static records and exact IDs/body coverage are in [the capture manifest](ghidra-retrospective-recovery/manifest.json). They were compared with `player_position.cpp`, `player_input.cpp`, `player_input_producer.cpp`, `player_speed.cpp` and `player_coordinates.cpp`.

- **PSetPosition:** reset +44/+48/+4c, preserve +5c in+60, then clear +5c; collision flags10/4 and task4 gates; selected record stride30 and push low3bits; raw normal stores and local normalization; speed thresholds15/crash/run and dot thresholds-0.98/0; form0/1 size/damage turn gates; held flag400 response; taskID12 and yaw3000 gate; 902/903 choose445d10 else449500; preserve provisional outcome; choose airborne branch before collision and reread landing flag afterward; local airborne floor stores/vertical correction. The unusual repeated form3-then4 condition is present in executable control flow and is preserved. Full downstream scene routines remain separate incomplete owners.
- **Collision form:** signed index bounds0..15; null task/collision returns-1; record stride30, unsigned byte+1. Required invalid direct resources remain explicit failures.
- **GetAnalog:** event4000 mode gate; forced2000 cases202/400 with current-character0; forced magnitude1 and optional angle stores; source reload at43be6d explains the native stage requirement even on ordinary input; normalized magnitude0 branch, timer+a bypass under2000, character0 flag100 stage902/903 fallback. Pseudocode alone obscures the unconditional source stage load and unordered x87 branch details.
- **WriteAnalogs:** eight slots, raw axes only first two; signed16×256; ±3072 deadzone; camera yaw/sign choice; low16 angle; stored squared sum and actual signed-sqrt call; magnitude uses sqrt(sum)×sum×source scale capped1; raw outputs precede enabled outputs, enabled bytes must equal1. This is base executable behavior; updated Input DLL behavior remains separately instruction-checked and was not audited with this Ghidra target.
- **PGetSpeed:** acceleration stores+38/+3c/+40; jog×0.87 and dot0.7 gates; source shadow400000 priority over400002, factors0.8/0.5; attributes4000 floor transform versus task transform; vector-add785170, limiter43c380 and cleanup43c4c0 call order. Exact x87 status masks/spills remain instruction authority.
- **Coordinates:** null task no-op; source push/unit/Z/X/negative-Y/calculate/pop sequence; floor angle helper4b8e80 occurs before stack push. Ghidra assembly shows43edd8..43edf1 snapshot the input before43ee05, whereas the native floor owner copied after the provider. The copy now precedes the provider. A focused borrowed-binding regression proves the pre-helper snapshot is retained. Ordinary task/world instruction comparisons still match all23,724 vector/frame/depth cases, retaining21,240 status differences. That oracle does **not** exercise the general floor angle helper; the copy-order correction is covered by exact static instructions and the focused native test.

The retained file named `global-to-local.json` is selected by canonical address43edd0 and actually describes **floor-to-world**, not world-to-player. File labels do not establish identity. All seven analyses and the Evidence export completed. Close initially reported a process-signal permission error during exit; subsequent inspection found no JVM/decompiler for the owned installation and the private runtime directory was gone. No signal was sent manually.

## Shadow-specific task ownership

Ghidra body4a2ae0, retained in [the engine capture](ghidra-engine-recovery/player-shadow-body.json), writes an asset/table pointer at taskwk+8, overlapping the player-index/character bytes used by player task work. Therefore the auxiliary shadow task created by442750 must remain generic owned work until its dedicated native shadow view exists. The previous startup tail attached a `PlayerTaskState` merely to write scale.z, which gave this union the wrong owner. The correction removes that player binding and writes the actual source FILD/FST32 scale.z at+34 through `TaskWorkArena`. A focused native test checks the shadow block has no player view and retains its source index word. The original-instruction prefix comparison stops before this callback-dependent tail; it does not claim to execute or validate the shadow callback.
