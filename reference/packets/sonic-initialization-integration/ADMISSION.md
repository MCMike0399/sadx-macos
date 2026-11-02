# Finite current-Main admission

Qualified fixed base: `bdb61450`. Source: `fe617ecb`. Accepted candidate: `e057f048`.
Newer Main: `ba88c5cc` after fan integration `08884f1d`.

1. Adopt `fe617ecb` source and its historical candidate receipts. Its callback hunk
   changes only Sonic and leaves Gamma activation and shared headers intact.
2. Apply the final judge commit's Sonic tests/helpers/report changes. Reconcile
   Makefile and TSV append conflicts by keeping both fan and Sonic registrations;
   every useful TSV row has two columns, with all arguments in column two.
   ROOT-JUDGE-DELTA.json identifies the exact ten additions and changed old hashes.
   It is a finite proposal; never copy this packet's manifest over current Main.
3. Retain current `gamma_fan.*`, `gamma_selection_core.hpp`, `native_display.hpp`
   (`Display::sine`), `native_gamma_binding.cpp`, native-source list and nine fan
   protections. NEWER-MAIN-DELTA.json enumerates them. Combined protections: 1,460.
   Keep all 53 inherited locally absent binary protections as well.
4. Regenerate the inventory against Root's final tracked files. Preserve authored
   review.json, warning binding reviews, all open debt and the 64-byte matrix widths.
   This packet changes no shared Session/PlayerSystem/Display declaration.
5. Refresh only the finite judge hashes after source is committed, with the local
   INTEGRATION.json as focused evidence. Root's local binary evidence allows its
   final check to distinguish missing inputs from actual drift.
6. Execute the coherent full gate when Root's serial slot policy allows it.
   No full gate or cold export ran here. No push is needed from this worker.

Review the final delta with:

```
git diff fe617ecb packet/sonic-initialization-integration -- Makefile tests tools/oracles/verify_original_sonic_initialization.py reference/packets/sonic-initialization-native reference/packets/sonic-model-native reference/oracle-reports/sonic_initialization.json
```

Fresh focused commands are preserved verbatim in INTEGRATION.json. They include
actual app/force-load, thirteen generated test commands, source/helper dev and
holdout, both model cohorts, original creation rollback, three binding faults,
two stricter negative faults, focused Python tests and static/profile checks.
The two prepare helpers emit byte-identical shipping callback source on repeat.
All builds used `-j2` and `-o build/native-controller-deps/include/SDL3/SDL.h`;
SDL3/libusb is copied/dereferenced locally, never a writable Main symlink.

No combined Sonic+fan app run was performed. This tree stops at Fan1332; newer
Main stops at Display1332. Frozen controller 58,992/3,072 records and accepted
algorithm faults are reused only under documented unchanged contracts, not
reported as fresh runs. Active Sonic matrix processing, missing slot20 content,
later callbacks, actor frames, low-memory whole-app recovery and N3b remain open.

All worker heavy commands have finished. The heavy slot is clear.
