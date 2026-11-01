# Independent GuestHeap complete-module review

Reviewed packet `f93b3a3` on `packet/guest-heap-readability` in
`/Users/burbujamc/Developer/sadx-wt-inventory-review-closure`, against immutable
`1300815`. Baseline `c6fc0a0`, source `4632671`, contract/registration `5d3807e`,
consumer receipt `406d2ba`. Review performed on 2026-11-01 in an isolated review
checkout. Candidate, Main, source, build artifacts and protected inputs were read only.

## Findings and disposition

Standards: no actionable findings. Spec: no actionable findings.

Approve bounded complete-module code conformance for both
`src/loader/guest_heap.hpp` and `src/loader/guest_heap.cpp`, within their synthetic
reference-heap purpose. This is not Windows allocator/address parity, native product
heap ownership, or conformance of the other Loader modules or the wider campaign.

## Complete source assessment

Read the entire old and new header/body, both unit sources, all four packet documents,
the six saved fault-source diffs, consumer source and recorded evidence. Applied
`docs/STYLE.md`, especially its independent review checklist, with the reference
purpose stated in CONTEXT as the scope rather than inventing source heap semantics.

The rewrite retains all public methods, defaults, const/noexcept qualifications and
the public Call kind/a/b layout and trace. It keeps private requested size separately
from backing size. Structured control flow, semantic local/member names and named
alignment/gap/maximum constants clarify the complete module without adding an
abstraction or moving behavior elsewhere.

Allocation retains foreign-handle rejection, requested-size trace, one unknown backed
byte for zero requests, uint64 span rounding and strict UINT32_MAX guard. It preserves
caller-supplied unaligned bases and a successful optional address zero. The allocation
RHS still constructs bytes before map publication under C++17 assignment sequencing;
the next-address store remains after publication and before the success trace.
No host-OOM fallback was introduced.

Creation still returns one reusable token, and allocation still works before creation.
Free records its receipt before handle validation and erases only an exact block base.
Read/write preserve upper_bound lookup, gap/end rejection and exact byte bounds.
Fresh bytes remain disengaged optionals; writing zero establishes a known zero.
No new Windows addresses, allocator reuse policy, guessed contents or product global
owner is asserted.

## Evidence and discrimination

Read-only hash/byte checks verified that the saved baseline header/body exactly equal
the immutable `1300815` Git blobs. The original guest_heap unit is unchanged, and the
candidate source/contract hashes agree with QUALIFICATION.

The pre-edit before/current streams are byte-identical: 32,660 rows, 755,704 bytes,
SHA256 `23f273ed1262368a36aa2643c9f9538b33a2617e1f518202777f27b3b97fa855`.
Both augmented and final before/current pairs are byte-identical: 32,665 rows,
755,791 bytes, SHA256
`5d5aa54629207f97ceb10e9d7f409eaadd2157ff1fe7b3e810b1ab0c61b12685`.
Recomputing FNV-1a gives `e8974e7380706609`, matching the default registered contract.
The explicit foreign-free augmentation was replayed after the rewrite against immutable
old source; it is not represented as part of the original pre-edit characterization.

The contract observes 20,480 operations across five configurations, every operation's
result/live count and all C/A/F kind/a/b fields. Presence bits separately identify
optional zero and absent results; read presence separately identifies unknown and
known zero. Explicit initial assertions exercise the one-byte backing, failed end
write and foreign free preserving the block. The cohort includes an exact-equality
upper guard, exhaustion, zero base/handle and an unaligned base. Raw normal scenarios
contain successful allocations, writes, frees and known reads, alongside failures.
No magnitude assumption about data words or narrowed old expectation was introduced.

The default test checks the frozen digest AND size; --capture emits raw observations.
Its assertions are active under the registered Makefile flags (no NDEBUG). The old
unit remains intact, and registration appends guest-heap-contract without removing
any existing row. The saved six valid faults independently alter create return,
gap, fresh contents, foreign-free validation, upper-guard equality and free tracing.
Their receipts report compile 0/run -6. The initially uncompilable foreign-free
attempt is explicitly excluded; its corrected source discards the parameter and
removes the actual guard. These are meaningful semantic faults, not compile failures
counted as catches.

All 943 base protected paths remain; the only changed old protected hash is the
append-only unit manifest. All 944 candidate protected inputs match their hashes,
including all 53 binary paths. Existing logs record original/current guest unit and
actual Ninja-memory/no-CD consumer success; their source contracts were inspected.

## Limits and remaining Root work

This reviewer executed no build, unit binary, oracle, mutation, gate, provider,
network/storage operation or delegate. Runtime exits are existing packet evidence,
not fresh reviewer executions. Static inspection and independent raw byte/hash
verification support this approval; they do not replace final integration checks.

Root retains the additional boundary cohort, reference composition/import consumers,
parent manifest integration and integrated gate. Main's gate at frozen `1300815`
was not touched or represented as complete. Host allocation failure, races, physical
allocator behavior and Windows parity remain unqualified. No gameplay progress or
whole Loader/campaign closure is claimed.
