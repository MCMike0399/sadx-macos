# Runtime asset contracts: independent inventory review

Reviewed Main `73be9a5` in isolated branch `packet/review-runtime-asset-contracts`.
The companion JSON is the authored fragment for deliberate Root reconciliation.
It covers **all 13 exact headers, 316 physical lines, and 13 complete single-header
modules**. No source, judge, global inventory, shard, HANDOFF or STATUS was edited.

| Disposition | Complete header-only modules |
|---|---|
| already_conforming (11) | character_texture_assets, file_select_display_assets, font_assets, main_menu_assets, menu_dialog_assets, menu_texture_assets, message_font_assets, music_assets, regular_texture_assets, title_assets, voice_assets |
| open_review (1) | character_selection_assets |
| needs_migration (1) | gamma_actor_assets |

`SelectionActionStep.source_key` means `(origin kind, character, export index or
constructed ordinal)`. The importer constructs kind 0 from CHRMODELS exports and
kind 1 from statically recovered selection action declarations. The app validates
all 19 keys, then asks `CharacterPack::action(SelectionAction)` for a pack handle.
This is content identity, not address dispatch. However, sequence steps/link frames
have no live playback consumer: `PreviewServices::update_actor_actions` throws.
That missing acceptance/lifetime evidence keeps the complete header module open;
it is not authorization to invent playback behavior.

Gamma joins/cursors/question objects use typed pack record IDs. Zero references
represent actual null; nonzero IDs resolve through validated record kinds. Gamma
matrix node numbers are hierarchy traversal ordinals; material arguments are table
row/material index; preview palette 2 is a lighting selection index. Colors/masks,
float words and animation recipe fields retain their numeric meaning. Boundary text
for unavailable imported actions is diagnostic provenance, not a dispatch key.

The concrete Gamma migration is the native API's `PlayerAnimationTable`: its records
carry fixture-availability optionals, which the app populates and native binding
publishes directly into player animation state. The actual animation implementation
also retains instruction-site diagnostics and knownness-driven game decisions.
Migrate that contract and its real reference/native consumers together, preserving
Loaded/KnownNull/Unavailable, null export 61, request 0 Stand and all 91 rows. Native
material, setup/effects and model-matrix bindings remain loud unresolved seams;
imported material metadata is not yet carried/bound by this header. This review does
not claim those missing behaviors are recovered or that resource IDs are addresses.

`bind_texture_list` is behavior: validate both count vectors before publication,
create exactly count null entries, copy attributes, own immutable filename strings,
and preserve null names. It creates one cold list; callers preserve list aliases by
binding each distinct imported index once. Source sentinel/preloaded declarations
are rejected at import. Menu aliases check only count and deliberately ignore unused
alias filename/attribute payload. The helper does not promise filename interning.

`CharacterTextureViews` is also behavior: check each row's descriptor/list counts,
borrow exact strings/list pointers, preserve a stable nonnull empty table, and return
a catalog borrowing the actual signed loaded bytes. The caller keeps all storage
alive and unresized through synchronous lower calls. Copy/move is unavailable; the
view is invocation-local. Gamma binding tests specify actual pointer identity,
cold/warm/signed-wrap counters and the nonnull empty-table distinction.

All 13 header hashes equal those in separately reviewed character owner `515b0f7`.
That packet was inspected read-only at the supplied checkout; its commit-relative
receipts are described as separate metadata, not nonexistent local evidence links.
Main still has the five character fields in Session. The character packet's fixed
owner, 25 observations, six mutants, preserved 1,422 Gamma outputs/498 partial IDs
and three journeys do not prove Windows constructor or actor/rendering parity.

No builds, tests, oracles, mutants, providers, network/storage operations, delegates
or gates ran. JSON evidence anchors are tracked files with current fingerprints.
Historical reports retain limits: font/message/music/voice listed source hashes
match; dialog/display/title reports have recorded dependency drift, named in JSON.
Gamma's 498 FP partials remain partial. Contract acceptance for unchanged content
headers does not seal their consumer implementations or transitive judges. Root
retains current replay/partial-ID, source integration and combined-gate ownership.
