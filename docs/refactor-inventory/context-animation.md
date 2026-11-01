# Animation contract source context

Read-only research at `a0aa3426e4dc2781a4917cd15b0322fee175bdd3`, 2026-11-01.
Packet branch: `packet/context-animation`, isolated assigned worktree
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-243a9cf5`.
Companion `context-animation.json` records current full-file SHA-256 fingerprints,
tracked line anchors, cached evidence status and historical receipt dependencies.
This is source context and a proposed extraction boundary, **not** a module disposition,
formal review, fresh verdict, implementation or gameplay recovery. No builds, tests,
oracles, mutants, runtime, providers, network, storage transfers or delegates ran.
Only these two documents are changed. No policy, inventory shards, source, judge,
HANDOFF, STATUS or Main edits are part of this packet.

## Concrete finding

`GammaActorAssets` publishes `shared_ptr<const PlayerAnimationTable>` from the
legacy animation API. Its `PlayerAnimationRecord` wraps every field in fixture
availability. The app converts Loaded into an action identity, KnownNull into a
known reference with no identity, and Unavailable into an absent outer optional;
then `bind_animation_table` publishes the same shared table into the initialized
player. This is an actual imported-contract leak, not a conclusion from the word
`optional`. All 91 rows and their scalar metadata exist even when their action
payload is unavailable. Preserve declaration 61's genuine null, request 0's Stand,
78 declarations, constructed row 72's explicit unavailable payload and its reason.
[contract](../../src/runtime/gamma_actor_assets.hpp#L11),
[conversion](../../src/apps/native_gamma_actor_assets.hpp#L205),
[publication](../../src/runtime/native_gamma_binding.cpp#L150).

Do not migrate Gamma alone into a plain-looking shell while player animation still
consumes fixture rows. The common table/state/resource consumers include Sonic,
its initialized-table injection, startup, transition generation and release. Sonic
remains in the complete campaign although its implementation and animation CPPs
are absent from `native-sources.txt`; Gamma core is in that list. The existing
campaign includes excluded maintained behavior (ADR-0009), not just the native
route. No `PlayerAnimationState` declaration exists in this tree: the current
owner is `PlayerPhysicsState` plus `PlayerAnimationTail`, not a hypothetical type.
[physics](../../src/player/player_physics.hpp#L15),
[tail](../../src/player/player_animation_state.hpp#L18),
[Sonic call](../../src/player/sonic_player.cpp#L30).

## Source meanings and missingness

| Representation | Source meaning and migration obligation |
|---|---|
| Outer record/action/motion/node scalar optional | A caller/fixture did not establish the field. Not source gameplay absence. Missing row in `records` is separately `unresolved_resource`; missing outer optional is `unknown_field`. |
| `AnimationResourceReference::id`, `SceneResourceReference::id` | Known null versus an artifact-scoped resource identity. Null channel keys can mean use the object pose; null action/motion dereference fails. ID magnitude does not classify an address. |
| `ActorResourceStatus` | Imported payload closure: Loaded, KnownNull, Unavailable are distinct. Unavailable includes source provenance/reason, not an executable dispatch target. |
| Generic physics/tail default | Fixture availability is unknown. Actual successful 512-byte player calloc establishes zero/null, including table, join, buffer, request/current/previous, mode, timer, flags and frame fields. Do not manufacture these defaults for arbitrary fixtures. |
| Allocated action/motion header optionals | Partial source allocation/initialization. An 8-byte action and 12-byte motion are malloc allocations; source establishes fields in order, and can fail before later stores. This is different from initialized player calloc and requires per-stage characterization before eliminating availability. |
| `PlayerAnimationNode::keys/counts[2]` for MDATA2 | Decoder establishes known-null/zero for a channel absent from that layout; consumers select only reached channels. This is not evidence of an uninitialized scale field. |
| `scale_pointer_bits_at_8` | Genuine source numeric quirk: outgoing MDATA3 uses its scale pointer word as position-key count; declared position count becomes angle count. Preserve consumed raw bits separately from normalized ID; do not reinterpret a pack token as those bits. |
| Key backing extent | Caller-observed contiguous storage, not declared count. Out-of-declared-count source reads can reach adjacent captured bytes. Missing backing must stop at the reached read after preserving earlier output. |
| `field_masks`, `completed_records` | Initializer-local completeness accounting for the adapter/result. Not fields of PL_ACTION and not source game decisions. |
| `SonicAnimationOperation` | Only `address` is an instruction PC. `read` is an operation kind; `slot` is a loaded-value index or 65535 literal sentinel; `index` is an export ordinal; `record` is a table row; `field` is a byte offset; `bits` is a scalar encoding/null literal. Preserve meanings, not hex spelling. |
| `SonicAnimationRecipe` | Export selector/index, object count, signed-mode byte, next index, binary32 transition/speed words. Its current use in the decoder computes required node counts. These are not all executable addresses. |

Sources: [types](../../src/player/player_animation.hpp#L7),
[startup producer](../../src/player/player_startup.cpp#L57),
[decode](../../src/player/player_animation_decode.cpp#L32),
[MDATA consumption](../../src/player/player_animation_blend.cpp#L34),
[operation interpreter](../../src/player/sonic_animation_table.cpp#L6).
No whole field is declared dead merely because a synthetic probe omits it. The
unused `PlayerAnimationKey` declaration is not evidence that raw key bytes are dead:
production algorithms consume `PlayerAnimationKeyBytes` instead. Source-uninitialized
malloc bytes may remain genuinely unavailable until written; the A5 probe fill is
an experimental capture, never a game default. SDK names alone do not prove such
bytes initialized, nor justify removing them.

The prior 13-header review resolved selection `source_key` triplets as origin kind,
character and export/constructed ordinal. `ActionHandle` and CharacterPack
`ResourceId` identify validated pack records, including aliases; neither their
width nor a hexadecimal value makes them guest addresses. Gamma join references,
wire-model records and matrix traversal indices retain those meanings. No token
reinterpretation, magnitude heuristic or invented callback semantics is proposed.
[prior review](reviews/runtime-asset-contracts.md),
[key validation](../../src/apps/native_character_selection_assets.hpp#L41).

## Canonical owners and all current consumer families

| Owner / producer | Borrowers and observable boundary |
|---|---|
| `PlayerPhysicsState` owns words 120/122/124/126/128; its `animation_tail` owns 12a/12c/12e/130/134/140/148 and join pointer | Startup zero producer; initialized native view; Gamma request/current/table/weld bindings; Sonic request predicates, initialization and main common tail; animation reset/next/update/blend/release. `PlayerAnimationView` borrows physics and a tail; units/probes sometimes pass a separate tail. Preserve this fixture contract at the adapter rather than duplicate native state. |
| `PlayerAnimationTable` owns row map; each row owns the six PL_ACTION facts | Sonic initializer mutates in place; Gamma loader builds 91 rows; table consumers in both animation CPPs lazily read action/count/mode/next/transition/speed; Sonic stores initialized shared table; startup stores known null. No other source row-reader family was found by repository search. |
| `GammaActorAssets` owns declarations, recipes and one shared immutable table; `PlayerSystem` uniquely owns assets | App loader validates pack/action/motion, Session permits attachment once, binds weld/cursor resources first, then moves assets into owner. Players share table identity; do not create per-player tables. Gamma's initialized view returns references into canonical physics, not copies. |
| `SonicPlayerRuntime::initialized_animation_table` borrows shared initialized table; `animation_runtime` borrows animation resource/arena/constants owners | Sonic initialization rejects unpublished/null differently at 49ab47, then stores the table and request 0. `call` routes 44a800 to the implemented shared updater before generic external dispatch; preserve lower diagnostics, caller boundary and failure propagation. Do not replace implemented behavior with a callback. |
| `PlayerAnimationResources` owns action/motion/node/key maps, borrows scene objects | Reset/update/transition lookups retain unknown/null/unresolved distinctions. Keys share immutable bytes, offsets and extents. `DecodedPlayerAnimations` owns its object map; returned unique_ptr keeps it stable and `resources.objects` points into it. Moving the pointee/map or letting it die invalidates that borrow. |
| `DecodedPlayerAnimations` owns decoded Sonic export/resources/locations | Decoder reads 149 export slots without executing DLL code; per-kind source-pointer maps preserve alias IDs, and caller-selected first ID gives artifact namespace. `sonic_animation_recipes` derives maximum node count per aliased motion. Model payload is unresolved here. Its existing use is unit/source attachment, not a composed native Sonic actor loader. |
| `PlayerAnimationAllocatedAction` and `PlayerAnimationAllocatedMotion` own typed header facts linked to TaskWorkArena allocation handles | Update allocates/publishes; producer writes object/key metadata; release frees generated keys; default path releases data, motion, then action, clearing typed references in order. Arena owns generated MDATA/key bytes and references; raw malloc headers are not a second initialized snapshot. Partial allocations are retained when an operation fails. |
| TaskWorkArena owns player allocation references at 138/13c | Animation modes resolve live interior speed.x/self charge at consumption, requiring same allocation and exact offset. No duplicate cached speed/work scalar. Startup supplies these aliases. Shared scene owns hierarchy/poses; traversal visits repeated identities and stops at actual capacity 111, not deduplication. |
| `PlayerSystem::gamma_weld`, shared CharacterPack SceneState and cursor storage | Separate geometry owners. Session declares scene before players; reverse destruction retains scene through player destruction. Weld references and cursor spans borrow stable scene data. Keep these owners separate from table migration, preserve join pointer borrowing. |

Anchors: [initialized view](../../src/player/player_initialization.hpp#L99),
[view binding](../../src/player/player_startup.cpp#L431),
[assets attachment](../../src/runtime/native_gamma_assets_native.cpp#L4),
[geometry binding](../../src/runtime/native_gamma_resources_native.cpp#L4),
[system](../../src/runtime/native_player_system.hpp#L23),
[decode owner](../../src/player/player_animation_decode.hpp#L6).
JSON enumerates exact producer/consumer paths, probes and unit binders. No existing
guest animation-specific adapter was found: legacy reference diagnostics currently
live in player CPPs/probes and Sonic. A destination adapter is proposed, not present.

## Algorithms to keep in the game system

The implemented dispatcher is real recovered behavior, not fixture infrastructure.
It publishes previous animation and clears flags before later reads; request changes
reset mode/timer except modes 0/2/14, otherwise timer wraps at 16 bits. Mode 0 stores
current/mode/start/frame before allocation and publishes action/motion/data in source
order. Modes 1/2 perform same-object/non-unit-transition selection and interpolation;
3/9/10 advance and repeatedly wrap, with live speed/charge scaling; 12 wraps without
increment. Modes 4/11 advance to next; 5 clamps; 6/7 reverse; 8 transitions toward the
saved next. Default cleanup remains implemented. NaN/unordered paths, signed modes,
retained x87 versus binary32 spills, negative/nonprogressing periods and low-byte
`next` loads are behavior, not diagnostics to relocate into guest.
[updater](../../src/player/player_animation.cpp#L64),
[source](../../reference/ghidra-action-fp-recovery/animation-44a800.pseudocode.txt#L16).

Reset selects end minus one for reverse modes, consumes and clears start for
3/9/10/12, otherwise zero. Next publishes requested/current, reads target signed
mode, ORs flag bit 1, stores low-byte next, then resets frame. Blend selects source
call boundary from type bit 4, calls the implemented transition generator, and
retains nested failure. Release reads requested row's count (not current row),
walks MDATA2/3, frees nonnull channel pointers and only then clears pointer/count.
Do not zero count for an already-null channel without source evidence.
[helpers](../../src/player/player_animation.cpp#L52),
[source next](../../reference/ghidra-action-fp-recovery/animation-446910.pseudocode.txt#L8).

Transition publishes destination object before hierarchy traversal and motion reads;
visits root/child/sibling with the fixed 111-entry boundary; determines channels
from type bit 4 and allocates 32-byte two-key outputs only for present channels.
It preserves MDATA2-to-3 count relocation, outgoing sample/incoming first-key order,
pose fallback for null channels, signed16 angle delta/unwrap and CRT truncation.
Output frame 0 and earlier axes remain when a later key read fails. Vector fraction
retains extended precision; angle fraction spills to float. These algorithms belong
in readable shared player code with narrow numerical helpers, not in a reference
interpreter or a generic callback framework.
[producer](../../src/player/player_animation_blend.cpp#L50),
[assembly](../../reference/animation-blend-43f2a0.asm#L10).

The diagnostic layer currently combines `known`, `record`, `action`, `motion`,
`scalar`, key range checks, arena failure translation, source PCs and result enums
with these algorithms. Split lazy fact access/reporting from the actual decisions,
using logical phases/access seams as existing fog/scene-position patterns do. Do
not eagerly validate the full table/resource graph: a path may never read an
unavailable action. Game errors for genuine null/unresolved resources and explicit
unsupported behavior still need semantic reporting; they must not become success.

## Sonic initializer evidence and ordering

The tracked assembly and operation metadata pin actual 7cec90, 7,674 bytes,
SHA-256 `4fb64e48d45e6008328627f24c9065e89240ec2b5458cedfb73a6ce26d183944`.
It loads `___SONIC_ACTIONS` through source global 38f6e7c and writes BSS table
3c56210: 147 rows, 882 field stores, 145 export reads, 127 distinct ordinals, maximum
148. Two action stores are literal null at rows 75/76. SDK's 146-element declaration
is a locator discrepancy, not the output bound. BSS zero is genuine pre-producer
source initialization; arbitrary missing fixture rows/fields are not proof of
uninitialized source storage. The current initializer leaves old later rows intact
if it stops on retry, including existing availability and shared identities.
[assembly](../../reference/animation-table-7cec90.asm#L7),
[metadata](../../reference/sonic-animation-operations.json#L1).

The interpreter's local mask records which of six fields were written in this
invocation; reaching 63 increments completed count then marks 255. Static inspection
of all operations found **zero export reads with any newly touched row incomplete**.
The two literal-null rows cross a row boundary without an export read; all rows
have all six stores by return. This observation is not a new executable judge or a
proof that arbitrary store reordering is safe. Slot-loaded aliases, explicit load
order, retries and reached failure prefixes must survive the replacement. Existing
unit examples stop at missing ordinal 122 after row 0 (7cecd2), or missing ordinal
123 after two rows (7ced0c), and retain row 146 from earlier success.
[unit](../../tests/unit/sonic_animation_table.cpp#L5).

Use named initialization/content operations with typed values in the game system;
keep source read/store PCs, mapping of logical steps and reference missingness at
the adapter/import boundary. Merely replacing hex tokens with enums while retaining
an instruction interpreter is not the extraction. A final recipe array alone does
not prove read/publication ordering. Keep the current decoder's content-count
consumer when replacing either generated header.

Cached `animation-producer-7cec90.json` is an **error response** (no procedure at
that address), not a successful dossier. The tracked assembly, operation metadata,
recipe metadata, original-instruction report and provenance ledger are the usable
anchors; the ledger's claimed successful dossier ID is not independently present
there. Cached xrefs for table 3c56210 show 49ab47 and 7cec98; global export xrefs
contain many other original-game readers. This packet maps existing maintained
consumers, not every unrecovered original-game export caller. No new decompilation
was attempted. External SDK and absent symbol database are labeled in JSON.

## Gamma dependencies and deliberately unresolved behavior

Importer folds the pinned linear Gamma initializer 7ca300..7cb560 into 91 records,
using known-bit register tracking and BSS initialization, not runtime emulation.
Sentinel folding distinguishes declaration indices despite pointer aliases, then
inverse reconstruction verifies recipe bytes. Actual source declaration table is
78 exports, null only at 61. Only request-0's payload is newly imported; existing
selection aliases can already be Loaded. Other valid declarations stay Unavailable
because payloads lie outside the reached import closure. Constructed row 72 has a
proved producer but unavailable payload. These are missing imported facts, not
source-uninitialized actions. [fold](../../tools/import/gamma_declarations.py#L20),
[payload closure](../../tools/import/gamma_actor_resources.py#L154).

Gamma constructor order: PInitialize, reacquire player from captured motion,
bind table, request 0, collision, textures, weld, callbacks, extra allocation and
cursor initialization, then setup/material/effect/common-tail operations. Preserve
canonical references through the invocation and source callback reacquisition;
never invent table playback in a loader. `native_gamma_binding::set_motion` still
throws `native_gamma_set_motion`; probe lower bindings record receipt calls and do
not run actual player animation. The CharacterPack contains pack-native handles,
but no composed Gamma bridge to `PlayerAnimationResources`/transition playback was
found. Completing this refactor does not authorize inventing that bridge's behavior,
recovering callbacks, changing the STUB, or claiming native rendering. Material,
setup/effects, model matrices/display and other modes retain their present limits.
[constructor](../../src/player/gamma_selection_core.cpp#L90),
[missing motion binding](../../src/runtime/native_gamma_binding.cpp#L241),
[receipt binding](../../tests/probes/native_gamma_selection_core_probe.cpp#L236).

## Existing judges and actual receipt limits

All numbers below are read from tracked historical receipts; none was rerun.
The companion JSON includes receipt SHA, compared input hashes and omissions.

| Receipt | What it establishes; what it does not |
|---|---|
| `reference/oracle-reports/sonic_animation_table.json` | 103 fixtures, 149 slots, 147 rows, zero byte/FP differences, historical passed=true. Complete inputs only; no missing-export injection, retries or per-store trace. All six listed native input hashes match now, but operation header/emitter are omitted from that report dependency list. |
| `reference/oracle-reports/player_animation.json` | 16,440 cases, zero stored/boundary mismatches, 4,140 FP-status differences, passed=false (partial). Four synthetic rows, object_count 0 release, source stops before blend; not full allocation lifecycle or real 147-row composition. Four declared input hashes match now. |
| `reference/oracle-reports/player_animation_blend.json` | 7,944 cases, zero stored mismatches, 1,668 FP-status differences, passed=false. Single-object MDATA2/3 fixtures; distinct 32-byte allocator receipts/A5 adjacent backing. Four declared input hashes match now. Not real allocator policy, graph capacity/cycles, Windows hardware or all malformed ranges. |
| Old `original-player-animation-blend-comparison.json` | 2,808 stored differences and 1,032 status differences remain a historical failure. Repaired and repaired-v2 reports have zero stored / 1,668 status differences. Do not erase the old failure or treat its differing counts as current regression coverage. |
| Gamma `proof.json` / canonical Gamma report | Historical default 1,422 value-exact / 498 FP partials, exit 2; holdout 1,080 value-exact / 690 FP partials, exit 2; seven compiled faults caught. Tests constructor/common-tail receipts, not PSetMotion playback or rendering. Proof's Gamma header hash has drifted; other six listed files match. Prior character-owner report repeats 1,422/498, not a new animation judge. |

The three animation source verifiers currently return success when values match,
even when report `passed` is false due to FP differences. Exit 0 therefore does not
mean strict FP pass. Canonical summaries retain only ten examples and truncate
input arrays; `tracked_summary.full_sha256` names a larger raw report, not a full
per-case identity ledger available in the summary. No historical animation-specific
mutation runner/receipt or reserved animation holdout was located in tracked paths;
`tools/gate/mutate.py` contains no animation target. Gamma's seven faults/holdout
qualify its constructor slice, not the shared animation owner. Do not claim those
receipts cover table/state missingness. Source-input hash agreement is bounded to
the listed paths: tail/physics/arena/CPU and operation transitive inputs are not
all bound by these animation report dictionaries.

Existing registered units: `sonic-animation-table` (partial/retry/null),
`player-animation` (actual DLL, 73-node transition/release, 12 PC/RC combinations,
10 outgoing MDATA3 rows, bounded versus full backing), `player-animation-decode`
(malformed PE, namespaces, alias graph/extent), `sonic-player` (routing/lower
failure), player startup and initialization, native Gamma assets/binding/weld tests.
These are source specifications, not new passes. Probe ABIs and source ranges are
listed in JSON. The ignored `player_animation.constants.bin` is absent here; the
original DLL, raw libraries and full report artifacts were not restored or run.

## First coherent bounded extraction and prerequisites

Recommend **the complete shared animation table/resource access and motion-state
consumer boundary**, not the whole player/physics/rendering system. It includes
all public animation types, both implemented animation CPPs and helpers, Sonic's
complete table producer/content-count decoder use, startup/initialized views,
Gamma asset conversion/publication and Sonic's actual animation injection/routing.
A prospective semantic motion-state owner must migrate the existing physics words
and tail together (or borrow the unchanged canonical owner until that atomic step);
it must not duplicate the first five words in a new state. Join storage remains
its existing owner and is borrowed. The source PE parser belongs at importer or
reference boundary; its graph/alias/key-backing semantics survive. Broader Sonic
physics/dispatch rewrite remains campaign debt, not an excuse to exclude its table
and animation callers. The small public contract migration can be split into
implementation attempts, but the whole owner plus all consumers is the review unit.

Before source edits, prepare a separate evidenced judge/missingness packet:

1. Freeze complete baseline outputs and individual FP-partial/boundary identities
   for all three animation judges plus affected Sonic/startup/Gamma consumers. Pin
   the actual transitive source closure, operation header/emitter, resource backing,
   allocation observations and constants. Existing example summaries are inadequate.
2. Characterize every first-reached missing table pointer/row/field, export read,
   action/object/motion/node/key dependency and arena reference, with earlier stores
   retained. Exercise retry into a seeded table with distinct old rows, repeated
   export identities, null versus unavailable, non-PC field meanings and all six
   field masks; verify the zero-partial-row-at-read observation independently.
3. Characterize partial allocation/publication failures, exhausted allocator results,
   free/null count behavior, live 138/13c aliases, hierarchy repeats/capacity,
   source-uninitialized malloc bytes versus source calloc, and MDATA3 raw-count
   behavior with bounded/adjacent backing. Preserve status/control and nested errors.
4. Add discriminating compiled faults for eager validation, unavailable-to-null,
   row-count 146, next high-byte consumption, cached aliases, reordered publication,
   declared-count clamping, normalized raw pointer counts, spilled vector fraction
   and release order. These are proposed judges, **not executed/caught mutants**.
   Reserve distinct final cohorts; do not adjust old expectations or tolerances.
5. Establish a plain typed native contract plus lazy reference fact access mapped
   to logical phases, with one shared readable implementation of recovered choices.
   Bind existing Gamma/Sonic/startup callers atomically; keep unresolved native
   STUBs loud. Then perform independent STYLE review, actual native closure and
   affected comparison/mutant/holdout/composition qualification before integration.

Current evidence is sufficient to propose this boundary, not to seal it. Missing
cached full dossiers, absent baseline raw identities/animation mutants/holdouts,
source-header drift in Gamma proof and absent native playback acceptance remain
explicit prerequisites. No new fidelity verdict or formal conformance follows
from this research. Root owns baseline execution, judge changes, source work,
review, reconciliation and combined integration qualification.
