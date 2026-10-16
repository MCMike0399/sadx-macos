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

## MSVC 7.1 x87 idioms (establish precision at the call site)

* The reference census (`reference/fp-control-census.json`) observes raw FPCW `0x023f`
  and MXCSR `0x1f80` at every reached entry in the intro/title and controller journeys.
  The precision-control bits select 53-bit precision, not the 24-bit premise of plain
  float rewrites. These are Unicorn observations, not Windows hardware captures. Ground
  acceleration and other unreached physics entries have no measured runtime control word;
  their oracle fixtures do not supply that missing stage evidence. Keep STYLE §7's four
  preconditions when selecting an arithmetic level.

* `fcom`/`fcomp` + `fnstsw ax` + `test ah, mask` + `jp`/`jnp`/`jz`/`jnz`. C0 is AH bit 0x01,
  C2 is 0x04, C3 is 0x40. Results: greater 000, less C0, equal C3, unordered C3|C2|C0.
  * `test ah,0x41` / `jp` jumps on **greater or unordered**, which is `!(st0 <= src)` in C++.
    Write it that way, not as `st0 > src`.
  * `test ah,0x05` / `jp` jumps on **not less** (≥ or unordered): `!(st0 < src)`.
  * `test ah,0x44` / `jp` jumps on **not equal** (including unordered): `!(st0 == src)`.
  * `test ah,0x41` / `jnz` jumps on less, equal or unordered: `!(st0 > src)`.

  Plain C++ comparisons are false for NaN, so `!(a <= b)` keeps the unordered case exact.
  Confirm each translation with the oracle (`player/ground_arithmetic.hpp` now names
  greater-or-unordered directly; legacy reference code retains the parity-mask idiom).
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
  observations"). The reference census above measures the emulator's reached entries;
  stage call sites and Windows hardware history remain separate evidence gaps.
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


## Ground acceleration refactor evidence

* The old lift eagerly loaded the input magnitude before speed tests. The Ghidra assembly
  shows branch-specific loads after those tests; active event input can leave both scratch
  outputs untouched. An adapter must report the first consumed missing fact, preserving
  prior contact/rotation work. Direction reset replaces the angle before its normal read.
* In the fast input band, compare magnitude before multiplying by run acceleration. Reversing
  those operations can change the first unmasked exception even when masked outputs match.
* Shared ground/alternate friction computes the final Y sum before storing X, then spills Y
  before storing Y/Z. A failed Y spill retains X; a failed Y addition precedes X. Dedicated
  tests catch both reorderings; masked final-value comparisons alone cannot prove this.
* Reference evidence: `inventory-raw/ground-pilot-context.json`,
  `ev_b9e2502725c195a77b096e64d97288e6b5f9db93725b1886a588320ac34dee6d`.

## Frame-control refactor evidence

* The interval helper validates the frequency high word, then the low word, before storing
  its multiplier. Ghidra pseudocode may reorder the apparent assignment; assembly and the
  missing-word oracle establish the actual order.
* Mode setup publishes all three timing stores before reading the selected renderer setting.
  Frequency errors nested inside that call retain both the caller and missing-word boundary.
* Limiter clock callbacks may mutate timing or previous-counter state. Read timing after QPF
  and previous-counter after each QPC; a snapshot at entry loses those changes. Nested
  reference wrappers restore the enclosing binding on both success and missing-fact stops.
* The source settings at 389d7dc and 89295c are file-backed 1 and 3 respectively. The timing
  frame multiplier is zero-filled, so the earlier STYLE sketch's initial value 1 was not a
  recovered default. Native tests require the actual defaults.

## Immutable engine code tables

* BlendModes has twelve entries, but the unchecked four-bit selector can consume four
  following words from CompareModes. Native tables must retain that adjacency behavior at
  the reference boundary rather than inventing a sixteen-entry blend-mode meaning.
* PollKeyboard consumes a kind halfword and a code byte per scan code: kind 0 emits a key,
  kind 1 ORs a modifier, and other kinds emit neither. The pinned table has 237 entries and
  zero reserved bytes. Native records need no original padding; the reference probe
  serializes kind/code explicitly and verifies every source word.
* The stored Ninja unit matrix is positive identity. njInitMatrix copies it and then
  negates X/Y in the destination; pre-negating the maintained table would apply it twice.
* Native code-table selection belongs before any PE decoding of that family. A poisoned-PE
  runtime test catches an accidental read even when unchanged pinned table bytes would
  hide it. This does not remove the composition's other image dependencies.


## Input initialization values

* PDS_PERIPHERAL's 52-byte layout has three resource words: name at +0x24, extend
  at +0x28 and info at +0x30. The previous-held word at +0x2c is scalar input.
  Keep all three resources separate when migrating input data, including byte-granular
  missing facts. An all-zero source record does not establish a native resource identity.
* The load-time not-held mask is zero in the game template/raw records and Input mod's
  raw/DreamPad data. DreamPad polling later writes the complement of held; applying that
  invariant during load initialization changes the original state.
* input_init clears only through the last raw row's first lane. Its last two lanes still
  have independent load-time values; do not infer them from the clear loop. Gate153 evidence
  `input-initial-153-*` compares all load-time bytes and preserves varied injected values.


## Ninja quarter-wave coefficients

* The classic SDK declares BAMSTable as a pointer cell, but njSin's direct scaled FLD
  accesses prove inline binary32 storage. The justified symbol override records the full
  1,025-coefficient array and its endpoint. Do not dereference its first float as a pointer.
* Ghidra's cosine pseudocode labels some base constants with unrelated strings. Combine
  each base with its bounded quadrant index before classifying the effective read: these
  accesses still land in the coefficient array, not in string content.
* The library's rounded coefficients differ from analytic sine. Preserve every original
  binary32 word as a round-tripping decimal literal; host sine generation changes fidelity.
  The emitter is a development rendering check; normal native builds use maintained C++.
* A missing byte in the final knot keeps the old reference initializer's entire table
  unknown. Earlier readable knots must not become a partially published/default-filled table.
  Source/range evidence and six caught mutations are in `sine-initial-154-*`.


## Fog initialization boundaries

* Cached fog start/end begin at positive65535.0F in file data. The disabled runtime
  sentinel is negative65535.0F. Preloading that sentinel would skip the first source update.
* All three128-entry fog buffers begin at zero before device setup generates W/reciprocal
  data. The SDK's float* declarations describe inline buffers here: direct buffer arguments
  and indexed writes establish float[128], not resource-pointer cells.
* The fallback-density word has observed immediate bit-pattern stores but no established
  floating-point consumer. Retain its word representation rather than guessing a new type.
  Frame flags/profile and saved ranges have other owners; GPU capabilities are not initial
  fog defaults. Evidence and seven caught mutations: `fog-initial-155-*`.


## Numeric screen defaults and resources

* The stored range pair is -1.0/-2049.0, not the round power-of-two guess -2048.0.
  The base transform is identity, but projection and working matrices start at zero;
  viewport MinZ/MaxZ also start at zero before device setup.
* NJS_POINT2COL has three pointer words: points, colors and texture coordinates. Its
  third word is not numeric padding. Keep all three reference identities independent of
  the numeric polygon count/coordinates, including unknown and injected values.
* Screen setup's saved second argument and polygon setup word lack established broader
  meanings; retain their observed values and those limits. Existing projection scale,
  center and color fields not loaded by this initializer remain untouched. Evidence:
  `screen-initial-156-*`, including exact values and seven caught mutations.


## Sound queue load-time fields

* SDK `_SEcallbuf`/`sebuf` starts at the priority word; the legacy native timer owner
  starts four bytes later. Keep the 36-record stride at 56 bytes. Only the selected
  numeric fields are migrated; priority and other unloaded fields stay untouched.
* SDK `angle` feeds `SetPanPCM`, while `pitch` feeds `IsndPitch`. The existing adapter's
  `frequency_at_3b2930c` and `pan_at_3b29318` names are misleading; native defaults use
  `pan_angle` and `pitch`, preserving the old live owners at the boundary.
* `gu8overlap_se` is 72 bytes, cleared by 18 word stores. Native storage is bytes, but
  fallback reference availability remains word-granular: a three-byte partial word
  must not publish three known bytes. The load-time tone is zero; -1 is the server's
  later finished-record value. Evidence: `sound-initial-157-*` and fresh sound dossiers.


## Game-view load-time geometry and colors

* `ScreenRaitoX/Y` are stretch factors (initially 1), despite the legacy view owner's
  width/height names. View setup later multiplies by 640/480 to compute dimensions.
* `NJS_SCREEN.dist` starts at zero, as do width/height/centers. Do not substitute the
  common distance guess of 1. SetupScreen copies distance and center words; the
  constructor's projection-center copies remain separate from the projection owner
  fields it never initializes.
* The three four-float ARGB groups (view material, reset material, current sprite
  material) start at zero, including alpha. Clear colors also start at zero; opaque
  black is written later by setup. Reference color availability is whole-group: an
  incomplete fourth float keeps all four components unknown.
* `QueueCharacterAnimations` is an int; initialization proves its zero default, not
  the wider meaning of every nonzero value. The reset-pending byte belongs to view
  state, while the reset path's pause flag retains its session-control owner.
  Evidence: `view-initial-158-*`, including historical constructor equivalence.


## Render-queue initialization and parameter copies

* Default depth comparison is Ninja table index1, not a D3D comparison enumeration.
  Packed blend starts at zero; source/destination nibble stores happen before the
  render call can fail. Numeric defaults must not create a device or allocation.
* Keep the four float triplets distinct: reset source(-3000,-1200,-1200), reset
  capture(0,0,0), allocation source(-3200,-1600,-1600), allocation capture
  (-3000,-2000,-2000). Reset/allocation later overwrite captures; those later
  values are not load-time defaults.
* Float types are established by the three-argument setter's x87 comparisons/stores
  and CHAOS_Action's componentwise copy from allocation capture into those fields.
  Only captured component0's comparison against model depth is established here;
  do not invent spatial/vector semantics for the other components.
* Nearest inventory entries are navigation hints, not containing-function evidence.
  The candidate before CHAOS_DrawModel ends at its RET before the needed references.
  CHAOS_DrawModel itself lacks a Ghidra procedure; use the existing CHAOS_Action
  dossier for the copy chain instead of guessing boundaries. Evidence: queue-initial-159-*.


## Input 2.6.1 immutable coefficients

* Epsilon is exactly 2^-23, not an approximate decimal tolerance chosen by the port.
  Raw-axis maximum is32767, keyboard scale127, controller scale128, magnitude cap1,
  trigger scale255 and half-press scale0.5. The controller scale differs from its
  final signed output clamp; do not merge keyboard/controller coefficients.
* `input_mod_261_constants()` returns the existing Sse32-based InputModPadConstants
  with readable literals copied as bits. Boot uses it explicitly; caller injection
  and the reference decoder remain available. The DLL is still needed for other
  state/resources, so this is not whole-DLL independence.
* A missing-byte PE reader fixture must preserve exports and other sections to test
  coefficient independence. The synthetic fixture splits .rdata around one byte,
  retains the tail and verifies OnInput exports; it is never executed.
* Standalone player-input-producer returns nonzero for its existing strict FP flag
  discrepancies even when every value matches. Compare complete report fingerprints
  after restoring only documented library/timing metadata; do not count partial as pass.
  Evidence: `input-constants-160-*`.
