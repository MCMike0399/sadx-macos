# Independent Ghidra audit: startup and hold

TargetSHA256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`. These are genuine retained Ghidra dossiers from root’s completed owned-provider run, independently checked against frozen C++ and exact LLVM instructions. No duplicate provider was launched.

| Native scope | New static audit | Existing/updated execution evidence | Genuine Ghidra Evidence ID |
|---|---|---|---|
| [442750 startup](ghidra-engine-recovery/player-initialize.json) | Agrees on prefix; corrected auxiliary union ownership and added calloc facts | 2004 original-prefix evaluations match values/resources/boundaries/CW/new producer facts | `ev_606370da7ae1e50c5153fa8d695b2c48aa7e0cb6780c6c76302d7597609eb34f` |
| [417e80 object allocation](ghidra-engine-recovery/object-work-initialize.json) | Agrees; earlier start-position label disproved | Included realcallee execution in startup prefix | `ev_ce3e11a1e2bf9d984e102592202daf8667a657c9f814495d915be8abafe0ab9c` |
| [43b9c0 hold query](ghidra-engine-recovery/hold-object.json) | No producing mismatch found | 133410 real-instruction return/state/fault fixtures already pass | `ev_f6d91d07b03e9c02681253bab7dce13e300dd8985ff113f29093bb31dfd29142` |

## Checks and correction

- Startup: confirmed200h player allocation,84h parameter-copy stride21dwords to+9c, motion+0 linkage before417e80, nullable84h contact allocation, four registration stores in source order, radius×binary32(0.2) at+1fc, animation/join zero stores,138 speed/self13c aliases, equipment bit-byte offsets and Sonic→Tails/Gamma→Big fallthrough, player-speed clearing, world friction1,0→1 task ID fallback,15×20h auxiliary work, unit taskscale and2/2/4a2b60 auxiliary creation/FILD index tail.
- Object work: confirmed existing nonnull+3c skips allocation; otherwise1×7ch zeroed work is stored before byte0=3. Missing/null failure retains exactly the preceding motion/player linkage. No start-position calculation occurs.
- Hold: confirmed flag800h split; held pointer lookup uses full player-index byte and reads+68 without source bounds/default-null protection. Held exec40b570/null, absentmotion24, primary20 and collision38 return0 in order. Held ID+2 is read before out-of-range character default. Ground branch requires collisionflags1/4 and taskflag4 clear, follows+a4→+9c→+20, tests low actorflag40h, readsID before highflag1000h, then applies exact class/actor masks. Characters1/4/>7 return0, ordinary4→200h/5→400h/6→0/7→800h, Big5/6→600h. No arithmetic/FPU/callee invocation occurs in the hold body.
- Ownership: live primary_state and live scene actor state take precedence; unknown ID never falls back to stale arena bytes. Nullable native references preserve unknown versus observed-null. Startup’s work aliases are owned identities with weak metadata and a typed live speed accessor. Hold’s bound runtime is borrowed and current, not a stale copied registration snapshot.
- Concrete auxiliary ownership correction: Ghidra4a2ae0 stores a table pointer at shadow taskwk+8, which overlaps the player byte counters. Startup incorrectly attached PlayerTaskState to that auxiliary block solely to set scale.z. It now keeps the block in generic owned storage and performs exact FILD/FST32 at+34. A focused creation/store test uses an explicit uninvoked callback-boundary token that fails if executed, then proves an opaque+8 reference remains outside player fields. No shadow/render callback implementation or successful no-op was supplied. Source4a2ae0 Evidence ID `ev_372543d1f01eb90878c2dded101b09fff492c6ee43972f9e8efb4545e88227d2`; thunk4a2b60 `ev_143a940f346c6b8504cbf74180c3ab4e2abc2079e21ba508f50b86371eb8a4bb`.
- Producer correction: source calloc1×200h makes physics flags_at_10=0, no_control_time_at_a=0, moving_task_at_6c observednull and cached_land_object_at_74 observednull. Previously those newly added optional fields remained unknown; startup now supplies the proven facts. Generic primary binding reads known actionbyte0, preserving omission when unknown; newlycalloc typed child produces action0. Existing live task unknown action is retained. No caller motion angle1c/24 facts are invented.

## Verification and limits

The refreshed original startup prefix has167 fixtures×12 CW =2004 evaluations, with0 state/resource/boundary/CW/producer-fact mismatches. FP flags differ924 times and remain explicit. Startup and task ownership unit checks pass; startup sources/test also pass ASan+UBSan with numeric archive outside sanitizer instrumentation. Frozen hold’s existing133410-case production report remains its execution evidence; this new Ghidra pass corroborates its static branches/fields rather than claiming a new runtime run.

Startup original execution still stops at44297b for missing actual4a2b60 owner, or an earlier declared unsupported/fault boundary. Original return, shadow/render callbacks, start-position/caller initialization, true loaded-mod/hardware state and the full native game are not proven. Native failure tags for source crashes/unsafe indices and unsupported ownership domains are deliberate bounded adaptations, not original successful behavior.

The shared typed state remains authoritative for mutable gameplay; canonical probe snapshots are test projections of that state and allocated resources. A static decompiler’s pseudocode does not establish execution, and neither Ghidra nor the emulator establishes original hardware x87 flags.

## Audited producer files

| File | SHA256 |
|---|---|
| `src/player_startup.cpp` | `ac35ab9df3a9603f85471a6ed51a67b0a756516208e4b1775eef4ade6645ffc0` |
| `src/player_startup.hpp` | `eacec671c7ece03a548d7b9bc90729c2a6ac7f93e87e5feb15663d26b757d6fd` |
| `src/player_hold.cpp` | `0b8cee44972e5a56427a2efd0898a946ca53d7f0a435032097a0ea7aaab370a2` |
| `src/player_hold.hpp` | `022e162652fce72adc6e48b483450274024ff3a6d6ece0b82cbde3b3c3755ed0` |
| `src/task.cpp` | `f193077f71550ef3ddabf7b8e2884636a812e4ad1a02ba0d49dbd50b3fa7d1ab` |
| `src/task.hpp` | `31c5cba241695d0f3fe2a65e30de351afebbbafafb1dc6913243ec10bcfbea74` |
| `src/player_physics.hpp` | `ba300cc2aa4b393871fff06b9740c599f10aae6a519c14409fb20a58fdddb7d2` |
| `src/player_task.hpp` | `b3a56feb51a789af5366d8ecfe651a433f33c46460f32207372ae0101b2f183e` |
| `tests/player_startup.cpp` | `743b3f6e7e3c89ccd0034d372c04271b3924e52a06d017f608d6da02083a6177` |
| `tests/task_lifecycle.cpp` | `08f0341b6c86ba3a194596fc8e34d9c226e3c70d9dd6e861d3811be5060e4e45` |
| `tests/native_player_startup_probe.cpp` | `1462d7e8608a748a8021de7d5cb9920e06b9d49b90e7b0a560e6e3ff883f5a44` |
| `tools/verify_original_player_startup.py` | `04b8ea45477e23f09fc5f2ab2a72d9068e2bec9444921cba4114ae043facc496` |
