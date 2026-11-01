# Versioned complete-refactor inventory

Snapshot base: `f6c4e538af463a2eaf4c1fe961f04e8443d51878`. This packet implements
source accounting for [the campaign brief](../PROMPT-complete-refactor.md#1-establish-scope-and-preserve-the-baseline).
It does not establish the root's baseline, change scheduling or close semantic review.

The denominator is **every tracked path** under `src/`, `tools/`, `tests/`,
`reference/` and `third_party/`, without relying on source suffixes or native build
inclusion to omit a maintained file. It includes production app/platform code,
excluded native sources, support/configuration data, and guest/HLE composition.
Gitlinks are single tracked parent paths; their upstream descendants are outside
this repository's denominator. Ignored/untracked installations, assets, build
outputs and preserved packets on other branches are not imported or inventoried.

## Artifacts and reading order

| Artifact | Meaning |
| --- | --- |
| [summary.json](summary.json) | Counts and verification limits, regenerated from tracked paths |
| [review.json](review.json) | Authored findings, provisional system assignments, destination and dependency contracts |
| [modules.jsonl](modules.jsonl) | Groups basename header/implementation pairs; tools group by directory; tests remain independently visible |
| [files/](files/) | Exhaustive maintained per-file records, sharded by directory below the 2 MiB repository limit |
| [stubs.jsonl](stubs.jsonl) | Every explicit `STUB:` annotation in tracked `src` plus separately typed plain STUB mentions |
| [boundaries.jsonl](boundaries.jsonl) | Every remaining reference or third-party tracked path, including Git object identities |
| [PACKET.md](PACKET.md) | Integration contract and performed checks |

At this snapshot there are 3,648 paths: 637 in src, 353 in tools (including the new
inventory generator), 574 in tests, 1,929 in reference and 155 in third_party.
There are 1,615 maintained/script records in 1,026 bookkeeping modules and 2,033
boundary rows. The 51 repository-owned reference source/script records include
retained historical harnesses; those are individually indexed and remain open
usage review, rather than being silently dismissed as obsolete.

Nine entries have observed migration findings, five have a reviewed reference
infrastructure purpose, and 1,601 explicitly remain `open_review`. There are **zero
already-conforming or obsolete claims**. Both are supported dispositions with
additional evidence requirements in the generator. Prior successful migrations
(fog, frame control, scene position, game mode, polybuffers) remain review targets.
A purpose classification for a reference script is not approval to execute it or
a claim that its whole body is already readable/correct.

## Per-file evidence and limitations

Each record provides a stable path/module/system, source SHA-256, base membership,
disposition and explanation, review evidence links, proposed destination, source
annotations, owner/type candidates, static consumers, judge candidates and
acceptance links. Nine findings additionally document observed ownership and
borrowing. All canonical-ownership closure remains explicitly open: a declaration
scan cannot prove one owner per source address, aliases, callback lifetimes or the
absence of duplicated state. C++ forward declarations can appear as type candidates.

Consumer records are objects with path, line and evidence kind. Quoted includes
are resolved against the local directory, src, tests and repository root; literal
tracked paths in maintained code and Makefile are also indexed. They do not prove
call edges. Symbol references, Python imports, dynamic callbacks, generated code,
conditional compilation and link-time consumers need semantic review. A missing
consumer is an unresolved question, never proof of obsolescence.

Judge links come from reverse include reachability, paired public headers and
existing same-slug files. They are **candidates**, not a coverage verdict. Some
include paths make the set broad; historical slug differences can leave genuine
judges absent. Every existing oracle/probe/test is independently covered in the
inventory even if association is unresolved. For migration, resolve the actual
probe/oracle, mutation locations, holdout and current per-case verdicts before
editing behavior. Preserve inherited failures and FP partials individually;
this packet neither reruns nor upgrades their receipts.

`native_build` records list membership only. `production_recipe_membership` also
accounts for Makefile's native wildcard, explicit platform objects and production
entrypoint: 128 listed translation units, 22 additional wildcard units, 10 platform
units and one entrypoint. Another 149 translation units are absent from these
observed production recipes. Headers/support files are separate. These are static
recipe facts, not results of a build, closure link or runtime independence test.

System assignments are a work queue, not semantic certification. For example,
HLE remains open review for embedded policies; it receives no blanket exemption.
Loader heap identity infrastructure is assigned to reference-runtime, while PE
resource decoding is an asset-boundary concern. Repository script names such as
`recover-gameplay.mjs` are not proof of current support: several retain old Hopper,
`_reference` or `/private/tmp` paths. Preserve them until consumer/obsolescence
review, without launching providers. Upstream QEMU/Unicorn/SDL excerpts under
reference are separately classified from repository-owned scripts, based on their
copyright/license headers; `.gitmodules` identifies the mod-loader boundary.

The STUB inventory contains 83 annotations and 11 other mentions. It preserves
annotation text and following declaration/source, module, consumer/judge candidates
and acceptance. These are declaration sites, **not 83 unique unrecovered original
functions**. Native/reference binding, recovery status and ownership are explicitly
open. A stale STUB may already have another implementation, a prefix can be partial,
and unannotated loud failure seams are not inferred complete by this scan. Plain
STUB mentions in `player_scene_position_native.cpp` remain separately visible.

## Proposed system dependency order

The dependency graph is machine-readable in `review.json` under `systems`; this is
a proposed migration order, not the current C++ include graph or a gameplay plan.
Independent reference/tool reviews can proceed throughout. Resolve coverage gaps
as separate judge work before their affected behavior is rewritten.

1. Audit narrow numeric foundations and the imported-resource/reference decoding
   boundary. Preserve exact arithmetic where evidence requires it. Do not start
   with blanket FP simplification.
2. Establish task/work/lifetime and render/device owners, then texture ownership.
   These are prerequisites for shared queue/sprite/object submission. Include the
   algorithms now under guest queue lifecycle, rather than relocating them again.
3. Audit input/frame/profile state and the player common geometry/contact/physics
   owners; then character animation/actions and instruction-shaped Sonic tables.
   Preserve Sonic's borrowed reset word and remove its dependency on guest-defined
   acceleration through an actual shared implementation.
4. Establish save/profile and sound/movie owners, then flow/startup/reset/UI. Carry
   existing native and reference consumers together; production UI is in scope.
5. Split native Session into those cohesive system owners with explicit borrowing;
   validate platform/app composition and independent production closure last.

**Proposed next system: input/frame policy and its shared state ownership.** Start
with `engine/game_logic.hpp`, `guest/game_logic.cpp`, the frame-control pair and
`game_logic_support` consumers. The observed tick loop, edge accumulation and reset
policy are recovered algorithms mixed with fixture diagnostics. This is bounded
existing behavior with discoverable judges (`tests/unit/game_logic.cpp`,
`tests/probes/native_game_logic_probe.cpp`, `tools/oracles/verify_original_game_logic.py`)
and it establishes ownership needed by later queue/player/composition work. First
finish semantic owner/consumer review and freeze the actual judge cases; root must
supply its baseline before any rewrite. Numeric/asset contracts remain review
prerequisites, not permission to perform gameplay recovery.

## Regeneration and checks

From the repository root:

```sh
python3 tools/inventory/refactor_inventory.py --write
python3 tools/inventory/refactor_inventory.py --check
```

The generator reads Git/local text only and writes only these generated artifacts.
It never imports project tools, launches providers, runs tests or changes judges.
New maintained paths must be tracked (`git add`) to enter the denominator. The
checker compares deterministic regenerated bytes and exact tracked-path equality;
no regex result assigns a semantic disposition. Edited source makes the snapshot
stale, including the generator itself. Authored findings in `review.json` survive
regeneration. Removed base paths fail until an explicit reviewed lineage policy is
added, so deletion/renaming cannot silently shrink the initial denominator.

The checker does not replace a semantic reviewer or the gate. It deliberately is
not registered in protected Makefile/checker/test manifests in this packet. Root
can separately choose enforcement and judge integration. Baseline/gate, ADR, GOAL,
STATUS, HANDOFF and preserved packet integration remain root-owned.

## Reviewed STUB bindings

`review.json` names explicit binding-review fragments. Each entry binds the exact
annotation, following declaration and source hash, plus recursively fingerprinted
owner, native/reference binding and coverage evidence. Unknown, duplicate, moved
or changed entries fail reconciliation. Binding review records transport and
explicit failure behavior; it never retires an unrecovered callee. All annotations
and other mentions remain in the denominator and retain `unrecovered_open_debt`.
The initial save-warning audit distinguishes explicit native failure from an
original-machine-call fixture; neither claims the warning helper is recovered.

## Bounded module records

`summary.json` lists every ordered `module_shards` path, record count and SHA-256.
Read all listed shards to reconcile the module denominator. `modules.jsonl` is the
first shard; additional files are `modules-002.jsonl` onward. Complete JSON records
retain their order and evidence, with UTF-8 bytes capped at the existing 2 MiB limit.
Oversized single records and unexpected stale shards fail; nothing is truncated or
automatically deleted. This layout changes packaging, not module classification.
