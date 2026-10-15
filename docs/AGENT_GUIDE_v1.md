> **Historical (2026-10-12, `rea` repository era).** Superseded by `AGENTS.md`, `docs/GOAL.md`,
> `docs/STYLE.md` and `docs/METHOD.md`. It describes Hopper and paths that are retired; read it as history.

# SADX native port: agent guide and handoff (2026-10-12)

This guide is for the next coding agent working on the native Apple Silicon port
of PC Sonic Adventure DX. It explains:

- what the goal is and what "done" means;
- where the work stands right now;
- how the work is done, step by step, including the reasoning behind each step;
- how to use REA (this repository's reverse-engineering tool) with its Hopper
  and Ghidra providers, and the Unicorn oracles;
- every convention, tool, pitfall and assumption collected so far;
- the prioritised next steps.

Read it together with [SADX_NATIVE_PORT_HANDOFF.md](SADX_NATIVE_PORT_HANDOFF.md).
The handoff is the detailed step-by-step ledger: each step's evidence ids,
oracle counts and gates. This file is the method and the map. When you finish a
step, add an entry to the handoff (see section 9) and keep this guide's
"Current status" section true.

---

## 1. The goal

Standing goal, as set by the user: *"continue implementing
SADX_NATIVE_PORT_HANDOFF.md"*. In concrete terms:

- **Target.** A native macOS arm64 reconstruction of the PC `Sonic Adventure DX/sonic.exe`
  (SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`,
  PE32 i386, base 0x400000, 55,549,952 bytes). It must use **Metal** rendering
  and support the **keyboard**, **Xbox** and **Switch 2 Pro** controllers.
- **Speedrun-ready** with the **updated speedrun mods**: SADX Mod Loader, Input mod 2.6.1
  (`mods/sadx-input-mod/sadx-input-mod.dll`, with SDL2.dll 25003b5b…), and
  Speedrun mod 2.0 (CCEF, EHCF, Skipma, `good-63-fps` = 63 FPS frame limiter).
  The reference profile is `updated-speedrun.json` (see the handoff).
- **Maximum fidelity to the Windows reference.** Deviations must be explicit,
  reported assumptions, never silent approximations. "Faster" or "looks right"
  does not satisfy the goal.
- **Use REA's own tooling** for reverse engineering (the user asked this three times; it is
  recorded in `_reference/sadx/goal-amendments.json` →
  `rea_tools_and_guides_user_request_20261012`). Analysis goes through the Hopper
  and Ghidra providers via the REA CLI/MCP, so it produces Evidence with
  provenance. Raw byte decoding and Unicorn are only for exact-byte checks and
  execution oracles.
- **Repository rules** ([AGENTS.md](AGENTS.md)):
  - One owner per state; dependencies flow inward.
  - Tagged results; explicit unknowns.
  - Report unverified scope honestly.
  - Never commit binaries, game assets, `build/`, credentials or planning artifacts.
  - Commits only when asked. No Claude attribution lines.
- **Stop hook.** The session has a goal stop hook that blocks stopping at a summary
  checkpoint. Implementation keeps going until the goal is met or the user says
  otherwise.

---

## 2. Current status (end of 2026-10-12, after gate `endscene-98`)

### 2.1 What runs natively

`build/sadx-boot SONIC.EXE INPUT_MOD.DLL [frames] [--survey]` composes every
ported owner in `NativeGame` (`sadx-native/src/game_runtime.*`) and runs the game
from its own load image. Under the `stop_at_unported` policy it is real native
execution. Today it:

1. Runs the parts of pre-40cad0 setup that are ported, in the source's order:
   - the CRT static initializer 7c4e90 → **401ac0** model-DLL bindings;
   - **WinMain 4025b0's settings span** (`game_winmain`: `.\sonicDX.ini` reads,
     the loader's SetLanguage and 40297a patches, and the 401a20 CD-drive scan
     over njMalloc/njFree and the native `GuestHeap`);
   - **781070** library init → 793d80 → **791610** device setup (native device,
     640×480 viewport, 794ec0 fog init from the caps block), **780c30** screen
     setup (`ninja_screen`), 77f150 and 789c50, before 401830;
   - **401830** system init (`ninja_system_init`, the texture pool in
     `ninja_texture_pool`) with the Input mod's keyboard/mouse/DirectInput patches.
2. Runs **GameLoop 40cad0** startup, **40c030**, completely: the input edge clear, 408630
   queue allocation, 410510 audio (DSERR_NODRIVER path), 411e30, **40bf40 reset
   with all its leaves including fog 411af0**, 4253c0→40d0d0 stream players,
   and 78c5a0.
3. Runs every frame owner up to **411d00**:
   - 77f090 keyboard poll and 40f460 input (Input mod OnInput);
   - 420830, 407ab0, 77ed00, the **40c090 game mode** (415000 including
     414470, 422680, 413c00 and 783260, 42cea0, 421f70/80, 42c4f0 fade, 426050…);
   - **4250d0 sound manager**, 411b60/403180 fog range, **4086f0 queue
     consumer**, save 40be40 checks, 40d4d0, 40eab0, 42f990, 40d3c0, 420840;
   - 411d00 frame completion, **including its D3D8 frame thunks 781010..781050**
     (`ninja_frame_calls`: 780030 callback list + hooks, 7959a0 hook table,
     EndScene path, Present as the loader's `Direct3D_Present_r`, Clear + 794800,
     BeginScene path), then the Speedrun 63 FPS limiter 781060;
   - the loader's 426063 `ProcessCodes` after 426050, which under the profile is
     the Speedrun mod's `OnFrame` (`mod_loader_frame`);
   - 40ef30/40ef20 key queries.
4. **A whole source frame completes natively.** The device methods go to a
   recording owner (`D3D8StateOwner` counts them; no display backend yet).
5. Frame 2 runs the SEGA logo setup 42c730 (and 432ea0) up to its LoadPVM.
6. Runs the per-frame callbacks (77f110/77efc0 device polls, 785190 vibration,
   789bd0 as the loader's `HandleWindowMessages_r`) and BeginScene's **77dfc0**
   polygon with its 78dbe0 depth through the D3DX8 SSE path (`d3dx_math`).
7. **Stops in frame 2 at `40cad0>40c090>42c7b0>421180`**: LoadPVM("SEGALOGO_E"),
   the loader's `LoadPVM_r`. WinMain's 402c92..402d7c span (cliplevel inits,
   SetClip exports, 781570 → 791170 projection, 781250, 40e2e0) is not run and
   is reported; 3d129c0 therefore stays zero and projected depths are NaN.

The 5-frame survey (`--survey`, which treats unported owners as no-ops; that is
navigation, not behaviour) is in
`_reference/sadx/production-integration-20261012/sadx-boot-survey.txt`. Remaining
unported owners in first-reached order:

| Path | Site | Note |
|---|---|---|
| 40cad0>40c090>42c7b0>421180 | 42c756 | **LoadPVM("SEGALOGO_E", 913a48)**: the loader's `LoadPVM_r` (TextureReplacement.cpp) → `LoadSystemPVM_r` → texture-pack probe, else njLoadTexturePvmFile (also the loader's) |
| 40cad0>40c090>42c7b0>42d3b0 | 42c7cb | large save-data routine (0x57c locals, 4b5870 …) |
| 40cad0>40c090>42cc70 | 40c124 | fade state machine: 40b170 RunTask, then a switch on 3b29d6c |
| 40cad0>411d00>781020>793cc0 | 78b9e3 | debug-text flush (ECX=1): 787230, 77f3d0(38a5cf8), 77dd90(1), the 3d08854 queue (count 3d088bc), 77dde0, 787220 (= 794950, 3d0849c=1); then 3d088bc = 3d088a8 = 0. Needs 793c70/793be0's font texture first (see handoff gate 98) |

Nested owners that are not yet ported, all reported precisely when reached:

- 403290: texture release walks;
- 40d520: the 40d4d0 draw list;
- 40eac0: the 40eab0 effect body;
- 42f9cc and 40d3f5: release walks;
- 4073b0: rectangle enqueue;
- 4246c0 / 425710 / 4244a0 / 423c20 / 410260 / 423cc0: active sound records;
- 424b10 with a listener;
- fyl2x EXP2 fog density (794afc);
- frexp of Inf/NaN (78728e);
- 422680's infinite loop for characters 1, 4 and above 7.

### 2.2 Assumptions the runtime reports (each printed by `sadx-boot`)

1. 781070/791610: the device is created by a native owner for 7943c0 (the
   loader's CreateDirect3DDevice_r is not ported) with the loader's 640×480 back
   buffer; the skipped owners (window 793ef0, 789f80, 78d0d0, 796710, 791860,
   7943f0, 7914b0, 7800a0, 780f20, 793e70) keep load-image values, and 788000
   is taken to succeed on its first try. WinMain 402c92..402d7c is not run.
2. GetDeviceCaps: the native Metal device declares only D3DPRASTERCAPS_WFOG (the
   reference GPU's D3DCAPS8 were not captured).
3. 7958d0/793f70: registry value CTRLFLAG0 is absent (3d08ae0 = 0).
4. 401ac0: slot 0 follows the pinned loader's (eb4cdcca, DLL 3293cf35) 401ae1/ae2
   patch from its source: `LoadChrmodels` stores `CHRMODELS_orig.dll`'s handle in
   3ab9170 and its exports are bound. The loader's machine code was not executed.
5. 401ac0: all the model DLLs prefer base 0x10000000 (so do the loader and the Input mod).
   The native loader maps them at ascending 64 KiB-aligned bases from 0x20000000
   with relocations applied.
6. 401830 → 780180: the matrix stack is as njInitMatrix leaves it. Other
   pre-40cad0 matrix use (781250 view setup) is unported.
7. 401830: under the Input mod no DirectInput keyboard device exists (the mod's
   keyboard drives input); the DirectInput8 mouse objects are host tokens; window
   creation is unported, so 3d0fd30/3d0fd34 keep load-image values; 793c70,
   794540 (the nine dynamic vertex buffers) and 785240 are skipped.
14. WinMain: GetPrivateProfile semantics re-implemented; imm32/ImmDisableIME host
    tokens; the media-player MessageBoxA is reached (3ab918c/3ab9190's static
    constructor is unported); 401a20 sees one fixed C:\ drive without the disc.
15. Heap: HeapCreate/HeapAlloc results are native identities (handle 0x510000,
    blocks from 0x50000000).
8. 408630: the HeapAlloc address is unknown, so the queue cursor assumes a 32-byte-aligned base.
9. 410510: no native audio backend. The device owner reports DSERR_NODRIVER, so the
   source's silent path runs (the reference has sound).
10. D3D8 transform reads before any SetTransform use the D3D8 creation default
    (identity); render states 0xe/0x17 read before any SetRenderState use 1/4.
11. 780030/7959a0: the registrations at 77f048, 77f20f, 785261 and 78123a>789c50
    are ported; those inside still-unported owners (78818f in 788000, 793e75 in
    793e70, 795380 via 788007>795420) are absent natively.
12. 78ba50: the 3d0fe00 vertex-buffer records hold only 791610's zero stores,
    so Clear's 794800 walk finds no buffers.
16. 78dbe0 → D3DX8: the reference CPU is taken to have SSE without 3DNow! and
    no DisableD3DXPSGP value (dispatch slots 774310/774150); RCPPS is modelled
    as IEEE division (the reference CPU's estimate table was not captured);
    3d129c0 keeps load-image zeros because 791170 is not run.
17. 789bd0: the host message queue is empty headless, so 3d08534 is unknown.
13. D3D8 frame methods: no display backend; the recording owner returns D3D_OK.

### 2.3 Latest gates (all pass)

They are in `_reference/sadx/production-integration-20261012/`, newest last:

- `fog-82`, `keyboard-83`, `owners-83`, `mode-leaves-84`, `model-dlls-85`,
  `mode-413c00-85`, `fade-86`, `sound-87`, `consumer-87`, `frame-owners-88`,
  `frame-calls-89`, `sega-setup-90`, `winmain-91`, `heap-cd-92`, `library-device-93`,
  `system-init-94`, `registrations-95`, `frame-callbacks-96`, `screen-97`, **`endscene-98`**.
- At `endscene-98`: 103 native programs, 4 Python tests, the owner checker and the profile
  patch checker clean (0 UNHANDLED), exit 0.

Oracle reports are copied into `post-repair-comparisons/` and indexed
(`index.json`, 60 reports after gate 98).

### 2.4 What does not exist yet

- The Metal backend behind `D3D8FrameDevice` (BeginScene/EndScene/Clear/Present/
  GetRenderState) and real draw submission. Frame methods and draws are only
  recorded in `D3D8StateOwner`.
- The game's file and asset loading: 421180 LoadPVM → texture lists, the PRS
  files, .rel stage loaders (4221e0's real body), and SET/CAM files.
- Task execution of real objects (40b170 RunTask is in `task.hpp`, but most
  object exec callbacks are unported). Most gameplay subsystems are ported as
  module-level owners (player physics, collision, animation) but are not composed
  into `NativeGame` yet.
- A CoreAudio backend and the 40ff50 stream thread.
- The AppKit app window driving the loop.
- Physical controller verification, and the macOS keyboard → DIK and Input-mod mapping in the app.
- Pre-40cad0 WinMain 402c92..402d7c (cliplevel inits 5850f0…600700, SetClip
  exports, 781570 → 791170 projection, 781250, 40e2e0), the skipped 781070/791610
  owners, and the 77ffc0 registrations at 78818f and 793e75.

---

## 3. Workspace layout

| Path | What it is |
|---|---|
| `sadx-native/` | The C++17 port (untracked in the rea repo). `src/` modules, `tests/` unit tests and `native_*_probe.cpp` oracle probes, `tools/` Python oracles and generators, `third_party/` (Berkeley SoftFloat, Bochs FPATAN), `Makefile` |
| `SADX_NATIVE_PORT_HANDOFF.md` | Step ledger (≈158 KB). The goal's own file |
| `SADX_AGENT_GUIDE.md` | This guide |
| `Sonic Adventure DX/` | The game install (reference only; never commit). `system/` has the model DLLs; `mods/` has the speedrun mods |
| `sadx-mod-loader/` | Mod loader material (untracked) |
| `_reference/sadx/` | Evidence and tooling (≈5.9 GB, untracked): REA Ghidra/Hopper evidence folders, snapshots, runner scripts, request files, gates, comparisons, the pinned Python venv (`_reference/sadx/python`), and Ghidra 12.1.4 (`_reference/sadx/tools/ghidra_12.1.4_PUBLIC`) |
| `/private/tmp/sadx-rea/` | Installed REA package used by the CLI (`node_modules/rea-agents/scripts/rea.mjs`) |
| `/private/tmp/sadx-claude-work/` | Scratch (Hopper JSON dumps, logs) |

### Build and test commands (run from the rea root or `sadx-native/`)

```sh
make -C sadx-native -j10 all test probes build/metal-texture-test build/metal-sprite-test build/metal-sprite-composition-test   # the full gate command
make -C sadx-native reference-<name>-test        # one original-instruction oracle (74+ targets)
python3 sadx-native/tools/check_global_owners.py # one-owner-per-global checker (also run by `make test`)
sadx-native/build/sadx-boot "Sonic Adventure DX/sonic.exe" "Sonic Adventure DX/mods/sadx-input-mod/sadx-input-mod.dll" 5 --survey
```

Makefile facts:

- `CORE_NAMES` lists core modules. Every name needs `src/<name>.cpp/.hpp` **and**
  `tests/<name>.cpp`, and the test must also be added explicitly to the `test:`
  recipe (`./build/<name-with-dashes>-test …`).
- `PROBE_NAMES` lists probes: `tests/native_<name>_probe.cpp` builds
  `build/native-<name>-probe.dylib`. Add a `reference-<name>-test` target and its
  `.PHONY` entry.
- Variables: `GAME_EXE ?= ../Sonic Adventure DX/sonic.exe`,
  `GAME_SYSTEM ?= ../Sonic Adventure DX/system`,
  `REFERENCE_PYTHON ?= ../_reference/sadx/python/bin/python` (Unicorn 2.1.4).
  Paths contain spaces, so never use `$(dir …)` on them.
- Compiler flags: `-std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -arch arm64`.
  `-Wmissing-field-initializers` is on: aggregate inits must list every field, or
  the struct gets a constructor.

### Recording a gate

```sh
D=_reference/sadx/production-integration-20261012; CMD='make -C sadx-native -j10 all test probes build/metal-texture-test build/metal-sprite-test build/metal-sprite-composition-test'
eval "$CMD" > $D/<name>-build-and-test.log 2>&1; rc=$?
_reference/sadx/python/bin/python -I sadx-native/tools/record_gate.py --log $D/<name>-build-and-test.log --command "$CMD" --exit-status $rc --limit "what was added; oracle counts" --limit "No playable game." --output $D/<name>-gate.json
cp sadx-native/build/original-<x>-comparison.json $D/post-repair-comparisons/
_reference/sadx/python/bin/python -I sadx-native/tools/index_comparisons.py $D/post-repair-comparisons --gate <name>-gate.json --note "…"
sadx-native/build/sadx-boot "…sonic.exe" "…sadx-input-mod.dll" 5 --survey > $D/sadx-boot-survey.txt
```

---

## 4. The method, step by step

Every step follows the same loop. The thinking behind it: the binary is the
only authority. Each piece of native code must be a direct translation of exact
instructions, proven against those instructions executing, with every unknown
kept explicit.

### Step 1: find the next owner

Run `sadx-boot` (stop policy). It prints the first unported owner with its call
path and site (for example `40cad0>411d00>781010 (call site 411de8)`) and every assumption.
The survey gives the breadth. Work depth-first along the real boot path: the
first stop is what blocks real execution.

### Step 2: read the code with REA (Hopper first, Ghidra to cross-check)

**Hopper** (fast, one function per call):

```sh
export HOPPER_LAUNCHER_PATH="$HOME/Applications/Hopper Disassembler.app/Contents/MacOS/hopper" REA_HOPPER_STARTUP_TIMEOUT_MS=900000
node /private/tmp/sadx-rea/node_modules/rea-agents/scripts/rea.mjs instructions "Sonic Adventure DX/sonic.exe" 0x415000 \
  --provider hopper --snapshot _reference/sadx/hopper-recovery/sonic-hopper-snapshot.json --format json > /private/tmp/sadx-claude-work/0x415000.json
node …/rea.mjs xrefs "Sonic Adventure DX/sonic.exe" 0x3d128a4 --provider hopper --snapshot … --format json
```

Facts about Hopper output:

- **Sort `normalized_result.instructions` by address.** Hopper lists them in basic-block
  order, and the address-sorted list is the linear disassembly. Use `sorted(set(…), key=address)`.
- `procedure.address` may be a *containing* function. Filter by address range to
  see the function you asked for.
- Labels like `dword_value_1142947840+4` or `aImm32dll+16` are **address labels**,
  not values. Decode the actual operand from the instruction bytes (Step 3).
- Hopper xrefs miss indexed accesses (`[reg+0x3d12a00]`). Scan `.text` for the
  little-endian address bytes instead (Step 3).
- The Hopper here is a demo build. It sometimes hangs for 10+ minutes
  ("REA timed out waiting for Hopper bridge readiness"). Run long batches with
  `run_in_background`, and kill a hung query
  (`pkill -f 'rea.mjs instructions'; pkill -f 'Hopper Disassembler'`) before retrying.
- Record the evidence ids (`evidence_id`, e.g. `ev_4277b26b…`) in the handoff entry.

**Ghidra** (decompiles; slower to start, about 3 minutes per import): use the owned
runner. One-off `rea decompile --provider ghidra --snapshot …` calls fail with
`profile_mismatch`, so don't use them.

```sh
cat > _reference/sadx/ghidra-<topic>-requests.json <<'EOF'
[{"tool":"analyze_function","args":{"procedure":"0x7949f0"},"label":"fog-table-7949f0"},
 {"tool":"xrefs","args":{"address":"0x3d128a4"},"label":"fog-supported-3d128a4-xrefs"}]
EOF
L=/private/tmp/sadx-claude-work/ghidra-run.log; : > $L
{ while ! grep -q READY $L; do sleep 2; done; echo '{"tool":"batch","requests_file":"_reference/sadx/ghidra-<topic>-requests.json"}';
  while ! grep -q '"retained":"<last-label>"\|REA rejected <last-label>' $L; do sleep 3; done; echo '{"tool":"close"}'; } \
| node _reference/sadx/ghidra-live.mjs _reference/sadx/ghidra-game-loop-config.json > $L 2>&1     # run_in_background
```

- **Gate stdin on READY.** If the lines are piped before the runner prints READY,
  readline drops them and the runner idles forever with no Java process. That
  happened once.
- Results land in `_reference/sadx/ghidra-game-loop-recovery/<label>.json` and
  `<label>.pseudocode.txt`.
- A `close` reporting `cleanup_incomplete` (process ownership token unreadable)
  has been transient. Check `pgrep -fl java` and the temp directory, and move on if both are gone.
- Ghidra pseudocode is a guide, never authority. Its NaN handling and argument
  passing through COM calls are often wrong. The instructions decide.

### Step 3: byte-level checks (exact operands, strings, tables, writers)

Use the pinned Python with `tools/pe_image.py`:

```python
import sys,struct;sys.path.insert(0,'sadx-native/tools')
from pe_image import PEImage
im=PEImage('Sonic Adventure DX/sonic.exe')
b=im.read(0x794a58,10)       # D8 1D disp32 = fcomp dword [disp32]; disp at +2
im.read(0x88bfa0,12)          # strings, e.g. b'CTRLFLAG0'
# Writers of a global: scan .text for its LE bytes, inspect the 2 opcode bytes before:
t=[s for s in im.sections if s['name']=='.text'][0];text=im.slice(t['file_offset'],t['raw_size'])
```

- `PEImage.read` raises for BSS (virtual tail): "not uniquely file-backed", which
  means the load image value is zero.
- Use this for constant decoding (x87 m32/m64 operands), jump tables (e.g. 422680's table at 422710),
  string arguments (`push imm32`), IAT slots (7db040 LoadLibraryA, 7db044 GetProcAddress),
  and call-site scans (`e8 rel32` to find callers).
- Big regular functions can be **transcribed by a script from the bytes**, for example 401ac0's 85
  `push name / call GetProcAddress / mov [global],eax` triples into the
  `model_dlls.hpp` table. The table is then verified by an oracle running the real function.

### Step 4: translate into a native owner (C++)

Conventions (follow the surrounding code's dense style):

- **Facts are `std::optional`.** Unknown is never silently 0. A read of an unknown
  fact stops with the **source boundary** (the instruction address that would
  read it) and the `required_fact` (the global's address).
- **One owner per global.** Each module has a `*State` struct owning the globals no
  other module owns. Shared globals are **borrowed** (references or pointers) from
  their owner. Members are named `<meaning>_at_<hexaddress>`. Call-argument
  snapshots are named `*_from_<address>`. Contiguous runs without a typed owner
  use `SourceBytes<base,size> block_at_<base>` (`src/source_bytes.hpp`). Tables of
  globals bound by one routine use an `// owner-table: Struct … // end owner-table`
  block. **`tools/check_global_owners.py` enforces this** (scalar widths, whole
  blocks, owner tables; `.rdata` constants 0x7db000..0x891b88 are exempt). When it
  flags a duplicate, make the non-writing side a borrow.
- **Unported callees are nested owners.** A `std::function` invoke callback
  receives `{source_call, callee, arguments…}` and returns
  `GameLogicOwnerResult` (plus EAX where the source consumes it). The runtime
  routes ported callees and reports the rest through `unported()`. Loop bodies
  that would draw or release are nested owners named by their **first body
  instruction** (40d520, 40eac0, 42f9cc, 40d3f5).
- **Partial state on failure.** Stores that happen before the failing instruction
  stay. For example, 794ec0 stores 3d12798 between the device load and its
  dereference. Renderer failures keep earlier stores.
- **Explicit boundaries instead of guesses:**
  - `nonterminating`: 422680's `jmp 422700` loop;
  - `unmodeled_operation`: FYL2X, and frexp of Inf/NaN;
  - `index_out_of_range`: a read past a table, such as 787260 reading 3b11178;
  - `unsupported_records`: keyboard records not at 3ce6e38.
- **x87 is modelled exactly** with `X87Context` (SoftFloat ext80, CW precision and rounding):
  - Use `from_float`/`from_double` for FLD, `*_float_memory` for m32 operands
    (reverse forms are memory op register), and `compare` (FCOM, signalling) vs
    `compare_quiet` (FUCOM).
  - Use `to_float`/`to_double` for FST and `to_int64_current_rounding` for FISTP.
  - Keep extended intermediates where the source keeps them (for example 7949f0's ext D versus float v_14).
  - Model `_ftol2` 644340 exactly (in `ninja_fog.cpp`): FISTP under the current RC, then a one-step correction from the FST m32 signs.
- **Branch decoding after `fnstsw ax`** (C0=0x01, C2=0x04, C3=0x40 in AH):
  - `test ah,0x41 / jne`: only *greater* falls through.
  - `test ah,5 / jp`: only *less* falls through. `test ah,5 / jnp`: jumps on less.
  - `test ah,0x44 / jp`: only *equal* falls through. `test ah,0x44 / jnp`: jumps on equal.
  - `test ah,0x41 / jp`: jumps on greater or unordered (falls through on less or equal). `jnp`: jumps on less or equal.
  - Unordered sets C0, C2 and C3.
- **Set EAX wherever the source defines it.** Callers in other functions may consume
  it. Examples: 427ec0 returns 0x3b0efdc, 414470 returns 0x3c4e440, 425fe0 defines only AX.
- **Calling conventions.** cdecl arguments are in push order, reversed. Register arguments
  appear too (4244a0 takes EAX; njInitMatrix takes ECX/EDX). D3D8 methods are
  stdcall COM: SetRenderState is `[vtbl+0xc8]`, GetRenderState `+0xcc`, GetMaterial `+0xac`,
  SetMaterial `+0xa8`. DirectInput device: GetDeviceState `+0x24`, Acquire `+0x1c`,
  SetDataFormat `+0x2c`, SetCooperativeLevel `+0x34`.
- **DLL images.** `ExecutableImage` gives load-image bytes (`byte`/`u32`), exports
  (`export_named`, exact names, forwarders explicit) and `rebased(base)` (full mapping
  plus HIGHLOW relocations). Pointers into DLL data are VAs at the native base and
  are dereferenced through `ModelDllSet::u32`.

### Step 5: unit test (`tests/<module>.cpp`)

The unit test covers what the oracle cannot compare: unknown facts, missing
owners, explicit boundaries, partial state, and nested-owner stops. Keep it short.

### Step 6: original-instruction oracle (the proof)

Each oracle has two parts:

- **Probe**: `tests/native_<name>_probe.cpp`, an `extern "C"` function taking a
  flat `uint32`/byte layout of all inputs and returning all outputs.
- **Oracle**: `tools/verify_original_<name>.py`, which runs the **original
  instructions in Unicorn 2.1.4**, randomises inputs, and compares byte for byte.

The harness lives in `tools/verify_original_math.py`. `Original(image, cw).machine`
maps `.text`/`.rdata`; `STACK`, `STOP`, `DATA` and `CODE` are scratch regions.

Recurring techniques:

- **Map the whole PE**: map every non-.text/.rdata section with file bytes,
  including the zero tail (see `verify_original_fog.py`), so the real CRT code
  (frexp, pow, _ftol2) runs.
- **Map page 0** when code reads `fs:[0]` (SEH prologues; FS base is 0). Map the
  stack-cookie page 0x2bcb000 when code checks the cookie 2bcbddc.
- **COM stubs**: a vtable at `DATA+0x400`, a method at `CODE+0x300`
  (`xor eax,eax; ret 0xc`), and a hook recording `(state, value, ret-6)`.
- **Callee fixtures**: hook the callee entry, record `(callee, ret-5, args)`,
  return a case-derived EAX by popping the return address, or `emu_stop()` at a
  chosen call ("fail-at-call N"). The native probe's invoke behaves the same way.
- **IAT fixtures**: write stub addresses into the IAT slots (`ret 4`/`ret 8`
  stubs plus hooks).
- **Loop-body stops**: hook the body's first instruction, stop there, and compare
  the state at that point with the native nested-owner stop.
- **Jumped-into owners**: when a callee is reached by `jmp` and tests its own
  condition first (424b10), hook its post-condition continuation (424b26) and
  unwind through its own epilogue (424c14).
- **Source faults**: catch `UcError` (READ/WRITE_UNMAPPED) as a source stop, for
  example a null data pointer. Always print the EIP and inputs when debugging an
  abort.
- **Independent reimplementation**: where the native port relies on a native
  loader policy, the Python side reimplements it independently (for example
  `rebase()` in the leaves oracle), so the native code is cross-checked.
- **Explicit cases**: cases that hit a native explicit boundary while the source
  continues into unported code are counted, not compared.
- **x87 status flags**: compare and report them separately. Unicorn does not raise PE,
  nor IE for FCOM on QNaN. The native port does, as Intel specifies.
- **Report** the source SHA, native dylib SHA, source-file SHAs, original-range
  SHAs, cases, outcomes, mismatches with examples, and `limits=__doc__`.

### Step 7: mutation testing (prove the oracle can fail)

Seed three to seven plausible bugs in the native code (thresholds, offsets,
operand order, flag masks), rebuild the probe, and confirm mismatches appear.
When a mutant survives, add fixtures for its distinguishing inputs. Examples:

- RC down/up control words for the `_ftol2` borrow;
- `end` values whose negation ends in 0xff (the carry only shows in bits 8–15);
- zero holes in the held keys;
- stage boundary words;
- flags with bit 0x02 but not 0x20.

**Always `time.sleep(1.1); os.utime(source, None)` before the rebuild.** `make`
has an mtime race that silently tests the previous build. That happened four
times, including once with future `utime` timestamps; sleeping past the
filesystem's timestamp granularity and stamping "now" avoids it.

### Step 8: wire into `NativeGame`

- Add the state and owners as members.
- Initialise every fact from the load image (`u8`/`u32`/`s32`/`f32` lambdas in
  `initialize_source_facts`; BSS is zero, outside the image is unknown).
- Route the loop, startup, mode or reset callee to the module. Unported nested
  callees go to `unported(callee, site, path)`. In survey mode those return
  completed with EAX 0.
- Report a new assumption whenever a native choice replaces an uncaptured
  reference fact.
- Run `sadx-boot`, confirm the stop moved, and check `tests/game_runtime.cpp`.

### Step 9: gate, index, survey, handoff

Record the gate (section 3). Insert a handoff entry at the top of the per-step
list (after the most recent entry; entries are lines starting `  - **…**`). The
entry needs:

- the REA evidence ids;
- what was ported, instruction by instruction;
- the oracle and mutant counts;
- the owner changes;
- assumptions;
- the new stop.

Update this guide's status section.

---

## 5. Module map (what exists, by area)

| Area | Modules |
|---|---|
| Runtime composition | `game_runtime` (NativeGame, D3D8StateOwner, survey/stop policies), `sadx_boot` |
| WinMain | `game_winmain` (4025b0 settings span, 401a20/401990 CD scan), `win32_profile` (GetPrivateProfile owner) |
| Memory | `guest_heap` (native Win32 heap identities), `ninja_memory` (njMalloc 780f60 / njFree 780fb0) |
| Library/device init | `game_library_init` (781070), `ninja_device_init` (791610), `ninja_system_init` (401830), `ninja_texture_pool` (77f610) |
| Profile patches | `tools/profile_patches.json` (every loader/mod patch, profile-evaluated) and `tools/check_profile_patches.py` |
| Game loop and modes | `game_loop` (40cad0), `game_startup` (40c030), `game_mode` (40c090, 27 modes), `game_mode_leaves` (415000/414470/422680/413c00/42c4f0 …), `game_reset` + `game_reset_leaves` (40bf40, 40d0d0 stream players), `game_frame_control`, `game_frame_owners` (40d4d0 …), `game_logic*`, `game_controller`, `game_scene_logic`, `game_view` (4208e0) |
| Frame timing | `speedrun` (63 FPS limiter 781060), `mac_clock` (10 MHz virtual QPC) |
| D3D8 frame boundary | `ninja_frame_calls` (411d00's 781010..781050 thunks; `D3D8FrameDevice` is what Metal implements), `ninja_screen` (780c30 screen setup, 7912e0 viewport refresh, 78dbe0 depth, 77dfc0 polygon; `NinjaScreenDevice`), `game_window` (789bd0 / the loader's `HandleWindowMessages_r`), `ninja_vibration` (785190), `ninja_input_devices` (792100/791df0), `ninja_debug_text` (793e70) |
| Mod loader / mods per frame | `mod_loader_frame` (426063 `ProcessCodes` = Speedrun 2.0 `OnFrame` under the profile) |
| Input | `input`, `input_mod_pad`/`input_mod_axes` (Input mod 2.6.1 OnInput), `mac_input` (GCController → SDL-label mapping), `ninja_keyboard` (DirectInput keyboard 791bd0/77f0a0), `scene_input_producer` |
| Ninja render layer | `ninja_render` (render states, blends, depth), `ninja_sprite*`, `ninja_queue_lifecycle`/`ninja_queue_consumer` (408630/407ab0/4086f0), `ninja_texture*` (PVM parse/decode, texture selection), `ninja_fog` (fog device, tables, switches), `ninja_projection_range`, `ninja_math`, `player_coordinates` (matrix stack) |
| Metal | `metal_texture`, `metal_sprite`, `metal_stage` (offscreen tests and a static Emerald Coast render) |
| Images and DLLs | `executable_image` (exports, relocating rebase), `model_dlls` (401ac0) |
| Audio | `audio_stream` (410510), `sound_manager` (4250d0) |
| Tasks | `task`, `task_work` (scheduler, heap 40b1a0/pool 40b460) |
| Gameplay (module-level, mostly not composed yet) | `player_*`, `sonic_player`, `sonic_animation_*`, `collision_*`, `player_scene_*` |
| CRT and maths | `crt_*` (rand, atan2, asin, acos, floor, sqrt, fabs, integer), `x87_math`, `x87_transcendental` (Bochs FPATAN), `sse_math`, `d3dx_math` (D3DX8 772615/774310/774150, the SSE dispatch path) |
| Owner check | `tools/check_global_owners.py` |

---

## 6. Pitfalls already hit (save yourself the time)

- **Shell.** zsh rejects `echo ===`. BSD `sed` has no `\b`. macOS `sed -i ''`.
  `$(dir …)` breaks on spaced paths. `grep error:`, not `grep error`, because
  `-Werror` lines match.
- **Hopper.**
  - Listings are in block order; sort them.
  - Labels are not values.
  - Hangs mean demo dialogs: kill and retry.
  - xrefs miss indexed accesses.
- **Ghidra runner.** Gate stdin on READY. Don't use one-off `--snapshot` decompiles
  (profile_mismatch).
- **`make` mtime race in mutation scripts.** Touch the source before rebuilding.
- **Aggregate structs** with new members break `-Wmissing-field-initializers` in
  tests and probes. Give the struct a constructor (as `GameModeLeafState` has),
  or update every initializer.
- **Duplicate owners** sneak in as "value copies refreshed before use". The
  runtime used to copy 3abdc7c/3b2a2e4 into InputModFrameState that way. Make
  them borrows.
- **Probe field order** must match struct declaration order. The `GameViewState`
  FOV words are declared 3aad0bc, 3ab98ec, 3b2cbb4.
- **The consumer's owner check**: `queue_runtime.render_state` must be
  `&sprite.texture_runtime.render`. `NinjaTextureSelectionRuntime` holds the
  runtime's render state by reference.
- **`decode_ninja_sprite_constants`** takes the 20 bytes of five floats (7db2c4,
  7dce6c, 7e9b40, 7dcb2c, 7dcb10). The 9279d8 run in the asset file is the test's
  sprite frame.
- **Oracles resolve source files from their own location**
  (`pathlib.Path(__file__).resolve().parents[1]`), so the Makefile targets work
  from `sadx-native/`.
- **`sadx-boot`'s Input mod path** is `mods/sadx-input-mod/sadx-input-mod.dll`.
- **Run Hopper queries one at a time.** Concurrent `rea.mjs instructions`
  calls (two batches at once, or exe and DLL together) fail with
  "Hopper's launcher helper failed before bridge readiness" (`cleanup_incomplete`).
  Use one serial script.
- **The profile patches game code.** The reference loader is 3293cf35 (eb4cdcca;
  the install's `mods/SADXModLoader.dll` e40fac49 is the old original). Before
  porting any sonic.exe range, check it against the loader's unconditional
  effects (`_reference/sadx/updated-pc-reference/README.md`, source in
  `sadx-mod-loader/SADXModLoader/`) and the Speedrun Init inventory
  (`_reference/sadx/speedrun-dll-recovery/init-patch-inventory.json`;
  `expected-hooks.json` is incomplete). Known unported patched sites: 425ff0's
  426028 store, 42608f/426134 (Gamma timer calls), 71adce (Chao race), 431517 and
  5132b1 (Skipma), 434870/438330 (CCEF), 533939/440ef5 (EHCF), the loader's
  402513, 40297a, 789e50, 50473e, 402614, 437547, 42f1c5/42f1e9, 40ff00, 794000,
  the texture/sound hooks, and data writes such as 7dce58 = 63.0f, 38a5db8 and
  87cba8/87cbac.
- **Read the gate's exit code before recording it.** `record_gate.py --exit-status`
  must be the real `make` status; one gate was briefly recorded as passing while
  `make test` had failed on the owner checker. Capture `rc=$?` and check it.
- **Run `tools/check_profile_patches.py` in every gate** (it is not part of
  `make test`). Mark a patch `handled_by` only when native code models it or the
  patched behaviour is shown to be equivalent.
- **Hopper's demo hangs often.** For more than a few functions use the owned REA
  Ghidra runner with a requests file (one startup, many `analyze_function`s,
  pseudocode and assembly retained).
- **The sonic player oracle** has 2,840 status-flag mismatches that predate this
  session (0 stored mismatches). That is its retained baseline, not a regression.

---

## 7. REA itself (how it fits)

REA (this repository) is a local reverse-engineering tool with shared CLI and
MCP workflows (see [README.md](README.md), `docs/cli.md`, `docs/mcp-contracts.md`,
`docs/ghidra-provider.md` and `docs/installation.md`). For this goal:

- **Every analysis is REA Evidence** (`evidence_id`, subject digest, provider
  version, analysis profile, limitations). Cite the evidence ids in the handoff.
- **Providers:** `hopper` (Hopper 6.6 demo under `~/Applications`; needs
  `HOPPER_LAUNCHER_PATH`) and `ghidra` (12.1.4 headless; env from
  `_reference/sadx/tools/ghidra-game-loop-environment.json`: `GHIDRA_INSTALL_DIR`,
  `JAVA_HOME=/opt/homebrew/Cellar/openjdk/27/...`, `GHIDRA_HEADLESS_MAXMEM=2G`,
  `REA_GHIDRA_STARTUP_TIMEOUT_MS=600000`, `REA_GHIDRA_SEED_FILE`).
- **Useful CLI commands:** `instructions`, `xrefs`, `decompile`, `function`,
  `inspect-native-instruction`, `resolve-native-call-targets`,
  `trace-native-values`, `read-bytes`, `address-to-file-offset` and `capabilities`
  (`rea.mjs --help` lists all of them).
- **Snapshot:** `_reference/sadx/hopper-recovery/sonic-hopper-snapshot.json`
  (Hopper). The Ghidra runner owns `sonic-ghidra-game-loop-snapshot.json`.
- **Retained Ghidra dossiers** for earlier work are in
  `_reference/sadx/ghidra-game-loop-recovery/` (game loop, consumer, fog, keyboard,
  callback list, …) and the older `ghidra-*-recovery/` folders. Check them before
  re-analysing.

---

## 8. Next steps, in priority order

1. **Pre-40cad0 WinMain init** (now the largest fidelity gap; the frame calls
   run on its missing state, see assumptions 1, 6, 7, 11, 12, 14 and 16). WinMain is
   4025b0 (called from 789d1a); its settings span is ported. Next:
   - **WinMain 402c92..402d7c**: the ShowCursor loop, the cliplevel inits
     5850f0/556e40/556d60/568d20/4f68e0/5adc40/600700, the SetClip_EC00/EC01/EC31
     model-DLL exports, **781570 (ECX 3aad0a0) → 781500 → 791170** (the projection:
     FPTAN/FPATAN, D3DX 7716df perspective, 7707f7 multiply, SetTransform 3/2,
     MultiplyTransform, the 389d398 copy and 770d03 inverse into 389d358; FixFOV's
     781529/791251 hooks take the 4:3 path), 781250 and 40e2e0. This fills 3d129c0,
     which 78dbe0 reads;
   - the CRT static constructor of the 3ab918c object (7c4e5c area);
   - 794540 (vertex buffers), 793c70, the window (793ef0 / the loader's 789e50),
     7914b0 (the TextureCaps 0x800 filter tables), 788000 and 402513 `InitMods`;
   - the profile divergences the patch inventory found outside ported ranges:
     785330 (Input rumble `pdVibMxStop_r`), 77e390 (the loader's
     `draw_particle`), and the `input::controller_enabled` pointer repoints at
     40fe0d/40fe2f (inside the ported 40fdc0) and 40f30c;
   - Port 401830's callees: 77f240, 780ea0, 780870, 7808c0, 780850/78e720
     (writes 3d128ac), 7803b0, 780180 (done), 77f610, 780140, 77ff60, 402f10,
     77f150, 77f0a0 (done), 77efd0 and 40bb20; and the 77ffc0/7959c0
     registrations (77f048, 77f20f, 785261, 78818f, 793e75; 78123a>789c50,
     788007>795420), so 780030/7959a0 see the reference lists.
   - Port 791610's callees: Direct3DCreate8 and 7943c0 device creation (under the
     loader, 794000 is `CreateDirect3DDevice_r`) as native device owners,
     794ec0 (done), 78d0d0, 796710, 791860 and 7943f0, including the 3d0fe00
     dynamic vertex buffers.
2. **40c090 transitions:**
   - **421180 LoadPVM** (the current stop): port the loader's `LoadPVM_r` /
     `LoadSystemPVM_r` / `njLoadTexturePvmFile_r` chain (source
     `sadx-mod-loader/SADXModLoader/TextureReplacement.cpp`, binary DLL 3293cf35),
     its replace-path map and texture-pack probe, and the game's PVM file loading
     into texture lists (the format parser is `ninja_texture`);
   - 42d3b0 (save data);
   - 42cc70 (the fade machine with **40b170 RunTask**);
   - 4073b0 (rectangle enqueue).
   - These lead to the first visible frame (the SEGA logo).
3. **Texture creation and the debug-text owners**: 7871a0/7871c0/78cbd0 (shared
   with LoadPVM), then 793c70 → 793be0 (the font texture at 3d088ac) and 793cc0
   (EndScene's debug-text flush, the current survey's last unported frame owner),
   then a vertex-buffer owner for 794800's Lock/Unlock.
4. **The Metal backend behind `D3D8FrameDevice`** and the recorded draws, then the
   AppKit window driving the loop.
5. **Asset and file loading** (PRS/PVM/.rel), and composing the task system with
   object callbacks.
6. **Audio:** a CoreAudio device owner, the 40ff50 stream thread, the active sound
   records (4246c0, 425710 …) and 424b10 3D sound.
7. **Input on the app:** the macOS keyboard → DIK and Input-mod keyboard mapping,
   and the GameController pads (Xbox, Switch 2 Pro) verified physically.
8. **The rest of the profile's patches as their ranges are ported** (see the
   pitfall list in section 6), then verification against the Windows reference
   PC (Windows 11 AtlasOS, Intel Ultra/NVIDIA), as the reference README describes.

---

## 9. Handoff entry template

```
  - **<Title> (gate `<name>-gate.json`):** REA Hopper (`ev_…` <addr>, …) [and Ghidra `ev_…`]. <module>.hpp/.cpp ports <addr> (<instruction-level summary: stores, branches with their exact x87/flag conditions, calls and their arguments, EAX>). Unported callees are nested owners: <list>. Explicit native boundaries: <list>. Owners: <new owners; borrows; checker result>. Oracle `verify_original_<x>.py` (<fixtures>): **<N> cases, 0 mismatches** (<explicit count>); <k> mutants caught. Assumptions: <new ones>. **Native execution now stops at <path> (call site <site>).**
```
