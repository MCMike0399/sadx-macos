# Method: analysing, porting and testing

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

Used for verified native behavior and reference adapters. For system-level source
recovery and the executable-independent destination, also follow `docs/GOAL.md` §4.4–4.5
and ADR-0005: gather functions and data dependencies into meaningful native interfaces,
then compare the same implementation through a reference adapter. Existing address-owned
state remains checked until replacement ownership/provenance validation is in place.

1. Read it (section 1). Cross-check every constant with `pe_image`.
2. Check the range against `tools/gate/profile_patches.json`: `python3 tools/gate/check_profile_patches.py`
   must report 0 UNHANDLED. If the loader or a mod replaces part of it, implement what
   runs under the profile and mark the patch `handled_by`.
3. Write the owner in `src/<layer>/` (the lowest layer whose dependencies it needs,
   `docs/ARCHITECTURE.md`): state struct with `<meaning>_at_<hex>` members (one
   owner per global), a `*Runtime` with borrowed dependencies, a `*Result` with failure
   tag + source boundary + required fact. Every store in instruction order; every branch
   with its exact x87/flag condition.
4. Unit test in `tests/unit/<slug>.cpp` (+ a manifest line if it takes arguments).
5. Oracle: `tests/probes/native_<slug>_probe.cpp` (flat `uint32_t` in/out layout) and
   `tools/oracles/verify_original_<slug>.py` (Unicorn runs the original bytes with fixtures for
   callees/Win32/COM; compares every store and the call trace). Write `passed` (strict) and
   `values_match` into the report and `limits` for every known flag difference. Run
   `make oracle-<slug-with-dashes>`; `python3 tools/oracles/verdict.py build/oracles/<slug>.json`
   must say `pass`. Track it: `python3 tools/gate/track_reports.py <slug>` (summary copy in
   `reference/oracle-reports/`; side files stay in `build/`).
6. Mutants: `python3 tools/gate/mutate.py src/<layer>/<file>.cpp oracle-<slug> build/native-<slug>-probe.dylib mutants.json`
   with 3–7 seeded bugs; every non-equivalent mutant must be caught.
7. Wire it into the runtime (`src/runtime`, or the override table once M1 lands).
8. Gate, record, status, handoff (AGENTS.md).

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
