# Root integration recipe

Consume source commit `18f3e844` and the separate judge/evidence commit on this
branch. No shared header replacement is needed. The source already adds two
native-source members, validates existing actor metadata and binds the model
owner to Session's character_scene_ during its existing Sonic asset load.

Apply ROOT-WIRING.patch to the current `exec_selection_sonic` only:

```cpp
context.animation_table = &assets->table();
context.model = assets->model();
animation::Runtime animation{scheduler.work_arena().get(), assets->resources(),
                             &scene.session_.character_scene_.objects};
context.animation = &animation;
sonic::run(context);
```

Preserve all newer shared fields and Gamma/particle bindings. No current-player
refresh follows PInitialize. The model tables and caches are session-lifetime
members of the existing Sonic owner; scene objects/geometry/materials stay in
character_scene_. No Source SceneGeometryObject mirror is populated.

The checked `sonic-model-native` unit compiles the actual current callback TU
with precisely this hunk into a build-only object. It does not substitute a
fake callback or edit the shared source file. After applying the hunk on Main,
prepare_wiring.py accepts the already-applied form and emits it unchanged.

Reproduce, with the user's locally copied/dereferenced SDL/libusb:

```sh
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h build/sonic-model-native-test native-closure
./build/sonic-model-native-test game/imported
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h oracle-sonic-model
reference/python/bin/python reference/packets/sonic-model-native/verify_source.py build/native-sonic-model-probe.dylib --holdout --output build/sonic-model/holdout.json
reference/python/bin/python reference/packets/sonic-model-native/verify_faults.py build/sonic-model/faults
python3 reference/packets/sonic-model-native/verify_binding_fault.py
```

Both source cohorts return 2, honestly partial. All unit argv are in TSV field 2;
focused-tests.json retains the generated commands actually executed. The old
numeric reference builder needs the already-existing character_pack.cpp object
provider added to its source list in memory; its unchanged original judges and
full captures were used. ATTEMPTS.md records this setup correction.

The packet registers one new unit and one new source oracle. Reconcile Main's
current protected manifest by addition and the explicit Make/TSV registration
changes; preserve every inherited key, including all 53 absent local binary
inputs. judge-registration.json records the exact retained set and local limits.
Do not use a blanket inventory refresh to claim conformance. Root should add the
five new source files to its maintained inventory with their honest NOTSA/prefix
scope, then reconcile the changed Sonic owner/context header dependencies in
existing open save-warning evidence. The packet leaves these Root-owned
inventories untouched; it introduces no provenance/counterpart exception.

The next real first-call boundary is source LoadCharTextures(0); nonzero Metal
also requires the later LoadCharTextures(8). The subsequent stores, setup helper,
PSetMotion, stage-specific join mutation, final PJoinVertexes and common tail
are not recovered by this prefix. No helper is skipped and no model implies a
completed actor frame. Root alone decides admission and the later coherent gate.
