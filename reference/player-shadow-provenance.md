# Auxiliary shadow owner: source-native foundation, incomplete rendering/update

Target: supplied `Sonic Adventure DX/sonic.exe`, SHA256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`. Native C++17 source uses the existing TaskScheduler, TaskWorkArena, shared player primary state and software x87 owner. This is a startup/callback foundation, not a functioning game or complete shadow renderer.

## Independent evidence

The Ghidra dossiers are read-only artifact observations. Names and call signatures in pseudocode are candidates: original LLVM instructions resolve register arguments, byte signedness and store order. No extra provider was launched by this worker.

| Source | Retained dossier | Genuine Evidence ID |
|---|---|---|
| 4a2b60 thunk | ghidra-engine-recovery/player-shadow-task.json | ev_143a940f346c6b8504cbf74180c3ab4e2abc2079e21ba508f50b86371eb8a4bb |
| 4a2ae0 initializer | ghidra-engine-recovery/player-shadow-body.json | ev_372543d1f01eb90878c2dded101b09fff492c6ee43972f9e8efb4545e88227d2 |
| 4a0720 destructor | ghidra-shadow-recovery/shadow-destroy.json | ev_f9ac9998f06c318027990c4bb0fa1f084e255fa5a1ee7a63a43f8b0fde519ed8 |
| 4a0760 display | ghidra-shadow-recovery/shadow-display.json | ev_9572a1487ee0d0dcbf1c5b5af51bc5a55b52ee2c1267cc353c47f0ed6cb88a67 |
| 4a2570 update | ghidra-shadow-recovery/shadow-update.json | ev_4dbc61775feb27d0f7927eaddaafa8a9ebd8178ddfb16a8a06e1e83e3385adf2 |
| 4f88a0 literal-zero leaf | ghidra-shadow-recovery/shadow-gate.json | ev_6b0dae3f79712184603d1fae2ae29c6a096086f4d4746816eff6273176b07e37 |
| 441970 registered character | ghidra-shadow-recovery/registered-character.json | ev_592a37f826776e92d9f3a8a61c854471ab9c59b9e14d8082d2eb662905110a4d |
| 4144e0 current character | ghidra-shadow-recovery/current-character.json | ev_f42ed0cb9550200e38c734b889d73840e26a0b6b2b351e3d1e27c244507e0151 |
| 4a04c0 particle producer | ghidra-gameplay-live-recovery/shadow-helper-4a04c0.json | ev_b71a8a1b52040ef063d065daf603318c86952bf7fd55d10d2b9384e970b943c7 |
| 4a0270 position/velocity helper | ghidra-gameplay-live-recovery/shadow-helper-4a0270.json | ev_5c963ccd98ee8f336b6aa6bc7cdb9372dca87c89cdad40d78a520d2dbfe1c3e2 |
| 441b00 registered player | ghidra-gameplay-live-recovery/shadow-helper-441b00.json | ev_79b983dc7d3de48ae551736cb85de7a82a189a1e0e0fbd0a01ade0f1ff001731 |
| 6443bf CRT rand | ghidra-gameplay-live-recovery/shadow-helper-6443bf.json | ev_f544cc021236f67bb17371b21ca8219b15d226fd0a801c4ae7dbfcbd24ea79f6 |

Asset9275a0 is exactly1080 bytes (five216-byte groups, three72-byte variants/group), SHA256 `c852bb548014ed621db8fc5c1cbb2abfcee449e0172801438f32526a5ba32201`. First group character DWORDs are2,7,0,3,5. Its original bytes stay immutable in the owned table; texture/resource references are not cast to host addresses.

## Producing ownership and source coverage

`Task.shadow_work` is the sole typed view of the dedicated primary union. `Task.primary_state` and this view are mutually exclusive at both binding boundaries. The source union pointer at+8 selects an owned immutable table group; it is never decoded as player_index/character_id. Mutable player lookup remains the existing live PlayerStartupRegistry/PlayerTaskState. The arena carries allocation identity, node links and generic scalar work. Byte+1 is the common authoritative `secondary_mode_at_1`; word+6 is `control_timer_at_6`, with source zero facts produced only by generic known-byte binding or actual calloc-created child work.

Native callback identities are separate from source VAs. The scheduler owns a callback context independently of field30, accepts repeat registration of the same owner, and rejects replacing a different active owner. Expected callback failures retain full typed diagnostics and source context through the existing failure transfer; arbitrary host exceptions keep prior scheduler behavior. Borrowed registry/FP references must outlive dispatch.

The complete123-byte4a2ae0 body stores real native display/update/destructor callbacks before primary dereference; clears head10, byte1, word6, byte3 and selection8; queries441970(0) for each group, including observed-null4144e0 fallback. Unknown registration/global/table facts stop at the corresponding source boundary after preceding stores. No matching character is a real successful initializer result with known-null selection.

The complete4a0720 body unlinks each owned52-byte head through prev2c/next30 in original order and uses the actual source-native arena release. Head is reloaded after release. Invalid/unknown links retain allocation and partial stores; no invented successor or successful cleanup.

The4a0760 local body preserves signed entry gate(>0), ordered blend calls(0,8),(1,10), signed16 angle, exact raw position/scalex reads and FLD/FMUL-m32/FST32 scaley. Inner4072f0 gate uses !=0, so negative gate still changes blend state/computes packets but skips sprite submission. Next is read after the drawing call. Runtime facts are re-read at their source boundaries; a callback that invalidates a required fact gets a tagged pending result. Unmasked software x87 failures retain the arithmetic site/node/exception reason. Full4030d0→77ec60 Direct3D and4072f0→404760 rendering are required external owners; defaults fail, not no-op success. Unit spies validate composition order only.

The4a2570 owner currently implements only the proven prefix: source scale.z FLD/CRT_ftol/signed AL registration lookup, original old-variant pointer-null test, actual FreeTask scheduling, live player byte1 read and source new-variant store. It then reports the required4a04c0/6443bf/441b00 boundary. It does not fake RNG, geometry, physics field130, timer changes, particle loops or source use-after-free behavior. Newly recovered helpers are evidence for the next implementation, not a completion claim. 4a0270 also calls matrix-owned4a01d0; recovery requested from the root's shared provider.

Actual512-byte startup calloc now produces known-zero source_flags6, timer7e and animation124/126/128. Newly calloc-created typed child primary establishes action0/secondary1/control6 zero; no caller motion angles or decoded event owner is guessed.

## Verification

`tests/player_shadow.cpp` takes the actual1080-byte table and1056-byte player-parameter table. It composes real442750 startup with the real scheduler initializer, tests live character/secondary mutation, context ownership/idempotence/conflict, mixed-view rejection, callback failure/context retention, real node cleanup, signed display gates, explicit renderer boundaries, invalidated live facts and unmasked signaling-NaN arithmetic. Startup and lifecycle owner tests also passed with the new source-produced fields. ASan+UBSan passed for touched shadow/task/work/startup/state sources; prebuilt numeric archive was not instrumented.

`tools/verify_original_player_shadow.py` loads the pinned executable and executes real original instructions, with explicit heap/pool fixture after unlifted acquisition. It uses no callee substitution.43 fixtures ×12 PC24/53/64 and RC profiles =516 evaluations. The final report retains zero stored arena/callback/boundary/CW mismatches and60 FP-status differences. All differences remain reported; strict emulator equivalence is false. This does not establish Windows hardware equality or initialized mod-runtime behavior.

Task-pool payload is excluded from this arena comparison because the native graph owns it; actual live callback identities are compared separately and complete pool semantics have a separate original task-lifecycle oracle. The test snapshot encodes owned selected-table identity in original pointer bytes solely for comparison. Complete source-body byte hashes, native source/library hashes and exact16-word fixture blob are retained in `original-player-shadow-comparison.json` and `.cases.bin`.

Full particle update, transformed producer inputs, Direct3D-to-Metal sprite resource binding, original runtime/hardware capture, other startup tasks, required mods and the playable native game remain unfinished.


## Historical naming correction

The address-bound asset audit found the EFF_REGULAR sprite130 named `bubble`. The original shadow naming was misleading; the active native owner is now `player_auxiliary_effect`, without claiming this is only a bubble system. This document/report is historical. Current source ownership and names are maintained in [player-auxiliary-effect-provenance.md](player-auxiliary-effect-provenance.md).
