# Root-owned registration proposal (not applied)

Root must add `src/player/gamma_cursors.cpp` to native-sources.txt. Runtime
implementations use existing native discovery:

* src/player/gamma_cursors.cpp (explicit native-sources.txt row)
* src/runtime/native_gamma_resources_native.cpp
* src/runtime/native_gamma_assets_native.cpp

New unit paths: tests/unit/gamma_cursors.cpp (portable; core archive),
tests/unit/native_gamma_weld_cursors.mm (platform/native archives; game/imported argument).
New probe path: tests/probes/native_gamma_cursors_probe.cpp, compiled with
src/player/gamma_cursors.cpp and src/engine/task_work.cpp only.
New original judge: tools/oracles/verify_original_gamma_cursors.py, arguments
--exe OWNED_GAME_EXE --probe PRIVATE_PROBE --output REPORT [--holdout].
Full exact original ranges: 4c2d40..4c2de3 and 4c2e10..4c2e47 inclusive.

symbol-additions.tsv proposes the three descriptive names. Existing GammaWeldInfo
and GammaInitializeCursors names come from the source SDK/prior symbols. Existing
GammaInitializeCursors seam annotation remains untouched; its native binding delegates
to the complete new implementation, preserving original Gamma core/probe/mutants.
No CPU baseline change is needed or allowed.

Existing tests/unit/native_gamma_binding.mm expects the preceding weld boundary.
It remains untouched in this implementation packet. Root owns a separate evidenced
consumer judge update once the actual next boundary and new unit are qualified.
Original E102/startup judges, tolerances, mutants and expected values remain frozen.

Qualification finished at source commit 110db08. Original development and final
untouched holdout both pass strictly; eight meaningful new mutants are caught.
Use tools/oracles/mutate_gamma_cursors.py --exe OWNED_EXE --output PRIVATE_JSON
for the isolated new mutation suite. It never edits product source or a judge.

The actual next boundary is native_gamma_load_setup_resource at frame 1382,
exit 1 with Root's exact normal-input route and a fresh private save directory.
The new composed unit independently reaches that boundary. Old binding unit
can be updated separately by Root to call bind_gamma_resources using its same
CharacterPack/SceneState, then assert actual callback/mode/cursor stores and
this next fail-loud setup seam. Do not adjust old original core/probe expectations.
