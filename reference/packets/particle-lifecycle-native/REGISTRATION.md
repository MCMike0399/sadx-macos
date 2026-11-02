# Finite Root registration

No shared source, unit, oracle, symbol or judge inventory is edited by this
packet. Preserve every existing protected path, including the 53 ignored
artifacts that are absent in this worktree. Do not refresh by omission.

New source particle_pool_lifecycle_native.cpp is selected by the existing
*_native.cpp wildcard. No NATIVE_SOURCES membership edit is needed.

Append the two rows in symbol-proposals.tsv to reference/symbols/overrides.tsv,
then rebuild symbols. A private copy of the proposed database passes provenance;
no canonical symbol inventory is changed here.

Append this literal-tab unit registration:

```tsv
particle-lifecycle
```

Add explicit compiler recipes (and normal source/header prerequisites):

```make
build/particle-lifecycle-test: tests/unit/particle_lifecycle.cpp src/engine/particle_pool.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-native.a -o $@
build/native-particle-lifecycle-probe.dylib: tests/probes/native_particle_lifecycle_probe.cpp src/engine/particle_pool.hpp src/engine/particle_pool_lifecycle_native.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_particle_lifecycle_probe.cpp src/engine/particle_pool_lifecycle_native.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a -o $@
```

Append the nondefault-input oracle registration:

```tsv
particle_lifecycle	build/native-particle-lifecycle-probe.dylib	tools/oracles/verify_original_particle_lifecycle.py --exe "$(GAME_EXE)" --probe build/native-particle-lifecycle-probe.dylib --output build/oracles/particle_lifecycle.json
```

The probe deliberately defines DisplayParticle and FreeMemory seams. Do not
link particle_pool_native.cpp or particle_pool.cpp into that probe. The real
scheduler/arena test links the native archive, including those production seams.
The old initializer probe retains its own four callback stops; do not add the
new lifecycle object to its explicitly bounded initializer comparison.

New protected bodies: tests/unit/particle_lifecycle.cpp,
tests/probes/native_particle_lifecycle_probe.cpp,
tools/oracles/verify_original_particle_lifecycle.py and
tools/oracles/mutate_particle_lifecycle.py. proposed-judge-inputs.json supplies
exact final hashes. One old unit's obsolete stop expectations are adapted in a
separate evidenced fix(judge) commit after the implementation commit; no old
oracle, tolerance, mutant, fixture or expected numeric observation is changed.
Root owns canonical judge reconciliation, inventory review, independent review,
shared bindings and the full gate. None ran in this packet.
