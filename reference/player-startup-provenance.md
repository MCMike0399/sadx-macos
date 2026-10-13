# Native player startup owner

Target: supplied PC `sonic.exe`, SHA256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`. This reconstructs the actual PInitialize442750 allocation/registration/initial-store prefix around the existing native scheduler and shared player state. It does not create the main player task, look up a start position, execute the unresolved auxiliary callback, or establish a running native game.

## Genuine evidence

| Entry | Dossier | Evidence ID |
|---|---|---|
| 0x442750 | [player-initialize.json](gameplay/player-initialize.json) | `ev_cbdc04201a437f7e05362459b8d60d4b53a592356dbf1aaf9bd8e9c2ba9af9cc` |
| 0x442750 | [player-initialize.json](ghidra-engine-recovery/player-initialize.json) | `ev_606370da7ae1e50c5153fa8d695b2c48aa7e0cb6780c6c76302d7597609eb34f` |
| 0x4a2b60 | [player-shadow-task.json](ghidra-engine-recovery/player-shadow-task.json) | `ev_143a940f346c6b8504cbf74180c3ab4e2abc2079e21ba508f50b86371eb8a4bb` |
| 0x4a2ae0 | [player-shadow-body.json](ghidra-engine-recovery/player-shadow-body.json) | `ev_372543d1f01eb90878c2dded101b09fff492c6ee43972f9e8efb4545e88227d2` |
| 0x417e80 | [object-work-initialize.json](ghidra-engine-recovery/object-work-initialize.json) | `ev_ce3e11a1e2bf9d984e102592202daf8667a657c9f814495d915be8abafe0ab9c` |

Exact LLVM listing: [player-startup.asm](player-startup.asm). Ghidra’s seeded recovered body and Hopper’s assembly agree on the source flow, including Sonic→Tails and Gamma→Big equipment fallthrough. The older movement dossier name `player-start-position` at417e80 is a lexical misclassification: this function allocates/initializes0x7c object work at taskwk+3c. No start-position calculation occurs there.

## Native ownership and preserved source order

- The caller already owns a native TaskHandle, primary work and live motion. Generic task work binds once to Task.primary_state; an existing typed view is authoritative, including unknown optional fields. Startup and hold read live task ID/flags/pose rather than stale arena numeric bytes. Child creation reads the live parent position then angles and creates a new source-zeroed typed child; unknown position fails at40b98d after the child is linked. No decoded event facts are invented from a pointer-nullness observation.
- Actual40b2d0 allocation semantics are used for known source arguments: player1×200h, object1×7ch, contact1×84h, auxiliary15×20h. Caller/parameter failure retains already allocated blocks. Initial main-task creation remains the caller’s responsibility.
- Parameter132-byte rows copy to new player+9c before motion+0 binds the same player work. A missing/out-of-domain source parameter row stops before copy; no fallback Sonic parameters are supplied.
- Actual417e80 leaves a known existing object reference intact. Otherwise it allocates zeroed7ch bytes, stores the new/null reference to taskwk+3c, then writes byte0=3. Allocation failure stops at417e9b after the null store.
- Contact84h allocation can return null: the source stores null at player+64 and continues. The same is true for auxiliary15×20h at+78. Native absence of those allocations is retained instead of becoming fabricated contact data. Player200h allocation null fails before destination copy; missing motion work fails at442793.
- Four registration stores occur in order: task3b42e30, primary work3b42e10, motion work3b36dd0, player work3b3cdf0. Registry omission is unknown; observed null references are distinct. Missing live motion preserves the first two stores and stops before4427bf, rather than registering a fake null MWP. Eight safe slots correspond to the observed adjacent32-byte task arrays; other indices stop before the unsupported source global write, not a claim that original442750 validates them.
- The registry also owns live PlayerStartupPlayer views by allocation identity, so partially initialized or overwritten slots cannot erase the state behind an existing MWP→player work reference. Game state uses the shared PlayerTaskState/PlayerMotionState/PlayerPhysicsState and live PlayerParameters, not a second mutable arena snapshot. Automatic player destruction/unregistration is not invented; it requires the actual cleanup owner.
- The shared player_initial_shadow_scale leaf retains the original FLD radius/FMUL m32(0.2)/FST32 semantics with the caller’s explicit X87Context. Its arithmetic is reused by the preexisting seed helper. Shadow, animation and join stores precede required event reads. Missing events stops at the first character-specific412d20 call with earlier registration/shadow state intact; characters without event calls need no event owner.
- Typed speed_at_138/self_at_13c accessors point to the authoritative physics speed/self. WorkArena records the same allocation identities using validated interior offset38h/self offset0. Existing whole-block reference consumers reject nonzero offsets. Reference metadata is weak; it introduces no strong self/target cycle. Released/expired targets retain explicit failure and allocation-offset facts.
- Player-relative speed clears in source order, while caller world speed remains unchanged. World friction3c becomes1. Task ID+2 changes0→1 and preserves nonzero; unknown live ID fails at44294c with no raw-byte fallback. The task scale becomes(1,1,1) after auxiliary work allocation, before the unresolved auxiliary task creation.
- Missing native4a2b60 callback fails at44297b, retaining all earlier work, registration, equipment and scale state. It is not replaced with51f680. The verified9-byte thunk enters123-byte4a2ae0; that body installs callbacks4a0760/4a2570/4a0720 and calls4f88a0/441970. Those real owners are the next closure, not a successful no-op.

## Original instruction comparison

[original-player-startup-comparison.json](original-player-startup-comparison.json) covers 167 fixtures ×12 CW profiles = 2,004 evaluations. Stored state, resource identities/free-head/registration facts, exact boundaries and CW all match. FP flags differ in 924 evaluations and remain separately reported.

The verifier executes actual442750,40b2d0,417e80 and412d20 with actual task/work allocation. Heap acquisition/task-pool initialization use the declared source fixture after unlifted CRT/Win32 setup. No callbacks or callees are replaced. It stops before44297b or the selected earlier unsafe/unknown boundary. `original_return_observed=false`; `full_emulator_match=false`. This is prefix equality, not complete PInitialize or game equivalence.

Fixtures cover all8 source rows, zero/nonzero/random save bytes, live and generic task views, preserved world state, existing/new object work, nullable contact/aux allocation, player/object allocation failure, missing source parameters/events/motion/ID and unsupported registration. The1376-byte canonical test snapshot serializes authoritative native views plus owned object/contact/aux allocation bytes; it is a test projection, not a runtime guest memory interpreter.

Fixture blob SHA256: `7eb3128a4cc3e14b1f29944121e2de4e7972b25625a5a187b9fc46b1c0236bf4`. Parameter table SHA256: `3f69076118fcc6a77c55e600eb337944eb6562e5b0979104b58058ad64ba5107`. Actual private probe SHA256: `9e5261fd2b7f7868452acc4098ea7200ec85ff03d4fa102e1fe7807f497538cd`. Source/control/fixture/binary digests are pinned in the report.

## Integration and checks

- CORE: player_startup. TEST: player-startup with build/assets/player-parameters.bin1056B. No new production assets. Updated source tests: task_work/task_lifecycle/player_state.
- PROBE: native_player_startup_probe.cpp; reusable tools/verify_original_player_startup.py. CABI: parameter1056B, event512B,33 input words, explicit CW→24 output words plus1376B canonical snapshot. Required4a2b60 callback deliberately absent for this prefix profile.
- Strict optimized tests pass. Startup, Task and WorkArena sources and startup test pass ASan+UBSan; task_work interior/self/free tests and task_lifecycle live-pose tests also pass sanitizer checks. Linked numeric archive is not instrumented in the startup check. Root owns Makefile/production archive integration.

## Source snapshot

| Native file | SHA256 |
|---|---|
| `src/player_startup.hpp` | `eacec671c7ece03a548d7b9bc90729c2a6ac7f93e87e5feb15663d26b757d6fd` |
| `src/player_startup.cpp` | `a30b811140fe883188e492a2c6ef7ead230abb34caec694e0ca95ed629f26f28` |
| `src/player_state.hpp` | `8af017430d94187086cf144797a7de81c9f1bf81703999aa047d96a0b7dd1854` |
| `src/player_state.cpp` | `c80f03f32d40135c4682c7bda8bed9027a81e0f5a3163c89ee0647696bbe4918` |
| `src/task.hpp` | `31c5cba241695d0f3fe2a65e30de351afebbbafafb1dc6913243ec10bcfbea74` |
| `src/task.cpp` | `b375f521fe62b6c54e5e4bf4c6167c478cdcd45e7a4d69371999a8760976a29e` |
| `src/task_work.hpp` | `11bc7fa233a475d970ba8954cc00a87fd50fde7fd7a6179621aed47ea4c6afd3` |
| `src/task_work.cpp` | `0fd985b4b3f6d71de7f4947206e6a214edc107e3a5285e15c70e50767e2b10b6` |
| `tests/player_startup.cpp` | `8b1ad809256e68d38050822e7c1bf78c8b6ca6c8f0602ac7d740a782ef9f7aa4` |
| `tests/native_player_startup_probe.cpp` | `061c0c794d063563f727999d7e3f4a7f6a4a608d49c7e7aa96a6359cfa26f371` |
| `tools/verify_original_player_startup.py` | `d31a76013a398eb57c426cffc331af4b87d6d9b73df4f36d852f760866ef7d55` |

Unfinished: actual auxiliary task callbacks and required scene/stage/render owners, start-position/caller initialization, player cleanup/global unregistration, original hardware/loaded-mod runtime control evidence, native full-game rendering/controller integration. Existing static/provider evidence does not prove those runtime behaviors.

## Independent Ghidra follow-up

[ghidra-startup-hold-audit.md](ghidra-startup-hold-audit.md) records the retrospective cross-provider checks and new action/calloc producer facts. Refreshed prefix remains2004 evaluations with0 new producer-fact differences and924 separately retained FP flag differences. The authoritative current source/probe digests are in that audit and the refreshed original-player-startup-comparison.json report.

The Ghidra follow-up also corrected the auxiliary task union representation: +8 belongs to a shadow-table reference, so the unlifted auxiliary task keeps generic owned work, with exact index FILD/FST32 at+34. It is not bound as PlayerTaskState. Focused owner test passes optimized and ASan+UBSan; its callback token is deliberately not invoked and is not a native shadow implementation. Prefix oracle coverage remains before this creation tail.


## Additional source-produced scalar facts after Sonic predicate ownership

The approved shared Physics fields `source_charge_at_0`, `jump_timer_at_8`, `idle_timer_at_e`, `timer_at_80` and `timer_at_82` are initialized to known zero only after the actual successful 512-byte source calloc in442750. Generic primary binding reads known arena DWORD+0xc into the single authoritative signed `source_counter_at_c`; it never overwrites an existing live view or falls back from a live unknown. Actual calloc-created child primary establishes counter+0xc zero before original pose copies. Parent counter values are not copied. Event `action_list_at_4c` remains unknown until the actual event resource producer establishes its identity; omission is not converted to observed null.

Before-producing source fingerprints are retained in `startup-scalar-producer-before.json`. Existing startup and task lifecycle owner tests cover source zeros, signed raw counter conversion and preservation of unknown live fields. No original-instruction arithmetic/body has been altered, no probe protocol changed, and no new runtime-equivalence claim is made by these typed fact additions.

## Animation-tail calloc producer facts

The source's successful512-byte calloc also establishes the single shared animation tail's words12a/12c/12e and floats130/134 as zero, and its table140/buffer148 references as observed-null. These facts are produced in442750 after actual allocation succeeds; a generic PlayerPhysicsState retains unknown optional fields. The source producer does not replace live speed138/self13c references or infer an animation table from asset existence. Focused startup tests check all eight character rows and the distinction between generic unknown and source-produced known-null references. Before this bounded producer addition, player_startup.cpp SHA256 was `e027e38ba29b0e41ef19d13231e0a5e41bc6b0bc255f0a43c96711bef7b3fead` and tests/player_startup.cpp was `5d92ac76610e2372ec93417bc3d8f792d99ffc3d7e6dbee1e1dd7df7097bbe67`. Original prefix protocol/arithmetic and its prior hardware limits are unchanged; root must repin the rebuilt production probe before treating it as current.
