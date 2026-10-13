# Sources and inputs

Nothing under `game/` or the large parts of `reference/` is committed. Everyone who
works on this repo supplies their own copies at these paths (or sets the Makefile
variables).

## The game

| Path | What | SHA-256 |
|---|---|---|
| `game/Sonic Adventure DX/sonic.exe` | 2004 PC release, retail binary with the standard CD-check bypass (one byte at 0x402633: `jnz`→`jmp`) | `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74` (55,549,952 B) |
| `game/Sonic Adventure DX/system/` | game data, 6,221 files, 1.3 GB (4,329 under `sounddata/`) | — |
| `game/Sonic Adventure DX/mods/` | SADXModLoader.dll (993,280 B, Apr 2024), Input mod 2.6.1, Frame Limit 1.1, CCEF, EHCF 1.1 | — |

Also under `game/` (ignored): `game/pristine/sonic.exe` — the untouched download from the
speedrun page (same hash as above; use it to re-verify the install if anything is in
doubt) and `game/disc/sonic-adventure-dx-directors-cut_202202/` — the retail disc images
(see below). `reference/updated-pc-reference/game/` is the pinned-profile install.

`tools/lib/extract.py` refuses any other `sonic.exe`. Verified 2026-10-12: the user's fresh
download from the speedrun page is byte-identical to this image; the install's sound
tree is complete against the retail disc (4,329 names match; 21 `.wma` differ only by
metadata size), i.e. nothing was deleted.

Other binaries next to it (`Sonic Adventure DX.exe`, timestamp 11 minutes later, different
entry 0x643fe6) are a different build and are not supported.

## The pinned profile ("updated speedrun")

`reference/updated-pc-reference/` is a second, pristine install with the exact DLLs the
oracles pin and the collector configuration for Windows captures:

| DLL | Identity |
|---|---|
| SADX Mod Loader | source eb4cdcca, DLL 3293cf35 (`mods/.modloader/SADXModLoader.dll`) |
| Input mod 2.6.1 | c5ccdcc7 (`mods/sadx-input-mod/sadx-input-mod.dll`) |
| Speedrun mod 2.0 | f3fc68a3 (`unpacked/speedrun/sadx-speedrun-mod/sadx-speedrun-mod.dll`) |

The Makefile variables `INPUT_DLL`, `LOADER_DLL`, `SPEEDRUN_DLL` point here. The mod loader
source at that revision is the git submodule `reference/sadx-mod-loader`
(https://github.com/X-Hax/sadx-mod-loader, GPL; not vendored into this repo's licence).

## The retail disc (Archive.org `sonic-adventure-dx-directors-cut_202202`)

Two BIN/CUE images (MODE1/2352). Disc 1: `sonic.exe` (differs from the pinned image by
the one CD-check byte), `system.dat` (installer container "archive V2.2", 1,891
compressed entries), DirectX/WMP redistributables. Disc 2: `SoundData/` (4,329 files).
Useful as an independent check of data integrity; not needed to build or test.
`tools/analysis/iso_list.py` lists the images read-only.

## Reference machine facts not captured yet

The Windows reference GPU's D3DCAPS8, the CPU's transcendental/RCPSS tables, registry
values (CTRLFLAG0) and the HeapAlloc addresses. The runtime reports an assumption for each.
M4 of the roadmap captures them with the collector in `reference/windows-transfer`.

## Windows system DLLs for the guest (local only)

`reference/windows-system/` holds the 32-bit `msvcp140.dll` and `vcruntime140.dll` (14.51.36247.0) the
guest runs as real code under the mod loader and the mods (M1 4b). They come from a local VC++
redistributable install; hashes and the copy command are in `reference/windows-system/PROVENANCE.md`.
Never committed (`*.dll` is ignored).
