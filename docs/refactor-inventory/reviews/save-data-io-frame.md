# Save data, IO and frame inventory review

Reviewed frozen Main `e694d4b4f82a330d351ebd2b89e8671abe0dcc17` in the isolated
review checkout. The [authored JSON](save-data-io-frame.json) records all bodies,
current hashes, exact module membership, owners, actual consumers, judge limits,
tracked dependency anchors and historical pin comparisons.

Exactly **11 files, 873 lines, 6 modules**: 5 files / 3 modules conform;
4 files / 2 modules need migration; 2 files / 1 module remain open.

| Exact module | Members | Disposition |
| --- | --- | --- |
| `src/game/save_data` | `.hpp`, `.cpp` | already conforming |
| `src/game/save_creation` | `.hpp`, `.cpp` | already conforming |
| `src/game/save_file_name` | `.hpp` | already conforming |
| `src/game/save_file_list` | `.hpp`, `.cpp` | needs migration |
| `src/game/save_frame` | `.hpp`, `.cpp` | needs migration |
| `src/game/save_write` | `.hpp`, `.cpp` | open review |

The conforming records accept the explicit little-endian codec, literal record
content and complete-local-before-publication initialization; capture preserves
unassigned padding, packs only flags equal to 1 and publishes checksum last.
SaveName publishes a borrowed pointer independently of its allocation. Unknown
reference identity remains distinct from known null. Profiles and SaveCompletion
retain their separate existing reviews; no consumer implementation is implicitly
approved by this fragment.

Five actionable findings remain:

1. **SL1:** separate list sentinel identity from indeterminate source name/time in
   the game API, preserving first-append and partially linked append behavior.
2. **SL2:** qualify list allocation and retained-alias lifetime, partial reads and
   file/search close ordering. Existing reverse-free cases have no retained aliases.
3. **SF1:** adapt the legacy reference loop to the shared scheduling core. Its
   duplicated branches and optional old facts remain migration debt; keep lazy reads,
   callback mutations and failure prefixes rather than eagerly supplying defaults.
4. **SW1:** qualify writer callback-visible snapshots and failure prefixes, including
   publication before formatting, encoding after open, reset ordering and OK after close.
   Existing final-state receipt does not cover these mutations/failures completely.
5. **SC1:** complete narrow service identity/failure contracts. Saving-message,
   close-message and writer already have real native implementations; do not label
   them unrecovered. Genuine missing outer services and separate warning/delete/cursor
   behavior remain explicit boundaries and require named STUBs when exposed.

Historical discovery/load/free reports match every source/probe hash they record,
but omit transitive codec/tool closure. Save-data body/probe match; its header pin
is stale, with a directly inspected equivalent 8/43 dimension-only diff. Capture's
historical source pins also differ only by the reviewed dimension/alias change.
Writer/frame match the historical connected receipt's four source hashes.
Preparation and owner receipts contain additional stale pins, listed individually
in JSON; matching counts never manufacture a new pass.

Raw Ghidra dossiers, private temporary packets and complete observation streams are
unavailable here. No dedicated current eleven-file mutant/holdout proof was found;
canonical owner mutants and menu predicate holdouts are separate scopes. Controls'
938 inherited FP partials remain partial, with existing identity context untouched.
The review ran no builds, tests, oracles, mutations, providers, network/storage work,
delegates or gate. Main gate 24054 was not touched or polled. No whole save, Session,
Windows parity, gameplay or campaign closure is claimed.
