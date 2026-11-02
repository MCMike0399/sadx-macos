# Root admission

Consume the four source commits and the separate judge commit. Apply ROOT-WIRING.patch
to the current `exec_selection_sonic`, preserving newer owners and registration.
It borrows existing CharacterResources, RegularResources, stage/act and SceneState;
it adds no shared field. Do not copy a Session/PlayerSystem/Display header.

Apply ROOT-CALLER-TESTS.patch at the same time. It supplies real existing regular
resources to the two old shipping-callback witnesses and advances their expected
first stop from absent texture composition to slot 20. Their other assertions and
inputs remain. Both exact proposed variants passed against the prepared callback.
Before Root wiring, the candidate's old standalone callers intentionally retain
the missing-owner boundary; its new and adapted model witnesses use the build-only
hunk. prepare_wiring.py also accepts the already-applied exact hunk.

Reproduce with copied/dereferenced SDL3/libusb and read-only local game inputs:

```sh
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h build/sadx-native native-closure build/sonic-initialization-native-test build/sonic-model-native-test
python3 tools/gate/gen_make.py
# Execute the selected commands verbatim from build/tests.mk; argv are TSV field 2.
./build/sonic-initialization-native-test game/imported
./build/sonic-model-native-test game/imported
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h oracle-sonic-initialization
reference/python/bin/python reference/packets/sonic-initialization-native/verify_source.py build/native-sonic-initialization-probe.dylib --holdout --output build/sonic-initialization-holdout.json
reference/python/bin/python reference/packets/sonic-initialization-native/verify_helpers.py build/native-sonic-initialization-probe.dylib --holdout --output build/sonic-initialization-helpers-holdout.json
reference/python/bin/python reference/packets/sonic-initialization-native/verify_faults.py
reference/python/bin/python reference/packets/sonic-initialization-native/verify_binding_faults.py
```

The retained model judge and all inherited faults remain in sonic-model-native.
Only its pair-content fault's textual insertion anchor was re-expressed after the
model binding loop became shared; its semantic bug and before/after expectation
are unchanged. Preserve all 1,441 current Main manifest paths, union the nine new protections,
and retain the 53 locally absent ignored inputs. Thirteen matrix-judge paths from
Main are absent only because this packet remains on its supplied older base. Do not replace a newer Root
manifest with this candidate snapshot. No gate is claimed here.

Next normal source requirement is the actual live SONIC_OBJECTS slot 20 mapping
(and subsequent 19/15/14/62/3 or upgraded alternatives) on the existing object owner.
Do not infer these export slots from a similar mesh in the pack. Metal and the
changed-sibling path need the other worker's real matrix producer. Native attachment,
sound, actor display/destruction and effect algorithms remain real stubs/boundaries.

MAIN-COMPATIBILITY.json checks both minimal patches against bdb61450 after the
candidate one-line caller diagnostic edits. Preserve Main's model-matrix owner,
Gamma getter/Z activation and all newer hashes. The matrix core exists on Main;
Sonic active-PJoin argument selection remains unbound in this independent packet.
