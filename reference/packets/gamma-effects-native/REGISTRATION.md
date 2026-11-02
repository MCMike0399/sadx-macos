# Root registration rows

The source membership additions are already in native-sources.txt. The native
allocation/callback binding is included by the existing *_native.cpp wildcard.
Append symbol-proposals.tsv (without its header) to overrides.tsv after review,
then rebuild symbols. Private proposal-database provenance passes; stock
provenance intentionally reports the ten unregistered symbols. No global
inventory, judge manifest, Makefile or shared test manifest was edited here.

Import the required new content before launching the app:

```sh
python3 tools/import/gamma_effects_resource.py "${GAME_EXE}" \
  game/imported/gamma-effects.bin --proof game/imported/gamma-effects.provenance.json
```

Missing content fails in the app asset reader. The separate old setup-only unit
continues to test a deliberately absent effects owner; its one diagnostic string
is adapted separately, preserving every original input and assertion.

Append to tests/unit/manifest.tsv (literal tabs):

```tsv
gamma-effects	game/imported/gamma-effects.bin
native-gamma-effects	game/imported
```

Append to tools/oracles/manifest.tsv:

```tsv
gamma_effects	build/native-gamma-effects-probe.dylib	tools/oracles/verify_original_gamma_effects.py --exe "$(GAME_EXE)" --probe build/native-gamma-effects-probe.dylib --output build/oracles/gamma_effects.json
```

Add these explicit Make targets (normal tabbed recipes), using the commands in
verify.sh. The probe deliberately supplies allocation/creation/callback seams;
it must not link particle_pool_native.cpp directly. Source and header prerequisites:

```make
build/native-gamma-effects-probe.dylib: tests/probes/native_gamma_effects_probe.cpp src/player/gamma_effects.cpp src/player/gamma_effects.hpp src/engine/particle_pool.cpp src/engine/particle_pool.hpp src/engine/task.cpp src/engine/task.hpp src/engine/task_work.cpp src/engine/task_work.hpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_gamma_effects_probe.cpp src/player/gamma_effects.cpp src/engine/particle_pool.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a -o $@
build/gamma-effects-test: tests/unit/gamma_effects.cpp src/player/gamma_effects.hpp src/engine/particle_pool.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-native.a -o $@
build/native-gamma-effects-test: tests/unit/native_gamma_effects.mm src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp src/player/gamma_effects.hpp src/engine/particle_pool.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@
```

New protected verification inputs are the two new tests, new probe, new oracle,
mutant driver and importer. Refresh the canonical judge manifest only in Root's
integration judge step, preserving all existing inputs. The worker supplies exact
hashes in proposed-judge-inputs.json. No inherited tolerance, input, expected value,
exclusion or frozen corpus is changed. Full gate and independent Astra review are
Root-owned; this packet does neither.
