# Root admission recipe

Admit only the recovered prefix. First-call initialization is real; active
processing and actor drawing remain open. Main's current game source matches
acdb5a04; its two corrected TSV rows and e1946df8 qualification stay intact.

1. Apply ROOT-SOURCE.patch with git apply --3way --unidiff-zero to the current Main tree. It contains the complete
   final product diff, so it avoids replaying the intermediate held-slot drafts.
   It adds the core, read-only geometry metadata accessor and semantic borrow of
   the already-owned join-mode word, plus only the Gamma join_vertices binding.
   Session, PlayerSystem, Display and their global owners are untouched.
2. Apply ROOT-JUDGE.patch with the same flags, retaining Main's already-corrected TSV rows. Every
   argv for each new unit is together in its second TSV field. The new caller
   creates its observation directory, so a fresh gate does not depend on this
   worker's build directory. The only old witness change is its stop assertion.
3. Add symbol-proposals.tsv through the canonical symbol process and reconcile
   the new module/touched fingerprints in the Root-owned inventories. The helper
   is semantic FindPlayerObjectNode, source 439360, size 82; GetNodeIndex is already
   present from the SDK. PJoinVertexes remains a partial source seam, not a fully
   recovered function. Do not admit the temporary proposed symbol database.
4. Register the finite judge delta from REGISTRATION-PROPOSAL.json, preserving all
   current protections including the 53 local binary inputs absent here. Do not
   copy this worker's canonical manifest or remove an absent identity. Final
   Makefile and test-manifest hashes must describe the merged Main files.
5. Run bounded builds and the focused commands below, then independent STYLE
   review, the Root full gate and the Root cold qualification when appropriate.

```
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h \
  build/sadx-native native-closure build/player-vertex-weld-test \
  build/native-gamma-vertex-weld-test build/native-gamma-motion-test
build/player-vertex-weld-test game/imported/characters/sonic-selection.actions
build/native-gamma-vertex-weld-test game/imported build/gamma-weld/imported-topology.tsv
build/native-gamma-motion-test game/imported build/gamma-weld/motion.tsv
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h oracle-player-vertex-weld
reference/python/bin/python tools/oracles/verify_original_player_vertex_weld.py \
  'game/Sonic Adventure DX/sonic.exe' build/native-player-vertex-weld-probe.dylib \
  --imported-topology build/gamma-weld/imported-topology.tsv \
  --output build/gamma-weld/imported-original.json
```

The final holdout is retained at build/gamma-weld/final-holdout.json; its command
uses --cases 2048 --seed 0x57454c44, plus the preserved appended cohorts, for 2,224
cases. Both mutation tools under tools/oracles are reproducible, bounded and
compile sequentially. VERIFICATION.json records actual commands, outputs, hashes,
failed attempts and limits. The actual same-save normal input command and state
are in its normal_input member; its screenshot remains outgoing title, with the
next genuine stop at laser targets. There is no actor-frame or N3b admission here.

For a deliberate fresh static recovery, the packet's ghidra-config.json uses its
own checked-in five-entry seed and a fresh replay snapshot path. Set
SADX_REA_MODULES to the existing read-only REA install when needed. Its existing
environment file caps the heap at 2 GiB. Do not import while Root holds the heavy
slot. The actual original run's profile and source digests are in the retained
raw dossiers; this portable replay config does not bypass a cache mismatch.
