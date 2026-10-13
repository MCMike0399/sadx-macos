# Native source plane intersection

Target sonic.exe SHA256 6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.

REA evidence: mesh-transform-dependencies/plane-intersection.json (7887d0),
plane-dependencies/plane-parallel-classifier.json (795820), and retained
physics-dependencies/normalize-cross-vector.json (788d40). Exact LLVM
instructions corroborate the original classifier and signed square-root call.
The new owned Hopper session was exported and closed with verified cleanup.

The native classifier preserves cached loads, binary32 dot/length spills,
signed square-root behavior and strict source threshold. The intersection
body is statically lifted into direct X87Context expressions by
sadx-native/tools/emit_player_plane_math.py. No runtime interpreter, guest
memory or address-based execution is present. Near-parallel input clears all
six output fields before actual point-to-plane distance evaluation. A null
nonparallel output skips intersection arithmetic. Input/output aliases
preserve original field reads and stores, including the retained normal-Z
value before its rounded store.

The private original-instruction comparison runs 1,487 fixtures across 12
masked control words (17,844 evaluations). All output/input fields, return
encodings and control words match, including aliased/null outputs, threshold
neighbors, nonfinites, subnormals and random finite planes. There are 15,264
low-six exception-status disagreements, retained separately. Tests are
selected synthetic local inputs, not observed gameplay or Intel hardware.
The native contact response43c580 now uses this owner; its scene normal query
and full geometry/game integration remain separate requirements.

Production integration update: the combined arm64 gate now passes44 native programs and4 extraction tests. Current production probe/report digests are pinned in continuation.json; older private-library measurements above remain historical. Full game/hardware equivalence is still unverified.
