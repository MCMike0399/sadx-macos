# Private reproduction commands

These run from this durable checkout. Use at most two compiler jobs. The owned
assets/EXE and Python environment are ignored read-only inputs, not committed.
Root owns maintained recipe/source/test/symbol registrations. The private Make
file under build/gamma-weld-cursors/private.mk extends existing recipes without
editing Makefile, native-sources.txt, test manifests or judge protections.

Portable exact CPU slice:

```sh
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math \
  -Isrc tests/unit/gamma_cursors.cpp src/player/gamma_cursors.cpp \
  src/engine/task_work.cpp -o build/gamma-weld-cursors/cursor-unit
./build/gamma-weld-cursors/cursor-unit
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math \
  -dynamiclib -Isrc tests/probes/native_gamma_cursors_probe.cpp \
  src/player/gamma_cursors.cpp src/engine/task_work.cpp \
  -o build/gamma-weld-cursors/native-gamma-cursors-probe.dylib
reference/python/bin/python tools/oracles/verify_original_gamma_cursors.py \
  --exe 'game/Sonic Adventure DX/sonic.exe' \
  --probe build/gamma-weld-cursors/native-gamma-cursors-probe.dylib \
  --output build/gamma-weld-cursors/development.json
reference/python/bin/python tools/oracles/mutate_gamma_cursors.py \
  --exe 'game/Sonic Adventure DX/sonic.exe' --output build/gamma-weld-cursors/mutations.json
reference/python/bin/python tools/oracles/verify_original_gamma_cursors.py \
  --exe 'game/Sonic Adventure DX/sonic.exe' \
  --probe build/gamma-weld-cursors/native-gamma-cursors-probe.dylib \
  --output build/gamma-weld-cursors/holdout.json --holdout
```

For the native composition, add src/player/gamma_cursors.cpp to the private
NATIVE_SOURCES/NATIVE_OBJECTS and as an extra libsadx-native.a prerequisite.
Existing runtime/native_* discovery includes both resource/session binder files.
Build the native library, then compile tests/unit/native_gamma_weld_cursors.mm
with -fobjc-arc, -Isrc, libsadx-native.a and -framework Foundation. Pass the actual
imported-assets directory. Product app and force-load closure use the existing
build/sadx-native and native-closure recipes with those private source additions.

The unchanged Gamma unit and probe link against the rebuilt native archive.
The unchanged startup unit and probe additionally link the unchanged reference
adapter src/guest/player_initialization_reference.cpp. Both original scripts
need PYTHONPATH=tools/lib:tools/oracles. Commands:

```sh
PYTHONPATH=tools/lib:tools/oracles reference/python/bin/python \
  tools/oracles/verify_original_gamma_selection_core.py 'game/Sonic Adventure DX/sonic.exe' \
  build/gamma-weld-cursors/gamma-core-probe.dylib \
  --output build/gamma-weld-cursors/gamma-core-development.json
PYTHONPATH=tools/lib:tools/oracles reference/python/bin/python \
  tools/oracles/verify_original_gamma_selection_core.py 'game/Sonic Adventure DX/sonic.exe' \
  build/gamma-weld-cursors/gamma-core-probe.dylib \
  --output build/gamma-weld-cursors/gamma-core-holdout.json --holdout
PYTHONPATH=tools/lib:tools/oracles reference/python/bin/python \
  tools/oracles/verify_original_player_startup.py 'game/Sonic Adventure DX/sonic.exe' \
  build/gamma-weld-cursors/startup-probe.dylib --output build/gamma-weld-cursors/startup.json
```

Each Gamma command exits 2 for inherited FP partials. Startup exits 0 but verdict
remains partial. The exact actual normal-input command array is in proof.json;
use new save/capture paths if repeating it. It exits 1 at the explicit setup seam.
No full gate was run here. Root must run independent review and integrated checks.
