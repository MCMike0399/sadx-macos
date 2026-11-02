# Proposed Root registrations

Append unit manifest rows (literal tabs). Keep the synthetic invocation from
verify.sh as well as the imported-byte invocation used by the registered test.

```tsv
sprite-registry
gamma-setup-resource	game/imported/gamma-setup.bin
gamma-setup-allocation
gamma-setup-caller	game/imported/gamma-setup.bin
```

The importer command for that destination is:

```sh
python3 tools/import/gamma_setup_resource.py "${GAME_EXE}" \
  game/imported/gamma-setup.bin --proof game/imported/gamma-setup.provenance.json
```

Append the custom oracle row below because this new oracle's explicit --exe CLI
differs from gen_make.py's positional default. Do the corresponding registration
for the inherited sprite_registry oracle; its full prefix recipe is in the
constructor packet's verify.sh, including build_sprite_registry_probe.py.

```tsv
gamma_setup_resource	build/native-gamma-setup-resource-probe.dylib	tools/oracles/verify_original_gamma_setup_resource.py --exe "$(GAME_EXE)" --probe build/native-gamma-setup-resource-probe.dylib --output build/oracles/gamma_setup_resource.json
sprite_registry	build/native-sprite-registry-probe.dylib build/native-sprite-registry-prefix-probe.dylib	tools/oracles/verify_original_sprite_registry.py --exe "$(GAME_EXE)" --probe build/native-sprite-registry-probe.dylib --prefix-probe build/native-sprite-registry-prefix-probe.dylib --output build/oracles/sprite_registry.json
```

New native source list additions, only if not already present/discovered:

```text
src/engine/sprite_registry.cpp
src/engine/gamma_setup_allocation_native.cpp
src/player/gamma_setup_resource.cpp
```

Compiler source sets for Root Makefile rules are exactly the commands in verify.sh.
Do not link gamma_setup_allocation_native.cpp into the resource unit or the probe:
those deliberately supply the reentrant/failing lower seam. Link it into the real
allocator unit and caller test. All these rules need the three new headers and
existing task_work headers as prerequisites, or compiler-generated dependencies.
The caller also directly includes native_gamma_selection_core_probe.cpp; retain
that file as a prerequisite. Its other lower functions remain inherited receipts.
Python discovery picks up test_gamma_setup_resource.py without a manifest row.

Judge integration must preserve the separate enum compatibility fix. The inherited
constructor tests/probe are newly added relative to Main ca640c5; their final hashes
differ from the older constructor packet only by the renamed failure enumerator.
All comparisons, cases, exclusions, prefixes and seven mutants are preserved.
Register new protected paths with the usual evidence-backed process; this worker
has not edited the canonical judge manifest, Makefile or shared registration files.
