# Native preview editor binding

Candidate `packet/sonic-editor-native`, base Main `05eaa6c3`. This is the bounded
native counterpart binding described in
[the packet](../../reference/packets/sonic-editor-native/PACKET.md).

Session's existing uint16 editor_status_ remains the native ssEditorStatus owner;
cleanup::State still borrows it. A separate uint16 mission_editor_status_ supplies
ssMiEditorStatus. Both are PE virtual zero, not SDK 32-bit Bool values. The
reference GameResetState word and SonicPlayerGlobals optional fixture stay intact.
Sonic reads mission then editor as signed words, including the second read after
a nonzero first read. Gamma borrows the same words for CheckEditMode. Its absent
binding still fails explicitly. SoftResetState::metal_sonic remains the sole
native Metal Sonic byte; the callback now borrows it too.

Actual Sonic callback initialization reaches sonic_model_graph with its own
imported animation table and canonical arena/player. The first current-player
publication stays genuinely null. Model graph, common tail and the normal app's
earlier PJoinVertexes boundary remain open. No reset collector, reserved vector,
Task owner migration or full reset qualification follows from this binding.

## Checked native/reference counterpart

Session's mission-editor low word has a GLOBAL annotation covering exactly two
source bytes. reference/symbols/native-counterparts.json pairs that declaration
with the existing reference-only SonicPlayerGlobals fixture fact. The catalog
is judge-protected and pins the source evidence. check_provenance requires both
identities and exactly equal scalar extents; a missing annotation, changed extent,
stale evidence, second native owner, extra fixture owner or duplicate registration
fails. This permits no second product owner and changes no fixture behavior.
