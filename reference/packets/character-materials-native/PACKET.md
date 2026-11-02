# Canonical mutable character materials

Candidate branch: `packet/character-materials-native`. Base:
`eb49adc75d5398780664b43d77d5061be6260a7d`. Source: `4a33d28`.
Separate judge adaptation and consumer evidence: `12c2e14`.
This is an N3b dependency candidate for Root's independent review and integration.

## Ownership and source semantics

`engine/character_material.hpp` declares the lossless 20-byte material record.
Its legacy `words` view preserves existing reference/probe compatibility; native
Gamma uses named diffuse/attribute accessors. `MaterialState`, implemented in the
already registered `character_geometry.cpp`, constructs fixed-size typed arrays
once per imported ResourceId. `SceneState.materials` owns them for the scene's
lifetime. There is no separate native Gamma material copy and no new source owner.
No new translation unit, native-source registration, symbol row or GLOBAL row is needed.

The source has two distinct paths. Gamma 483430 reads `E102_MATERIALS` directly,
in order `(1,6), (5,2), (4,2), (3,2), (2,2)`; it does **not** traverse an object/model
for these stores. The imported five tuples bind those initial aliases. Every
native material call rereads its live declaration; unknown accesses still stop.
Row 2 publishes a pointer into the canonical stable array. Rebinding the table
later does not retroactively change that published pointer. Only these five
source accesses are currently bound; this is not recovery of every table index.

The renderer traverses the live `SceneState.objects` object/model link through
4034b0. At entry, 77eda0 captures model points, normals, meshsets, material-array
base and count. Its 77ee24 load is from the retained stack slot, **not** a fresh
model+16 load. Each material transition then reads the selected canonical record;
the source callback snapshot and arithmetic/state/primitive order are unchanged.
No new mutable model-descriptor behavior is introduced. All actors using the
actual runtime object/model renderer now borrow the same scene material owner.

`source-context.json` records exact input hashes, original table/record identities,
real model users and cached dossier evidence. All 249 imported material arrays
(1,990 words) inverse exactly to the pinned original DLL. The pinned profile's
loader explicitly loads `CHRMODELS_orig.dll`; its `CHRMODELS.dll` is a proxy, not
the asset archive. `profile.json` checks 14 original ranges with zero UNHANDLED or
undetermined overlaps. No ResourceId is hardcoded in game/runtime source.

## Actual native consumption and frontier

`native_gamma_actor_assets.hpp` validates all five real declarations, their row/
index roles, ResourceId kinds and record bounds. Missing/invalid material content
fails at import/binding. Gamma's Context borrows Session's actual scene. Existing
setup storage/registry/resource code remains unchanged.

The actual native Gamma witness executes Task/PInitialize, ordinary texture load,
weld/cursors, real imported setup and all five source material stores, stopping at
`native_gamma_initialize_effects` (49fb10). It checks all five words, canonical
headlight identity, live table rebinding and stable retained publication.

The consumer witness then records three real `graphics::Draw` commands through
actual object traversal, basic-model parser, native material/device bindings and
normal-UV buffer implementation. Two model identities alias the Gamma-mutated
array; a changed live object/model link produces a different third material.
**This uses a controlled graph fixture:** two model/mesh descriptors are rewired
to the existing supported Question strip; original material/geometry payloads are
retained. It proves real consumer visibility, not Gamma actor mesh pixels. The
compiled immutable-reader fault fails specifically at recorded Draw inspection;
a detached Gamma owner fault also fails. Neither fault changes an expected value,
tolerance or original comparison input.

Normal input with the retained setup-frontier schedule reaches that same effects
stop at frame 1332, exit 1, with one setup sprite. The screenshot was inspected:
it remains the outgoing title and is byte-identical to the baseline. A source
archive of `4a33d28` contains zero EXE/DLL/prebuilt objects; all maintained and
numerical code rebuilds, native force-load closure passes, and its app reaches the
same stop/image with external imported assets. Pinned public SDL3/libusb alone
is reused. `frontier.json` and `source-only.json` bind the commands/state/hashes.
No actor frame, N3b, whole renderer, gameplay, cadence or Windows parity claim.
Effects/common tail and unsupported actual Gamma mesh primitives remain open.

## Verification

`verification.json` preserves the exact results and mutant receipts.

- Seven focused native programs pass: native Gamma binding (both missing-setup
  and real-setup paths), weld/cursors, shared animation binding, asset decoding,
  setup caller, basic-model unit and actual model binding. Old assertions remain;
  the real-setup witness's next stop is extended with stronger source-backed checks
  in the separate judge commit.
- Seven malformed material declarations are rejected. Every imported material
  word inverses to the pack, and unusual raw word/NaN patterns survive construction.
- Unchanged original renderer: 2,427 development / 3,427 retained holdout cases,
  zero value failures, 172 / 274 FP partials, strict exit **2**. Complete ordered
  receipts, including individual FP observations, equal the frozen old probe for
  both cohorts. All seven existing value mutants remain caught.
- Unchanged original Gamma: 1,422 development / 1,080 retained holdout cases,
  zero value failures, 498 / 690 FP partials, strict exit **2**. Every complete native
  record, including each FP status, matches the frozen baseline over all 2,502
  cases. All seven existing mutants remain caught.
- Two new actual native ownership/consumer faults compile and fail as intended.
- Layers, readability, global owners and provenance pass. App and force-load
  closure pass, including the clean source archive. Full gate is Root's step.

Build in this worktree with at most two jobs:

```sh
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h \
  build/sadx-native native-closure build/native-basic-model-probe.dylib \
  build/native-gamma-selection-core-probe.dylib build/native-gamma-binding-test \
  build/native-gamma-weld-cursors-test build/animation-gamma-shared-binding-test \
  build/animation-gamma-assets-test build/model-binding-test \
  build/gamma-setup-caller-test build/basic-model-test
reference/python/bin/python reference/packets/character-materials-native/verify.py
```

Touch/rebuild the Gamma test when changing its new fixture header until Root adds
the dependency below. The verifier expects frozen baseline libraries under
`build/character-materials/baseline/`, whose hashes are in `verification.json`.
They remain in this durable worktree. Copy only after hash verification, or rebuild
from the stated base archive; never manufacture a baseline from candidate source.
All raw reports/cases, capture images, rejected launches and build logs remain
ignored under `build/character-materials/`; the initial build log is `build/baseline-build.log`.
`inspect_source.py` takes assets, pinned EXE, `CHRMODELS_orig.dll`, and output path.

## Root integration requirements and limits

Do not cherry-pick unrelated candidates. Sonic/event-action code, Session's broader
composition, source manifests, global inventory, original comparison scripts,
tolerances and frozen inputs were not changed. Root's existing root PACKET.md and
prior production receipts are preserved.

Add this prerequisite to the existing Make target (no new recipe or test row):

```make
build/native-gamma-binding-test: tests/fixtures/character_material_consumer.hpp
```

Existing native test/probe registrations remain correct. Refresh the shared judge
manifest in Root's separate integration judge step after independent review;
`proposed-judge-inputs.json` gives exact changed/new candidate hashes. No shared
manifest, inventory or generated STATUS was edited here. The standalone judge
check currently exits 1 with 61 violations: 53 inherited ignored corpus inputs are
absent, five adapted protected test/probe inputs changed, and three inputs await
Root registration. No old protected path or corpus was removed to hide that.
Full gate, Root review and final registration are unrun integration work.

**Actual cache incident:** the first build followed a dependency symlink and
installed rebuilt libusb into Root's `build/native-controller-deps` before detection.
The builder was terminated during SDL configuration; the link was removed and
all subsequent builds used local dependency copies with the header target marked
old. `execution-notes.json` lists the four affected Root cache files and retained
log hash. Root must not assume that cache is byte-unchanged; its `.pc`/`.la` prefix
metadata may name this worktree. No Root game source or game asset was changed.
