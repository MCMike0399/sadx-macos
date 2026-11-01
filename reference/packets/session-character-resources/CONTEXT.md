# Native character texture resource ownership context

Source base92dbd07. This packet is planning only, with no maintained source edits.
The contiguous five Session members following LogoScene own copied archive metadata,
nine vectors of actual texture lists, nine cleanup pointer vectors, their nullable
published bindings and nine signed-byte loaded values. Common/regular resources,
menu bindings, CharacterPack, GPU memory, actor tasks and player state stay separate.

The constructor copies assets at member initialization, but binds the lists in its
body AFTER every later Session member has been constructed. Moving the entire loop
into a new owner constructor would move allocations/errors ahead of those later
constructors. Preserve two phases: metadata construction at the original location,
then one binding operation at the original body point, BEFORE profile/frame-rate
writes. Each character must have declarations; each declaration must have a nonzero
texture count. Earlier list resizes and cleanup-pointer appends precede later errors.
The existing constructor failure destroys the incomplete Session; no retry/rollback,
new readiness cache or permissive empty-table behavior is justified.

All actual consumers: Session::texture_state borrows the same cleanup bindings and
signed loaded bytes; unload/free paths reach game/game_cleanup. Public telemetry
character_texture_counts returns a const reference to those same bytes. PreviewServices
constructs its own archive/list bindings from retained strings and actual lists, with
its existing size-mismatch failure. Gamma creates invocation-local CharacterTextureViews,
which owns only temporary descriptor vectors, borrows source strings/lists and returns
a CharacterCatalog borrowing loaded.data(). Keep those live temporary views; do not
cache them in the owner. Loader callbacks and cleanup may mutate actual lists/counts.

Lifetime: cleanup pointer vectors and published bindings borrow the preceding list
vectors. Their owner must have stable identity and disallow copy/move. Lists outlive
cleanup views during ordinary destruction; metadata strings outlive invocation-local
preview/Gamma views under the existing Session/task contract. GPU resources remain
Display-owned, and this packet cannot invent callback-after-Session safety.

Before implementation: freeze the exact native constructor binding loop and a synthetic
public observation baseline, including pointer alias classes, list capacities/entries,
null initial bindings, signed loaded-byte identity and ordered invalid-declaration
prefixes. Preserve actual Session placement with a body-forwarding seam, never a moved
initializer. Original character-texture/preview/Gamma/cleanup judges and existing
character-view units cover the real consumers; inspect exact counts/FP partial IDs and
mutants before changes. Native attachment baseline is not Windows attachment parity.
Root still owes full context qualification, unchanged judges, discriminating mutations,
independent review, fresh native source closure/journeys and composite gate. Whole
character/regular/GPU/Session/campaign migration remains open.
