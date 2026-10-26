# Exact source material correction

Frozen on packet/material-correction, based on Main 04c40ae. Independent review and
Root runtime binding remain pending. No renderer, GPU, N3b or story claim.

Source: full fresh CorrectMaterial at 40a340, source ID ev_93b5607dad9b91f61c967d313222d9ddb617919575a33ea9d2ff782a063f3d89.
Dossier hash/range/constant bytes are in reference/production-integration-20261026/material-correction/source.json.

The plain engine function borrows its device material, arithmetic context and source
specular power through ordinary link-time functions. No service hierarchy, manager
or owning game-state copy was introduced. The local material is the source's stack
copy. Nine RGB channels are normalized in ambient, diffuse, specular order; alpha
and emissive remain unchanged. The first multiply precedes the Power read, whose
store precedes the first binary32 channel store. Maintained scale 0.003921569f matches
original m32 bytes 8180803b. Every new game readability debt category is zero.

Development 4227 cases: zero value/API mismatches, 3231 FP status mismatches.
Holdout 6227 cases: zero value/API mismatches, 4567 FP status mismatches. Both are
PARTIAL, never a strict pass. Six masked precision/rounding controls, independent
channels, signed zero, subnormals, infinities, NaN payloads, random bits and missing
API receipts are covered. The original instructions execute without patches.
GetMaterial receipts supply all 17 words even for failing HRESULTs; this proves
that the source ignores HRESULT, not real failed-driver output initialization.
Unmasked exception prefixes remain unverified.

All 5 value mutants compile and are caught: wrong scale, missing ambient red store,
skipped specular, modified alpha and Power captured before GetMaterial. The unchanged
mutation runner reports restored baseline partial, 5 of 5 valid and zero invalid.
The registered unit additionally observes exact load/multiply/store/Power-read order
and local-copy publication; it passes. Provenance, layers and owners pass; profile
checks one exact full range and reports zero UNHANDLED overlaps.

Receipts: reference/production-integration-20261026/material-correction/{development,holdout,profile,proof}.json and mutants.log.
Raw artifacts remain in build. Main's supporting core archive was initially copied
for the standalone ordering unit; the mutation make run subsequently rebuilt the
private core from maintained sources. No original executable is consumed by the
component builds; the source oracle alone reads it. No source-only runtime claim.

Reproduce with the default oracle-material-correction target, or the oracle Python
CLI for an independent holdout seed/count. The explicit Make rules compile this
engine source with link-time receipts; the existing native product source registry
is unchanged until Root binds real device operations. No old judge expected values,
fixtures or tolerances changed. The only existing Make edit adds two new targets.

Root must bind get_material/set_material to the current device state, exact to the
existing Display arithmetic context, and specular_power to one source owner after
producer/lifetime evidence. Bootstrap's actual callback identity must be preserved
and snapshotted by its caller. Native runtime integration/full gate remain pending.
