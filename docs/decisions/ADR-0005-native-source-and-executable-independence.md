# ADR-0005: Native source and executable independence define the finished port

Date: 2026-10-15. Status: accepted direction, requested by the user in this task.
Refines ADR-0001's end state; retains its hybrid runtime for development and verification.

The user clarified that the deliverable is a readable, maintainable macOS arm64 game
that runs from imported game assets without downloading or retaining `sonic.exe`.
ADR-0001 and the original M5 performance target allowed a native process with guest
execution remaining off the hot path. That does not establish this deliverable.
The source comparison and REA findings are in
[RESEARCH-native-recompilation.md](../RESEARCH-native-recompilation.md).

The shipping game will execute compiled native game and pinned-profile behavior,
including cold paths, with no x86 interpreter/JIT, PE code loader, original mod DLL
execution, or fallback to original instructions. Its normal source build will use
checked-in source and declared native dependencies, without decompiling or lifting an
original executable. REA and the pinned Windows binaries remain development inputs;
Unicorn and the hybrid app remain separate reference tools.

The asset importer may read the user's own disc/install, including embedded data in
EXE/model DLL containers, once. It must emit identified data with resolved resource
references, versions and hashes. The game reads that data independently afterward.
An opaque executable image renamed as an asset pack does not satisfy independence.
Support for each disc/install variant must be established explicitly; the existing
partial geometry extractor does not establish a complete asset import.

Game modules use meaningful state types, named operations and native resource/callback
identities. Original addresses belong in provenance records, reference adapters and
diagnostics; they must not be the shipping game's dispatch or storage model. Existing
`*_at_<hex>` owners and their checker remain valid during migration. Move each owner
and its source mapping together, with equivalent validation, before retiring that
representation; renaming fields alone is insufficient. Preserve exact numeric,
ordering, aliasing and speedrun semantics wherever the reference requires them.

Recover coherent systems through REA evidence, verify them against the existing oracle
and replay infrastructure, and make native callers invoke the same implementations
that reference adapters exercise. Static recompilation is an optional recovery aid;
its compiler, indirect-call, memory and FP correctness must be demonstrated separately.
Generated register/PC-dispatch code alone does not meet the maintainability goal.

M1–M4 retain their existing tests and incomplete status. M5 gains explicit source-build,
asset-import, runtime-independence and maintenance acceptance criteria in
[ROADMAP.md](../ROADMAP.md#m5--native-source-and-executable-independence).
Begin a bounded native slice now rather than postponing independence work until every
guest bridge is complete. This decision changes neither fidelity nor repository
asset/licence policy and makes no claim that the current app is independent.
