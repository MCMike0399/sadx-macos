#!/bin/sh
set -eu
# Run from this packet's worktree root. Sequential compilation; no shared builds.
GAMMA_PYTHON=${GAMMA_PYTHON:-/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python}
GAMMA_EXE=${GAMMA_EXE:-/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe}
mkdir -p build/gamma-setup
python3 tools/import/gamma_setup_resource.py "$GAMMA_EXE" build/gamma-setup/gamma-setup.bin \
  --proof build/gamma-setup/import.json
python3 -m unittest discover -s tests/python -p test_gamma_setup_resource.py
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Wmissing-field-initializers -Isrc \
  tests/unit/gamma_setup_resource.cpp src/player/gamma_setup_resource.cpp \
  src/engine/sprite_registry.cpp src/engine/task_work.cpp -o build/gamma-setup-resource-test
./build/gamma-setup-resource-test
./build/gamma-setup-resource-test build/gamma-setup/gamma-setup.bin
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Wmissing-field-initializers -Isrc \
  tests/unit/gamma_setup_allocation.cpp src/player/gamma_setup_resource.cpp \
  src/engine/sprite_registry.cpp src/engine/gamma_setup_allocation_native.cpp \
  src/engine/task_work.cpp -o build/gamma-setup-allocation-test
./build/gamma-setup-allocation-test
sh third_party/softfloat/build.sh build/gamma-setup/softfloat
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Wmissing-field-initializers \
  -ffp-contract=off -fno-fast-math -Isrc tests/unit/gamma_setup_caller.cpp \
  src/player/gamma_setup_resource.cpp src/engine/sprite_registry.cpp \
  src/engine/gamma_setup_allocation_native.cpp src/player/gamma_selection_core.cpp \
  src/player/player_startup.cpp src/player/player_state.cpp src/engine/task.cpp \
  src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp \
  build/gamma-setup/softfloat/softfloat.a -o build/gamma-setup-caller-test
./build/gamma-setup-caller-test build/gamma-setup/gamma-setup.bin
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Wmissing-field-initializers -shared -fPIC -Isrc \
  tests/probes/native_gamma_setup_resource_probe.cpp src/player/gamma_setup_resource.cpp \
  src/engine/sprite_registry.cpp src/engine/task_work.cpp -o build/native-gamma-setup-resource-probe.dylib
"$GAMMA_PYTHON" tools/oracles/verify_original_gamma_setup_resource.py --exe "$GAMMA_EXE" \
  --output build/gamma-setup/development-final.json
"$GAMMA_PYTHON" tools/oracles/mutate_gamma_setup_resource.py --exe "$GAMMA_EXE"
python3 tools/gate/check_readability.py
python3 tools/gate/check_global_owners.py
python3 tools/gate/check_layers.py
