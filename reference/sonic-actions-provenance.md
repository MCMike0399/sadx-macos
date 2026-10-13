# Grounded Sonic action predicates

This extends the frozen initial Sonic slice with actual `496f50` actions1–5 and their bounded native predicates. It preserves caller-selected x87 context, including the current hardware candidate main-thread CW007f and worker CW027f; neither is silently selected as a native default. Original executable SHA256 remains `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.

The previous passed source/probe/verifier files and production report were copied to `/private/tmp/sadx-sonic-action-before`. Their SHA256 and original28,800-case production status are retained in `sonic-action-before-ledger.json`. That report had zero stored-state differences and2,840 status differences. It is a distinct earlier scope.

## Evidence

The complete14,592-byte `496f50` Ghidra dossier is retained in `ghidra-gameplay-live-recovery/sonic-action-checks-complete.evidence.json`, Evidence `ev_e3a6565f7df411064b095eb1715902fa6b6d847bc7600ed63256bb8673543971`. The stdio operation completed but its response exceeded the10MB transport budget; the complete retained record was extracted without another import. P-code truncation is explicitly recorded. Entry annotations are caller assertions, not discovered execution.

The follow-up source-backed Ghidra dossiers are under `ghidra-action-fp-recovery/`:

| Source owner | Evidence |
|---|---|
| `43bf40` pressed-button query | `ev_2c23e20f87d45ee37de79f300419d674042df35ac11b3bf0759041dccdae3d72` |
| `4b83f0` hit-task classifier | `ev_7f4f2f55ae6d637397ab65e10d37a8f6bb1d1a6380713ccba1c456ef5dddb2f5` |
| `4b8320` ten task action resets | `ev_83e248e4e259959afaecd979ca38f5c69a8158dfb6e56f354fad029d00c45b14` |
| `495c30` animation request | `ev_599869988d2acc28533eb423a0c9fe842fbc80a2f6e6a7ee18c33f744d414f80` |
| `494ff0` idle transition | `ev_a6c337883fc6a6413ffb6d0bec11b06ea16718b952a0a27623ec9d1c99814808` |
| `4429c0` turn/braking query | `ev_ee4e5bb0aa8d21fb294fd4eb881b66edf773eaccb6c333d9a6092a1401d99f9c` |
| `43bfe0` collision-normal direction class | `ev_a721f42023220b558ef39a618d72c61ccd9f3654ff424766e2acf144e67db235` |
| `495b50` animation request | `ev_dcc4a7171a2e922c832ad86e33c1ba67bbc2ace6cc8b16a04e0b12f1fb5056cd` |
| `495ba0` animation request | `ev_1d8515e73812a8b1d495871acf3e7c5c1b748cd7d6914d16c647302273c0a99f` |
| `495080` charge tick | `ev_87d17081f86ed7742f69c053624f20cd15c20ef67dbaf1d731f8758a813c2d4a` |

Earlier live recovery contains `495fa0`, `492f90`, `494f70`, `495e60`, `493120`, `496ee0`, `442680` and `442570` dossiers. Exact LLVM instructions determine predicates, widths and operation order. In particular Ghidra types some equipment integer bytes as float union reads; native code follows actual TESTB/MOVW instructions, not that inferred type.

## Native behavior and ownership

`sonic_action_checks` now composes the actual ordinary grounded, running, rolling, charging and release paths. Closed branches can keep idle, enter running action2/requested animation9, enter sliding action19/animation31, fall, stop running, brake, and recover several jump/spindash transitions. Their event, held-button release, grounded/fall, jump, hold, charge and special-stage tests occur in source order. Mandatory sound, animation, collision or effect operations remain required native owners; earlier state writes survive failure.

The source pad contract separately supplies per-slot held `3b0e7f8` and pressed `3b0e800`, plus mutable masks `909f94/98/a4/a8/ac`. No source mask default or physical-controller label is guessed. It does not remap Nintendo/Xbox physical buttons, invent SDL raw axes, update keyboard ramps, or sample an OS backend. Those remain their existing input owners.

The event action-list reference at+4c has a distinct native event identity domain; unknown, observed null and non-null references are separate. Task+0c signed counter and actor word+6 have one authoritative live task owner. Snapshot values are used only for capture-only actors, with no stale fallback when a live field is unknown. Physics charge0, jump timer8, idle timerE and timers80/82 are appended optional source facts. Genuine successful calloc establishes zero through the startup/task producer; omitted event work never establishes a null action list.

`43bf40` calls actual `4b83f0`, which consumes real collision links, actor ID8, word+6, signed int32+0c and exact `7df1c8` threshold. Normal reset paths call the real `4b8320` loop over ten source task references. The owning field is neutrally named `tasks_at_3c5a27c`; the purposes of those tasks are not inferred from their caller. Source null task entries skip writes; a failure on a later task preserves earlier action-byte stores.

GetHold uses the actual integer-only `43b9c0` owner and the same registry/arena resource owners; no FPU wait or fixed hold result is introduced. `43bfe0` uses the real normal owner, FPATAN of negatedZ/negatedX, binary64 source factors, CRT `_ftol` and wrapped angle distance. A no-write/unknown normal cannot become a zero vector. This math path is implemented but its complete normal producer is not exercised by the new prefix fixtures.

`495080` preserves signed timer thresholds120/210/300, byte equipment tests, charge comparison/increment (exact source10.0/0.4 operands), release speed copy, state/animation/flag stores, and the exact sound/effect call order. Alternate-model animation requests and enhanced-charge creation use the real task factory only when their named native callback owners are supplied. Unknown callbacks do not create successful effects. Actual source-pool exhaustion is treated as the observed null creation result, while other incomplete factory failures retain diagnostics.

The five-argument `423f50` contract preserves the live frame taskwk as an explicit typed argument index, rather than coercing its native identity into a scalar address. Collision lookup pointer returns similarly preserve unknown versus observed null native task-work views. Sound remains unavailable by default. Completing one callback never certifies the remainder of a game frame.

Main dispatch retains its movement position diagnostics across local-speed and later animation calls. The independent auxiliary-effect rename changes the old misleading `Task.shadow_work` union view to `auxiliary_effect_work`; Sonic uses the new canonical owner name.

## Verification

`original-sonic-actions-comparison.json` records30,192 original-instruction cases across all12 masked PC/RC profiles. There are zero stored-state/return differences and zero mandatory-boundary differences. The2,172 exception-status differences are retained; strict `passed` is false and the narrower `stored_state_matches` is true. The CLI exits nonzero for stored or boundary discrepancies, not for documented status differences alone.

The oracle executes actual source predicates, their real GetAnalog/GetHold/length leaves and recovered actions1–5 in private fixture memory. It stops at actual CALL instructions before unbound audio/task/model/animation/normal procedures, or at explicitly unported dispatches. It never returns a fabricated success from those procedures. Its pad masks, event fields, task tables and scene bytes are explicit experimental fixtures, not runtime defaults. Additional native tests cover live-state precedence, unknown references, partial writes, no-button nullable-player guards, dynamic mask requirements, exact five-slot callback arguments, all12 control profiles, and unknown effect/audio owners. Optimized and UBSan runs pass.

The constants52-byte asset is extracted from the original PE in the header's13-address order; no constants were added after source freeze. Source/test/probe/verifier, fixture and library SHA256 are retained. Private builds use current task/work source before a private current-ABI core archive; production archive linkage is independently refreshed by root.

## Remaining required work

The remaining `495fa0` event subcommands and `496f50` actions6–87 are explicit incomplete dispatches. The exceptional damage `492d70` suffix after its actual notification call remains incomplete. Collision wall-turn `43c060`, upgraded release `492710`, special jump `4b8470`/`495c80`, real effect callbacks `495a50/4940e0/494120`, idle animation `491660`, sound implementations and several scene lookup owners remain required. Their retained dossiers do not themselves prove native implementation or physical execution.

The main animation table/model tree/common tail remains unresolved as described in `sonic-player-provenance.md`. This slice does not complete a playable native game frame. Windows updated-mod hook loading, actual hardware FPATAN/status parity, input-event cadence and physical controllers remain independent verification boundaries. Hardware testing remains deferred; no tester or Windows process was launched here. Reconstructed C++ uses instruction/decompilation evidence and the existing pinned native math owners/notices; no new third-party code or license grant was introduced.
