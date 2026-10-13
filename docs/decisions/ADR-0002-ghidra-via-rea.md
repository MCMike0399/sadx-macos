# ADR-0002: Ghidra through the owned REA runner is the analysis path

Date: 2026-10-12. Status: accepted.

## Context

Most analysis so far used `rea instructions … --provider hopper`, one function per query.
The Hopper demo build hangs often under load; sessions lost many minutes killing and
retrying. REA's Ghidra provider is headless, scriptable through the owned runner
(`reference/ghidra-live.mjs`), keeps a snapshot, and returns CFG, callers/callees,
pseudocode and p-code per function. It also offers `list_procedures`, `batch_decompile`,
`procedure_callers/callees` and a seed file for pointer-only functions, none of which
were used.

## Decision

All binary reading goes through the owned Ghidra runner (batch requests, one session).
Hopper is not used. Raw bytes (`tools/pe_image.py`) only for exact checks and oracle
fixtures. A whole-program inventory (`reference/inventory/`, see `docs/METHOD.md` §2) is
produced once and reused.

## Consequences

Longer wall-clock for large batches (serial bridge) but no hangs, no manual retries, and
machine-readable output for tooling (inventory, hot paths, FPU site list).
