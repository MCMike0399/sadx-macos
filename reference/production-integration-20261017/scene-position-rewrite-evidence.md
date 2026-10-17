# Scene positioning readability evidence

The pending native/reference rewrite keeps the existing 449500/445d10 and
43ca40/43cb40/444f10 fixtures and comparison logic. The current core exposes
optional fixture fields and numeric diagnostics to game code. The new reference
adapter needs guest linkage in the two existing probes and scene-position unit.
The binding unit already links that archive.

This preparation changes only link dependencies and source identity collection:
the two verifiers include the scene family, reference family and ExactFloat header.
No fixtures, expected values, comparison words, tolerances or case loops change.
Before rewriting, the original dylibs were copied to /private/tmp for direct
old/new comparison, including unmasked exception runs.

Evidence: reference/player-scene-position-provenance.md and cached Ghidra dossiers
reference/ghidra-scene-helper-recovery/{scene-platform-motion,scene-normal-nudge,
scene-floor-rotation}.json; source constant bytes decoded through PEImage.
Profile checker currently reports 0 UNHANDLED overlaps.

## First attempt: three leaf helpers

`player_scene_position_helpers.cpp` now owns all three game algorithms, with no
address literals, fixture optionals or emulator calls. Its lexical debt fell from
49 hex literals / 16 address names / 2 gotos / 27 long lines / 26 compressed lines
to zero. Constant decoding and lazy diagnostic transport moved to the guest;
no helper game decision moved there. The main dispatcher/header remain legacy
and their rewrite is still required. This does not complete GOAL 6.4's module.

The native interface borrows plain actor, motion, contact and platform owners.
ExactFloat retains nudge/platform values and software FP state under all twelve
precision/rounding profiles. Runtime census has no observed calls for those two
helpers; it does not justify a plain host-float substitution. Floor code keeps
its binary32 spills, Bochs FPATAN, two m64 products and CRT conversion. Byte-exact
checks compare all four native literal defaults with the source constant pack.

Saved old/new helper dylibs compare every status/output word in 56,532 runs:
673 existing cases across 12 masked and 72 single-exception-unmasked controls,
zero differences (output hash in scene-motion-173-mutants.json). On an arithmetic
trap the existing probe publishes no partial state; the new diagnostic unit adds
an explicit overflow-after-first-position-store check instead of claiming that
raw comparison observes all partial writes.

The new reference unit also passes when compiled against the old helper source,
with the same assertions for missing facts, source sites, partial pitch stores,
null force work, null motion, callback failure and temporary-position retention.
Native tests run without assets or guest linkage, covering successful and failed
callbacks, same-row reads after player-index mutation, modulo angle additions,
idle center calculation, floor copy and speed-limited approach at all 12 controls.

Independent STYLE review found a native registry alias defect: passing a slot by
reference let callback mutations change its argument. The regression clearing
that slot failed before the fix and passed after copying the TaskHandle locally.
The reviewer also corrected ambiguous FPATAN argument naming. Final review
approved this cluster only, subject to verification; the dispatcher was excluded.

Three arithmetic mutations (push distance, slow floor step, angle wrap) are caught
by the saved baseline comparisons, with 12,356 / 144 / 12,261 differing runs.
A fourth mutation retaining the temporary pose after callback success fails the
new native test. All source mutations were restored and the baseline rerun passed.
An initial mutation runner used an absent multiline edit; it stopped and restored
the source before the corrected four-mutation run. No mutation survived that run.
