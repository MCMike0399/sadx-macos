# Root admission

Apply the source commit `ace1a719` and the separate judge commits `aacb5e98` and `3a6f30d2` surgically,
preserving newer Root work. ROOT-SOURCE.patch and ROOT-JUDGE.patch contain the
same exact diffs. No whole-header replacement is needed.

The required shared hunks are small: Context borrows a normalized-input array;
Input exposes that existing owner; TitleScene's existing input services borrow it;
the app attaches that same Input array. Session/PlayerSystem declarations gain no
new state owner. NativeSceneInputState's existing normalized array is split from
the raw array only to add its unique input_dataG provenance annotation.

Review and seal the finite paths in REGISTRATION-PROPOSAL.json against the current
Main manifest, including its exact merged Makefile/test-manifest hashes. Retain
all inherited entries, including the 53 absent local evidence inputs. Reconcile
only the two changed native_input.cpp consumer fingerprints and affected module
inventory records. No counterpart exception or global-owner guard is broadened.

The old motion/weld callers intentionally have no input binding. Their sole
expectation edit identifies the precise new missing-owner failure. Their complete
prior observation files remain byte-identical. The new caller test binds the
real native Input and proves that the complete callback reaches GetMMMatrix.

```
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h \
  build/sadx-native native-closure build/native-gamma-input-buffer-test \
  build/native-gamma-motion-test build/native-gamma-vertex-weld-test \
  oracle-gamma-input-buffer
build/native-gamma-input-buffer-test game/imported
build/native-gamma-motion-test game/imported build/gamma-laser-targets/motion.tsv
build/native-gamma-vertex-weld-test game/imported build/gamma-laser-targets/topology.tsv
reference/python/bin/python tools/analysis/prove_gamma_input_buffer_initialization.py \
  'game/Sonic Adventure DX/sonic.exe'
reference/python/bin/python tools/oracles/verify_original_gamma_input_buffer.py \
  'game/Sonic Adventure DX/sonic.exe' build/native-gamma-input-buffer-probe.dylib \
  --cases 2048 --seed 0x47494e50 --output build/gamma-laser-targets/holdout.json
reference/python/bin/python tools/oracles/mutate_gamma_input_buffer.py \
  --exe 'game/Sonic Adventure DX/sonic.exe'
```

All arguments for the new native unit are in TSV field 2; its generated command
was executed. VERIFICATION.json retains the normal-input app command, state,
capture hash and exact scope. App output stops at the existing GetMMMatrix seam;
the producer contract in gamma-weld-native/ACTIVE-BOUNDARY.md is still open.
Root must not substitute an identity/zero matrix or empty-target shortcut.
