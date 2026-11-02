#!/bin/sh
# Run only with a coordinated build slot. Public SDL/libusb dependencies must be
# copied locally with symlinks dereferenced, as in the worker qualification.
set -eu
mkdir -p build/particle-qualification
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h \
 build/sadx-native native-closure build/native-gamma-effects-test \
 build/native-gamma-effects-probe.dylib
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -Isrc -dynamiclib \
 tests/probes/native_particle_lifecycle_probe.cpp src/engine/particle_pool_lifecycle_native.cpp \
 src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp \
 build/softfloat/softfloat.a -o build/native-particle-lifecycle-probe.dylib
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -Isrc \
 tests/unit/particle_lifecycle.cpp build/libsadx-native.a -o build/particle-lifecycle-test
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -Isrc \
 tests/unit/gamma_effects.cpp build/libsadx-native.a -o build/gamma-effects-test
build/particle-lifecycle-test
build/gamma-effects-test game/imported/gamma-effects.bin
build/native-gamma-effects-test game/imported
# Exit 2 is retained and reported as FP partial, never called a passing oracle.
partial() {
 set +e
 "$@"
 result=$?
 set -e
 test "$result" -eq 2
}
partial reference/python/bin/python tools/oracles/verify_original_particle_lifecycle.py \
 --exe 'game/Sonic Adventure DX/sonic.exe' --output build/particle-qualification/development.json
partial reference/python/bin/python tools/oracles/verify_original_particle_lifecycle.py \
 --exe 'game/Sonic Adventure DX/sonic.exe' --holdout --seed 773129 --count 2048 \
 --output build/particle-qualification/holdout.json
partial reference/python/bin/python tools/oracles/verify_original_gamma_effects.py \
 --exe 'game/Sonic Adventure DX/sonic.exe' --probe build/native-gamma-effects-probe.dylib \
 --output build/particle-qualification/initializer.json
partial reference/python/bin/python tools/oracles/verify_original_gamma_effects.py \
 --exe 'game/Sonic Adventure DX/sonic.exe' --probe build/native-gamma-effects-probe.dylib \
 --holdout --seed 981764 --count 2048 --output build/particle-qualification/initializer-holdout.json
if test "${1:-}" = --mutants; then
 reference/python/bin/python tools/oracles/mutate_particle_lifecycle.py
 # The inherited driver writes its old receipt. Restore that exact file even
 # on a failed run; keep the new receipt only in this packet.
 reference/python/bin/python - <<'PY'
from pathlib import Path
import subprocess
p=Path('reference/packets/gamma-effects-native/mutants.json')
original=p.read_bytes()
try:
 result=subprocess.run(['reference/python/bin/python','tools/oracles/mutate_gamma_effects.py'])
 if result.returncode:raise SystemExit(result.returncode)
 Path('reference/packets/particle-lifecycle-native/initializer-mutants-final.json').write_bytes(p.read_bytes())
finally:p.write_bytes(original)
PY
fi
