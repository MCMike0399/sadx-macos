# Independent before-source save IO/frame judge review

Reviewed candidate `defb94e0d7d19eff7dbbe0564ea7315a9c7a7054`, branch
`packet/save-io-judges-integration`, read-only in `t3-5acabf9a`.
Worker anchor: `821861ff2b6b9d87e1c96e32d9ff1ae56b756911`.
Base: `353d528bbb1a50e8cb0e4cdeaf7657dc9ad8607d`.
Separate Root checker fix: `98e2e6af6d7ce340c0bf0228faa989d6a4f737bd`.
Review branch starts at Main `bda836fc8c14fc83458703fb64b408f6e2ad8f36`;
integrated source review `dd5ab93` supplies SL1/SL2/SF1/SW1/SC1 criteria.

The frozen baseline is supported, including the narrowly justified void-return
correction. Two enforcement defects should be corrected in separate evidenced
judge work before accepting these helpers as the migration prerequisite. Neither
finding demonstrates corruption of the retained baseline. No source migration,
whole Save/Session system, product closure or campaign conformity is approved.

## Findings

1. **P2 — Packet directory symlinks bypass new helper recognition.**
   `tools/gate/check_judge.py:49` discovers only paths matching `**/*.py`.
   Recursive globbing does not descend into directory symlinks, and an alias
   directory itself does not match that pattern. Consequently the refusal at
   lines 58–59 never sees it. In an isolated temporary source-archive fixture,
   adding `reference/packets/alias -> <external-directory>` containing `helper.py`
   left `check(root)` at `([], 21)`, although
   `protected('reference/packets/alias/helper.py')` was true. Such an imported or
   executed helper can change outside the protected snapshot without addition or
   change detection. Direct `reference/packets/link.py` symlinks correctly fail.
   Inspect packet directory entries without following aliases and reject directory
   symlinks before applying the Python-file filter, including a symlink at the
   packet root. Add focused fixtures for directory aliases, dangling aliases and
   ordinary nested helpers in Git and archive contexts. Preserve narrow regular
   `.py` coverage and cache exclusions.

2. **P2 — Writer instruction comparator accepts missing scenario suffixes.**
   `reference/packets/save-io-frame-judges/writer_source_contract.py:82` discards
   writer IDs, accepts any number of groups up to 39, and the main loop at line 99
   compares only those present. Exit status at line 107 depends solely on
   mismatches. An empty capture therefore produces zero cases and success;
   removing complete trailing scenarios can also succeed without exercising
   their source prefixes. A pure-parser check, extracted by AST without importing
   or executing the original observer, returned 0/39/38 cases for empty/full/
   drop-last inputs. The frozen, hash-validated receipt has all 39 cases and is
   unaffected. Require exactly the ordered IDs 0 through 38, reject missing,
   duplicate, reordered and extra groups, and validate required terminal snapshots
   before comparison. Add parser-only failure fixtures; keep every existing
   snapshot, defined output, failure and unknown policy intact.

## Baseline and coverage assessment

- Independently verified all 650 `src/` hashes against both the source freeze and
  immutable base. All 955 old manifest paths remain; exactly four old hashes
  change: Makefile, unit registration TSV, checker and checker regression test.
  All 304 new manifest bindings equal immutable worker HEAD bytes and exactly
  match migration v2. Existing provenance/readability/dispatch/table ratchets and
  inherited evaluators/probes remain unchanged. The 53 restored corpora match
  manifest hashes; historical worker exit 1 remains historical.
- The four exempted probe exports are actually declared `extern "C" void`.
  Original ctypes harnesses ignore their default integer return. The companion
  comparator replaces only that register slot with an unknown tag; all input and
  output array bytes remain in the record. Defined returns, source observations,
  identity ordering and truncation remain compared. All ten hash-validated full
  before/qualified ABI streams compare equal on defined records (9,314 calls).
  No output buffer or failure is excused by the void policy.
- Reconstructed Controls directly from its validated local raw stream: 2,438
  cases, zero value mismatches, and all 938 inherited case/input/difference
  identities exactly equal. It remains partial, exit 1. The 688-case loop receipt
  concerns original local orchestration with fixture callees, not implemented
  subsystems or frame pacing.
- SL1/SL2 have useful before-source contracts: unknown sentinel payload stays
  separate from link identity; linked-before-error append, native node/string
  allocation failures, retained aliases, delayed destruction, partial read
  publication, file/search close order and error prefixes are represented.
  Native allocation/exception behavior is explicitly an adaptation. The list
  observer executes inherited original discovery instructions and records fixture
  addresses/poison independently; it does not establish Windows allocation or
  alias guarantees. Partial byte lengths are sampled at 0, 1, 2, 3, 4, 100, 1391
  and 1392; PACKET's phrase “all partial byte-read lengths” should be narrowed.
- SW1 freezes callback-visible scalar/data/pointer publication, separate retained
  allocation bytes, encoding after open, ignored write return, reset ordering and
  success after close. The additional original observer runs real writer
  instructions between explicit CRT/service receipts. Its allocation filtering
  distinguishes the 14-byte name owner from temporary strings; allocator contents
  before formatting remain unknown. Known snapshots and written bytes compare;
  general CRT throws, filesystem/locale behavior and native allocation lifetime
  are not original guarantees. Initial mismatches remain retained.
- SF1 compares the actual `run_game_loop_iteration` implementation with the native
  split across intervening completion, including callback mutations, lazy reads,
  count wrap, failure prefixes and clear-before-writer. Thirty missingness cases
  preserve optional unknown tags separately from known fixtures. This is shared
  routing characterization, not app scheduling/render/message completion parity.
  SC1 correctly retains real saving-message, close-message and writer services;
  warning/outer missing services remain separate boundaries.
- Complete IO/list and frame baseline/qualified captures are byte-identical:
  3,521,423 and 948,217 bytes respectively. Hash-bound receipts contain fourteen
  valid compiled new faults plus five historical completion faults. The initial
  pause-mutation survivor is preserved and its final combined callback case is
  discriminating. I inspected receipts and fault specifications, without running
  mutations or binaries.

## Checks and limits

Executed read-only in the candidate with bytecode writing disabled: fourteen
`test_judge.py` tests pass; comparator self-test passes; judge check reports
1,259 inputs / zero violations; packet verification reports 316 artifacts /
zero integrity errors / zero missing inherited inputs / 650 source pins checked.
Only after validating bound hashes did I parse trusted worker-local raw streams.
Additional pure Python/temp checks covered manifest ancestry, immutable new bytes,
complete ABI equality, Controls identities, native stream equality, packet file
versus directory symlink behavior and the writer parser's missing-cohort behavior.
An ordinary source-archive fixture verifies supplied hashes without Git; existing
focused tests preserve additions-only Git/evidence requirements and reject mixed
source changes, forged mode, old edits/removals and invalid registration appends.
The scanner change does not weaken those existing checks or activate an edit hook.

No maintained candidate/source/judge edits, compilation, original oracle execution,
mutation execution, gate, holdout, provider, network, host, storage or helper
administration was performed. Candidate remained clean. Only this independent
report is committed in the provided review checkout. Reserved final cohorts remain
**UNRUN**. Last Main gate `388c249` (292 native / 450 Python) is not this candidate's
verdict. Root still owns fixes/review, source migration, consumer rebinding, parent
manifest union, inventory, clean product/reference closure, journeys and full gate.
