# First animation judge / characterization packet

This packet establishes a usable before-Source judge foundation for the shared
animation table/resource/motion-state boundary, including maintained Sonic outside
the native archive, startup, decode and Gamma consumers. No maintained game,
player, runtime or other `src/` file is changed. No module, system or campaign
conformance, native actor playback or new gameplay is claimed.

Base: `99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3`.
Assigned worktree: `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-7dfc86b6`.
Durable branch: `packet/animation-judge-characterization`.
Root retains Main's frozen Sonic probe/unit; the original standalone command omitted its registered forced include. See corrected `OBSERVER-FIX.md`; e15d249 stays historical.
All Source remains byte-identical to the base. Main's source, build and Git state
were not written. The Main gate identified in the brief was not operated or polled.

## Executed baseline

| Stock judge | Cases | Stored / boundary mismatches | FP partial identities | Strict verdict / verifier exit |
|---|---:|---:|---:|---|
| Sonic animation table | 103 | 0 | 0 | pass / 0 |
| Player animation | 16,440 | 0 | 4,140 | partial / 0 |
| Player animation blend | 7,944 | 0 | 1,668 | partial / 0 |
| Sonic player, including ground prefix | 28,800 | 0 | 2,840 | partial / 0 |
| Player startup | 2,004 | 0 | 924 | partial / 0 |
| Gamma constructor/common tail, default only | 1,422 | 0 | 498 | partial / 2 |

Exit 0 from the value-only verifiers is not a strict FP pass. These numbers and
the original fixture digests are unchanged. `current-baseline.json` contains full
reports, source-only build commands, actual dependency hashes, raw stream paths
and hashes, library paths/hashes and complete identity-part indexes. Every partial,
native failure and native boundary is retained individually. There is no example
truncation in the identity parts or local raw streams. The motion cohort has 432
native failures/boundaries; Sonic has 12,168; startup has 2,004; Gamma has 468
boundaries. A successful bounded receipt is not promoted into gameplay success.

The three primary baselines and startup/Gamma were captured again using separately
rebuilt private libraries. Their full streams equal the first captures. The earlier
six-stream receipt self-compared Sonic and remains historical. Root subsequently
rebuilt the frozen registered Sonic probe with its existing probe-only forced include;
its distinct capture equals the private observer stream across all 28,800 records
and 24,801,598 decompressed bytes. No registered build defect or observer repair is
claimed. `root-frozen-sonic-comparison.json` supplies the additional proof; it does
not turn the old unit receipt into a fresh execution of the restored frozen unit.
`compare.py` compares every recorded source/native word, status, control, diagnostic,
return and input; aggregate mismatch counts cannot replace it.

## New judges

`table.py` uses actual original instruction memory hooks to derive the expected
write prefix. Its 8,727 cases retain all 147 rows, every reached export ordinal,
all 64 old six-field availability masks, distinct old row values, repeated export
identities, truncated exports, known nulls and unknown/null source pointers.
Observed metadata is independently checked against the original instruction PCs,
ordinal/slot/row/offset/width and scalar encodings. There are 145 export reads,
882 field writes and zero reads with an incomplete newly touched row. The old
operation interpreter and decoder's declarative recipe use remain covered.

`animation_characterization.cpp` adds 114 explicit existing-interface cases. It
checks source-backed failure prefixes, earlier stores, all three channel families,
malloc partial headers, real arena exhaustion, live aliases, hierarchy repeats and
capacity, adjacent/short key backing, nested errors, null counts and two free-order
boundaries. `source_order.py` records five actual original instruction experiments
covering action/motion publication, allocation-null fault PCs and free order.
Unknown fixture facts are projected onto source-backed prefixes; no claim is made
that original memory itself is missing. `MATRIX.md` gives evidence and limits for
each family; `missingness.jsonl` retains every new case identity/result/state.

`animation_gamma_assets.mm` records all 91 actual imported rows and validates all
scalar metadata while preserving Loaded / KnownNull / Unavailable. It retains
78 declarations, export 61 null, request 0 Stand and constructed row 72 unavailable
with reason. `gamma-assets.json` binds the pack/metadata hashes and narrow build;
`gamma-assets-rows.json` losslessly represents the full raw status/scalar stream. PSetMotion remains
the existing loud `native_gamma_set_motion` STUB.

Registered future Root rows are appended to `tests/unit/manifest.tsv`:
`animation-characterization` and `animation-gamma-assets game/imported`. The
existing Make discovery resolves both targets in a successful dry run. Executed
checks used narrow private source builds, not the broad Make targets.

## Compiled faults

All faults modify private source/header copies under ignored local `build/`.
All original tests, cases, seeds, expected values and tolerances remain intact.

| Fault | Judge result |
|---|---|
| Table eager validation | caught, 8,402 differing cases |
| Table unavailable→null | caught, 8,404 differing cases |
| Table 146-row bound | caught, 193 differing cases |
| Table reordered publication | caught, 8,337 differing cases |
| Next high-byte consumption | compiled and caught by new unit |
| Cached speed alias | compiled and caught by new unit |
| Reordered action publication | compiled and caught by new unit |
| Declared-count clamp | caught, 24,888 stored mismatches / 7,944 cases |
| Normalized raw pointer count | caught, 1,404 stored mismatches / 7,944 cases |
| Spilled vector fraction | caught, 302 stored mismatches / 7,944 cases |
| Channel release order | compiled and caught by new unit |
| Default action-before-data cleanup | compiled and caught by new unit |
| Gamma imported unavailable→null | compiled and caught by imported-table unit |

There are 13 caught algorithm/import faults. Compile failures are not counted.
`table-mutations.json`, `motion-mutations.json` and `gamma-assets.json` retain exact
build/judge exit statuses and binary/private-source hashes. Existing constructor
faults were not rerun and are not included in this count.

## Reuse and acceptance

Run from the packet worktree, with an existing local Python containing pinned
Unicorn 2.1.4; nothing downloads dependencies:

```sh
sh third_party/softfloat/build.sh build/animation-judges/softfloat
python3 tools/oracles/animation_judges/build.py player_animation build/animation-judges/current
python3 tools/oracles/animation_judges/build.py player_animation_blend build/animation-judges/current
python3 tools/oracles/animation_judges/build.py sonic_animation_table build/animation-judges/current
# For each slug, with the existing reference Python:
# capture.py SLUG EXE CURRENT/SLUG.dylib CURRENT/SLUG
# Consumer build groups and capture_consumers.py cover Sonic/startup/Gamma.
# Rebuild Bochs locally before the Sonic group, as in its recorded build closure.
# compare.py CURRENT --output RECEIPT.json requires all six captures.
```

`verifier-ast-proof.json` proves all six existing verifier files are byte- and
AST-identical to base. The transitive fingerprint is a separate sidecar, not a
change to their numeric logic or original source-fingerprint dictionaries.
`local-artifacts.json` pins raw outputs, constants, private libraries, faults and
the original input identities. Raw streams/libraries and copied before-Source
closure stay durable in this worktree's ignored `build/animation-judges/`.
No binaries or game assets are committed.

Remaining Source acceptance belongs to Root: complete the atomic shared owner and
all consumers, preserve these exact full streams/individual identities, rerun
faults against the new implementation, obtain independent Source review, then
execute reserved final cohorts. `reserved-holdouts.json` is RESERVED_UNRUN; none
was generated or executed. Historical Gamma 1,080/690 is not replayed or treated
as animation playback coverage. Native Gamma binding/weld/cursor integration,
full product source closure, journeys, Windows FP/allocator parity, and a coherent
integration gate remain unchecked here. The 114 cases do not exhaust all malformed
graph/state cross-products.

The interface stream includes explicit availability tags and malloc-header masks,
so unknown, known zero, known null and a present resource cannot collapse into one
serialized value. The comparator's own deliberate fault changes one existing FP
partial's flags while retaining exactly 4,140 partial cases; it exits 1 at the
changed individual case. This is a checker qualification, separate from the 13
algorithm/import faults. `observer-move-proof.json` records the observers' move
under the protected oracle directory without changing their observation logic.

`verify_packet.py --output RECEIPT.json` checks the complete identity indexes,
all frozen local artifact hashes, actual source closure, inherited manifest and
the recorded verdicts. It is a bounded packet integrity check, not the full gate.
`seal.py` refuses to reseal a before-Source baseline after maintained Source changes.

No global judge-manifest refresh was performed. The inherited 946 protected paths
and manifest hash are retained; all 53 ignored frozen raw corpora must be preserved
when Root seals integration from its complete checkout. Observer changes and two
appended registrations intentionally require that evidenced Root refresh. No
global inventory/review/shard, STATUS or HANDOFF changed. No full gate, delegates,
providers, network, downloads, host changes, remote storage operation or push ran.
