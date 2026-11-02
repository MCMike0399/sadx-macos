# Gamma setup source context

Target: pinned PC image SHA-256 6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
The nearest readable models are gamma_cursors and the existing sprite constructor.
The SDK's New header names SpLoopOnlyDisplay; the setup/global role names are
proposals, not claimed SDK names. See symbol-proposals.tsv and the inherited packet.

GammaLoadSetupResource, complete [4c5420,4c5436), has one caller, E102 [483430,4841cf).
It captures [982148], pushes zero extra bytes, calls RegisterSprite, then stores
EAX into [982154], even when NULL. There is no guard, release, caching or idempotence.
The preceding constructor packet proves [456c50,456ca1), including callback reentry,
wrapped count, ordered next/head/count/extra/definition stores and allocation failure.
Its real TaskWorkArena remains the allocation owner. The typed node owns pointer
fields; raw node header bytes are not synchronized for an unported raw consumer.

The source definition pointer initially points to 9820f8. Its first 20 bytes are:
texture list at 91c570 (E102EFFECT), NJS_TEXANIM table at 982058, count low half 8,
unused count high half 0, source blend 8, destination blend 10. SpLoopOnlyDisplay
loads both pointers and both blend words directly. Gamma's particle callback
loads the count as an unsigned half, compares its signed animation index, and
kills the particle at the last frame or when the existing game enable byte is zero.
The SDK NJS_TEXANIM is ten signed 16-bit fields, twenty bytes. Eight records end
exactly at the definition. Native code reuses sprite::AnimationFrame; these are
sprite data, not CharacterPack action/model records. No shared pack schema changes.

The trailing 60 bytes at 98210c are parameters. Gamma's spawn function publishes
that interior parameter pointer in the particle. Its callback consumes offsets
4 (scale growth), 8 (animation increment), 12 (drag), 16 (vertical acceleration).
The leading float and remaining ten floats are preserved without invented use.
Neither particle spawning nor updating is implemented by this packet.

The texture declaration has twelve entries and a source texture-name array in
zero-fill storage. Runtime handles must come from CharacterResources lists[6][2],
which is the existing E102EFFECT list loaded before setup. No second list or pixels
are constructed. Five existing Gamma material declarations and CharacterPack's
mutable shared material identities remain untouched.

SpLoopOnlyDisplay reads the registry head and node.definition. SpLoop cleanup
[456de0,456e46) frees each node allocation and clears head/count, without clearing
Gamma's result. Future native cleanup must mark those arena allocations released,
retain the source's stale Gamma result identity, and never treat typed node fields
as already synchronized raw headers. Particle publication/removal/pool/update and
renderer consumers remain explicit future work, not no-op implementations.

CAlloc [40b2d0,40b304) rounds the element size to a word, multiplies modulo 32 bits,
calls MAlloc, then clears extent/4 words on success. The production allocation
binding uses this existing recipe over the borrowed canonical arena; focused tests
exercise real allocation, zeroing, wrap, exhaustion and registry retention. The
original instruction comparator intercepts CAlloc; it does not requalify MAlloc.

Complete retained source summaries and hashes are in source-context.json. Local raw
REA envelopes remain in reference/gamma-setup-local/evidence. A parent snapshot
profile mismatch was retained; the successful owned import used a fresh snapshot
and GHIDRA_HEADLESS_MAXMEM=2G, then closed before the supplemental owned session.
