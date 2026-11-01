# Session save ownership prerequisites

This is source context at Main a9ed5bb, not a source migration or fresh fidelity
verdict. The [mapping](context-session-save-ownership.json) pins files, exact
consumer locations, the original copy range and seven existing owned bytes.
The full maintained-code campaign remains open.

Session's two Data values are one current persisted record and a distinct backup.
The logo borrows current data during save initialization; bootstrap, file listing,
file display, character predicates, load/capture/write and menu callbacks all use
that same record. Title preparation and main-menu control write the backup. They
do not restore current from backup: the existing helper called restore_save first
changes current.last_character, applies current through a callback, then copies
current into backup. A callback may change current before that copy. Title
preparation copies before invoking load_save. Preserve these different orders.

The pinned SDK declares SaveData as a three-record array at the current base.
That does not identify the menu backup or authorize inventing roles for the other
two records. The unchanged main_menu and cleanup judges identify current at
3b2b3a8 and backup at 3b29d70. Exact original instructions in 42ca10 load ECX=348,
ESI=current, EDI=backup and execute REP MOVSD at 42ca69, copying 1,392 bytes before
LoadSave. Native Data is serialized explicitly; do not use its host sizeof or
padding as the source extent. Backup still lacks a semantic symbol override.

A native-only container for the two fields is not the complete owner migration.
GameModeLeafState::block_at_3b2b5f9 already owns seven bytes inside current data:
options plus six life bytes. Source entries 413c00 and 414470 read them after
prior stores and nested callbacks, with precise unknown-fact stops. NativeGame
imports the bytes; the game-mode unit and protected probe populate and serialize
the optional-byte array. A new GLOBAL claiming the entire current record would
overlap that owner. The existing ownership checker identifies all seven bytes.
Do not exempt the new owner, silently zero missing reference facts, copy options
into another cache, or leave these readers outside the migration.

The next connected owner extraction should share one canonical profile-owner type
across runtime worlds. In each world it contains the real current and backup
records; reference availability and source-byte diagnostics stay in an adapter.
GameModeLeafState borrows typed options/lives values through that adapter. A
missing option byte must still stop at 413c1e after preceding stores; a missing
selected life byte stops at 4144a7 after character/motion work. Known zero and
unknown remain distinct. Host record layout must never be traversed as raw source
bytes. Retain full source widths and source character-index mapping.

Changing the reference byte view requires explicitly rebinding the existing
fixture/probe. Keep public exports, every input byte, output word and case; any
judge edit is separate evidenced work, followed by unchanged original comparisons.
Fresh baseline/partial-case outputs are needed for game_mode_leaves and actual
native-game composition as well as save, bootstrap, logo, main_menu and cleanup.
No such new qualification is claimed here. This context records why merely moving
two Session fields would leave incomplete ownership.

Save IO filenames/list nodes and attached backend, warning/menu tasks, audio
players/banks/listeners, texture resources and scheduler lifetimes remain separate
ownership questions. Their lifetimes are not established by the Data copy proof.
Keep their current ordering until each owner and all consumers are mapped; do not
collect them into a universal Services object or move every Session member at once.
The frame/editor/reset alias prerequisites remain in context-game-reset.md.
