# Decompiling SADX: tips and idioms

A knowledge base for agents and people turning `sonic.exe` evidence into game-tier code, in the
spirit of gta-reversed's `docs/ai/TipsAndTricks.md`. Add an entry whenever a step learns
something that would have saved time, and cite the HANDOFF entry or evidence id. Rules live
in `AGENTS.md` and `STYLE.md`; this page holds facts and patterns.

## Names and types

* `build/symbols/symbols.json` (`make symbols`) merges the mod loader's `*New.h` headers
  (original-symbol names, preferred), the classic headers (community names), sa_tools labels
  and the inventory. `FUN_<hex>` as the only name means unnamed: choose a semantic name from
  behaviour and record it in `reference/symbols/overrides.tsv` with the evidence id.
* Ninja conventions in the original symbols: `nj*` is the Ninja library; `tp` is a `task*`,
  `twp` a `taskwk*`, `mwp` a `motionwk*`/`motionwk2*`, `pwp` a `playerwk*`. A function
  declared `(taskwk* twp, motionwk2* mwp, playerwk* pwp)`, such as 44c270 `PGetAcceleration`,
  is player physics.
* `ulGlobalMode` (3abdc7c) holds an `MD_GameMode` (`SADXEnumsNew.h`): 0x01 `MD_LOGO`, 0x08
  `MD_MOVIE`, 0x0d `MD_NOVMSWND_INIT`, 0x17 `MD_TVSETTING_END`, 0x18 `MD_LOGO_INIT` … The
  classic `GameModes` enum numbers differ in places; use the `New` one and check the switch.
* `UsercallFunc*` declarations in the mod loader headers state which registers carry
  arguments. Ghidra's guessed calling convention is often wrong for these, so trust the header
  and confirm with the assembly.
* Functions reached only through pointers (task `exec`/`disp`/`dest`, vtables, the D3DX
  dispatch table) are not auto-defined by Ghidra. Seed them (`METHOD.md` §1).
* The mod loader headers are GPL. Use the *names and layouts* as facts; write our own
  declarations and do not copy header text or comments into `src/`.

## MSVC 7.1 x87 idioms (the game thread runs control word 0x007f)

* `fcom`/`fcomp` + `fnstsw ax` + `test ah, mask` + `jp`/`jnp`/`jz`/`jnz`. C0 is AH bit 0x01,
  C2 is 0x04, C3 is 0x40. Results: greater 000, less C0, equal C3, unordered C3|C2|C0.
  * `test ah,0x41` / `jp` jumps on **greater or unordered**, which is `!(st0 <= src)` in C++.
    Write it that way, not as `st0 > src`.
  * `test ah,0x05` / `jp` jumps on **not less** (≥ or unordered): `!(st0 < src)`.
  * `test ah,0x44` / `jp` jumps on **not equal** (including unordered): `!(st0 == src)`.
  * `test ah,0x41` / `jnz` jumps on less, equal or unordered: `!(st0 > src)`.

  Plain C++ comparisons are false for NaN, so `!(a <= b)` keeps the unordered case exact.
  Confirm each translation with the oracle (`player_acceleration_ground.cpp` has the
  parity-mask helper for reference).
* `fsubr`/`fdivr` reverse the operands: `fsubr m32` computes `m32 - st0`. For example,
  `update(g.y, reverse_subtract(product(w, 5.0f), g.y))` is `g.y -= w * 5.0f`.
* Float literals live in a constant pool in `.rdata` and are shared between functions. The ones
  ported so far sit between 7dcb0c and 7e01a8. They are source literals (`0.1f` at 7dd568, `1.16f` at 7e0178,
  `5.0f` at 7dccfc). Write them as literals (STYLE §8).
* `fld qword` of a `double` constant followed by single operands: at 24-bit precision the exact
  product is rounded once. C++ `float * double` rounds twice (to double, then to float), so use
  the fidelity helper.
* Float→int goes through `_ftol`/`_ftol2` (644340: `fistp` under the current rounding
  control plus a sign-based correction, modelled exactly in `src/cpu`) or inline `fistp`.
  Neither is a C++ cast unless the oracle agrees.
* Transcendentals (`fsin`, `fcos`, `fsincos`, `fptan`, `fpatan`, `f2xm1`, `fyl2x`) ignore
  precision control. Use the Bochs models in `src/cpu`, never `std::sin`.
* The CRT changes the control word temporarily (for example `crt_atan2` at 64b43d `FLDCW`) and
  restores it. Other threads run at 53-bit (`0x027f`). Direct3D 8 `CreateDevice` is called
  without `FPU_PRESERVE` (794060/794000). One Windows capture saw the earliest thread at
  `0x007f` (24-bit). Direct3D's documented default supports the hypothesis that the main thread
  runs at 24-bit, but it does not prove the full execution history (HANDOFF "Floating-point
  observations"). The census in GOAL §6 item 5 settles it per call site.
* SSE paths (D3DX math, the Input mod) are IEEE single already. RCPSS/RSQRTSS estimates use the
  project's estimate models. Unicorn 2.1.4 raises no SSE flags, which is the cause of most
  "partial" verdicts.

## Integer and control-flow idioms

* `movsx eax, word […]` means the source reads an `int16`/`short`. Write `std::int16_t`.
* `ja`/`jb` are unsigned comparisons. A switch guarded by `cmp eax,0x1a; ja default` treats
  negative values as large unsigned ones.
* `goto` labels in old ports are usually compiler **tail merging**: several branches share one
  epilogue or call sequence. Recover it as a named helper or `return`.
* Jump tables show up as data between functions. A linear disassembly once misread table data at
  522a04 as an `FLDCW` site.
* Fastcall callees pop their stack arguments, and stdcall pop counts are load-bearing for the
  guest (CreateWindowEx: 48, not 60).

## Pinned profile

* Under the profile the loader and mods replace code. `profile-applied` observed all 119
  active patch sites in guest memory; `profile_patches.json` lists every evaluated entry. Check
  `check_profile_patches.py` before porting a range. The Speedrun 2.0 limiter hooks 7899e0
  (DLL +0x10f0), and the Input mod skips the original controller update (40f460 → 40fdb3).
  Implement what runs and annotate it `// PATCH: <MOD> +0x… -> 0x…`.

## Data and assets

* Many globals point into model DLLs (`CHRMODELS_orig.dll` and friends) or into data embedded in
  `sonic.exe`. sa_tools' `GameConfig/PC_SADX/*.ini` gives each item's `address=`, `type=` and the
  code sites (`pointer=`) that reference it. Resolve such a pointer to an imported resource id;
  never keep the address in the game tier.

## Process lessons

* After three failed approaches on one function, stop and record it (AGENTS.md). Past the third
  attempt the odds of success fall sharply (Mizuchi data).
* A fresh context per function or owner beats one long session. Reread `STYLE.md` before each
  refactor.
* `make` mtime races: the probe `.dylib` must be newer than the edited source before an oracle
  runs.
