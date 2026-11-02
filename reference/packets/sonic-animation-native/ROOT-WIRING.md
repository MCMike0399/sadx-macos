# Root integration contract (apply to current Main)

The baseline is `08820f79`; Gamma effects has since changed shared files on Main.
The following is a contract, not a patch over those newer fields. No legacy
header, accepted Sonic algorithm/binding, Gamma loader, parameter or geometry
owner needs replacement.

1. Add `src/runtime/sonic_animation_assets.cpp` to `native-sources.txt`.
   The portable core wildcard already discovers it. No new reference dependency.
2. Include `runtime/sonic_animation_assets.hpp` in `native_player_system.hpp` and
   add one field: `std::unique_ptr<SonicAnimationAssets> sonic_animation_assets;`.
   Keep that pointee for the session lifetime; never replace it while a Runtime
   borrows `resources()`. Player table borrowers retain their own shared ownership.
3. Add Session load API accepting `std::unique_ptr<SonicAnimationAssets>`;
   reject null/double installation, then move it into the PlayerSystem field.
   It needs no new animation runtime, geometry, parameter or event field.
4. App imports `apps/native_sonic_animation_assets.hpp`, loads from the same
   directory and exact `*characters` pack used for all other actors, and passes
   the returned unique_ptr to Session. Loading the optional metadata when present
   follows the existing Gamma pattern; absence must leave the real missing-table
   boundary, never select Gamma's table or synthesize an empty export.
5. In `TitleScene::exec_selection_sonic`, retain all current event/parameter/editor
   bindings. When the assets exist, use this callback-local borrowing scope:

   ```cpp
   const auto &assets = scene.session_.players_.sonic_animation_assets;
   if (!assets) {
       sonic::run(context); // Same ordered missing-input boundary as before.
       return;
   }
   context.animation_table = &assets->table();
   animation::Runtime animation{scheduler.work_arena().get(), assets->resources()};
   context.animation = &animation;
   sonic::run(context);
   ```

   These pointers survive the entire invocation. `table()` returns a reference
   to the actual shared_ptr member; do not take a pointer to a converted temporary.
   Do not default editor/forced/alternate state to get past unrelated stops.
   `resources().objects` is deliberately absent; no shadow geometry map is added.
6. Add a focused Make rule and unit registration (shared files remain Root-owned;
   replace `<TAB>` with a literal Make recipe tab):

   ```make
   build/sonic-animation-assets-test: tests/unit/sonic_animation_assets.mm src/apps/native_sonic_animation_assets.hpp build/libsadx-native.a
   <TAB>$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@
   ```

   Append the manifest row with tabs:
   `sonic-animation-assets<TAB>game/imported<TAB>build/sonic-animation-table.tsv`.
   New protected paths are the unit and the three new packet Python verifiers.
   Preserve every existing manifest hash, including ignored binary evidence.
   No source symbol override, global owner, inventory rewrite, old probe or
   existing expected value needs alteration for this packet.

Generate metadata only after the existing Sonic/Gamma importer has produced the
combined pack, with pinned source binaries outside the imported directory:

```sh
reference/python/bin/python tools/import/sonic_animation_resources.py \
  "game/Sonic Adventure DX/system/CHRMODELS_orig.dll" \
  "game/Sonic Adventure DX/sonic.exe" game/imported/characters/sonic-selection.actions
```

It must report 3,328 records, zero append, four loaded/three null/142 unavailable
for this baseline pack. A newer combined pack mismatch requires explicit importer
reconciliation; do not overwrite the newer pack or omit Gamma effects contracts.

Isolated reproduction (no shared-file edits required):

```sh
make -j2 -o build/native-controller-deps/include/SDL3/SDL.h build/libsadx-native.a
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math \
  -Isrc -c src/runtime/sonic_animation_assets.cpp -o build/sonic-animation-native/assets.o
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math \
  -fobjc-arc -Isrc tests/unit/sonic_animation_assets.mm build/sonic-animation-native/assets.o \
  build/libsadx-native.a -framework Foundation -o build/sonic-animation-native/assets-test
build/sonic-animation-native/assets-test game/imported build/sonic-animation-native/table.tsv
reference/python/bin/python reference/packets/sonic-animation-native/verify_imported.py \
  "game/Sonic Adventure DX/sonic.exe" "game/Sonic Adventure DX/system/CHRMODELS_orig.dll" \
  game/imported/characters build/sonic-animation-native/table.tsv build/sonic-animation-native/imported.json
python3 reference/packets/sonic-animation-native/verify_faults.py game/imported build/sonic-animation-native/faults.json
```

No app/editor/model/event stepping or full-game claim is implied by these commands.
Root owns review, shared registration and the coherent gate after integration.

Suggested Root HANDOFF lines after independent admission:

- Sonic now has its own imported animation owner using the unchanged canonical
  147-row initializer. Export declarations retain four loaded, three null and
  142 unavailable entries; no payload or existing pack identity was added.
- Two real Sonic initializations borrow the same table and Stand advances by
  exactly half a frame. Original table bytes and all 103 existing cases match;
  882 ordered publications, invalid-input tests and six compiled faults pass.
- Production editor/model/PSetMotion and other actors remain separate stops;
  no completed actor frame or N3b claim. Shared bindings and gate are Root-owned.
