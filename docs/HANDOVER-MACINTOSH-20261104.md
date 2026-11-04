# Macintosh takeover prompt — SADX, 2026-11-04

Paste this prompt into the agent on **Macintosh**, or tell it to read this file in
full. Repository: <https://github.com/MCMike0399/sadx-macos>.

## Assignment and machine boundary

You are taking over the existing SADX native macOS project from DiveMac. First
recover and verify the checkpoint, then continue toward the next connected native
Character Select checkpoint when the user starts this takeover. Keep DiveMac's
SADX workers/services stopped. The November 2 stop was explicitly **SADX only**;
unrelated local and Pi production services stay running. This handover preparation
did not restart development, a scheduler, Ghidra or the app on DiveMac.

The user wants faithful gameplay results and readable maintained code together.
The orchestrator plans, integrates shared owners, reviews and accepts; GPT-6.1 Sol
workers implement independent clusters at **medium effort, priority/fast service**.
Use two builders where ownership is disjoint, with a third slot for review. Verify
available providers/settings on Macintosh; do not silently substitute a different
model/provider. The original orchestrator was Astra; the last packaging session
used Sol. Provider IDs and worktree paths from DiveMac are historical, not portable
configuration. User-authorized delegation remains relevant to the takeover.

The product is a readable native arm64 SADX (2004 PC) port under the pinned speedrun
profile, built/run without sonic.exe, original DLLs or an x86 runtime. Assets are
imported once from the user's own installation. The hybrid guest is the development
reference. A model exported into GTA V does not implement the original game's
controllers, animation, collision, menus, stages, events or profile behavior.

The connected acceptance route remains: startup → save → Character Select → Sonic
story entry → Chaos 0 defeated → Emerald Coast cleared → train to Mystic Ruins →
Egg Hornet defeated → Wind Stone placed → Windy Valley cleared, with gameplay at
measured 63 FPS, correct audio, keyboard and Switch 2 Pro input, CCEF/EHCF/Skipma and
save progression. **N3b Character Select is still incomplete.** Later N4–N10 and
cross-cutting gameplay requirements remain open.

Read `AGENTS.md`, `docs/STATUS.md`, `docs/GOAL.md`, `docs/STYLE.md`, the last
`docs/HANDOFF.md` entry, ADR-0005..0008 and **ADR-0010**. ADR-0010 supersedes
ADR-0009's exhaustive-refactor-before-route ordering. Preserve its inventory and
open debt; migrate the systems the route actually needs. For relevant work, read
METHOD, ARCHITECTURE, JUDGE, TESTING, SOURCES and TIPS. Older history and packet
notes can describe superseded frontiers; trust the pinned receipt and its scope.

## 1. Recover the Git checkpoint before building

- Qualified game/source-and-judge revision:
  `709f274447895b9e53a377d9b158bf8a87830a45`.
- Main before this handover documentation:
  `499ff5161ed7405ae85bf92bdff688dc8eecc288`.
- Main includes the tested implementation. Later handover commits add documentation
  and generated accounting only; they do not imply a new runtime gate.
- Published unfinished branch: `packet/gamma-display-native`, exact handover head
  `1aa91341d3d2c26b821c09b0701ed1c2709a0d18`.
- That branch's original draft was `b5a3cea9`; `28459d88` preserves the interrupted
  edit (`fact.c_str()` in native_gamma_display_binding.cpp). `1aa91341` records the
  actual partial/failed attempt logs discovered during the final audit.
- **Do not merge that draft into Main as qualified work.** Its shared-file patch
  targets older Main `ba88c5cc`; reconcile it with the current Sonic callback.

Use an existing Macintosh checkout without overwriting local edits. Fetch and
fast-forward Main only when its worktree is clean and the update is compatible:

```sh
git fetch origin
git status --short
git switch main
git merge --ff-only origin/main
git submodule update --init --recursive
git fetch origin refs/heads/packet/gamma-display-native:refs/remotes/origin/packet/gamma-display-native
git rev-parse origin/packet/gamma-display-native
```

The last command must identify the head above, or a later explicitly understood
revision. Use a T3-created isolated worktree for implementation. Do not create an
unbound child checkout with shell commands. Preserve the packet as a fixed review
base and integrate its source surgically into current Main after qualification.

## 2. What Main actually proves

Main's normal-input app stops explicitly at **native_gamma_display**, frame
**1332**, exit **1**, with **1330 completed frames**. The inspected image is still
the outgoing Sonic Adventure DX title. Its PPM SHA-256 is:

`c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.

The reference first-Gamma observations occur at frame **1334**. Preserve that
native/reference timing difference; no aligned actor-frame equivalence exists.
A fresh save can produce an earlier 1082-frame stop; it is a different input/save
context, not a speedup or an aligned comparison.

The actual full gate at source `709f2744` exited **0**:

- **333 native programs**, **622 Python tests**, seven reported skips.
- Reference R1–R4; three native FileSelect/MainMenu/fresh-save journeys.
- Sixteen native/reference startup comparisons.
- Six offline raster tests separately passed without skips. The inherited volume
  case-alias capability remains skipped.
- A fresh git source archive contained no original EXE/DLL or prebuilt game
  objects. All maintained native code rebuilt and force-load closure passed;
  only copied pinned public SDL3/libusb products were reused. Its normal-input
  run reached the identical Display stop and image.

Read these tracked receipts and their limits:

- `reference/production-integration-20261102/sonic-initialization-fan-gate.json`
- `reference/production-integration-20261102/sonic-initialization-fan-qualified-rasters.json`
- `reference/production-integration-20261102/sonic-initialization-fan-source-only.json`
- `reference/production-integration-20261102/sonic-initialization-root-integration.json`
- `reference/production-integration-20261102/gamma-fan-integration.json`
- `reference/production-integration-20261102/model-matrices-root-integration.json`

Final integration reviews `b1df6ead` (Standards) and `6be50e40` (Spec/Fidelity)
accepted the bounded combined integration without findings. Both verified all
**1,460 protected files**, no missing bytes/drift/removals on DiveMac. On a fresh
checkout, **53 ignored binary evidence inputs may be absent**. Copy/hydrate the
original bytes; retain their protections. Never regenerate or remove hashes to
silence absence. The full gate does not run every oracle/mutant/holdout. Preserve
separate function-level evidence and inherited failures/FP partials.

## 3. Integrated systems and exact remaining boundaries

The real app reaches native startup, movie/title, file/save menus and Character
Select task creation. Preview actors are actually scheduled. Implemented native
composition now includes:

- Preview action ordering, selected motion, event action allocation/admission,
  SetInputP and cleanup; canonical event nodes and retained stale-tail identities.
- Gamma setup definition/import, one registry/result owner and arena allocation.
- Stable shared mutable materials consumed by Gamma and the renderer.
- Gamma effects initialization; particle update/custom update/display iteration,
  destruction and animation stepping. Particle rendering/spawning and the Gamma
  global release binding remain open.
- Gamma PSetMotion, shared MDATA2 decoding, live ObjectState pose borrowing.
- PJoinVertexes initialization/restoration and complete GetNodeIndex/helper.
  Active matrix argument selection, deformation and attachments still stop.
- Gamma buffered analog restoration, actual model-matrix getter/Z bindings, fan.
- Readable shared Sonic controller and reference adapter, corrected physics
  capture boundaries, actual callback, editor/Metal state, animation/model owners,
  and initialization through textures/setup/motion/weld/effect factories/callbacks.

Important source facts:

1. `4c3870`, historically called laser targeting, actually restores buffered analog
   angle then stroke from E102WK offsets 556/560 into Input's canonical normalized
   analog array. Preserve ordered partial publication and raw float bits.
2. Source Gamma setup is `4c5420`; effects initialization is `49fb10`. Historical
   descriptive notes sometimes reversed names. Source bodies/annotations win.
3. One Session model-matrix state owns count, limit and 2048 words. Original runtime
   measured no prior CalcMMMatrix, initial count/limit/buffer/five Gamma/four weld
   matrices all zero. Get54 returns false and preserves all 16 destination words.
   Getter order: 54,30,39,2,37; Z rotation creates exactly four negative-zero words
   in node39 components 4–7. Do not invent identity matrices or refresh aliases.
4. Fan `4c2e90` reads speed Z then X, stores phase+28, sine height+24, reloads X and
   stores signed tilt+40. The pinned sine value at 2048 is `0x3e47c5ac`; host sin
   differs. Reuse exact arithmetic and existing Display sine/CRT conversion.
5. Sonic normal joins are one 888-byte owner at 3c55e28; 3c55f38 is row11's
   destination field inside it. The sibling cache 3c55d40 is separate. Metal's
   table is 192 bytes. Direct-ahead references at 3c55d58 are one 12-byte owner.
6. Sonic helpers preserve captured physics **and allocation** while parameters may
   reload live. Same-record replacement and nested recapture boundaries matter.
   Do not repair the entry-captured null current-player alias after initialization.
7. Sonic initialization now reaches missing **SONIC_OBJECTS slot20** on the normal
   imported graph. Metal/changed-sibling paths reach active CalcMMMatrix seams.
   It does not complete its common tail or display.
8. `vertex_weld::update_prefix` owns mode dispatch in player code. Active processing
   precedes table consumption; mode becomes 1 only after successful initialization;
   other signed modes restore. Runtime supplies lazy owner validation.
9. `sonic_model::create_initialization_task` treats pool exhaustion or successfully
   rolled-back primary/motion allocation exhaustion as null creation. It requires
   the exhausted task to be retired; unavailable writes/uninitialized pool/failed
   rollback and other steps remain failures. Both setup/effect factories use it.
   The source ignores setup's false return and continues to motion/publication.
10. Session mission/editor words are canonical signed 16-bit reads followed by
    non-normalized OR. Metal borrows existing SoftResetState. The strict native/
    reference counterpart catalog is protected; no extra shadow owner is allowed.
11. Event action head/list is +64; the source path list is +76. These remain distinct
    from Sonic's separate event link and preview's retained action tail.

Miles, Knuckles, Amy and Big callbacks remain open. Full cursor/cancel/reentry,
voice/music requirements and Sonic confirmation/story composition still need an
actual N3b journey checker with content/state/audio and draw-suppression fault.
Voice 2047 remains unavailable. Station Square/Chaos0 native scene/controller/
camera/event/boss composition is not established by the reference journey.

## 4. Function evidence to preserve

| Scope | Retained comparison/limit |
|---|---|
| Sonic controller | 58,992 default and 3,072 extra observations exact old/current; default 2,840/2,172 and extra 204/219 inherited FP partial identities remain partial. Full captures reused at final integration, not rerun. |
| Sonic initialization | 650 development/1,162 holdout exact (old 649/1,161 plus one exhausted-primary case per cohort); original body and helper use controlled lower calls. |
| Sonic model | Both 80-case cohorts exact values, 34 individual FP partials each, strict exit 2. Actual pair identity/content observer and same-length wrong-pair faults retained. |
| Gamma fan | 909 development/4,493 holdout, zero value differences, 733/3,434 individual FP partials, strict exit 2; seven core/four binding faults caught. |
| Model matrices | 1,223 development/4,935 holdout, zero value differences, 957/3,934 partials; 774 real/imported/generated rows with three partials, strict exit 2; twelve core/five binding faults. |
| Motion/blend | 16,440/7,944 observations preserving 4,140/1,668 individual partial identities; imported transition comparison 465 rows/two partials, strict exit 2. |
| Particle lifecycle | 1,109 development/2,048 holdout, zero values mismatched, 237/372 partials; nine lifecycle faults. Initializer retains 559/1,115 partial identities and seven faults. |
| Gamma selection | Inherited 498 development partials; some old receipts support counts only. Do not upgrade count-only preservation to individual identity proof. |

Task source case207's eleven differences, seven exclusions and inherited
floor_angles/player_reaction failures remain open. STATUS also records CRT/SSE/
x87/D3DX failures and missing reports. Gate-green does not erase those verdicts.
Reset packet `a27ef40` (source744d313) remains preserved/unmerged; eight reserved
vectors are unconsumed. Before use, resolve sealed collector startup, interpreter
execution/import closure, reached buffer presence/value validation, and media
lifetime through diagnostic output. It is not a demonstrated display prerequisite.

## 5. Resume the unfinished Gamma display work

Published draft:
`packet/gamma-display-native@1aa91341d3d2c26b821c09b0701ed1c2709a0d18`.
DiveMac checkout: `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-79a878e5`.

Read on that branch:

- `reference/packets/gamma-display-native/CHECKPOINT.md`
- `NEXT-DEPENDENCIES.md`, `SOURCE-EVIDENCE.json`, `INTERRUPTED-ATTEMPTS.json`
- `ROOT-WIRING.patch`, `JUDGE-PROPOSAL.json`, `verify_packet.py`
- New player/engine/runtime source, importer, probes and native caller test.

Earlier text says uncompiled because it describes the heavy-hold checkpoint.
The later audit found these **historical interrupted attempts**, now indexed in
INTERRUPTED-ATTEMPTS.json (read-only inspection, not rerun on November 4):

- Private app/force-load binary, both probes and caller compilation recorded exit0.
- Topology-prefix development: 1,126 cases, zero recorded value failures, exit0.
- Seven integer render-state leaves: 1,066 cases, zero recorded value failures, exit0.
- Actual caller: **exit -6**, assertion
  `collision->info[0].center.x == pose.position.x`, native_gamma_display.mm:142.
- Normal-input attempt: **exit1**, `imported sample resolves outside asset directory`.
- No holdout/fault/placement validation, passing actual display caller, successful
  draft normal-input frame or complete qualification exists. Audit exact source
  dependencies/private generated headers before attributing any result to the head.

The draft includes a full E102Display dossier, 2515 bytes at 47fd50–480722, helper
47ef40 and leaf dossiers. Primary details:

- NJS_OBJECT+48 is **sibling**, +44 is child.
- Equipped branch loads row22 before row16; ordinary branch row14 before row22.
- The object-table global reloads after each sibling store. Preserve the table
  captures across the exact intervening equipment/extra reads.
- Final row22.sibling.child is written into canonical
  `GammaWeldInfo[0].source_object` at 3c5455c, not a new cache.
- Obtain initialized player from task motion; preserve entry primary/allocation
  captures and lazy field validation. Keep current-player publication unchanged.
- Draft covers topology/render saved state, placement and collision-center prefix;
  first animated action draw, event-animation helper, walking sway, hidden tail and
  attachments still have explicit boundaries.
- Normal rendering requires ds_ActionClipNoScale → model callback/traversal and the
  Gamma object callback. A nonempty event action list requires helper47ef40 plus
  **417fb0(primary,4)** regardless of helper return. Measure the actual branch;
  do not replace it with ordinary drawing when blending is inactive.
- Temporary animation scratch 3c545d0–3c545fb is adjacent to weld info, not inside it.
- Existing combined pack: 3328 records/766112 bytes, SHA
  `c414e173bf4bf9f803f24d61642e233ff9650fe9276d70da2ac3939fee2b5d7f`.
- Private object-only importer appends169 records →3497/783448 bytes, SHA
  `382e3e53b613c42890c9e48afa3fae3e4d9398e6520019a1a2150451422688dc`.
  It preserves old IDs, record bytes except count, and all22 action bindings.
  Missing display object rows are supplied; **arm action export50 is still absent**.
  Keep this appended pack private until real import/composition acceptance.
- Overlay symlinks can violate the importer asset-directory containment guard.
  Diagnose the reported failure and construct a valid private asset tree with
  copied/reflinked contents. Preserve the guard and Main's original pack.

Order of work: recover ignored dossiers/logs → reproduce draft compile and the two
known failures → fix under source evidence → unchanged development comparisons →
placement and caller observation → compiled faults/holdout → actual constructor,
scheduler callback and normal-input draw → independent two-axis review → surgical
shared-owner integration → one coherent full gate and clean source export.
Continue through the first required renderer cluster; another context-only packet
is not delivery. Later explicit stops remain honest dependency progress.

## 6. Private inputs and ignored evidence: Git alone is insufficient

Read `docs/SOURCES.md` and verify pinned bytes before original comparisons:

- sonic.exe SHA256:
  `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
- Mod Loader source/submodule `eb4cdcca9637d611a98b910fcfed6ce18580451c`;
  Input2.6.1, Speedrun2.0 and exact DLL identities in SOURCES/profile metadata.
- Windows32 msvcp140.dll SHA
  `f0cda2a0cf1fe6fbbf579b9098462329d3aaa7513a207af2e0f33b01456e388a`;
  vcruntime140.dll SHA
  `2fa6efc053203460a23d3a25158f227d895d2dadc63acc1a372da97c3a4281c3`.
- sa_tools symbol input checkout pinned to
  `b72d52f37c21a2f7215711e3516360a63aea12da`.

DiveMac Main root is `/Users/burbujamc/Developer/sadx-macos`. Required local trees
may already exist on Macintosh from older work; verify before copying:

- `game/imported/` (native assets), `game/Sonic Adventure DX/` (development original),
  `reference/updated-pc-reference/`, `reference/windows-system/`.
- `reference/sa-tools/`, `reference/tools/rea/`, Ghidra tools where required.
- Ignored binary files named by `reference/judge-manifest.json`, frozen corpora,
  source dossiers under reference/inventory-raw and retained raw oracle streams.
- `reference/story-route-20261020/` only when extending native/reference story work;
  the retained reference replay has410/410 checkpoints through Mystic Ruins train
  arrival. It does not prove native gameplay or later reference Egg Hornet route.

Use trusted `ssh DiveMac` / `ssh pi` aliases and existing host verification after
reading Macintosh's operational notes. Copy into a new staging directory first;
compare bytes, inspect links and promote explicit files. Preserve local work and
saves. Do not copy T3 SQLite/WAL or run evidence/binaries across machines blindly.
Game assets/EXE/DLLs, private memory and raw source caches stay out of GitHub.

A compact **private handover supplement** is ready on DiveMac:

`~/.local/state/sadx-macintosh-handover-20261104/`

It contains MANIFEST.json, a 2.4MB compressed dossier/log/project-memory/storage
bundle, six canonical archive receipts/manifests and local-packet-refs.txt. The
bundle `private-evidence-and-restore.tar.gz` SHA256 is
`b9582ee82d1164b888e68400d7d7d955f18ca331a46d16d4ab2fe6255b27a0b7`.
It excludes the full game/input trees and compiled game libraries. Private links
may still point to DiveMac paths: inspect rather than execute through them.

From Macintosh, copy it into a fresh private destination, for example:

```sh
mkdir -p "$HOME/.local/state/sadx-macintosh-handover-20261104"
rsync -a DiveMac:/Users/burbujamc/.local/state/sadx-macintosh-handover-20261104/ \
  "$HOME/.local/state/sadx-macintosh-handover-20261104/"
shasum -a 256 "$HOME/.local/state/sadx-macintosh-handover-20261104/private-evidence-and-restore.tar.gz"
```

Review tar entries before extracting to private staging. It includes the ignored
Gamma display dossiers, generated fixture/logs/saves and partial reports, project
memory, and storage restore maps. Use it as evidence/context, then verify current
checkout inputs. Import only relevant durable notes into Macintosh's own canonical
Claude memory; preserve its device-specific configuration. Never send it to devbox.
The full local packet branch/ref list is an index, not a claim that every historical
branch is published; required current source is on Main and the published draft.

## 7. Pi HDD offload and evidence recovery

On November2 the user stopped SADX and authorized offload. **46 closed build and
qualification directories**, **25,383,799,097 logical bytes (23.64GiB)**, were copied,
remote-checksummed, fully restore-rehearsed on Mac and locally released. DiveMac
free space was then68.76GiB; remeasure rather than assuming current capacity.

Archive root: `/srv/seafile/archive/divemac-artifacts`, on ext4 HDD UUID
`d7fdcd6c-c069-4dbd-945d-1bf2fe4cc1f5`, not the Pi SSD or Seafile-managed data.
Sources/Git/game/reference inputs and current display worktree stayed local.
Main's old cold-source output paths are now ARCHIVED markers; that is expected.

| Batch | Archive ID | Logical bytes |
|---|---|---:|
| 1 | sadx-34ded4363b5e4111a6dfa74ecb0adaf6 | 4,927,832,714 |
| 2 | sadx-f7371da6db7b42bcb1406d3cb6dad66f | 4,531,999,055 |
| 3 | sadx-bf9556c468ff4862bea2df1eaf19f063 | 5,240,029,486 |
| 4 | sadx-c421dba078e947a280267a84dcaeb959 | 5,149,030,224 |
| 5 | sadx-0a9310db29b94b8d9e9a9c84d479afae | 5,252,506,938 |
| 6 | sadx-f454e3ca500c47cba17f024bd9711e43 | 282,400,680 |

Private operator state on DiveMac:
`~/.local/state/sadx-stop-offload-20261102/`.
Read PLAN.json, COMPLETION.json, REMOTE-FINAL-CHECK.json, batch-N.json and RESTORE.md.
The handover supplement includes these and the canonical six manifests. Each
original build path has an adjacent ARCHIVED.json naming its batch/relative path.
Each package contains RELOCATION.json. All46 original mappings and all manifest
hashes are carried by those machine-readable files; do not infer from filenames.

The unchanged qualified helper is
`~/Developer/burbuja-cluster/storage/pi_artifacts.py`, SHA256
`29e0e8015e91cfbbbc4fd6bac5a820f00b1278ce66caa8f1a53a279f420046d2`.
Its original roots and paths are DiveMac-specific. Simplest supported hydration:
restore on DiveMac under its common-Git maintenance lock, then copy selected
hydrated evidence to Macintosh. Respect the helper's global mutex, HDD UUID/capacity,
Pi memory/load/service guards, full hashes and no-overwrite rules. Do not weaken
those guards or rewrite canonical receipts to pretend the source was Macintosh.
For direct Macintosh read-only recovery, fetch an archive into a new private tree
and verify every regular file, mode, link and hardlink group against its canonical
manifest before use; this is a new recovery operation, not the original-path helper
restore. Symlink mode bits differ across hosts. Avoid broad rehydration of all24GiB.

Main's source-only outputs are in batch4; source raw oracle evidence in worktree
build archives may span batches. Reusing a historical judge requires its exact
source/library/dependency/stream closure; restore dependencies first. New builds
can be rebuilt locally while historical receipts remain explicit.

There are also **13 earlier cold whole-worktree archives** from other cleanup
rounds. Their missing Git registrations are deliberately LOCKED. Preserve them;
restore exact original paths and validate before unlocking, never prune them.
Details are in DiveMac's cross-project memory `worktree-cold-archive-20261102.md`
and its private restore receipts. They are separate from these46 build outputs.
Storage is now complete; no recurring cleanup task should be restarted.

## 8. Macintosh environment and repeatable baseline

Macintosh historically has24GB RAM; verify hardware/headroom and tool versions.
Use a login shell and native arm64 Xcode command-line tools. The DiveMac qualified
build used Python3.14.7, Unicorn2.1.4, capstone5.0.9, clang-format18.1.8,
REA6.4.0, Ghidra12.1.4/JDK27, SDL3.4.2 and libusb1.0.29. These are reproduction
facts, not permission to upgrade toolchains or providers. Prefer the pinned setup.
Recreate a venv rather than copying its absolute-path scripts. `reference/python`
/bin/pip was stale; invoke `reference/python/bin/python -m pip`. Pin the same
interpreter across timed helper processes; macOS mixed3.14/3.9 clocks can disagree.

Regenerate symbols from pinned inputs (`make symbols`), then run checkers. A
missing generated symbol database can cause provenance failures before any source
error. Ghidra environment configs can contain old absolute paths: use the owned
runner and validated current inventory environment, verify REA doctor, and update
paths in a small evidenced change only when needed. Cached dossiers first; one
fresh Ghidra owner at2GiB, close it before the full gate. No GUI/live rename/Hopper.

Build fresh on Macintosh. Public dependencies can be rebuilt there or copied into
local ordinary directories with links dereferenced. **Never share a writable
controller-dependency install symlink into another checkout/Main.** A previous
libusb build damaged four Root cache files; the recovery receipt is retained.
When deliberately reusing an already qualified local dependency product, use
`-o build/native-controller-deps/include/SDL3/SDL.h`; first verify the product exists.

First establish source and inputs, then:

```sh
make symbols
make -j2 build/sadx-native native-closure
python3 tools/gate/check_judge.py
```

An isolated checkout missing53 protected files is an input-recovery issue, not
permission to change judges. Create a **new private copy** of the retained baseline
save set, never play against the only retained copy. Its three file hashes are in
`sonic-initialization-fan-source-only.json`; the set is included in the private
Gamma display baseline bundle. Verify before use. After saving stdout/stderr and
actual exit status, run:

```sh
build/sadx-native --assets game/imported \
  --controller-library build/native-controller-deps/lib/libSDL3.0.dylib \
  --offscreen --paced --exit-after 1550 \
  --save-directory build/macintosh-baseline/savedata \
  --key-events '500:13:down,501:13:up,750:13:down,751:13:up,950:13:down,951:13:up,1050:13:down,1051:13:up,1300:13:down,1301:13:up' \
  --screenshot build/macintosh-baseline/frontier.ppm
```

Expected current Main: exit1, Display1332/title hash above. Inspect the capture
before claiming pixels. Convert PPM to PNG for tools that cannot view PPM. Full
gate on Macintosh must run from a usable GUI console session. All physical USB
controller validation is machine-specific; requalify when hardware paths change.

## 9. Review, build and tool discipline

- Root owns Session/PlayerSystem/Display/shared state, callback registration and
  final integration. Workers own disjoint caller clusters in durable packet
  branches. One owner per source address; borrowers retain exact lifetimes.
- Max `-j2` per development worker; two concurrent heavy slots. Full
  `make -j10 gate` is serialized after explicit worker/Ghidra clear acknowledgement.
  Release held workers explicitly afterward; an idle completed T3 child may need
  `mode:auto` to resume. Current DiveMac children remain stopped.
- Two independent Standards/Spec reviews for a recovered system or coherent
  integration. Fix small findings as orchestrator; new recheck gets a fresh
  delegate, not a continuation of its previous reviewer.
- Context before coding: caller chain, source body/instructions, names/types,
  profile patches, canonical owners, existing maintained native callees.
- Game behavior stays in player/engine/game; adapters transport owners and lazy
  missing facts. Preserve source captures versus rereads, signed widths and store
  order. Explicit missing behavior uses STUBs; no guessed defaults/skipped actor.
- Existing inputs, cases, tolerances, expected values, exclusions and failure/FP
  identities remain. Judge corrections are separate evidenced commits. Unchanged
  valid evidence can be reused under exact source/execution contract identity.
- Tests require manifest registration. `tests/unit/manifest.tsv` has **two useful
  columns**; put every argument separated by spaces in column2. Column3 is ignored.
  Run the generated command. Two earlier rows lost arguments until corrected.
- Generated callback objects require `-MMD -MP`, included .d and a Makefile
  prerequisite to bootstrap old objects with missing dependencies. Verify actual
  shipping source matches any prepared callback source; private wiring alone is
  not shipping integration.
- Preserve all newer manifest entries by union on admission; an older worktree
  lacks locally ignored inputs. Never overwrite current shared headers/manifests
  with stale complete files. Source commits precede judge qualification commits.
- Inventory is generated from tracked scope. Stage new files, regenerate after
  final STATUS text changes, and preserve authored review.json/open debt/lineage.
  It is accounting, not semantic acceptance. Save-warning/constexpr full-file
  fingerprints need narrow rebinding when bodies/declarations are unchanged;
  warning debt stays open. Source annotations and symbol extents move together.
- Retained unified patches contain meaningful context-line whitespace. Preserve
  patch bytes/hashes; inspect whitespace on other files without breaking patches.
- `rg` bounded paths and filenames first. Never grep huge single-line
  `reference/*json` snapshots. It once emitted hundreds of MB.
- Run oracles with the project Python and ensure the probe is newer than source.
  Fixed-output mutation tools can overwrite ignored historical receipts/libraries;
  preserve prior outputs or use a new private output location. Superseded raw
  artifact paths are not fresh historical hash-preservation proof.
- After three failed approaches on a function, retain evidence/unknown and move
  to another independent prerequisite. Avoid building a new general collector
  framework without a concrete need. Stop adding context documents when source
  and testable integration work can proceed.
- Record only real exits. Full gate example: a Python subprocess recording
  returncode, or a shell with pipefail; do not report tee's status as make's.
  `record_gate.py`/`gen_status.py` handle frozen evidence/status. Run affected
  oracles separately and track their reports. A partial strict exit2 remains2.
- T3: discover capabilities once before custom delegation; user-requested
  medium/priority settings go in target.options. Separate/background work uses
  delegate_task. Follow next=end-turn; no polling running delegates. Read a given
  thread before reporting about it, use returned links. No new top-level thread
  unless requested. No service/provider/storage resumption hidden inside a prompt.

Completion for this takeover's first slice: Macintosh can reproduce Main's honest
baseline, the Gamma display draft failures are understood under exact source
observation, the required display/renderer cluster is implemented in readable
source with meaningful comparison/fault/caller evidence, independent reviews
accept it, and the next coherent visible checkpoint is exercised through normal
input and clean source build. Continue the route; report any remaining boundary
with its real state/image/exit. Preserve DiveMac's stopped state throughout.
