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
