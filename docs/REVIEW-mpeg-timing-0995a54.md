# Independent MPEG timing module review

Reviewed packet `0995a54545423758304de279e409f0a00da9f910` against
`a0aa3426e4dc2781a4917cd15b0322fee175bdd3`. Source revision is
`537691b0023ee769c76a5cd882ccdb29d0d86dd2`; later packet commits do not change
source. Review branch: `t3/review-mpeg-timing-module`, isolated worktree
`/Users/burbujamc/.t3/worktrees/sadx-macos/t3-facf06e4`.

No actionable findings. Both complete members, `src/loader/mpeg_timing.cpp`
and `src/loader/mpeg_timing.hpp`, conform within the reference metadata purpose
described below. This is a bounded complete-module code review, not integration
or campaign closure.

Read the complete old/new members, packet/context, baseline/current receipts,
judge dependency correction, qualification faults, restored-raw receipt, synthetic
fixture, contract unit, actual COM consumer unit, Make rules, registrations,
protected manifest changes and unchanged HleDshow consumer. Applied `docs/STYLE.md`
including section 12 and the supplied preservation specification.

## Standards

The compact walker becomes structured, named packet/timestamp/sequence steps.
Internal helpers remain private; added steps carry NOTSA explanations. No game
behavior moves to a reference adapter, and no guest address, register dispatch,
fixture absence or executable constant enters the module. MPEG tags, masks,
timestamp shifts and rate fractions are legitimate format encodings. The local
FrameRate record and fixed table expose the existing fractions without adding a
policy, registry or hierarchy. No unrecovered callee or silently skipped native
behavior is introduced. Integer arithmetic remains plain, explicit and exact.

The reference-infrastructure purpose is justified by the actual sole runtime
consumer, HleDshow AddSourceFilter. It does not exempt the module from readable
source requirements. Native AVFoundation playback has a separate responsibility.
The public header now explicitly states the uncaptured splitter assumption.

## Spec and fidelity

The public function, aggregate field order, field types and defaults are
preserved. There is no frame count: video_packets_with_pts counts qualifying
E0 packets. Timestamp extraction, post-increment count and min/max updates keep
their values and order; timestamps are not unwrapped. Duration retains separate
integer truncation of the PTS span and frame period.

The full control-flow comparison preserves fixed pack skipping, marker-nibble
rejection, E0 filtering, stuffing and STD skipping, permissive PTS marker bits,
and the ten-byte PTS+DTS advance even with truncated DTS. Sequence scanning
starts at the same payload offset, accepts the same first usable dimensions/rate,
stops this packet on an invalid rate and retries later packets. Zero dimensions
and streams with no pictures remain accepted. Short tails and end-code stopping
are unchanged. Every failure string, byte offset and failure precedence matches;
null error pointers remain valid and successful calls leave error text untouched.

The actual consumer remains byte-identical to base. The added test invokes its
registered COM traps for eight rates, exact duration/position, completion boundary,
one-shot event and parameters, clamp, Pause/Stop calls, reload position reset,
failure prefix and reported assumption. Pause/Stop coverage here is return/call
coverage at completed playback; this is not a general pause/resume qualification.
The test does not repeat completion after reload. Neither limitation changes the
bounded parser-preservation verdict.

## Evidence checked without execution

Context, baseline and current source hashes match their Git blobs. All three
added judge files match their frozen fd24501 versions and current receipt hashes.
The existing MPEG unit and HleDshow source are unchanged. Baseline precedes source
in commit history; the pinned old-source consumer replay is correctly described
as retrospective, following concurrent support-archive changes.

Read the candidate's stored captures directly. Before/current synthetic streams
are byte-identical: 30,657 rows, 7,774,713 bytes, SHA-256
`922d4562bb7b952b72778050d651dec4c1b00c7a6095ff243972191ace51ad00`.
Initial/current, pinned old/current and registered COM transcripts are all
byte-identical: 10,873 bytes, SHA-256
`da050b4379703ec05c9db6720936290d8de07855a90e1d5da8a6b66a0d535543`.
Both pinned support archive hashes match the current receipt.

Each of ten fault substitutions matches exactly once in candidate source and
reproduces its recorded source hash in memory. Each stored fault capture matches
its hash, has all 30,657 rows, and agrees with the receipt's full-row difference
count and exact first divergence. Receipts record compile/capture/contract exits
0/0/1 respectively; no compile failure is treated as discrimination. Sanitizer
exit 0 and unchanged real-asset unit output are prior receipt evidence only.
The existing real-asset unit prints metadata; it does not assert the particular
3,206-packet, 640x480, 107540 ms result.

The corrected consumer Make rule explicitly compiles current MPEG and HleDshow
sources before support archives and lists both sources plus parser header as
prerequisites. Contract and fixture dependencies and both no-argument manifest
rows are present. Both inherited registration files retain their exact byte
prefixes. No old judge input, expectation, fixture or tolerance is weakened.

The final manifest retains all 945 base paths and contains 947 paths, including
the protected fault receipt. Only inherited Makefile and unit-manifest hashes
change. The protected fault JSON is byte-identical to qualification-faults.json
and matches its manifest hash. All 53 restored-raw receipt entries agree with
the final manifest. None of those raw files exists in this review checkout;
there is no local full protected-input hash-pass claim. Earlier 946-path packet
statements describe the state before Root's 0995a54 fault-receipt addition.

## Limits and disposition

Approve complete-module STYLE/spec preservation for these two source members as
reference HLE metadata infrastructure. No builds, tests, oracles, mutation runs,
sanitizers, providers, network, storage services, delegates or gates were run by
this reviewer. Only this review is authored and committed in the isolated branch;
candidate/Main sources, builds and the frozen 99bdaf4 gate remain untouched.

Root final cohort, inventory reconciliation, integration, parent-preserving union,
combined gate and native/reference journeys remain pending. This review makes
no Windows splitter/codec, native movie clock, whole Loader or campaign claim.
