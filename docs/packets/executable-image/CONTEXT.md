# Bounded executable image packet

Base Main: 1300815. Assigned worktree only; packet/executable-image.
Review source: cbdf7a3, loader-boundaries.md/json in read-only t3-d1410585.
Both source hashes match the review. LB-1 is an unchecked empty optional at the
first relocation entry after a final eight-byte mapped block header.

Scope is the complete executable_image.hpp/cpp value module. ExecutableImage
owns immutable file/mapped bytes, section metadata and exact named exports.
X86Guest owns a separate mutable execution copy; HleModules parses/rebases DLLs,
HleRegistry consumes imports and publishes IAT addresses, ModelDllSet consumes
exports and rebased data, and reference configuration borrows shared const images.
The native executable-independent archive does not require this module.

The source contract is PE32 i386 only: header/section mapping, max(raw, virtual)
section extent, zero tails, case-sensitive named exports and forwarder strings,
OriginalFirstThunk fallback, ordinal imports, HIGHLOW and ABSOLUTE relocations.
Overlapping sections retain first-section byte lookup / later-copy rebase order.
Strings retain the 4096-byte bound. Empty import size retains mapped terminator
traversal. Rebased snapshots retain the existing one-shot relocation metadata
behavior. PE encodings and guest image VAs are legitimate boundary values.
No additional PE variants, Windows loader parity, gameplay or module exemption.

Read AGENTS, STYLE, GOAL, JUDGE, TESTING, STATUS and latest HANDOFF. The fresh
baseline must execute unchanged source before rewriting. Synthetic test bytes
are authored independently with literal expected outcomes, not parser helpers.
Malformed boundaries may change to explicit failure; all valid observations must
remain identical. Existing pinned judges remain unchanged; their asset-dependent
execution is pending if inputs are unavailable. No full gate, network, storage,
providers, delegates, Main writes or push. Root owns fresh review and integration.
The inherited judge manifest and all 53 raw corpora must survive any later union;
this worker will not refresh an incomplete filesystem baseline.
