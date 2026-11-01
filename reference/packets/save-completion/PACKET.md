# Save completion packet

Base: Main a9ed5bb404d0674115eb4b6762434a23552c5883. Branch: packet/save-completion.
Scope: existing FileSelect_Controls completion suffix, not new gameplay or warning recovery.

The suffix reuses file_select::ControlStage and names the five-frame delay and dialog 17.
All eleven references, signed byte comparisons, int16 waits, source fallthrough, call ordering,
partial stores and borrowed TaskHandle publication remain unchanged. No Session owner moved.
The original close entry annotation moves from ControlCalls::close_warning to the completion
handle-taking seam to keep one annotation per source entry. The former remains the owner-bound
result-returning form; completion discards that result. This one comment-only companion-header
edit is needed beyond the assigned two files. No runtime consumer edit was necessary.

## Source context

Cached FileSelect_Controls dossier evidence ev_239a8b751587ea915e904f464857f5f846c6e0499ed69c43fde7018f5a9650d6
and exact PE bytes establish the contracts. SDK GetDialogStat and CmnAdvaModeProcedure names,
overrides FileSelect_Controls/MenuDialog_Req, and existing prepare/apply bindings are retained.
No SDK name was found for the warning entries, so their annotations retain the existing
symbol database FUN names; semantic C++ identifiers describe their role.

At 5038fa/50397c signed short comparisons use 5; 50392c pushes dialog 17. After the request,
503952 loads the live language, then 503958 reads its descriptor from table 10d79c4;
503960 pushes level 7, 503962 calls 421ab0, and 50396a publishes the task result.
The descriptor is structured data, not a fabricated warning string. The creation callee
copies eight words from it and can return null if task allocation fails. 421bd0 consumes
a task pointer, returns integer 0/1, and requests close through its work; completion ignores
that return and retains its warning handle. These remain genuine unrecovered STUBs.
Native TitleScene still throws native_save_warning_message/native_save_warning_close.
Seven recovered dialog/transition/save callbacks remain intact. No warning text or assets added.

Success still transitions before reading the selected-file owner. Failure still clears
phase, ready, exec and alert before close. Callback tests establish publication/failure
prefixes at request/create/selection/close/transition/prepare/apply boundaries. Plain integer
arithmetic is retained; no FP arithmetic was rewritten. No missing result was converted to
success. The existing probe ABI is byte-identical.

## Qualification

qualification.json records source/probe/judge hashes, actual statuses, full-output digests,
archive identity, checks and all mutant results. inherited-control-partials.json records all
938 inherited FP partial identities with exact inputs and differences; none are new.
Raw reports, complete per-case expected/observed outputs and PE disassembly stay locally in
build/save-completion-packet, with digests here. qualify.py observes unchanged judges through
Python tracing; it changes neither vectors nor comparisons. Original direct dedicated judge
also ran before edits: 1000 cases, exit 0. The later traced replay has identical outputs.

* Dedicated completion: unchanged default seed/cases, 1000 exact, baseline/current exit 0.
* Actual capture: 1000 exact each, exit 0; this is additional qualification, not completion.
* Controls: 2438 cases, 938 FP partials, no value mismatches, baseline/current exit 1.
  Every output word and every partial identity is identical. This consumer baseline was
  compiled after the naming edit from the base header; companion hashes document that timing
  and correct the protected report's unavoidable current-checkout source metadata.
* Unchanged actual save-creation and file-select-controls units exit 0. New callback unit
  exits 0. Native title tasks compile at C++17 O2 with normal strict flags.
* Five valid compiled mutants caught: three by the unchanged completion judge (125, 125,
  211 mismatches; exit 1) and two by callback assertions (SIGABRT, subprocess return -6).
* Provenance: 0 errors. Readability: 0 regressions; baseline untouched. Scoped profile:
  0 unhandled overlaps. This scans packet reports, not the full integrated build.

All bounded builds were serial and local. Explicit completion/control/native-title-task
objects precede the only borrowed immutable archive, SHA256
bfcaf022672ca30c74fac35b7649a2f837bba5f533a740886de2fab13e216c29 (source a9ed5bb).
No live Main library or build output was used or written. No full build/gate/journey,
provider, storage, network or public push operation was run. No native closure claim.

## Root integration

Fresh independent STYLE review and the final fresh-seed holdout are RESERVED for Root.
After source integration, register tests/unit/save_completion_callbacks.cpp as
save-completion-callbacks in tests/unit/manifest.tsv and regenerate the existing unit target
registration as required. Register the new protected test in a separate evidenced judge
step; no existing harness, expectations, inputs, summary or manifest was changed here.
No constexpr table registration is needed for the two scalar constants. Root owns any
inventory reconciliation, integrated full gate, clean native closure, journeys, STATUS and
HANDOFF. Do not classify the full save lifecycle, UI system or complete campaign as closed.
