#!/bin/sh
# Run from the supplied packet worktree; writable dependencies must be local copies.
set -eu
mkdir -p build/gamma-motion
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h \
 build/sadx-native build/sadx-native-closure build/native-gamma-motion-test \
 build/sonic-animation-assets-test build/sonic-animation-integration-test \
 build/native-gamma-effects-test build/animation-gamma-shared-binding-test \
 build/animation-table-publication-test
build/native-gamma-motion-test game/imported build/gamma-motion/observations.tsv
build/sonic-animation-assets-test game/imported build/gamma-motion/sonic-table.tsv
build/sonic-animation-integration-test game/imported
build/native-gamma-effects-test game/imported
build/animation-gamma-shared-binding-test game/imported
build/animation-table-publication-test
# Strict exit 2 is retained as a partial result, never rewritten to a pass.
set +e
reference/python/bin/python reference/packets/gamma-motion-native/verify_imported.py \
 'game/Sonic Adventure DX/sonic.exe' 'game/Sonic Adventure DX/system/CHRMODELS_orig.dll' \
 build/gamma-motion/observations.tsv build/gamma-motion/original-imported.json
result=$?
set -e
[ "$result" -eq 2 ]
reference/python/bin/python reference/packets/sonic-animation-native/verify_imported.py \
 'game/Sonic Adventure DX/sonic.exe' 'game/Sonic Adventure DX/system/CHRMODELS_orig.dll' \
 game/imported/characters build/gamma-motion/sonic-table.tsv build/gamma-motion/sonic-original-table.json
reference/python/bin/python reference/packets/gamma-motion-native/verify_faults.py \
 game/imported build/gamma-motion/faults.json
