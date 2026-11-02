# Sonic native editor binding candidate

Supplied worktree `t3-5cf04a5b`, branch `packet/sonic-editor-native`, base Main
`05eaa6c3`. Source commit `9f6ec53b`; separate judge/evidence commit follows.
Root owns independent review, shared-field admission and the full gate.

| Source owner | Native counterpart and initialization | Borrowers |
| --- | --- | --- |
| `3b29d08 ssEditorStatus`, low uint16 | Existing Session::editor_status_; PE virtual zero | Existing cleanup::State, new Sonic reader, Gamma CheckEditMode |
| `3b29d0c ssMiEditorStatus`, low uint16 | Only new field: Session::mission_editor_status_; PE virtual zero | New Sonic reader and Gamma CheckEditMode |
| `3b18db5 gu8flgPlayingMetalSonic`, uint8 | Existing SoftResetState::metal_sonic; PE virtual zero | Existing input/cleanup/save users and new Sonic alternate_model borrow |

CheckEditMode's complete cached dossier is `ev_077bb9d4…691c1e`: MOVSX mission,
MOVSX editor, OR, RET, range `[4258f0,425901)`. Its function annotation moves from
the Gamma STUB to game/editor_state.hpp; the Gamma seam still keeps reference
transport out of native code. Source and native return exact signed/non-normalized
OR results. Sonic retains its two existing live seams and their ordered calls.
View copies borrow references; no editor values, player or physics owners are copied.

`source-comparison.json` establishes source load-image zero for both complete
four-byte SDK slots, source read widths/order, poisoned upper halves and the
unchanged Reset word store at `40bf5c`. Raw .text operand scanning finds 62 editor
references (one word writer: Reset), one mission reference (CheckEditMode), and no
upper-half references. This is bounded direct-reference evidence, not a general
indirect-write theorem. SDK comments agree that neither editor is set in this PC
version. No pointer is inferred from the reference adapter's optional-fixture pointer.
GameResetState and all reference missing-fact behavior remain byte-identical.

The Metal byte reuses the already qualified native Input owner and consumer;
no new default or owner is added. Source ConsumeSoftReset clears it; the other
original direct writers are in Adventure state handling and `5064d0`, outside
this reached menu slice. Pinned Input/Speedrun DLLs contain no direct operands to
these three globals; the loader's sole direct Metal operand is a byte compare.
Its source testspawn Metal write requires a character command-line override,
absent from this normal profile/run. No editor code/data overlap is active in the
profile inventory. `profile-operands.json` retains the exact DLL identities/scan.

Actual production callback: publish captured current primary/player, read editor
words, real PInitialize, own Sonic animation table binding, requested animation
zero, actual Metal byte read, then explicit `sonic_model_graph`. The initial
current-player alias stays null after allocation. With metadata absent, the stop
is `sonic_animation_table`. The next missing behavior is the original Sonic
object/model graph branch beginning at `49ab64` (alternate branch `49abb0`);
Sonic resources.objects remains absent and no model pointers are invented.

Minimal shared hunks: Session adds that single mission word; Gamma Context adds
one optional borrowed View pointer. TitleScene binds both actors to the same
Session words and Sonic to soft_reset_.metal_sonic. No other Gamma method,
particle context, motion/animation asset or algorithm changes. Existing Context
construction without editor state retains native_gamma_check_edit_mode failure.

Qualification: 5,201 original comparisons including 1,024 holdouts, 81 real
callback/Gamma pairs, exact source read/store widths, actual Session cleanup
aliasing, eight compiled faults caught, ten native programs, app and force-load
closure. Generated two-column registrations execute with all arguments intact.
Full old/current 58,992 default and 3,072 extra streams were not rerun: unchanged
engine/reference/probe hashes and native binding body are proved in preservation.json;
all 2,840/2,172 default and 204/219 extra FP partial identities remain inherited.
Gamma's current native motion output is byte-identical to Main's retained output.

Normal input still stops at native_gamma_join_vertices, frame 1332, exit 1.
The inspected outgoing title PPM retains SHA c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427.
No completed actor frame, N3b, Main write, push, full gate, fresh Ghidra,
reserved Task/reset vector, collector, schedule, storage or provider operation.
All 1,398 inherited protections, including 53 locally absent binary evidence
paths, are retained; finite registrations and exact local judge limits are in
registration.json. Only two unrelated save-warning header fingerprints and their
mirror are narrowly rebound; their open debt is unchanged. Broad inventory
regeneration is left to Root after shared-field reconciliation.
