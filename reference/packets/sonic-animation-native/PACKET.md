# Sonic animation asset owner candidate

Base Main: `08820f79`. Branch: `packet/sonic-animation-native`, supplied worktree
`t3-2993746c`. Root owns independent review, current-Main integration and gate.
Source commits: `a77d7110`, correction `02a6d9a4`; separate judge qualification follows.
No preexisting Main source, judge, manifest, build registration or Gamma loader was edited.
No Main write, push, fresh Ghidra, reserved Task/reset vector or full gate occurred.

The new `native::SonicAnimationAssets` is a fixed-location, noncopyable owner.
It uses `animation::initial_sonic_table` / `initialize_sonic_table` unchanged,
retains all 147 scalar/identity rows and shares the same immutable table through
`TableStorage`. Its 149 export declarations distinguish loaded, known-null and
unavailable content. Declaration identities preserve source aliases independently
of payload availability (this source has no repeated nonnull export pointers;
the output table does reuse declarations). Original export slots 24/25/26 are
null; table rows 75/76 also have literal-null actions in the canonical recipes.

Only exports 1, 5, 100 and 101 have existing imported payloads. The other 142
nonnull declarations remain unavailable, with source-resource diagnostics.
The owner also resolves the existing constructed Sonic settled-preview action.
It converts their unchanged MDATA2 motions, nodes and key bytes to the existing
`animation::Resources` type, preserving pack identities and per-key shared backing.
No parameter, player, pose, geometry, event or Gamma owner is duplicated.
`resources.objects` remains null: hierarchy-dependent transition/model composition
must borrow the future shared object view and currently fails explicitly.
No MDATA3 payload or adjacent source bytes are invented.

`tools/import/sonic_animation_resources.py` writes only separate
`sonic-animation.json` / `.provenance.json` beside an existing combined pack.
It rebuilds the maintained Sonic + Gamma import and requires exact equality with
that pack before emitting metadata. All 3,328 records, all descriptor identities,
Gamma material/setup/texture contracts and existing files remain unchanged.
There are zero appended records and no resource renumbering. Inverse/source
hashes and declaration alias inverses remain in the local provenance output.
This command refuses a newer/different pack; Root must reconcile a later importer
change explicitly instead of replacing that pack with this baseline.

Pinned source hashes and results are in `original-imported.json`:

- PC `6b0937ca…24cd74`, original CHRMODELS `76bbaa1b…80c2a5`.
- Combined pack `c414e173bf4bf9f803f24d61642e233ff9650fe9276d70da2ac3939fee2b5d7f`.
- Export map `cca605d27263951f0649d79b26c6602e3731b735c8098c1cbc3c9f1162e12ce9`.
- Producer `7cec90`, 7,674 bytes, SHA-256
  `4fb64e48d45e6008328627f24c9065e89240ec2b5458cedfb73a6ce26d183944`.

Evidence comes from pinned images, maintained importer inverse proofs,
`reference/sonic-animation-table-recipes.json`, cached
`reference/ghidra-action-fp-recovery/animation-producer-7cec90.json`,
`reference/player-animation-provenance.md`, and the existing emitter/publication
judge. No upstream implementation is used as behavioral truth.

Qualification (final rerun after Root explicitly released the serialized gate slot):

- Native archive built locally with `-j2`, dereferenced local SDL/libusb copies,
  and `-o build/native-controller-deps/include/SDL3/SDL.h` reuse. The new owner
  is compiled explicitly for this isolated witness; native list remains Root-owned.
- Imported witness and UBSan variant pass: 149 declarations, all 147 table rows,
  exact reached motion/node/key metadata, seven invalid typed declarations,
  twelve malformed/missing/null JSON cases, two actual Sonic initializations,
  same-table binding, isolated state, exact Stand half-frame playback, explicit
  unavailable consumption and table lifetime retained by real player borrowers.
- Without controlled editor inputs, the witness retains the real editor stop.
  With explicit fixture reads, actual `sonic::run` initializes and binds before
  stopping at `sonic_model_graph`. Those fixture reads are not production defaults.
- Imported table matches all 2,352 bytes from original PC execution, including
  unavailable source identities and exact speed/transition encodings.
- Unchanged table oracle: 103 cases, zero byte or FP mismatches, pass.
  Unchanged publication unit: 150 prefixes, 145 ordered export reads,
  882 ordered field publications and 147 rows, pass.
- Five separately compiled faults caught: Stand identity, speed, motion length,
  node key count and copied table in the actual Sonic binding (`faults.json`).
  A sixth compiled before-case preserves a77d7110's default-null construction: it
  accepts an unfilled declaration array and fails the added seventh negative.
  The correction defaults declarations to unavailable; all old inputs remain.
- Import is repeatable; wrong source components and changed pack are rejected
  without replacing existing metadata (`importer.json`).
- Readability, layer, unique-owner and provenance checks pass. Provenance first
  failed to start because local symbol DB was absent; rerun with the existing
  Root DB succeeds. Initial profile-check invocation had zero reports and is
  not claimed as addressed-range evidence. Final check addresses the full
  producer range and has zero UNHANDLED overlaps.

`VERIFICATION.json` records all 20 final commands at exit 0, source fingerprints,
counts, diagnostics and limitations. Five distinct native source programs pass;
the asset witness also passes with UBSan on the test and new owner. Existing
archive objects are not sanitizer-instrumented. Final before-case/compiled faults
are separate from the successful source witnesses. No existing judge inputs,
thresholds, tolerances or expected values were weakened. Root must register the
new unit and three verifier scripts without removing any inherited protection.

Normal app progress is Root-owned. Root reports its newer Main now reaches
PSetMotion after accepted Gamma effects. This packet does not move that route,
complete an actor frame, qualify N3b, or recover PSetMotion, editor/forced state,
Sonic model/weld/common tail/gameplay, other actors or Gamma effects. Existing
floor_angles/player_reaction failures and all FP partials remain unchanged;
their suites were not rerun because their accepted code is byte-identical.

See `ROOT-WIRING.md` for the small shared-file integration contract and exact
local reproduction commands. No existing judge was corrected; all new tests
and packet verifiers are additions awaiting Root's manifest registration.
