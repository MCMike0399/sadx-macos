#!/bin/sh
# Run from the repository root. This recipe writes only worktree-local build outputs.
set -eu
mkdir -p build/gamma-effects
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h build/sadx-native native-closure
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -Isrc -fPIC -dynamiclib \
 tests/probes/native_gamma_effects_probe.cpp src/player/gamma_effects.cpp src/engine/particle_pool.cpp \
 src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp \
 build/softfloat/softfloat.a -o build/native-gamma-effects-probe.dylib
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -Isrc \
 tests/unit/gamma_effects.cpp build/libsadx-native.a -o build/gamma-effects-test
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -Isrc -Itests/fixtures -fobjc-arc \
 tests/unit/native_gamma_effects.mm build/libsadx-native.a -framework Foundation -o build/native-gamma-effects-test
# Supply a local imported asset directory containing the new declaration pack.
assets=${1:-game/imported}
build/gamma-effects-test "$assets/gamma-effects.bin"
build/native-gamma-effects-test "$assets"
