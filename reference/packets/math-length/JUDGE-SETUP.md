# Historical standalone Sonic probe publication rebinding

Root correction after independent review a9e07eb: both base 3efc9b7 and Main 92dbd07
registered Makefile recipes already force-include the frozen actor-current fixture.
The direct compiler command below omitted that include. It demonstrates a standalone
setup failure, not an inherited project/unit/probe build defect. Main retains its
frozen probe, unit, Makefile and fixture; e411eff is not integrated. The private
observer is equivalent for null-versus-unpublished receipts by independent source
review; canonical Main replay with its existing fixture remains required.


At source base `3efc9b7`, the unchanged Sonic probe fails compilation on four
expressions reading three retired members of `SonicPlayerGlobals`. Reproduced by
compiling `git show 3efc9b7:tests/probes/native_sonic_player_probe.cpp` with current
headers, `clang++ -std=c++17 -Isrc -fsyntax-only`: exit 1, four missing-member errors.
Local complete compiler output: `build/math-length/stale-sonic-probe.log`.

Commit `148d092` migrated those fields into the canonical `actor_current::State`.
Its parent still shows the three optional assignments in `sonic_player.cpp`:
primary work, nullable player work, and the action-derived SuperSonicFlag byte.
The existing `actor_current::Observation` callbacks execute after each store,
including null and same-value stores. Three local publication booleans therefore
recover the old `has_value()` observations without mistaking a null publication
for an absent store. Read the canonical byte only after its observed publication;
otherwise retain the old output zero. No second game owner is introduced.

Only the native probe binding changes. Its C ABI, input/output size and word
positions remain unchanged. Fixtures, expected values, source evaluator, control
words, comparison code and tolerances are unchanged. This is prerequisite judge
setup, preceding all length-source edits.

Fresh corrected baseline: all 28,800 comparisons complete; stored mismatches 0,
status mismatches 2,840; script exits 0 because its success condition is stored
state only. This is an inherited FP partial, not a full pass. Full native words
and original returns are frozen in the worktree's
`build/math-length/baseline/sonic_player.full.jsonl.gz`, with its JSON report and
current-source linker/build receipt alongside it.

The deliberate publication-observer fault sets the primary publication receipt
false. The unchanged source evaluator detects it. See the qualification receipt
for its actual counts and file hashes. No source behavior changed for this setup.
No gate was run: this packet explicitly reserves integration/gates to Root.

All inherited manifest entries are preserved, including unavailable local frozen
corpora. A broad refresh initially omitted those entries and was corrected before
source work. The local manifest checker therefore reports those pre-existing
unavailable files; it does not justify dropping them or certify a whole gate.
