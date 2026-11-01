# Loader boundary review at Main 73be9a5

Reviewed all six tracked source bodies (263 lines), grouped into three complete
basename modules. The JSON fragment carries exact source hashes, tracked evidence
anchors, owners, actual consumers, accepted judge paths and qualification limits.

## Findings and dispositions

1. **P2, LB-1: unchecked relocation entry.** In
   `src/loader/executable_image.cpp:59–61`, `rebased` checks a block against the
   declared directory size but dereferences `le(mapped, e, 2)` without checking
   whether the entry exists in mapped bytes. A readable final eight-byte block
   header can declare a ten-byte block whose entry falls beyond `SizeOfImage`.
   That reaches an empty optional dereference instead of the explicit malformed
   image result. This is static reasoning; no malformed input was executed.
2. **Readability work remains.** Compact declarations and multi-statement control
   flow are maintained-source debt even in justified reference infrastructure.
   The purpose dispositions below grant no code-conformity exemption.

| Complete module | Members | Existing disposition | Actual boundary |
| --- | ---: | --- | --- |
| `src/loader/executable_image` | 2 | `reference_infrastructure` | PE image bytes, guest VAs, exports/imports and rebasing |
| `src/loader/guest_heap` | 2 | `reference_infrastructure` | Synthetic reference heap tokens, blocks and byte availability |
| `src/loader/mpeg_timing` | 2 | `reference_infrastructure` | DirectShow HLE metadata duration assumption |

All six member files receive explicit matching purpose dispositions in the JSON.
There are **zero already-conforming files or closed conforming modules** in this
packet. These classifications neither excuse the P2 nor close readability work.
None is obsolete: each has maintained consumers.

## Boundary conclusions

`mpeg1_timing` is called by `HleDshow` AddSourceFilter. Its result supplies graph
position/duration and EC_COMPLETE timing, with the uncaptured Windows splitter
policy explicitly reported as an assumption. Native movie playback instead uses
AVFoundation samples, audio presentation frames and track end. Movie import
copies/hashes MPEG bytes and extracts content through Python PEImage; it does
not call this C++ helper. MPEG tags, rate fractions and masks are format values.

`ExecutableImage` owns immutable load-time data; guest execution memory is a
separate mutable copy. Its guest locations are established through actual mapping,
IAT publication and export consumers. PE signatures and relocation/ordinal masks
are legitimate encodings. `game/model_dlls` and reference initialization can still
carry address-shaped game/fixture concerns; their placement does not make them
conforming, and this packet does not disposition those callers.

`GuestHeap` is owned by NativeGame reference composition, with a separate CRT
instance. It does not own the Ninja heap-handle source global. Unwritten optional
bytes preserve indeterminate reference data; failed allocation and unmapped reads
are boundary absence. Lazy source heap creation and CD/system allocation policies
remain in engine/game callers. Actual x86 HLE allocation uses HleMemory separately.
Synthetic token/base choices are not captured Windows address identities.

## Evidence limits

The existing menu-texture-owner gate receipt records exit 0 and matching hashes
for all six source files. Its program list and the tracked unit manifest support
historical coverage attribution only. The JSON names each accepted unit and its
bounded assertions; no fresh pass, parser completeness, Windows parity, codec,
hardware, native runtime or campaign verdict is inferred.

No builds, tests, oracles, mutations, providers, network, storage operations,
delegates or gates were run. Only this authored review and its JSON fragment were
written. Root reconciliation of policy/shards remains separate.
