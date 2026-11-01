# Reproduction and retained execution

Preparation used this assigned checkout only, now on
`packet/animation-final-cohorts`, based on c75cd09. Source was read from immutable
Git blobs, never from a writable candidate working tree. No additional Git
checkout or worktree was created.

The exact preparation sequence was:

```sh
mkdir -p build/animation-final-cohorts/current build/animation-final-cohorts/old
git archive 38e1c69ecae029e38bf1d51c404e6a6358a75025 src tests tools third_party reference/sonic-animation-operations.json | tar -x -C build/animation-final-cohorts/current
git archive 53da4de17a1c1f21745d8403cd5bf6068afeb9f5 src tests tools third_party reference/sonic-animation-operations.json | tar -x -C build/animation-final-cohorts/old
/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 tools/oracles/animation_final_cohorts/prepare.py
/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 tools/oracles/animation_final_cohorts/build_consumer.py
/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 tools/oracles/animation_final_cohorts/build_gamma.py
/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 tools/oracles/animation_final_cohorts/run.py freeze
```

`prepare.py` verifies every materialized file against its exact Git blob before
compiling. Its fixture support is derived verbatim from the reviewed unit, and
both sides compile the identical probe/support against their respective source.
Builders invoke a single compiler driver at a time; preparation used at most two
concurrent source builds after dependency preparation. Corrected setup failures
are retained in `toolchain-and-setup.json`; none executed a final case.

`freeze.json` records all successful compiler commands, dependency hashes and
libraries. The numerical archives come only from existing qualified local paths.
The six old/current numeric libraries and two current plain-native libraries are
copied into this build and their hashes checked against the committed foundation
and Root requalification ledgers. No maintained Main archive is used.
Compiler executable, pinned Python/Unicorn binaries and DLL provenance are also
hashed in `toolchain-and-setup.json`. CHRMODELS_orig.dll is not consumed by these
synthetic/imported final fixtures; the actual imported CharacterPack and metadata
are pinned in the freeze.

The fixture-definition freeze was committed as 0026c8a before this command:

```sh
/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python3 tools/oracles/animation_final_cohorts/run.py execute > build/animation-final-cohorts/execution.log 2>&1
cohort_rc=$?
printf '%s\n' "$cohort_rc" > build/animation-final-cohorts/execution.exit
exit "$cohort_rc"
```

This is an execution receipt, not permission to rerun reserved cohorts. The
exclusive `EXECUTED_ONCE` marker prevents regenerating or re-executing them here.
Do not remove it. Future validation should inspect retained artifacts or request
new Root authorization for any separate execution. No source, fixture, tolerance,
flag policy or expected field changed after execution began.

All inputs are complete JSONL files named `<cohort>.inputs.jsonl` under the ignored
build. Outputs are complete `<cohort>.outputs.jsonl.gz` records with case inputs,
original observations, all old/current/native output words and interface state.
`generated-input-audit.json` verifies the exact ID sets, counts and input hashes.
Per-cohort receipts retain every FP-partial identity and the real bounded-run
status. `execution.exit` is the whole process's original exit status. These statuses
are not results of invoking an unchanged stock verifier CLI: this runner calls its
original classes and uses the unchanged defined-field comparison policy.

The consumer coverage gaps in DEFINITIONS.md predate execution. An exit zero or
matching stored fields cannot close them, prove hardware FP behavior, qualify
PSetMotion, or establish whole player/product/campaign closure.
