# ExecutableImage packet for Root

Base Main `1300815`, branch `packet/executable-image`, assigned worktree
`t3-6e5fa2fc`. Source commit `fc4be98`; context `f1de322`, fresh pre-rewrite
baseline `85def98`, separate judge registration `06e42d8`. SUMMARY.json records
complete member hashes, exact review input hashes and qualification limits.

The whole public header and implementation are rewritten into structured C++.
The module still owns immutable PE32 i386 mapping facts, exact named exports and
forwarders, imports with lookup fallback, and one-shot flat HIGHLOW rebasing.
Private helpers name file/section, RVA/string, export and relocation boundaries.
PE signatures/offsets/masks and guest VAs remain legitimate format values. No
layout owner moves to another module, new public API, general PE support, game
address translation, guessed policy or blanket reference exemption is introduced.

LB-1 is reproduced with independently authored synthetic bytes: a final mapped
eight-byte relocation header declares ten bytes. Before rewriting, the test
reported exit 1 because the old source incorrectly accepted it; the candidate
returns an explicit error. Bounds now cover mapped relocation directories and
entries, target words, partial headers/entries, raw/virtual section extents,
header truncation, VA/RVA overflow, export tables/strings, imports and IAT slots.
Widening address arithmetic prevents malformed offsets from wrapping into valid
bytes. PE32 relocated word addition deliberately remains modulo 2^32.

Valid behavior checks are unchanged: 65,630 fresh pre-rewrite checks include all
synthetic mapped/rebased bytes and expected exports/imports. Additional 11 valid
edge checks match the exact original header/body, including top-aligned mappings,
overlap order, gap zeroing and flat-snapshot repeat behavior. The candidate prints
65,674 contract and 8,221 edge assertions before final CLI validation. Both suites
pass ASan/UBSan; six compiled faults are caught. Existing model_dlls.cpp is
unchanged and passes with six read-only actual model DLLs against base/candidate.
The pinned-model receipt records asset hashes and commands. Those additional
base comparisons were run after the rewrite, not before it.

Reproduce narrow checks from this worktree:

```sh
python3 docs/packets/executable-image/qualify.py
make -j1 build/executable-image-test build/executable-image-edges-test
./build/executable-image-test
./build/executable-image-edges-test
```

The script writes its fresh receipt under ignored build/executable-image-packet;
qualification.json is the committed receipt of this run. The registered defaults
need only synthesized bytes, no external assets, libraries, guest CPU or network.
Existing behavior tests, expectations, tolerances and fixtures are unchanged.

Root integration steps:

1. Review all two-file source bodies and evidence afresh against STYLE section 12.
   This implementation worker ran no delegates and grants no conformity verdict.
2. Merge/cherry-pick packet commits on Root after its active frozen gate completes.
   Keep source and judge commits distinct; reconcile additive registrations.
3. Restore all 53 raw corpora byte-identically. Preserve all 943 inherited protected
   entries and every newer Root parent entry. Add the two test source paths and
   rehash only the reviewed append edits to Makefile/unit manifest. The manifest
   here was deliberately never refreshed; manifest-prerequisites.json lists the
   exact absent paths. Never use incomplete filesystem discovery as a new baseline.
4. Reconcile only these two member identities into the module inventory after
   fresh review. Retain open guest_heap/mpeg and other loader/campaign findings.
5. Run actual guest-imports/guest-dlls integration, the composite full gate and any
   required Root qualification. Record only its real exit and regenerate STATUS.

No full gate, journeys, HLE integration, Windows parity, fresh independent review,
Main write, push, provider, download or storage operation was performed here.
No whole Loader, native journey or campaign closure is claimed. Resource exhaustion
for attacker-selected huge images is not newly bounded by an invented allocation
limit; this module remains the existing local reference PE subset.
