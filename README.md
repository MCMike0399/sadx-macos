# sadx-macos

A native Apple Silicon port of *Sonic Adventure DX* (2004 PC) with the speedrun mod
profile, built by exact reverse engineering and verified by automated tests.

* **Plan:** [`docs/ROADMAP.md`](docs/ROADMAP.md) — milestones, each defined by tests.
* **Where we are:** [`docs/STATUS.md`](docs/STATUS.md) (generated from the last gate).
* **How work is done:** [`AGENTS.md`](AGENTS.md), [`docs/METHOD.md`](docs/METHOD.md),
  [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md), [`docs/decisions/`](docs/decisions).
* **Inputs you must supply yourself:** [`docs/SOURCES.md`](docs/SOURCES.md). This
  repository contains no game code, data or binaries.

## Build and test (macOS 14+, Xcode command line tools, Python 3)

```sh
# your own install at game/Sonic Adventure DX/ (see docs/SOURCES.md)
make -j10 all        # libraries, sadx-boot, sadx-app, sadx-metal
make -j10 test       # unit tests + owner checker
make -j10 gate       # everything a gate requires (tests, probes, Metal tests, layer/owner/profile/hygiene checks)
make oracle-screen-init          # one Unicorn oracle (needs reference/python venv); `make oracles` = all
make oracles-verdict             # pass / partial / fail per report in build/oracles/
make app-smoke       # opens the window for 120 frames and writes build/app-smoke.ppm
./build/sadx-boot "game/Sonic Adventure DX/sonic.exe" "game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll" 5 --survey
```

## Status in one paragraph

The boot sequence and one full frame of the original game run natively from exact
re-implementations (verified instruction-for-instruction against the original with a
Unicorn oracle per function), but that method covers 0.15 % of the game's code per day.
The project is now building a hybrid runtime that executes all remaining original code
in-process behind native Win32/Direct3D 8/input/audio layers, so the whole game runs and
can be tested while native code replaces it progressively. Details in
[`docs/REVIEW-2026-10-12.md`](docs/REVIEW-2026-10-12.md).

## Licence

MIT for this repository's code. `third_party/` keeps its own licences (Berkeley SoftFloat
BSD-3, Bochs LGPL-2.1). The SADX Mod Loader source referenced as a submodule is GPL and is
not part of this repository's licence. Sonic Adventure DX is © SEGA; you need your own copy.
