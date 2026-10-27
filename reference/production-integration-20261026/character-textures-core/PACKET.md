# Ordinary character texture loader

Frozen implementation packet on `packet/character-textures-core`, based on Main `afc3e94`.
This recovers complete ordinary `LoadCharTextures` at SONIC `421280..4212d8`, not the
selection wrapper `512550`. It removes the ordinary texture dependency reached by the
private native Gamma candidate at frame 1382. No runtime binding or actor/N3b completion
is included. Root owns independent review and integration.

## Contract

Header: `src/game/character_textures.hpp`, namespace `sadx::regular_textures`.

```cpp
struct CharacterBinding {
    std::optional<std::string_view> archive;
    List *textures;
};
struct CharacterTable {
    const CharacterBinding *entries;
    std::size_t count;
};
struct CharacterCatalog {
    const CharacterTable *tables;
    std::int8_t *loaded;
    std::size_t count;
};
void load_character(std::size_t character, CharacterCatalog catalog,
                    State &state, Services &services);
void load_egg_robo_textures(State &state, Services &services); // STUB, no body
```

`State` and `Services` are the existing regular texture system types. Each binding borrows
archive text and a canonical `List`; it owns no resource or string. Root can construct
local pointer/view descriptors from `Session::texture_assets_` and
`Session::character_textures_`, then borrow `character_textures_loaded_.data()` directly.
The descriptors must survive the call. No new session attachment, shadow counter,
texture-list owner, PE access, service hierarchy or vtable is needed.

Catalog count bounds both arrays. The table's count bounds readable descriptor capacity;
source sentinels may be represented as null texture members or by the view's implicit end.
A nonnull entries pointer with zero count is an empty table. A null entries pointer is an
absent table and returns without touching the counter. All pointed-to storage and archive
text must remain valid through lower callbacks; callbacks must keep termination within the
borrowed capacity. Invalid character indices throw as a named NOTSA host validation, not as
recovered source behavior. The native imported catalog and existing Session have nine
slots; the classic SDK's six-element loaded-array declaration is not used as a bound.

The source captures the table once; each next descriptor is read after the lower call.
When the initial loaded byte is zero, named entries call the existing profile-correct
`load_archive`, nameless entries call existing `load_no_name`, and Amy (ID 5) calls the
extra seam even for an empty table. Every nonnull-table return then increments the live
signed byte modulo 256, including warm negative counts. A lower exception propagates
before the increment. Warm Amy does not invoke its extra seam.

`load_egg_robo_textures` is deliberately unresolved in this game module. Root's native
binding must fail loudly there until recovered; no fake success body is supplied.
`State &` / `Services &` provide its existing texture context without a new service class.
The source's unused callee return is intentionally discarded.

## Evidence and proof

Source evidence ID:
`ev_49279b700b5fec055393ce16c7aa84e0ef6d683b686745a628bcc0a54435447b`.
`source-evidence.json` retains the complete 89-byte body bounds, full assembly,
pseudocode, callee list, original dossier hash/path and false truncation/zero omitted
counts. Root supplied the completed, closed owned Ghidra session; this worker launched
no provider. Classic SDK declarations corroborate the names and PVMEntry layout.

`dev.json`: 5,297 cases, zero mismatches, strict pass.
`holdout.json`: 4,096 untouched seeded cases, zero mismatches, strict pass.
No floating-point arithmetic occurs in this wrapper; FP partial count is zero.
The oracle executes original wrapper instruction bytes from the pinned executable,
with ordered lower-callee receipts. It compares every loaded byte, descriptor identities,
catalog rebinding, call arguments/order and failure prefixes. It covers all 256 initial
bytes across nine slots and null catalogs, empty/sentinel tables, named/nameless dispatch,
Amy cold/warm/empty behavior, counter wrap/reentry, live byte mutations, next-entry pointer
and filename mutations, captured-directory behavior and lower-call failure.

This is an upper-wrapper judge. It does not prove lower loader algorithms, archive
contents, GPU upload or Amy's body. Existing lower implementations and their judges are
unchanged. In particular, original `421180` is profile-hooked to loader `LoadPVM_r`;
existing `regular_textures::load_archive` is the correct native target. `4228e0` is a
separate individual selection path and is not substituted here.

`mutations.json`: all 10 compiled behavioral mutants killed by actual value/receipt
mismatches (no compile failures or crashes counted): negative-is-cold, null-table-increments,
nonwrapping counter, cached counter, warm Amy extra, empty Amy omission, named-as-nameless,
prefetched next entry, recaptured table, and ignored null sentinel.

The C++17 native unit links the real maintained `regular_textures.cpp`, not receipt
substitutes. It covers actual named/nameless service delivery, all 256 loaded values,
null tables, cold/warm/empty Amy receipts, explicit Amy failure, lower failure, next-entry
mutation, live byte mutation and captured table identity. The Amy unit definition is
only a test receipt and fail-loud test seam, not a recovered callee.

Readability: both new game files have zero debt in every metric. Layers: zero violations.
`profile.json`: zero UNHANDLED overlaps for the exact original wrapper range.
Provenance currently exits 1 only for the expected duplicate `421280` STUB in frozen
`gamma_selection_core.hpp`. Root must remove that obsolete annotation while binding the
new canonical FUNCTION. This packet does not edit Gamma or runtime files.
`proof.json` records source/probe/tool hashes and check statuses.

## Reproduction (private serial compilation, at most -j2)

Run from this packet checkout. Substitute local executable/interpreter paths as needed.
No command writes Main or changes an existing judge.

```sh
PY=/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python
EXE='/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe'
mkdir -p build/character-textures-core
clang++ -std=c++17 -Wall -Wextra -Werror -Wmissing-field-initializers -Isrc -dynamiclib src/game/character_textures.cpp tests/probes/native_character_textures_probe.cpp -o build/character-textures-core/native-character-textures-probe.dylib
clang++ -std=c++17 -Wall -Wextra -Werror -Wmissing-field-initializers -Isrc src/game/character_textures.cpp src/game/regular_textures.cpp tests/unit/character_textures.cpp -o build/character-textures-core/character-textures-test
build/character-textures-core/character-textures-test
"$PY" tools/oracles/verify_original_character_textures.py "$EXE" build/character-textures-core/native-character-textures-probe.dylib --output build/character-textures-core/dev.json
"$PY" tools/oracles/verify_original_character_textures.py "$EXE" build/character-textures-core/native-character-textures-probe.dylib --split holdout --output build/character-textures-core/holdout.json
"$PY" tools/oracles/mutate_character_textures.py "$EXE"
python3 tools/oracles/verdict.py build/character-textures-core/dev.json build/character-textures-core/holdout.json
python3 tools/gate/check_profile_patches.py --build-dir build/character-textures-core --json
python3 tools/gate/check_readability.py
python3 tools/gate/check_layers.py
python3 tools/gate/check_provenance.py --database /Users/burbujamc/Developer/sadx-macos/build/symbols/symbols.json
```

## Root integration proposal (not applied here)

Add `src/game/character_textures.cpp` to `native-sources.txt`. Append unit row
`character-textures<TAB>` to `tests/unit/manifest.tsv`. Add explicit Make rules:

```make
build/character-textures-test: tests/unit/character_textures.cpp src/game/character_textures.cpp src/game/character_textures.hpp src/game/regular_textures.cpp src/game/regular_textures.hpp
	$(CXX) $(CXXFLAGS) -Isrc tests/unit/character_textures.cpp src/game/character_textures.cpp src/game/regular_textures.cpp -o $@
build/native-character-textures-probe.dylib: tests/probes/native_character_textures_probe.cpp src/game/character_textures.cpp src/game/character_textures.hpp src/game/regular_textures.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_character_textures_probe.cpp src/game/character_textures.cpp -o $@
```

Default oracle discovery supplies `oracle-character-textures`; no extra oracle manifest
row is needed. Root must register new protected files with the additions-only judge
workflow, provide a native fail-loud Amy seam, resolve Gamma's old STUB annotation,
bind its actual ordinary texture caller to these canonical owners, and rerun focused
checks plus the next normal-input native diagnostic. Confirm the next actual seam and
inspect its image before making a visible-progress claim. Full gate, app/journeys,
independent review and subsequent Gamma weld/effects/animation/drawing are not run here.
No existing judge, tolerance, expectation, Makefile, registration or Main file was edited.
No push.
