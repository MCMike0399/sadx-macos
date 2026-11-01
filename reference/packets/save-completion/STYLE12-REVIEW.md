# Independent save completion STYLE §12 review

Reviewed packet/save-completion at 88fbff79fe6a599682fefb27340a804426c6930d,
against a9ed5bb; source ed643ae, qualification ee5a1c5, registration d9640a5.
Verdict: bounded approval, no blocking findings. **save_completion.hpp and
save_completion.cpp conform to STYLE §12 within this named completion slice.**
The comment-only file_select.hpp change accurately identifies its owner-bound close
form. This is not a full save lifecycle, UI, Session, or campaign verdict.

Read canonical instructions/relevant menu and registration memory, STYLE, JUDGE,
and PACKET.md. Per the review brief, all verification below was read-only local
inspection; no builds, oracles, mutations, gates, providers, network, storage work,
or additional workers were run. Test outcomes below are verified recorded evidence,
not fresh executions.

- Cached Main dossier ev_239a8b751587ea915e904f464857f5f846c6e0499ed69c43fde7018f5a9650d6
  and all 287 cached disassembly rows match the pinned PE bytes (SHA256
  6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74).
  At 50394d dialog 17 is requested before the live language/descriptor reads at
  503952/503958; creation receives level 7 at 503960. Creation copies eight dwords
  of descriptor data and returns null on the task-allocation failure branch.
  Close reads the supplied task's work and returns 0/1; completion discards that
  integer, retaining its stored handle. The other controls caller consumes the
  owner-bound close result for navigation retry. The SDK/override names agree.
- Only three game files change. Shared ControlStage retains 0/10/12/13 and unknown
  stages still return false. Signed int8 save/selection comparisons, int16 waits,
  the five-frame boundary and same-call fallthrough remain intact. Plain integer
  arithmetic is appropriate; no FP calculation or content table was changed.
  Structured names/constants replace literals without address-shaped game logic,
  invented guards, new ownership, or speculative abstraction. Added structure is
  marked NOTSA; the existing service interface remains the extension seam.
- Request/create publication, pending-state stores, close/transition/prepare/apply
  order, and exception prefixes remain unchanged. Success reads selected-file state
  after transition; failure reads the live warning owner after selection. The
  by-value shared TaskHandle keeps the close argument alive across a callback;
  borrowed state references preserve owner changes and create publishes only after
  return. The new callback unit checks those ordering/publication boundaries.
- The only new STUB annotations describe genuinely unrecovered warning creation
  and close. Existing native_save_warning_message/native_save_warning_close throws
  remain unchanged; recovered dialog/transition/prepare/apply behavior stays native.
  No recovered behavior became a STUB. No platform replacement was introduced.
- Qualification source/probe/judge/helper hashes, dossier/disassembly/archive hashes,
  probe-library hashes, raw full-output digests and frozen receipt agree. Dedicated
  completion: 1000 exact, recorded exit 0 on both sides; capture: 1000 exact each,
  recorded exit 0. Controls: 2438 identical complete vectors, recorded exit 1 each,
  with the same 938 inherited FP partial identities (only output slot 25 differs
  from source). These are partials, not passes. The late controls baseline and its
  checkout-metadata limitation are explicitly documented. Existing C ABI, input
  generation, output layout, scripts/probes/tolerances and expectations are unchanged.
- Five recorded mutants have matching semantic source substitutions and local
  object artifacts: three completion-judge failures (125/125/211 mismatches,
  exit 1), two callback-unit assertion failures (SIGABRT, return -6). Compile
  success is recorded for all five; no invalid mutant is counted as a kill.
- Registration is an exact manifest-prefix append and the generated build/tests.mk
  recipe invokes save-completion-callbacks-test. All 928 prior protected paths are
  retained; only unit registration changes an inherited hash, and the new frozen
  mutation receipt makes 929 paths. Both updated hashes and the registration
  evidence hash match. Unit C++ bodies remain under existing source-review policy;
  this review does not claim they are all protected by the judge manifest.

Final fresh-seed holdout is RESERVED for Root after approval. No new save packet
full gate, clean native closure, lifecycle completion, or visible journey is claimed.
Main's previous green menu gate does not qualify this packet's integration.
