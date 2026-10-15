# ADR-0006: Symbol names in code, source addresses in provenance annotations

Date: 2026-10-15. Status: accepted direction (user request: "fully readable and maintainable,
right now the code maps the hex to the C code"). Implements the maintainability part of
ADR-0005. The checker migration in rule 5 happens as `/goal` steps (GOAL §6 items 1–2).

## Context

Ported state is named after its address (`frame_multiplier_at_389d7dc`,
`flags_at_3b0eaa0`), and `tools/gate/check_global_owners.py` enforces one owner per address by
parsing those identifiers. That made the M1 ports verifiable, but it makes the source read as
a memory map. Meanwhile the tracked mod loader submodule already names ~3,800 function
addresses, ~3,150 globals and ~470 structs, partly from original symbols
(`SADXFunctionsNew.h` & co.), and the project never used them. Projects that reached readable
native code (isledecomp/isle-portable, OpenLoco, re3) all keep addresses as comments and
names as identifiers; static recompilers that keep `f_<hex>` names never became readable
(`docs/RESEARCH-native-recompilation.md`).

## Decision

1. **Symbol database.** A generated database (`tools/symbols/`) merges the mod loader's
   symbol-based `*New.h` headers (preferred), its classic headers, X-Hax sa_tools data labels,
   Ghidra boundaries and the tracked project decisions `reference/symbols/overrides.tsv`.
   Every entry carries its source; conflicts are reported, not resolved silently. A name is a
   hypothesis until the evidence of the function's behaviour agrees with it. The generated
   database itself is not committed (it is rebuilt from the pinned submodule); the overrides
   file and the generator are.
2. **Ghidra gets the names** through REA's seed file (`address<TAB>function|label<TAB>name`),
   the only durable naming path REA offers. Types stay in project headers (REA cannot import
   them).
3. **Identifiers are semantic** in the project's style. Record types follow the recovered
   layout (`taskwk` → `TaskWork`); while guest memory is still marshalled, offsets are
   `static_assert`ed in the reference adapter, not encoded in names.
4. **Annotations** on the line above a declaration carry provenance:
   `// FUNCTION: <MODULE> 0x<addr> <symbol>`, `// GLOBAL: …`, `// DATA: …` (content the
   importer supplies), `// PATCH: <MOD> +0x<offset> → <target>` (pinned-profile behaviour),
   `// STUB: … <reason>` (declared, not recovered), `// LIBRARY: …` (CRT/D3DX/SDK). Modules:
   `SONIC`, `CHRMODELS`, `ADV00`…, `LOADER`, `INPUT`, `SPEEDRUN`, `SDL2`.
5. **Checker.** `tools/gate/check_provenance.py` enforces one `GLOBAL` owner per byte range and
   one `FUNCTION` per entry and validates names against the database. It runs next to
   `check_global_owners.py`; the old checker is retired only when the new one covers every
   owner it covered (both run during migration). An owner moves with its annotation in one
   step; a cosmetic rename that drops the provenance is a regression.
6. **Address-keyed registration** (override tables, guest marshalling) stays in the reference
   adapters (`src/guest`, `src/hle`). Production game code never takes a guest address.

## Consequences

The AGENTS.md "one owner per source global" rule now reads "one owner per source address,
expressed by an identifier (legacy) or an annotation (new code)". Existing `*_at_<hex>` code is
not mass-renamed: it migrates system by system with its tests. Progress gains two metrics:
procedures with a database name, and address-named owners remaining (to reach zero).
