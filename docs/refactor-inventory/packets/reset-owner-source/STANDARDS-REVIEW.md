# Independent standards review

Reviewed head `e8984111fc218cfcf2d57bc0471c1db6ab75a1dc`, production Source
`744d313ed6a7f02c7c3c5b47ecb78078dca592aa`, against immutable base
`6255cc6cabcf14adb32867ea011f7491dbbddd2d` using the requested three-dot diff
and commit range. Branch: `packet/reset-standards-review-cfbabec6`.

**Source: bounded standards acceptance. Binding/tests: changes requested.**
No new semantic-core/ownership blocker found. Shared cleanup owns recovered
policy; reference adapters retain availability, ABI/site translation and support
cuts. Private fact storage prevents stale editor reads; mandatory borrowed
validation survives invalidation. Frame/editor aliases, real tokens, CRT write
ownership, lazy callback rereads and native media bindings are coherent. No
unchanged reset algorithm was merely relocated; native reset dependencies remain
independent of guest execution. This is no fidelity approval.

**R1 — P2, documented maintainability violation:**
`tests/unit/reset_order_characterization.cpp:50` and `:79` compress independent
assignments, branches and lambda bodies onto single lines, contrary to STYLE §10
(“one statement per line”, no chained lambdas). Tests are outside the readability
checker's scope, so this is not a tooling-enforced finding. Expand the new test
bodies using repository formatting. Preserve every emitted byte, case order,
input and expected matrix; verify against the frozen matrix afterward.
Additionally, numeric scenario coupling at `:75` and
`tests/unit/reset_leaf_contract.cpp:25` is **possible Mysterious Name**, a heuristic
judgment: name scenario/setup decisions while retaining frozen numeric output
identifiers. Do not widen this repair to historical compressed consumers.

Checks actually run: seven fake-only observer tests pass under pinned Python 3.14;
53 Source, 55 scope, 661 immutable-old Source, 21 qualification-artifact, eight
recovery-artifact and nine oracle-report hashes match. Read-only decoding confirms
all five stored ABI/original domains (688/28,800/3,000/600/1,956), retaining all
1,956 raw undefined void-register differences. Stored 55/113/15-row matrices match.
No builds, library loads, original/native execution, mutations or reserved inputs ran.

Initial capture defects and retrospective recovery remain disclosed. Sonic
2,840/2,172 and Title 270 FP partials, Task207 source FAIL, reference initialized
StopPCM, native nonnull release, mission/audio/allocator/replacement limitations
remain open. The 53 absent corpora preclude complete protected-input refresh.
Whole Session/loop/Sonic conformance is not granted. FINAL-UNRUN remains unrun;
Root owns independent reserved admission, integration, closure, journeys and gate.

## File dispositions and evidence appendix

The complete new bodies below were directly inspected, including helper/test
implementations. Acceptance here concerns bounded standards only. STYLE §12 was
applied, including side-effect order, arithmetic/literals, explicit seams,
provenance, evidence, hooks without speculative frameworks and platform boundaries.

| Complete new file | Disposition |
|---|---|
| `src/engine/reset_control.hpp` | Accept: plain owners and inseparable lazy editor observation. |
| `src/game/game_cleanup_internal.hpp` | Accept: reset and every recovered leaf policy use semantic lazy access. |
| `src/game/game_reset.hpp` | Accept compatibility declarations; no behavior remains here. |
| `src/game/game_reset_leaves.hpp` | Accept compatibility declarations; no behavior remains here. |
| `src/guest/game_loop_reference.hpp` | Accept transported declaration lineage; no whole-loop rewrite credit. |
| `src/guest/game_reset_reference.hpp` | Accept reference state/call/result transport. |
| `src/guest/game_reset_reference.cpp` | Accept pending payload, call-site mapping, live reads and stop conversion. |
| `src/guest/game_reset_leaves_reference.hpp` | Accept reference facts and explicit support/result contracts. |
| `src/guest/game_reset_leaves_reference.cpp` | Accept all leaf routes/access bindings; policy shared, cuts explicit. |
| `src/guest/reset_control_reference.hpp` | Accept private values/knowledge, guarded imports and borrowing. |
| `src/guest/reset_buffer_tokens.hpp` | Accept stable real tokens, null mapping, foreign rejection; no resource-lifetime proof. |
| `reference/packets/reset-owner-source/capture_consumers.py` | Accept complete serializer, wrapper, trace, fixed-domain and exclusive-output bodies. |
| `reference/packets/reset-owner-source/test_capture_consumers.py` | Accept every fake test, including actual comprehension tracing and reuse rejection. |
| `reference/packets/reset-owner-source/run_reset_faults.py` | Accept finite faults, build/setup discrimination and restoration; inspected only. |
| `tests/fixtures/reset_composition_fixture.hpp` | Accept bounded borrow interface without fixture-dependent NativeGame layout. |
| `tests/probes/reset_composition_fixture.cpp` | Accept unforced real-layout TU and lifetime owner. |
| `tests/unit/reset_native_contract.cpp` | Accept failure-prefix, guarded-read, token and callback tests. |
| `tests/unit/reset_pending_contract.cpp` | Accept all positions/outcomes and complete pending argument presence/payload. |
| `tests/unit/reset_leaf_contract.cpp` | All bodies inspected; R1 scenario-naming judgment. |
| `tests/unit/reset_order_characterization.cpp` | Complete fixture/emitter/matrix inspected; R1 changes requested. |

For remaining REVIEW-SCOPE members, disposition is **binding/consumer closure
only**, with these explicit boundaries:

- `game_cleanup.{hpp,cpp}`: every new native access body and shared-policy call
  inspected; unrelated retained cleanup behavior receives no new fidelity claim.
- `audio_playback.cpp`, `mac_audio_playback.mm`: actual close/reacquire and cold-start
  creation bindings inspected, including platform annotation and support limits.
- `native_game_reference.{hpp,cpp}`, `native_session.{hpp,cpp}`,
  `native_file_select_display.cpp`, `sonic_player.{hpp,cpp}`: relevant owner,
  import/default/export, reset dispatch and reached reader/guard closure inspected.
  Large unchanged algorithms and owner breadth remain campaign debt.
- `game_loop.hpp`, `guest/game_loop.cpp`, `game_mode_reference.hpp`,
  `music_state_reference.hpp`: migrated include/owner transport inspected; no
  whole-system conformance inferred.
- `tests/unit/sonic_player.cpp`, `native_logo_scene.cpp`: every new alias/read-order
  and held-handle callback body inspected. Unforced fixture isolates the macro ODR
  trap; logo test deliberately avoids dereferencing dead-arena work handles.
- All seven changed `native_*` probes in REVIEW-SCOPE: explicit include/control
  binding migration inspected. Complete cleanup reset export uses real tokens;
  composed export preserves word/halfword widths. Other inherited probe exports
  remain bounded historical infrastructure.
- Remaining four changed unit consumers (`game_loop`, `game_reset`,
  `music_state_reference`, `save_frame_characterization`) and `manifest.tsv`:
  owner/include closure and four new program registrations inspected.
- All six changed oracle scripts: dependency/hash binding additions inspected;
  original algorithm, inputs and expectations retained. Makefile's reference
  links and separately compiled unforced fixture recipe inspected.

Evidence root: `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-6a2279e4`.
Inspected QUALIFICATION, ATTEMPTS, RECOVERY-BINDING, VOID-RETURN-BINDING,
CAPTURE-CONTRACTS, SOURCE-FREEZE and REVIEW-SCOPE, alongside reset-owner context,
setup and baseline. Existing oracle/fault/closure results were inspected, not rerun.
Decoded old streams from `build/reset-consumer-old-v2`, stream-player v4 and
WinMain v7 against `build/reset-current-qualified`; only array element-type metadata,
missing monotonic case labels and the undefined void return register were normalized.
No input/output buffer bytes were excluded. Raw artifacts were untouched.

The first combined decode command exited 1 after successful stream comparisons
because it looked for `build/old-order.txt`. The corrected matrix-only command used
the tracked packet `old-order.txt` and exited 0; pending and leaf files also match.
This review neither refreshes manifests nor treats metadata seals as execution.
