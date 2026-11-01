# Session regular texture resource owner

Base Main e5cf19d, packet/session-regular-resources. Native implementation baseline
and context: 9d3938f. Source: 8090d81. This packet extracts exactly the seven
contiguous regular-resource members and imported binding body. All actual Session,
TitleScene individual load, selection preview/Gamma and callback consumers migrate.
Character/menu/task/audio/GPU owners remain separate. MenuTextureBindings borrows
the same vector, cleanup borrows the same common/32 regular slots, and nullable
State remains lazily allocated after declarations are bound. State references live
loop resync, text language and owned localization storage. Attachment still publishes
State before display validation and callback binding. Error identifier literals,
partial mutations, failed retry append behavior and destruction order are preserved.

The source-bound native fixture emits 55 full observations (native-baseline.txt).
Its attachment body is verified verbatim against e5cf19d. The checker also proves
that the extracted owner body and Session transport suffix match that source;
no Windows counterpart is invented for native import attachment. Public-interface
comparison covers alias classes, catalogs, ordered partial failures, duplicate
errors, callback-time service/state identity, mutable language/resync, localization
and scratch/cache effects. Additional tests check retained localization/cache
lifetimes and actual cleanup callbacks, including failed-release partial clearing.
Owner copy/move are disabled because catalogs and State borrow retained storage.

Run serially from this checkout:

```
python3 tools/testing/check_native_regular_resources.py
```

Both strict native test builds pass; all six valid compiled semantic mutants are
caught. Original default judges are unchanged: regular_textures 609 before/after,
menu_resource_lifetime 1401 before/after, native_texture_lifetime 1416 before/after;
all exit 0 with no mismatches/partials. The additional unchanged game_cleanup judge
passes 2604 cases, including 300 unchanged-reset cases; this is post-edit evidence,
not a new pre-edit cleanup baseline. Reports retain each judge's fixture limits.
All probes are compiled directly from current sources, with save_data.cpp included
for cleanup. No borrowed archive, Main library, dependency download or private full
native build was needed. The pinned original EXE/loader DLL and Python are read from
Main. No writes to Main's source/build or its live full gate were made.

Twelve affected runtime/app translation units compile serially with strict C++17 O2
flags; see consumer-compiles.json. Layer, readability, ownership, provenance and
scoped profile checks pass (zero unhandled overlaps). Provenance reads the existing
Main symbol database. Source/error hashes and all limits are in SUMMARY.json.
Qualification receipts include full compile/mutant diagnostics and original reports.
No game/numerical logic, original judge, expected value, fixture vector, tolerance,
existing mutant or comparison changed. The new native fixture/test is additional
coverage and is not yet registered in Root's gate.

## Root integration prerequisites

Cherry-pick the baseline/context, source and qualification commits in order.
Add `native-regular-resources` to tests/unit/manifest.tsv; refresh protected-input
registration with packet evidence under the normal judge workflow. Do not overwrite
other workers' manifests or generated shards. Native source discovery already
includes runtime/native_*.cpp; no native-sources.txt or Make rule change is needed.
The new test links core normally; its packet checker builds only the narrow source
closure with no archive. tools/testing/check_native_regular_resources.py is an
explicit local qualification command, not automatically part of the full gate.

Review all six source files against STYLE section 12, this context, the frozen
native baseline and original reports. Run fresh reserved heldout qualification only
after independent Root review. Root must rebuild the full affected native closure
(including every Session-header consumer), force-load check it, preserve native
menu/save journeys and original resource judges, and run/record its real full gate.
Integrate inventory/summary/HANDOFF/STATUS updates there. This packet deliberately
makes no whole Session, GPU, actor, journey, full-gate or campaign-completion claim.
