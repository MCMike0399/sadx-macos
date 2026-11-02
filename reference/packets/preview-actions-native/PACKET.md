# Preview actions native candidate

Base `ca640c5`; branch `packet/preview-actions-native`. This is new recovery of
`CharSel_KeepCharactersFromFalling` and the selection-byte setter, not an unbound
existing implementation. The existing upper preview lifecycle stays unchanged.

Source: pinned PC image `6b0937ca…24cd74`, cached REA callee evidence
`ev_386942168b7f965037441ed40f3c2aeb73266c445fbd9ff228f98c561c098dd6`,
mod-loader `CharMdlWk`, `EVENT_ACTION_LIST`, `EV_SetAction`, `SetInputP`, existing
selection importer/metadata and native owner declarations. `source-context.json`
records exact instruction observations, hashes, names, field mapping and owners.
The cache had no function body dossier; fresh Ghidra was reserved for Gamma.
Capstone decoded bytes read with PEImage; actual original execution judges behavior.
A fresh REA body dossier remains a review prerequisite if Root requires one.

Owned changes: `src/game/selection_preview.hpp`, new
`src/game/selection_preview_actions.cpp`, a new reference-only ABI probe, oracle,
and scheduler-backed unit. No runtime, menu, shared player schema, Makefile,
registration, inventory, frozen input or existing judge was edited.

The semantic core sets input 12 for active actor slots 0–5; Miles clears counter
bit 29 and receives input 49. Idle queues once at speed .25 (.5 for Miles).
Confirmation queues the remaining imported sequence, looping only the final action;
its count increments before each call and its selection byte clears after the last
call. A callback can change selection, counts, later task slots and primary fields.
The captured task and primary are retained, later slots and counters are read live,
and the actual last event node is published. Null actor slots and slot 6 are skipped.
`select_motion` borrows the current preview Work and truncates to the source byte.

Checks and exact streams are in `qualification.json`. Development: 3,837 original
cases; final new holdout: 6,837. Seven discriminating mutants are caught, including
primary recapture and truncating tail traversal. Unchanged upper judge: 3,608 dev
and 5,608 final holdout, strict pass. Source-only native unit passes through the real
TaskScheduler/Task/Work/PlayerTaskState and stops at the unimplemented event engine
without publishing an idle count. It checks current-work rebinding and task-level
remove/reentry; it does not qualify menu cancellation or actor animation/rendering.
Readability, layer and unique legacy owner checks pass. Profile: zero UNHANDLED.
No full gate or native app run was attempted in the worker. Last actual gate remains
`bf61403`; Root's frame 1332 receipt is unchanged. No visible N3b completion claim.

Root integration requirements:

1. Add `src/game/selection_preview_actions.cpp` to the native source list. Core wildcard
   discovery handles the reference core. Append `selection-preview-actions` with no
   arguments to `tests/unit/manifest.tsv`; the default core-linked test recipe suffices.
   The oracle/probe naming auto-registers `oracle-selection-preview-actions` with EXE
   and probe arguments. Apply the additions-only judge refresh after integration;
   this worker intentionally did not touch the shared manifest.
2. Add the symbol override row supplied in `symbol-overrides.tsv`, regenerate symbols,
   and run provenance. Stock provenance currently reports exactly one missing symbol:
   `CharSelActionTails`. The source range has no existing annotated/legacy owner.
   The packet does not mutate the shared symbols/inventory files to hide this condition.
3. Own one `selection_preview::ActionState` for the lifetime of the title/session,
   alongside the existing preview State. Do not reset its six borrowed tail references
   on cancel/create: original teardown/initialize do not clear those globals. Do not
   duplicate existing primary, player, texture, motion or imported resource owners.
4. Supply the canonical `PlayerEventActionNode` definition and event owner under the
   Root-owned player/animation schema. Preview only forward-declares and borrows it.
   `action_head(primary)` must read the captured primary's live event action list;
   absent primary/event/list must stop explicitly. `next_action(node)` reads its next
   link. Both must borrow actual event nodes, never a synthesized preview pose/list.
5. Bind `ActionCalls::primary` to `task->primary_state` without eagerly rejecting null
   (source defers dereference, including slot 6). Bind `miles_counter` to the live
   registered primary at index 1, `source_counter_at_c.value()` after explicit validation.
   It is not necessarily the task primary captured for the current actor.
6. Bind `action(slot,index)` to `CharacterSelectionAssets.sequences[slot].steps[index].action`;
   index equal to the sequence length returns a null ActionHandle terminator. Other
   unavailable indices fail explicitly. Bind `link_frames` to the same indexed step,
   and `textures` to the source descriptor's existing character texlist identity (the
   descriptor list from the canonical resource owner). Exact `(character,binding)`
   pairs by preview slot are `(0,0), (2,0), (3,1), (5,0), (7,1), (6,0)`: Knuckles
   and Big use index 1. The source-byte map is in `source-context.json`. Do not substitute
   the source_key, ordinal or an animation table index for the actual ActionHandle.
7. `PreviewServices::update_actor_actions(Work &work)` must call
   `selection_preview::update_actions(work, <canonical action state>,
   session.bootstrap_.menu.player_character, <ActionCalls adapter>)` at its existing
   warmup-gated display position, after the existing perspective stores.
   In `CharacterSelectServices::select_preview(int motion)` call
   `selection_preview::select_motion(scene.preview_, motion)` using its actual class
   name/access. This stores only the byte; it does not run actors or queue immediately.
8. Lower native seams remain `SetInputP` (0x441260), `EV_SetAction` (0x42fe00 →
   0x417ef0), canonical event-action allocation/queue ownership and actor event-action
   consumers. The source oracle runs actual SetInputP; its native receipt mirrors the
   writes for comparison, not a production implementation. Do not bind that test receipt
   into the app. Missing event behavior must remain a loud stop. Actor controller,
   Gamma/Sonic setup, animation stepping, display and destruction stay Root/sibling owned.

N3b acceptance still needs one normal-input native run with all seven real scheduled
actors, initialized and repeated content-bearing actor frames versus aligned reference
frames, cursor/selected-motion changes, pending-confirmation behavior, cancel/reentry,
Sonic confirmation into story entry, reached voice/music, source-only closure and full
gate. A checker must reject suppressed actor draws; preserve the distinct frame 1332
and older frame 1382 schedules. This packet edits no journey goldens or checker.

Reproduce with `python reference/packets/preview-actions-native/verify_mutants.py EXE`
using the reference Python environment; direct compile commands and report identities
are in `qualification.json`. Worker outputs are only `build/preview-actions`.
