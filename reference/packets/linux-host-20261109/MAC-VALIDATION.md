# Mac validation packet (2026-11-09)

Linux built the portable core. It did not run the product. Metal, AppKit, an
arm64 Apple binary, audio, the Switch 2 Pro path and a measured 63 FPS cadence
still require a Mac. No self-hosted Mac worker was connected when this packet
was written.

## Missing inputs (do not delete judge entries because they are absent)

Place the user's own files at the paths in `docs/SOURCES.md`.

| Path | Identity |
|---|---|
| `game/Sonic Adventure DX/sonic.exe` | SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`, 55,549,952 bytes |
| `game/Sonic Adventure DX/system/` | game data, 6,221 files |
| `game/imported/` | imported assets already used by the native app. Do not regenerate audio. |
| `reference/updated-pc-reference/game/mods/.modloader/SADXModLoader.dll` | loader DLL prefix `3293cf35` |
| `reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll` | Input mod 2.6.1, prefix `c5ccdcc7` |
| `reference/updated-pc-reference/unpacked/speedrun/sadx-speedrun-mod/sadx-speedrun-mod.dll` | Speedrun mod 2.0, prefix `f3fc68a3` |
| `reference/windows-system/` | 32-bit `msvcp140.dll` and `vcruntime140.dll` |
| `reference/tools/ghidra_12.1.4_PUBLIC/` | Ghidra 12.1.4 |
| `reference/tools/rea/` | REA 6.4.0 |
| `reference/python/` | Unicorn 2.1.4 virtualenv. Do not reuse a copied macOS venv on Linux, or the reverse. |

`make symbols` also needs `reference/sa-tools` at `b72d52f37c21a2f7215711e3516360a63aea12da`
and the mod-loader submodule at `eb4cdcca9637d611a98b910fcfed6ce18580451c`.
Those two are public source, not disc assets.

The judge manifest still lists 53 `reference/oracle-reports/*.cases.bin` (and
`.constants.bin` / `.dispatch.bin`) inputs. They were absent in this clone.
Leave the manifest entries in place.

## Linux host packages used here

This Ubuntu image had clang 18 and GCC 14's runtime, but only GCC 13's
libstdc++ headers. Clang selected GCC 14 and then could not find `<optional>`.
`libstdc++-14-dev` provides the matching headers. That package is an image
prerequisite, not a game dependency. Python 3.12, Node 22 and OpenJDK 21 were
already present. Ghidra and Unicorn were not installed. Installing them does
not replace `sonic.exe`.

## Darwin contract to re-check before trusting the host split

On a Mac, from a clean tree of this branch:

```sh
make HOST_OS=Darwin -n build/obj/cpu/crt_integer.o
```

The compile line must contain `xcrun clang++`, `-arch arm64` and
`-mmacosx-version-min=14.0`, and must not contain `-funsigned-char`.
The archive recipe must call `tools/build/static_archive.sh`, which on Darwin
runs `xcrun libtool -static`.

Then:

```sh
make -j10 portable
make -j10 portable-units
```

`portable-units` does not need the disc. It is not the full gate.

## Judge refresh (separate commit, after the full gate)

The Makefile is a protected judge input. This branch changes it and does not
refresh `reference/judge-manifest.json`, because the required full gate cannot
run here. `9e279b4` also left `tests/unit/native_gamma_fan.mm` different from
the manifest. Do not drop either fact.

After a green Mac gate, in its own `fix(judge):` commit:

```sh
python3 tools/gate/check_judge.py --declare-change \
  --reason "Host selection in the Makefile. Darwin flags stay arm64/macOS 14. No tolerance, fixture or expected value changed." \
  --evidence reference/production-integration-20261109/linux-host-toolchain.json
python3 tools/gate/check_judge.py --update \
  --reason "Record the host-selectable Makefile after a green Mac gate. Inherited fan-file drift and absent case bins stay explicit." \
  --evidence reference/production-integration-20261109/linux-host-toolchain.json
```

Run that only when `make -j10 gate` has already exited 0 on the Mac. Do not
edit tolerances to absorb the Linux result.

## Reproduce the admitted frontier

Use a fresh native binary, normal input and an isolated copy of the save.
The admitted stop is frame 1332, exit 1, `native_gamma_update_target_effects`,
outgoing title image. The caller has already submitted 106 draws and 3,060
vertices. A different stop means the tree is not the admitted frontier.

## Recover GammaUpdateTargetEffects, then present the frame

`0x4c4da0` is 752 bytes and ends at `0x4c5090`. Direct callees:

| Address | Name | Native state |
|---|---|---|
| `0x40b2d0` | `CAlloc` | stub seam |
| `0x40b570` | `DestroyTask` | task owner exists |
| `0x40b860` | `CreateElementalTask` | task owner exists |
| `0x423d70` | `dsPlay_oneshot` | sound owner exists |
| `0x424210` | `dsStop_num` | stub |
| `0x47f980` | unnamed, 96 bytes | not established |
| `0x4cf060` | unnamed, 48 bytes, calls `0x4cefe0` | not established |
| `0x644340` | unnamed, 117 bytes | not established |
| `0x7871f0` | `njSqrt` | not established |

The native binding now calls the existing `CCL_CalcRange` and `LoopTaskC`
owners. `make portable-units` includes `gamma-common-tail-binding`, which checks
that range store and a repeated child dispatch. The live caller still throws at
target effects before either call. Gun effects, the held task, shadow, ground
attributes, running effects, `EntryColliList` and underwater remain stubs.

Two `FPATAN` instructions are at `0x4c4ed4` and `0x4c4ef0`. Profile intersection
for `0x4c4da0`–`0x4c508f` reported no active patch overlap from the symbol
bundle. That is not a runtime trace.

```sh
export GHIDRA_INSTALL_DIR="$PWD/reference/tools/ghidra_12.1.4_PUBLIC"
export REA_GHIDRA_STARTUP_TIMEOUT_MS=600000
export REA_GHIDRA_SEED_FILE="$PWD/build/symbols/rea-seeds.tsv"
make symbols
python3 tools/symbols/lookup.py 0x4c4da0 0x47f980 0x4cf060 0x644340 0x7871f0
node reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs doctor --provider ghidra --json
```

Take one `analyze_function` dossier for `0x4c4da0` through
`reference/ghidra-live.mjs` before writing the body. Rewrite it as named C++.
Keep the explicit stop until the body matches an oracle. Then advance the real
caller and qualify the drawn CharacterSelect checker against a presented frame,
including a run where suppressing the real draw fails the checker.

## Acceptance this packet does not grant

- N3b, or any later N row
- a full gate, STATUS regeneration, or a judge-manifest refresh
- Windows parity, hardware 63 FPS, or physical controller output
- executable independence of a newly linked Mac app (re-run the source-only
  closure on the Mac after the next integration)
