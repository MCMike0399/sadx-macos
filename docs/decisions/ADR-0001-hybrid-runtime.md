# ADR-0001: Hybrid runtime instead of porting every function first

Date: 2026-10-12. Status: accepted (user decision).

## Context

The port-everything method verified 6 KB of 3.85 MB of game code in a day of gates
(`docs/REVIEW-2026-10-12.md`). A playable build on that path is years away. The user's
goal is a playable, faithful, testable native build, soon enough to matter.

## Decision

Run the original x86 code of every not-yet-ported function inside the native process
(Unicorn/QEMU TCG, the project's existing oracle engine) behind native HLE for Win32,
Direct3D 8 (→ Metal), DirectInput (→ GameController/keyboard) and DirectSound (→ CoreAudio).
Ported modules are bound as native overrides of guest addresses. x87 transcendental and
SSE estimate instructions are shimmed with the exact models in `src/cpu` because QEMU
uses host libm / division for them.

## Consequences

* The whole game can run (and be tested) long before most code is native.
* The guest is a permanent runtime oracle: `override-equivalence` compares any native
  override with the original on live inputs.
* Performance is a risk (TCG without block chaining); it is measured in M1 and mitigated
  by moving hot paths to native overrides first.
* "Native port" now means: the process is a native arm64 app whose unported code executes
  under a software CPU; M5 drives that fraction to zero.

## Alternatives rejected

* Continue port-only: no running game for years.
* Static recompilation first (x86 → C via Ghidra CFG/p-code): months of compiler work with
  nothing visible; kept as an M5 tool.
* Wine/CrossOver: not native, not the project.
