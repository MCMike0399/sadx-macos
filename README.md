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
make app-smoke       # opens the window for 120 frames (the SEGA logo) and writes build/app-smoke.ppm
# Enter / pad Start skips the black intro movie; live keyboard and controllers.
# The 3D stages do not draw yet (docs/GOAL.md §3.1 R4).
make play
make play-title             # automatically skip the movie, then play live
make play-character-select  # automatically navigate the menus using the reference save
make journeys        # one offscreen run checks SEGA, title and character select; reports open 3D
./build/sadx-boot "game/Sonic Adventure DX/sonic.exe" "game/Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll" 5 --survey
```

## What this is, and where it is going

The destination is a normal, readable C++ game codebase that anyone can clone, build, and
play with assets imported once from their own disc or install. It needs no `sonic.exe`, no
original DLLs and no emulation. The code is organised by game system and written to be
maintained and extended: new features sit on top of the faithful game as opt-in
enhancements. See [`docs/GOAL.md`](docs/GOAL.md), [`docs/STYLE.md`](docs/STYLE.md) and
ADR-0005/0006/0007 in [`docs/decisions/`](docs/decisions).

Today the original game runs in-process under a hybrid runtime: an x86 guest behind native
Win32/Direct3D 8/input layers, with Metal drawing. It boots through the intro and title
deterministically and serves as the judge. Native code, verified function by function
against the original with Unicorn oracles, replaces it progressively. The earliest native
code was written in an address-shaped style that is being refactored into readable form under
the same oracles. Details in [`docs/STATUS.md`](docs/STATUS.md) and
[`docs/REVIEW-2026-10-12.md`](docs/REVIEW-2026-10-12.md) and
[`docs/REVIEW-2026-10-16.md`](docs/REVIEW-2026-10-16.md).

## Licence

MIT for this repository's code. `third_party/` keeps its own licences (Berkeley SoftFloat
BSD-3, Bochs LGPL-2.1). The SADX Mod Loader source referenced as a submodule is GPL and is
not part of this repository's licence. Sonic Adventure DX is © SEGA; you need your own copy.
