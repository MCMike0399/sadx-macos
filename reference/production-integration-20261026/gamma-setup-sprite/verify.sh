#!/bin/sh
set -eu
# Run at the packet worktree root. No app, provider, full gate or shared recipe.
PYTHON=${SPRITE_PYTHON:-/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python}
EXE=${SPRITE_EXE:-/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe}
mkdir -p build/oracles
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Wmissing-field-initializers -Isrc \
  tests/unit/sprite_registry.cpp src/engine/sprite_registry.cpp src/engine/task_work.cpp \
  -o build/sprite-registry-test
./build/sprite-registry-test
c++ -std=c++17 -O2 -Wall -Wextra -Werror -Wmissing-field-initializers -shared -fPIC -Isrc \
  tests/probes/native_sprite_registry_probe.cpp src/engine/sprite_registry.cpp src/engine/task_work.cpp \
  -o build/native-sprite-registry-probe.dylib
"$PYTHON" tools/oracles/build_sprite_registry_probe.py
"$PYTHON" tools/oracles/verify_original_sprite_registry.py --exe "$EXE" \
  --prefix-probe "$PWD/build/native-sprite-registry-prefix-probe.dylib"
"$PYTHON" tools/oracles/mutate_sprite_registry.py --exe "$EXE"
# Holdout is run once after the implementation and verification code are frozen.
"$PYTHON" tools/oracles/verify_original_sprite_registry.py --exe "$EXE" --holdout \
  --seed 982148 --count 2048 --output build/oracles/sprite_registry_holdout.json \
  --prefix-probe "$PWD/build/native-sprite-registry-prefix-probe.dylib"
"$PYTHON" tools/oracles/verdict.py build/oracles/sprite_registry.json build/oracles/sprite_registry_holdout.json
python3 tools/gate/check_readability.py
python3 tools/gate/check_global_owners.py
python3 tools/gate/check_layers.py
python3 tools/gate/check_profile_patches.py --json > build/gamma-profile.json
