# Method: analysing, porting and testing

## 0. Cookbook: which tool answers which question

Run from the repository root. One-time setup per shell (the paths are this Mac's;
`docs/GOAL.md` §8 lists the inputs):

```sh
export GHIDRA_INSTALL_DIR="$PWD/reference/tools/ghidra_12.1.4_PUBLIC"
export JAVA_HOME=/opt/homebrew/Cellar/openjdk/27/libexec/openjdk.jdk/Contents/Home
export REA_GHIDRA_STARTUP_TIMEOUT_MS=600000 REA_GHIDRA_SEED_FILE="$PWD/build/symbols/rea-seeds.tsv"
REA="$PWD/reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs"
EXE="game/Sonic Adventure DX/sonic.exe"; SNAP="$PWD/reference/sonic-ghidra-inventory-snapshot.json"
make symbols          # build/symbols/{symbols,conflicts,coverage}.json + rea-seeds.tsv
```

| I need… | Do this |
|---|---|
| **The name, signature or type of an address** (always first) | symbol lookup, below; then `grep -in '0x0*40c090' reference/sadx-mod-loader/SADXModLoader/include/*.h` for the declaration and its comment |
| A struct layout (`taskwk`, `playerwk`, `motionwk`…) | `grep -n -A60 '^struct playerwk' reference/sadx-mod-loader/SADXModLoader/include/SADXStructsNew.h` (the classic `SADXStructs.h` has older names); verify field offsets against the pseudocode before trusting them |
| An enum (`MD_GameMode`, characters, levels) | `grep -n -A40 '^enum MD_GameMode' …/SADXEnumsNew.h` |
| Callees of a function and which are named | the `functions.json` query below; then `grep` the override tables and `src/` for the callee's annotation or legacy name to see whether it is native |
| Named pseudocode of **one** function | `node $REA decompile "$EXE" 0x44c270 --provider ghidra --snapshot "$SNAP" --json` (snapshot hit = seconds; miss = a fresh import, minutes) |
| The full dossier (CFG, p-code, callers, API) of a function you port | the owned runner with `analyze_function` (§1); or `node $REA function "$EXE" 0x44c270 … --json` |
| Pseudocode of a **cluster** (10–200 functions) | the runner with `batch_decompile {addresses:[…]}`, then split `items[].pseudocode` into files (snippet below) |
| Who reads or writes a global | `node $REA xrefs "$EXE" 0x3abdc7c --provider ghidra --snapshot "$SNAP" --json`; raw `.text` scan: `python3 tools/analysis/xscan.py 0x3abdc7c` |
| Who calls a function | the runner with `procedure_callers`; the static callee graph inverted from `functions.json` |
| Assembly of a stored result | `python3 tools/analysis/ghidra_asm.py reference/inventory-raw/<label>.json` |
| One instruction's decoded operands | `node $REA instructions "$EXE" 0x781060 …` or the runner's `inspect_native_instruction` |
| Exact bytes / a constant's value | `PYTHONPATH=tools/lib python3 -c "from pe_image import PEImage;import struct;print(struct.unpack('<f',PEImage('$EXE').read(0x7e0178,4)))"` (prints 1.16) |
| Is the range patched by the loader or a mod? | `python3 tools/gate/check_profile_patches.py` (0 UNHANDLED) and `tools/gate/profile_patches.json`; the mod sources under `reference/` |
| Format of embedded data (models, landtables, texlists, physics) | `reference/sa-tools/GameConfig/PC_SADX/*.ini` (pinned `b72d52f3`, `reference/symbols/sources.json`): `address=`, `type=`, `pointer=` per item |
| A runtime value (what actually runs, which control word, which branch) | the guest: `build/sadx-boot`/`sadx-app --guest` with an override or trace hook (§4); HLE assumptions print at exit |
| Is my code equal to the original? | its oracle: `make oracle-<slug>`, `python3 tools/oracles/verdict.py build/oracles/<slug>.json`; mutants: `tools/gate/mutate.py` (§3) |
| Is REA/Ghidra healthy? | `node $REA doctor --provider ghidra --json` before blaming the provider |

Symbol lookup (prints the preferred name and every alternative with its source header):

```sh
python3 - 0x40c090 0x3abdc7c 0x44c270 <<'EOF'
import json,sys
db={e['address']:e for e in json.load(open('build/symbols/symbols.json'))['symbols']}
for a in sys.argv[1:]:
    e=db.get(hex(int(a,16)))
    if not e: print(a,'UNNAMED: record a decision in reference/symbols/overrides.tsv'); continue
    alts=', '.join(x['name']+' ('+x['source'].split('/')[-1]+')' for x in e.get('alternatives',[])[1:])
    print(e['address'],e['kind'],e['name'],e.get('type') or '',e.get('signature') or '','| also:',alts)
EOF
# 0x40c090 function GameModeHandler void () | also: FUN_0040c090 (functions.json)
# 0x3abdc7c global ulGlobalMode unsigned int  | also: GameMode (SADXVariables.h)
# 0x44c270 function PGetAcceleration void (taskwk* twp, motionwk2* mwp, playerwk* pwp) | …
```

Callees with names (the start of every context bundle):

```sh
python3 - 0x40c090 <<'EOF'
import json,sys
inv={f['address']:f for f in json.load(open('reference/inventory/functions.json'))['functions']}
db={e['address']:e['name'] for e in json.load(open('build/symbols/symbols.json'))['symbols']}
f=inv[hex(int(sys.argv[1],16))]
for c in f.get('callees',[]): print(c, db.get(c,'?'))
EOF
```

Splitting a `batch_decompile` result into one file per function:

```sh
python3 - reference/inventory-raw/<label>.json <<'EOF'
import json,sys,pathlib
r=json.load(open(sys.argv[1])); body=r['content'][0]['text'] if 'content' in r else r
body=json.loads(body) if isinstance(body,str) else body
out=pathlib.Path(sys.argv[1]).with_suffix(''); out.mkdir(exist_ok=True)
for it in body.get('items',[]): (out/f"{it.get('address','unknown')}.c").write_text(it.get('pseudocode') or '')
EOF
```

(Check the stored result's envelope once. The runner writes the MCP result as returned, and
the snippet handles both the wrapped and the plain form.)

What REA cannot do (do not wait for it): import types or signatures into Ghidra, emulate,
infer structs, or compile pseudocode to source. Types live in project headers and are checked
by oracles. Runtime facts come from the guest. Pseudocode is evidence, never source
(`STYLE.md`).

## 1. Reading the binary with REA (Ghidra)

Run `make symbols` before the inventory runner. `tools/symbols/build_symbol_db.py`
merges pinned mod-loader declarations, sa_tools labels, inventory boundaries and
`reference/symbols/overrides.tsv`. Generated `build/symbols/` contains the full
database, conflict candidates, coverage (including unnamed owner addresses) and
`rea-seeds.tsv`. Unknown ABI sizes remain null. `make check-symbols` regenerates
and checks these outputs in the gate; `python3 tools/symbols/build_symbol_db.py --check`
alone detects absent/stale outputs. Fixture tests need neither binary nor upstream
checkout. The source checkouts are required for generation.

REA (`~/Developer/rea`, CLI + MCP) drives a headless Ghidra. The project owns one Ghidra
session through `reference/ghidra-live.mjs` with `reference/ghidra-inventory-config.json`
(snapshot + environment; the older `ghidra-game-loop-config.json` snapshot is tied to the pre-move
binary path and is not used). It takes JSON requests on stdin and writes
`reference/inventory-raw/<label>.json` and `<label>.pseudocode.txt`. The runner imports
`rea-agents` 6.4.0 from `$SADX_REA_MODULES`, else from the git-ignored install
`reference/tools/rea/node_modules` (since 2026-10-13: `npm ci --prefix reference/tools/rea`
with the tracked-nowhere lockfile copied from the first install; recreate it with
`npm install --prefix reference/tools/rea rea-agents@6.4.0`), else from the original
`/private/tmp/sadx-rea/node_modules`. The CLI is `node reference/tools/rea/node_modules/rea-agents/scripts/rea.mjs`
(`docs/GOAL.md` §4.2).

```sh
cd ~/Developer/sadx-macos
python3 - <<'EOF'
import json
reqs=[{"tool":"analyze_function","args":{"procedure":"0x780c30"},"label":"screen-780c30"}]
json.dump(reqs,open('reference/ghidra-requests.json','w'))
EOF
L=/tmp/ghidra-run.log; : > $L
{ while ! grep -q READY $L; do sleep 2; done
  echo '{"tool":"batch","requests_file":"reference/ghidra-requests.json"}'
  while ! grep -q '"retained":"screen-780c30"' $L; do sleep 3; done; sleep 2
  echo '{"tool":"close"}'
} | node reference/ghidra-live.mjs reference/ghidra-inventory-config.json >> $L 2>&1
```

Print assembly from a result: `python3 tools/analysis/ghidra_asm.py reference/inventory-raw/screen-780c30.json`.

The owned runner buffers piped commands during startup; waiting for READY remains
useful for interactive orchestration but is not required for an early command pipe.

Facts about the provider (from REA's docs, verified 2026-10-12):

* `analyze_function` returns, per function: body ranges and size, assembly, pseudocode,
  callers/callees (direct edges only), basic blocks (CFG), p-code value flow
  (capped at 3,000 ops) and `native_api` (calling convention, parameters).
* `list_procedures` returns every function (address, name, thunk/external flags) in one
  call; `procedure_callers`/`procedure_callees`/`procedure_references`/`xrefs` are per
  address; `batch_decompile {addresses[]}` decompiles many (still serial inside Ghidra).
  Add any of these to the runner's `allowed` set (one line) before using them.
* The inventory config selects `build/symbols/rea-seeds.tsv` through `seed_file`
  (repo-relative), overriding the environment file's legacy seed path. The generator
  preserves every seed and explicit name from `reference/ghidra-game-loop-seeds.tsv`.
  CLI calls must set `REA_GHIDRA_SEED_FILE` to this generated file to share the snapshot
  profile. The pre-symbol snapshot is backed up locally as
  `reference/sonic-ghidra-inventory-before-symbol-seeds.json`.
* Functions reached only through pointers (vtables, callbacks, the D3DX dispatch table)
  are not auto-defined. Seed them: `REA_GHIDRA_SEED_FILE` = TSV of
  `address<TAB>function|code|label[<TAB>name]` applied before analysis (a named `function` seed
  renames, a `label` seed names a global, also in `.bss`; names without whitespace). This is the
  only durable way to put names into Ghidra: the symbol database emits the names part of this
  file (ADR-0006, GOAL §4.4). REA cannot import struct types or signatures. Keep the seed list in
  `reference/ghidra-game-loop-seeds.tsv` and grow it. Seed contents are part of
  REA's snapshot profile: after adding seeds, preserve the old ignored snapshot
  under a backup name and run the owned runner without a snapshot at its configured
  path so it can save a fresh one. Do not bypass a profile_mismatch rejection.
* The bridge is a single serial queue: a whole-program dump of ~9,300 functions is hours of
  wall-clock, ~300 KB per function on disk. Run it once, unattended, keep the snapshot.
* `~/Developer/rea/_reference`-style raw Hopper queries are retired: the Hopper demo hangs
  under load and reads one function per query.

Byte-exact checks use `tools/lib/pe_image.py` (`PEImage(path).read(va, n)`), never guesses from
listings. `tools/analysis/xscan.py ADDR…` lists `.text` references to data addresses.

## 2. Whole-program inventory (do once per binary, reuse forever)

`tools/inventory/` holds the scripts that produce `reference/inventory/`:

* `functions.json` — every function: address, size, callers, callees, and whether it is a
  thunk/import; built from `list_procedures` + `analyze_function` (or `procedure_info` +
  `procedure_callees` for speed).
* `hot-paths.json` — functions reachable from the frame loop (`40cad0`) ranked by static
  call depth, later replaced by runtime profiles from the guest.
* `fpu-sites.json` — every FPATAN/FSINCOS/FPTAN/F2XM1/FYL2X/RCPSS/RCPPS/RSQRTSS/RSQRTPS
  site in `.text`: `run_inventory.py fpu` byte-scans candidates, `tools/analysis/fpu_sites.py` confirms each
  by a capstone linear sweep of its procedure (`confirmed`); the shim list for the guest (src/guest/fpu_shims).
  capstone 5.0.7 is installed in the reference venv for this (`reference/python/bin/python -m pip install
  capstone`; Unicorn stays pinned at 2.1.4).

These are inputs to M1 and M5 of the roadmap.

## 3. Porting a function natively (the exact-port method)

### Provenance during ownership migration

`make check` runs `check_provenance.py` after generating the symbol database.
For migrated state, annotate one owning value declaration directly:

```cpp
// GLOBAL: SONIC 0x3b11188 FrameMultiplier size=4
std::int32_t frame_multiplier{};
```

The annotation uses a database symbol (canonical or retained alternative);
the C++ identifier is semantic. `size=` is the source byte extent, not native
`sizeof`; it must match a known symbol size when one exists. Unknown source ABI
sizes require an explicit extent justified by recovery evidence. One declaration
per annotation; use type aliases for complex types. Annotations on borrowed
references/pointers or ambiguous declarations fail. Native pointer-sized values
such as callback identities can be stored through a named type alias while their
source extent remains the original four bytes.

`FUNCTION`/`LIBRARY`/`STUB` identify one source entry, using its database name;
put the annotation at a single declaration/definition, not both. `DATA` describes
imported content and does not claim mutable storage. `PATCH` uses module-relative
provenance, for example `// PATCH: SPEEDRUN +0x10f0 -> 0x7899e0`, immediately
before the implementing function. Existing profile-patch checks remain separate.

`reference/symbols/legacy-identifiers.json` freezes existing per-file legacy
identifier spellings and retains a separate ledger of the original owned ranges.
Do not add identifiers to admit new address-named code. Remove retired spelling
allowances together with a whole-owner annotated migration; stale allowances
fail. Preserve the range ledger: it rejects cosmetic renames that discard source
ownership, even if the spelling allowance was removed. A migrated `GLOBAL` must
cover the retained source extent.

Legacy extent rules remain deliberately unchanged (including array-first-byte
and read-only constant allowances), so both ownership checkers continue running.
The retained ledger is **not** full coverage of legacy arrays: source records can
be strided, and native array `sizeof` is not evidence of their source span. Before
an aggregate owner migrates, establish its source layout and annotate its full
extent; do not interpret this checker as proof about unexamined array interiors.
Legacy identifiers carry no source-module tag: an annotated
non-SONIC claim at a legacy VA is conservatively treated as a potential overlap
until that owner migrates. The ledger's legacy `SONIC` namespace mirrors the old
checker's flat address space; it does not establish that DLL globals belong to
sonic.exe. Settle a DLL owner's module from evidence when migrating it. This is
not a full C++ parser or proof of behavior.

### Recovery and verification

The first native composition check is `make native-frame-control`. Its isolated
build reuses `game_frame_control` and `speedrun` directly and replays the tracked
numeric frame-control corpus through the existing probe. To regenerate that
frozen reference intentionally, run
`$(REFERENCE_PYTHON) tools/oracles/freeze_frame_control.py "$(GAME_EXE)"` (expand
the Make variables when running directly). This development-only generator needs
the pinned EXE and Unicorn; the independent checker does not. A corpus change
requires source evidence and review, not accommodation of a native mismatch.

Used for verified native behavior and reference adapters. For system-level source
recovery and the executable-independent destination, also follow `docs/GOAL.md` §4.4–4.5
and ADR-0005: gather functions and data dependencies into meaningful native interfaces,
then compare the same implementation through a reference adapter. Existing address-owned
state remains checked until replacement ownership/provenance validation is in place.

This recipe is for **new** recovery. It was rewritten on 2026-10-15 (ADR-0007): the old step 3
asked for `<meaning>_at_<hex>` members and instruction-order transliteration, and produced the
unreadable code the project now refactors.

1. **Context bundle** (§0): names, signatures, structs and enums for every address involved;
   callees with names and native status; callers; named pseudocode (cluster via
   `batch_decompile`, dossier for the functions you port); xrefs for every global; profile
   status; the nearest recovered module in game-tier form as a style model. Cross-check every
   constant's value with `pe_image`.
2. Check the range against `tools/gate/profile_patches.json`: `python3 tools/gate/check_profile_patches.py`
   must report 0 UNHANDLED. If the loader or a mod replaces part of it, implement what
   runs under the profile and mark the patch `handled_by`.
3. **Write the game-tier code** (`docs/STYLE.md`) in the system's module (`docs/ARCHITECTURE.md`):
   named state struct with `GLOBAL` annotations and recovered initial values, named functions
   with `FUNCTION` annotations, structured control flow over named enums, literal constants,
   arithmetic at the most readable level the oracle accepts (STYLE §7), and service seams with
   `STUB` annotations for unrecovered callees. Observable side-effect order and exact
   arithmetic are kept. It is fine to write an assembly-shaped draft first to get the oracle
   green, but only within the step: refactor it under the same oracle before committing.
4. **Write the reference adapter** (reference tier): marshal guest memory to and from the state
   struct, fill the seams from the guest, report missing fixture facts with `source_boundary`
   and `required_fact`, and register the override. All addresses live here.
5. Unit test in `tests/unit/<slug>.cpp` (+ a manifest line). It exercises the game-tier API
   with no guest.
6. Oracle: `tests/probes/native_<slug>_probe.cpp` (flat `uint32_t` in/out layout, calling the
   game-tier code through the adapter) and
   `tools/oracles/verify_original_<slug>.py` (Unicorn runs the original bytes with fixtures for
   callees/Win32/COM; compares every store and the call trace). Write `passed` (strict) and
   `values_match` into the report and `limits` for every known flag difference. Run
   `make oracle-<slug-with-dashes>`; `python3 tools/oracles/verdict.py build/oracles/<slug>.json`
   must say `pass`. Track it: `python3 tools/gate/track_reports.py <slug>` (summary copy in
   `reference/oracle-reports/`; side files stay in `build/`).
7. Mutants: `python3 tools/gate/mutate.py src/<layer>/<file>.cpp oracle-<slug> build/native-<slug>-probe.dylib mutants.json`
   with 3–7 seeded bugs; every non-equivalent mutant must be caught.
8. Wire it into the native session (`src/runtime` today) and into the override table through
   the adapter.
9. Readability review (STYLE §12, by a fresh subagent for a recovered system), then gate,
   record, status, handoff (AGENTS.md).

### Refactoring an existing port under its oracle (track R)

The existing ports are verified, so they can be made readable without new reverse
engineering:

1. Run the module's oracle on the unchanged tree and record the verdict (pass, partial or
   fail, with counts). That is the bar.
2. Split the module into game-tier code plus a reference adapter (STYLE §1, §5). The probe's
   flat C ABI stays byte-for-byte the same, and the probe now calls through the adapter.
3. Refactor in small commits-in-waiting: names and enums, then structured control flow, then
   constants as literals, then arithmetic level (STYLE §7), then state without fixture
   optionals. Rerun the oracle after each stage; the verdict and counts must not get worse.
   "Partial" stays partial, and the flag differences are not yours to fix here.
4. Rerun the module's recorded mutants. A mutant spec that no longer applies textually is
   re-expressed against the new code with the same semantic bug. That is a change to the judge,
   so it goes in its own `fix(judge)` commit listing old and new specs. Each re-expressed
   mutant must be shown caught before the refactor counts as verified. Never drop a mutant.
5. Update `reference/symbols/legacy-identifiers.json` (spellings retired with the whole-owner
   migration) and the readability baseline once the checker exists.

### Readable exact arithmetic: the control-word premise

STYLE §7 allows plain `float` expressions where the game thread runs x87 at 24-bit precision
and round-to-nearest (control word `0x007f`, observed on the Windows reference main thread;
`HANDOFF.md` "Floating-point observations"). With that control word, `+ - * / sqrt` round
like IEEE single unless an operand is a `double` (an `fld qword` constant), a result leaves
the single exponent range while still in a register, or the value is converted to an integer.
The premise is checked, not assumed: the census in GOAL §6 records `FPCW` at the entry of
every override in the guest across the boot/title/attract run and the replays. A module may
use plain floats only for call sites where the census saw `0x007f`, only for expressions
whose assembly has single-precision operands only (no `fld qword`, `fistp` or
transcendental), and only when its oracle, run under that control word, keeps its verdict.
The oracle alone is not enough: its fixtures may never reach the divergent paths.

## 4. Runtime work (M1+)

* The guest (`src/guest`) runs original code; HLE (`src/hle`) implements imports.
  Every HLE function records what it was asked (arguments, results) so a trace can be
  diffed between runs and against the reference.
* An override is a native function bound to a guest address; the guest calls it
  instead of the original. `override-equivalence` runs the original in the guest on the
  recorded inputs and compares.
* Deterministic by construction: no host time, randomness or thread scheduling leaks
  into the guest except through HLE functions that log their answers (QueryPerformanceCounter
  returns the virtual 10 MHz counter from `mac_clock`).

## 5. Testing levels (what proves what)

| Level | Proves | Tool |
|---|---|---|
| Unit | the native code does what its comments say | `tests/unit` |
| Oracle | the native code equals the original instructions on random cases | Unicorn oracle + mutants |
| Override equivalence | the override equals the original inside the live game on real inputs | guest (M1) |
| Replay | the game progresses as expected under scripted input | input scripts + memory assertions (M3) |
| Golden | the picture is right | offscreen Metal renders vs PNGs (M2/M4) |
| Trace parity | the native build equals the Windows reference frame by frame | collector traces (M4) |

A test must fail for the right reason: when adding one, break the code once and watch it
fail (that is what the mutant runs are for).

M5 also tests a clean native source build and execution with original binaries and the
reference runtime unavailable (`docs/ROADMAP.md` M5). Oracle equivalence establishes
behavior on the compared inputs; it does not establish executable independence or
maintainable interfaces. REA pseudocode and bounded p-code are analysis evidence, not
complete compiler input. Preserve unresolved call targets and truncation explicitly.
