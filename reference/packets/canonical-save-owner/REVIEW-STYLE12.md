# Independent STYLE12 review: canonical save owner

Verdict: bounded ownership/binding approval; no blocking findings.

Reviewed packet `packet/canonical-save-owner` at `5ecc49dc00db9aec38041f01760c5f9694aeb518`,
source `a490d56cad9fdc3957f27f9f3ff67a0977fe27c6`, in
`/Users/burbujamc/Developer/sadx-wt-inventory-review-closure`. The fixed comparison
is `e7d43df489a6ad1caf13ec5e9abf59d10ff2ef03`, not newer Main `3efc9b7`.
Main's later SaveCompletion work is independent and additive; its absence from
this packet is not a proposed deletion. Review performed independently by one
reviewer, without workers or source edits.

## Ownership, consumers and order

`src/game/save_profiles.hpp` adds a small, NOTSA-labelled value owner with plain
`Data current` and `Data menu_backup`. Session and GameModeLeafState instantiate
that same type in independent runtime worlds. There is no second options/lives
value cache. All native Session `save_`/`save_backup_` value fields are removed.

Inspected every old record use and its replacement in native_session.hpp/cpp,
native_save.cpp, native_main_menu.cpp, native_character_select.cpp,
native_file_select_display.cpp and native_title_tasks.cpp. Public save_data(),
bootstrap/start/setup, load/capture/write, list counting/selected-file loading,
file display, character/menu eligibility, language, cursor, confirmation and
new-save preparation all borrow `saves_.current`. Main-menu control and title
preparation borrow the distinct `saves_.menu_backup`. These are reference
substitutions; algorithms and callback order are unchanged.

LogoScene still stores `save::Data &save_`, bound to `saves_.current`. Profiles
remains before logo_ in Session declaration order, so it exists throughout logo
construction and destruction. The value records contain no backend, task,
texture or audio resource ownership. Title preparation still copies current to
backup before load_save; main_menu::restore_save still clears last_character,
calls apply_save, then copies the live current record. Callback changes therefore
remain visible to that later copy. No new snapshot or lifetime dependency appears.

## Reference availability and source identity

GameModeLeafState's old seven-byte SourceBytes owner is gone. Its Profiles is
private, and only the formerly supplied options byte and six life bytes are
exposed. Reference-only bool metadata carries availability, with no fixture
optionals in the core owner. Importing unknown clears knownness without exposing
stale/default storage as known zero. Other current fields and the backup remain
private and unexposed; their physical initialization does not supply new facts.

The six-slot bounds check precedes both metadata and Data indexing. It is safely
within Data.lives' seven elements and deliberately does not expose the seventh
element. Out-of-range reads return unknown, matching the former seven-byte
source view; invalid imports throw. NativeGame imports the same seven addresses.

The two changed leaf reads retain their original source boundaries and facts:
413c00 stops at 413c1e for missing 3b2b5f9 after the earlier stores; 414470
publishes character, performs model work, rereads character/index and the live
selected life count, then stops at 4144a7 if unavailable. No eager read or
unknown-to-zero fallback was introduced. The missing-options/prefix and callback
units exercise this distinction. Recorded runs cover both old and new leaf
bodies; five successfully compiled mutants are rejected for availability,
known-zero, wrong-record, wrong-slot and eager-read faults.

Cached symbol evidence preserves the SDK declaration
`DataArray(SAVE_DATA, SaveData, 0x3B2B3A8, 3)`. Current names its first element;
the other two roles remain unresolved. Backup is separate at 3b29d70. I directly
read the local original PE bytes using PEImage: 42ca54 loads ECX=0x15c,
42ca59 loads ESI=3b2b3a8, 42ca5e loads EDI=3b29d70, and 42ca69 is REP MOVSD.
The 42ca10..42ca99 range hashes to
`780932f7185c6cbe14d6d6ad3dbecda9feb97f4a9f53c5a831550e56b93e8559`, matching
the recorded evidence. Thus each annotated source extent is 348 * 4 = 1392
encoded bytes, independently of host padding. Existing explicit codec use is
unchanged; no host-layout traversal is introduced.

Symbol overrides and GLOBAL annotations agree. fa063aa retains historical
seven-byte ownership and adds both complete record ranges. The unchanged
provenance checker checks lost coverage and overlapping claims per byte; my
focused run exits 0 with 86 GLOBALs, 167428 byte identities and zero errors.

## STYLE12 and judge integrity

The new game header has semantic names, provenance annotations, plain values and
no addresses in executable code, guest dispatch, arithmetic, hidden behavior or
speculative service abstraction. No recovered algorithm was moved into guest
code or replaced by a stub. The reference leaf algorithms retain their existing
address-shaped debt; this approval does not call them readable game modules.
No new platform replacement or enhancement hook is needed for this value owner.

Inspected the separate 768c0a0 probe rebind: existing C exports, argument types,
kScalars, kBytes, input offsets, output offsets and unknown markers remain intact.
Only seven-byte import/export binding changes. The stock evaluator is byte-for-byte
identical to e7d43df. The 9942630 names-table revalidation does not change its
declaration, selectors or loading behavior; no table debt is silently retired.

Read the packet and baseline/qualification/mutant receipts. Independently checked
all 15 recorded source fingerprints, all four tracked 650-case shard hashes,
and byte identity of baseline versus typed native-full.bin, original-full.bin
and case-identities.json. The evidence retains 2600 native outputs, 2526 original
comparisons and the same 74 source-excluded native boundary identities. Native
result words include FP status, but original FP flags are NOT compared by this
judge. This is no new original-FP-status pass claim.

Inspected check_game_mode_leaves_holdout.py: it substitutes only the evaluator's
Random constructor, guards the stock seed, calls unchanged original.main(),
preserves its exit code and restores module/argv state in finally. Report metadata
is added afterward. It changes no vector after generation, case count, comparison,
expectation or exclusion. After removing that metadata, the stored stock-seed
adapter report equals the stored typed evaluator report exactly. Seed 20261105
remains RESERVED and was not run during review. Approval permits Root to use this
adapter for that additional cohort; inherited native exclusions still apply.

## Qualification and limits

The recorded corrected native closure exits 0; the first wrong-target invocation
exits 2 and is documented. I checked hashes of the corrected build log,
libsadx-native.a and actual force-load executable build/sadx-native-closure
against the qualification receipt. The receipt describes rebuilt maintained
native/game/numerical source, reusing only pinned installed SDL/libusb; it is
not a release-export proof. Three recorded actual native menu/save journeys
pass, including fresh creation and restart persistence, with their stated pixel,
gameplay and controller limits retained.

I ran no builds, oracles, mutations, journeys, reserved cohorts, providers,
network/storage operations or workers. Verification here was source/evidence
inspection, local byte/hash comparisons and the read-only provenance check.
Build, mutant and journey results above are inspected producer evidence, not
fresh executions by this reviewer. The candidate worktree remained unchanged.

Root owns the final reserved cohort, additive Source integration with newer
SaveCompletion/Math, explicit protected-manifest union, reference journeys and
full integrated gate. No Main edits or manifest refresh occurred here. Complete
save IO/UI/audio/resource lifecycle ownership, Session decomposition, unresolved
SDK save-array roles, broader reference-tier migration and the exhaustive
maintained-code refactor campaign remain open. This approval closes only the
reviewed current/backup ownership and connected binding change.
