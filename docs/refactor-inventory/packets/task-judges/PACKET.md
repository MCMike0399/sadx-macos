# Task scheduler judge packet

Base: immutable Main `99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3`.
Branch: `packet/task-judges`, worktree `t3-686afca5`.
This is a judge and characterization prerequisite. Maintained `src/` is unchanged.
TaskActor review `5f5c17f` supplies TA1–TA4 ownership/context. Animation judge
`8d5eb020` independently owns arena interior/animation allocation characterization;
none of its source, tests or probes is changed here. Recovered scheduling and
lifecycle behavior stays in engine; no adapter or gameplay callback is recovered.

## First freeze, before characterization

Four probes were freshly compiled from actual current `task.cpp` and `task_work.cpp`.
No Main maintained archive was borrowed. `compile.json` records commands and exits.
The unchanged original judges retain their inputs, seeds, tolerances and comparisons:

| Judge | Actual current result | Preserved limit |
| --- | --- | --- |
| task_work | exit 0, 16 sequences / 4,890 operations | Explicit heap fixture; corruption/hang cases remain outside source returns |
| task_lifecycle | exit 0, 19 sequences / 2,316 operations | Inactive destroy stops at real `0x401000`, retains work, does not observe a source return |
| task_display | exit 0, 236 comparisons | Outcomes: 179 complete, 55 callback/material failures, 2 saved-next boundary cases; these are not 236 successful source walks |
| task_delete | exit 0, 244 comparisons / 250 attempts | Six historical exclusions: 85, 87, 91, 121, 143, 201; no added skips |

Each `*-identity.json` lists every native call, return, argument count, complete
buffer length and hash, including commands, arenas, traces, callback contexts and
inactive-boundary output. Each `*-report.json` is the full new original-comparison
report, not a size-bounded tracked summary. Full native ABI streams remain locally
under `build/task-judges/frozen/*.native.jsonl.gz`; each stream retains every byte,
including output padding. Native bytes are source-equal only where the unchanged
judge actually compares them. Unspecified/void EAX, hardware flags and substituted
callback behavior acquire no new source guarantee from this freeze.

`corpora.json` distinguishes historical raw input from the current generator.
Work is byte-identical. Historical lifecycle has 17 sequences / 2,302 operations;
current has 19 / 2,316. Both remain intact with distinct hashes. Historical delete
summary inputs are ten-word prefixes, not complete fixtures. Their IDs, reasons
and retained prefixes match the complete current unchanged generator vectors.
`excluded-source-boundaries.json` additionally pins actual registers and complete
pool bytes for those same six excluded inputs. Reentrancy stops in the callback
receipt; free-list-cycle detection happens during post-execution serialization.
Neither is promoted to a native comparison or successful source execution.

## Actual consumers and characterization

The five critical old units task, task_work, task_lifecycle, task_display and
**task_callback_lifetime** run with exit 0. The last regression explicitly passes
an owning handle as destroy_task's argument while its destructor clears/replaces
that very handle; all four work allocations release before native auxiliary data.

Fresh local core/guest archives compile all their maintained dependencies. Actual
HLE TaskBridge replay exits 0 against historical raw work/lifecycle inputs: exact
allocation EAX, globals, source callback frames/context/registers, HLE trace and
full arena bytes; 18 dispatch and 6 lifetime scenarios; 12 built-in bridge input
faults caught. Graph-sync and inactive-lifetime units exit 0, preserving callback
stores to current and free slots. Actual NativeGame unit exits 0 through startup
and three survey frames with five unported owners. Its assertions qualify existing
facts/routing, not all private task state. Actor-current and player-task units exit 0.
`consumers.json`, `nativegame.json` and `registered-units-final.json` retain receipts.

New registered scheduler contract emits 20 deterministic observations across:

* Unavailable default versus known-null, reached work and published task on failure;
  slot zero versus null links, pool exhaustion and failed pool-fill publication.
* Explicit callback-context table, conflicting ownership and cross-world isolation;
  retained context, callback replacement, saved-next slot reuse/new handle generation.
* Native value dispatch/lifecycle continuations across graph replacement; siblings
  captured after destructor mutation, callback fields cleared before children.
* Callback failure at calls 1–3; destructor exception at children 1–3 with exact retired
  prefixes and later cleanup; distinct top-level/child host-exception context rules.
* Cleanup read failure at calls 1–2, lazy remaining reads, unchanged retained work,
  explicit inactive retry and successful resume; native auxiliary release ordering.
* Creation payload-write failure at calls 1–4, reached allocation fields and exact
  source boundaries; nonowning links and outermost nested-WalkGuard reclamation.

The separate actual bridge contract covers invalid-index and zero-free-head lazy
exits before unavailable metadata, missing arena/default reads, unreadable pool with
unchanged head/free publication, and an interior pool write failure at `0x40b4bd`.
Its complete accessible first-page prefix is captured; failure is not assumed atomic.
It does not copy native snapshots over callback mutations or refresh actor state
from a registry. Tests are in protected `tests/probes/task_*_contract.hpp`, invoked
by minimal unit wrappers. No generic framework or new Services interface is added.

Compiled copied task faults and three actual copied TaskBridge faults are recorded
in `mutants-qualified.json` and `bridge-mutants.json`. Maintained files are never
edited by mutation tools: copies come from immutable Git base blobs. Historical
skip-destructor, retain-exec and skip-auxiliary-free intents are located in the
original commit journals, then caught by the unchanged current lifecycle oracle.
The historical counts are not substituted for new runs. Initial skip-unlink-state
survived; adding a graph linked-state assertion discriminated it. That initial run
remains in `mutants-initial.json`. Invalid compilation never counts as a mutant.

`compare_streams.py` compares every native ABI field and byte, record count and
excluded cohort without tolerances or masking. `comparator-tamper.json` proves it
rejects a changed padding byte, scalar return and missing record. The self-check
against all four real baseline streams passes; this is comparator validation,
not a new independent source cohort.

## Evidence, missingness and reproduction

`source-freeze.json` pins all maintained source separately from compiled dependency
coverage. `transitive-dependencies.json` pins actual MMD dependencies, fresh local
objects/archives, link maps and logs; system SDK headers are outside MMD.
`clang-declarations.json` comes from Clang's parsed complete scheduler/WalkGuard
records and signatures, not lexical word counts. The raw AST remains local.
`check_baseline.py` checks those base-source anchors against Git objects and current
source, and preserves all 946 inherited protected inputs. All 53 local ignored raw
corpora were restored read-only from known local Main inputs only after exact-hash
checks. Missing inherited protected inputs: **0**. Initial private build directory
and archives were absent. No summaries became expected fixtures.

Initial lifecycle narrow link omitted player_task.cpp and failed; corrected link
and actual unit pass. First bridge registration used the generic core-only rule
and failed linking; explicit new reference recipe fixes it. The first pool-gap
experiment used a non-page-aligned heap base and failed setup; aligned setup passes.
The initial saved-next fixture recycled into its own list and failed the intended
finite visit expectation; it now uses the existing source-lifecycle test's other-list
reuse shape. An initial cleanup-read test asserted an incorrect object boundary
(`0x40b623`); inspection of unchanged source established `0x40b628` before freeze.
Neither correction changes an old judge, input cohort or maintained algorithm.
Those failures are not passes or mutants. No full gate, network/download, host or
storage administration, Main source/build/Git write, delegate, push, STATUS or
HANDOFF edit occurred.

From this worktree, after local pinned Unicorn is available:

```sh
python3 tools/oracles/task_judges/check_baseline.py
make -j2 build/task-scheduler-contract-test build/task-bridge-contract-test UNICORN_DIR=<local Unicorn package>
./build/task-scheduler-contract-test
./build/task-bridge-contract-test
PYTHONPATH=tools/lib:tools/oracles <pinned-python> tools/oracles/task_judges/freeze.py task_lifecycle <sonic.exe> <fresh-probe.dylib> <new-output-directory>
python3 tools/oracles/task_judges/compare_streams.py build/task-judges/frozen <candidate-stream-directory> --output <comparison.json>
python3 tools/oracles/task_judges/mutate.py --exe <sonic.exe> --python <pinned-python> --out <fresh-mutation-directory>
python3 tools/oracles/task_judges/bridge_faults.py --unicorn <local Unicorn package>
```

The freeze tool refuses an existing slug output. Keep the old private streams and
libraries; rebuild candidate probes/consumers from all maintained dependencies.
Judge registration is an additive evidenced step; Root must reconcile its parent
manifest union before integrating, with no old path removed or unreviewed rebind.

## Unchecked Source acceptance

**Final independent holdout: UNRUN**, reserved until subsequent Source freeze and
fresh review. Default oracles, same-input exclusion observations, deterministic
characterization and comparator self-check are development evidence, not holdout.

Root still needs fresh independent packet review, candidate full-stream comparison,
all inherited boundary/exclusion and corpus identities, candidate compiled faults,
reserved holdout and composed gate/integration when Source work is actually ready.
Scheduler lifecycle failure injection is bounded to named calls; all cleanup-chain,
metadata callback, graph corruption, reentrancy and HLE write-failure combinations
remain unchecked. Arbitrary callbacks, Windows hardware/heap parity, NativeGame's
complete private scheduler state, animation allocation, whole Task/player/product
journeys and full campaign acceptance remain open. No whole-system closure claim.
