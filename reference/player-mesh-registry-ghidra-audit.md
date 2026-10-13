# Retrospective Ghidra audit: native mesh registry

The retained Ghidra dossiers confirm the frozen source reconstruction against a
second static provider. Canonical procedure addresses, rather than file labels,
select the scope: this capture contains12 function dossiers plus two storage-xref
records. For example, `mesh-buckets-initialize` is actually4510e0, the pool-link
caller; it is not4518d0. The ledger retains genuine Evidence IDs, source/profile
hashes and complete provider limitations.

One concrete omission was found and corrected: initializer451030 returns EAX=1
at4510cb after linking both pools. Native `initialize_mesh_registry` now retains
that completed outcome. The probe measures the original initializer EAX directly;
1,068 comparisons match it along with prior compared pool/geometry/array/frame/CW
facts. The504 status-only differences remain, and strict equivalence is false.

Confirmed semantics include256/16,384 physical slots with255/16,383 usable slots,
terminal sentinels, complemented registry-head backlinks, whole-node-chain
recycling, ascending bucket joins, child-first object traversal with Z/Y/X low16
transforms, exact strip/material and binary32 geometry stores, forced-object
refresh behavior and retained partial geometry on node exhaustion.

The storage xrefs confirm separate ownership: dynamic source records3b32728,
and combined mutable work records3b2f720. Registration and broadphase both produce
the latter;4511d0 consumes it. The earlier missing-static-tail interpretation was
superseded and no false repair remains.

Ghidra's inferred signatures do not resolve custom register arguments, and its
pseudocode float10/float/type propagation can reorder or obscure x87 operations.
Exact bytes and instruction comparisons remain numeric/ABI authority. The capture
contains one caller-asserted callback seed; unresolved targetless computed flows
remain unknown. This audit does not prove physical Intel behavior, updated-mod
runtime state, stage startup, complete position composition or unmasked traps.
The direct451910 return is source-derived and retained natively; current profiles
compare its link stores while the source master ignores that return.

| Source address | Retained record | Genuine Evidence ID |
| --- | --- | --- |
| `0x43b580` | [land-registration-alternate](ghidra-engine-recovery/land-registration-alternate.json) | `ev_dd5fcb92d4bde84c124f74ea325d68385d031152af9c21c5f50fc5217485e767` |
| `0x4510e0` | [mesh-buckets-initialize](ghidra-engine-recovery/mesh-buckets-initialize.json) | `ev_9ff61abd89c3af9b75c0c33ef5ff887dfd597e08955b80665ecc493e12b55032` |
| `0x451790` | [mesh-group-create](ghidra-engine-recovery/mesh-group-create.json) | `ev_54dc809a38b0e187d297ac80fbb769fa258a687c20030d2c5bd0d979c4a11b2f` |
| `0x451860` | [mesh-group-recycle](ghidra-engine-recovery/mesh-group-recycle.json) | `ev_2902194fb1aa6b167a5e72c674822df22baccc0a26d923295de196f578082c5d` |
| `0x451800` | [mesh-group-remove](ghidra-engine-recovery/mesh-group-remove.json) | `ev_e1491423c10e0899c98c7674b425a8e2389c55d2a60f25c85241c124015e041b` |
| `0x451910` | [mesh-node-order](ghidra-engine-recovery/mesh-node-order.json) | `ev_1fba0581c6a8239b43508ad950a2b6c812cb9692ce5d6990f5d1799bae8c7302` |
| `0x451a00` | [mesh-object-traverse](ghidra-engine-recovery/mesh-object-traverse.json) | `ev_6aa6b76b8c9942ec54532ac81b68ccaded35f7d67f6ef2a810d9279c3367c79a` |
| `0x451030` | [mesh-pools-initialize](ghidra-engine-recovery/mesh-pools-initialize.json) | `ev_4f28b63e571e4fc0fe84bfffa0cd80e8478a781069cfdff022de79883d6ca2fc` |
| `0x451110` | [mesh-pools-reset](ghidra-engine-recovery/mesh-pools-reset.json) | `ev_0f3a2802e966be82480c1921b3ee766c08a674d428d3610a4562037da12d81b4` |
| `0x451160` | [mesh-refresh-reset](ghidra-engine-recovery/mesh-refresh-reset.json) | `ev_4fdbc0c1d60f30e02dfa5e50e63985c55da84aa28bd461b611aa41d78af947a6` |
| `0x4511d0` | [mesh-registry-refresh](ghidra-engine-recovery/mesh-registry-refresh.json) | `ev_60ee3f5e3321363ff3f3239edede0ea61fd964a79693332d5e183d0159faa94d` |
| `0x3b2f720` | [xrefs-0x3b2f720](ghidra-engine-recovery/xrefs-0x3b2f720.json) | `ev_ed8f8c55aa8ad0d69863a1608500c0df158bdeb8d3066c9a7fd49090e9f86e1f` |
| `0x3b32728` | [xrefs-0x3b32728](ghidra-engine-recovery/xrefs-0x3b32728.json) | `ev_42292f5711c6e0951345e88fd2abb6545e8d9f9a555e7a12956a90d20391b1fc` |
| `0x451ad0` | [meshset-traverse](ghidra-engine-recovery/meshset-traverse.json) | `ev_f41a9e9174b06cb71522dbd691be044cf05ff2eda23afb8e457e2db8ac1ab75e` |
