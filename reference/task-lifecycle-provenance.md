# Native task lifecycle foundation

The shipped target is `Sonic Adventure DX/sonic.exe`, SHA256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`. This slice reconstructs native task/work ownership and local scheduling. A playable native game, complete startup, gameplay callback registration and mod lifecycle are still unfinished.

## Evidence and scope

Observed facts come from the retained genuine REA dossiers and exact LLVM instruction listings `task-lifecycle/task-alloc.asm` and `task-lifecycle/task-lifetime.asm`. Hopper pseudocode is a derived representation: its temporary variables around the arena freelist are not accepted as original source.

| Entry | Retained dossier | Genuine Evidence ID |
|---|---|---|
| 0x40b030 | [allocate-task.json](task-lifecycle/allocate-task.json) | `ev_889e04aafa2e1b86b26745aa0b0a5c269f410caeaac06ceeb5b963793724027a` |
| 0x40b220 | [allocate-work.json](task-lifecycle/allocate-work.json) | `ev_5de04160f0311904a68804890cc194c635f9973514a357595df7a29a90afc1fe` |
| 0x40b940 | [create-child-task.json](task-lifecycle/create-child-task.json) | `ev_ad163be5d84214cfd9d028114a4601cdba557631a009fe605567154f3631fce5` |
| 0x40b860 | [create-elemental-task.json](task-lifecycle/create-elemental-task.json) | `ev_8a7fc2a8a914f4d2f277bdd48e020e0cdacbf391e98b9ae0d372390ec3b9069a` |
| 0x40b570 | [destroy-task.json](task-lifecycle/destroy-task.json) | `ev_60d0da7501f9059dccf6c42b8a52c7bb9f2e00ba6696d20c5da55cf8a5f8beb9` |
| 0x40b7e0 | [free-children.json](task-lifecycle/free-children.json) | `ev_fdfe9b5271c5b322d6462f719e0139e815c000e71290580e8a85c7baf6e2c7b7` |
| 0x40b310 | [free-work.json](task-lifecycle/free-work.json) | `ev_3ae947f26dfa1b89936c5ff5c7cc6f93beecc1e37c8c3b843e4844f430a297de` |
| 0x41c4e0 | [collision-work-free.json](task-cleanup-dependencies/collision-work-free.json) | `ev_e2f4df500926677658ae3a9705b95b105249aa7b05abef03b4f9b75c256ed3be` |
| 0x4187a0 | [object-work-free.json](task-cleanup-dependencies/object-work-free.json) | `ev_b17f3640f2d8b7ef67ba2a31126817aba9c982ff9432c18cf7499efa446880d7` |
| 0x40af70 | [task-membership-recursive.json](task-cleanup-dependencies/task-membership-recursive.json) | `ev_a8d604a53e43b7890e5466a4c96a0e7e6c4a55e3f9317bc69778a68ff7f735e8` |
| 0x40b1a0 | [work-arena-initialize.json](task-cleanup-dependencies/work-arena-initialize.json) | `ev_15babfca2a405f944a42e8c2cdd4504fa487d788c66ecaca237ed800a6c73c09` |
| 0x417ea0 | [object-unlink-417ea0.json](object-cleanup-dependencies/object-unlink-417ea0.json) | `ev_c4f39ba3873b02de78473d18e834aff17f57bf2edeb3f02e31455f10e9230e02` |
| 0x418330 | [object-unlink-418330.json](object-cleanup-dependencies/object-unlink-418330.json) | `ev_67e686d667c6e539477608656b7bb26174aa4b3f6089c2ecc2ad56d6922e3823` |

## Recovered semantics

- `0x40b1a0` acquires a private heap with `HeapCreate(0,0,0)` then `HeapAlloc(heap,8,0x1000020)`. The exact PE IAT entries are `0x7db078=KERNEL32!HeapCreate` and `0x7db074=KERNEL32!HeapAlloc`. Native storage has the same 16 MiB usable span and 32-byte sentinel. Source REP STOSD fills the first 16 MiB with FF; the sentinel tail remains zero. CRT initialization and the actual Win32 acquisition are outside this native owner.
- Work allocation `0x40b220` uses free-list first-fit and total span `(requested+63)&~31`, including a 32-byte header. Allocation preserves existing payload and padding. Exact-fit removal, split replacement, all coalescing cases, sentinel termination and exhausted/zero-size behavior are source-bound. Released handles remain distinct from reused allocations. Overflow and invalid/double frees are explicit native failures; the source can hang or corrupt its arena.
- Native header links are optional arena offsets, with zero offset distinct from a null link. They never become host pointers. An optional source base is only a byte-provenance fixture: absent provenance marks nonnull pointer bytes unknown, and `read` rejects those bytes. Native `set_reference` owns typed work references for source pointer fields; raw nonzero words without a binding fail explicitly.
- Pool initialization begins at `0x40b466`, after the unlifted `0x51a740` call, and reserves actual `0x34000` work bytes for 4096 slots. It shares the work arena and scheduler; there is no second scheduler. Native task graph/link/resource representation replaces the guest task pointer fields. The pool allocation remains owned for the scheduler lifetime.
- `AllocateTask(0..6)` prepends to its corresponding list. Level 8 prepends to list 7; level 7 is a detached child candidate. Source level 7 retains the popped free node’s old next pointer, represented as a detached free-slot fact rather than an active-list edge. Null source callback becomes the proven one-byte RET at `0x51f680`; it is a real native no-op, not an unknown callback substitution.
- Elemental creation `0x40b860` examines the low flags byte in source order: bit2 allocates/zeroes64B primary work, bit4 allocates/zeroes56B force work, bit1 allocates/zeroes64B motion work, bit8 allocates/zeroes16B auxiliary work. Only requested bytes are zeroed. Context field30 wins when nonnull; otherwise the caller must supply the observed default-global field30 fact, including explicit null. Unknown default stops after existing allocations, retaining the task and work.
- Child creation `0x40b940` uses a level7 element then prepends under the given parent. If both primary works exist, raw position words at20h/24h/28h are copied before raw angle words at14h/18h/1ch. The parent’s field30 replaces creation context. NaN payloads and angle bits are copied without numeric conversion.
- Membership follows the same eight native lists and descendants. Detached tasks are not members. An inactive original membership lookup enters `0x401000` for diagnostic CRT formatting; native inactive results stop at that exact boundary. Logging side effects and a completed original return are not claimed.
- FreeTask `0x40b6c0` schedules the real destructor by replacing exec. Destruction invokes the destructor callback first, snapshots sibling links afterwards, clears exec/display/destructor, frees children, then unlinks before work cleanup. Cleanup failure retains the unlinked task and work. Explicit `resume_destruction` continues native ownership from the retained stage; it is not another original DestroyTask call, which would reject an unlinked node.
- Actual collision cleanup `0x41c4e0` checks resource byte+5 bit80, frees an optional owned nested block at+c, frees the collision block, and clears primary+38. Actual object cleanup `0x4187a0` follows `0x417ea0`’s +40 chain via node+10, optional +44 block and clears +3c, then `0x418330`’s +4c chain via node+c and clears +4c/+48, before freeing the object and clearing primary+3c. Every pointer requires a native owned reference. Missing or released references preserve partial frees and fail at their actual source read/call site.
- Work cleanup then frees primary, force, motion, auxiliary in source order. The object-control linkage clears task+4 and flags+2 bit0 without pretending the object-control resource itself was freed. Node recycling preserves old field30 until allocation. New reuse receives a new native Task identity/generation; old handles remain safe and retired.
- Ordinary FreeChildren destroys active children. The special exec=FreeChildren case dispatches direct children and schedules parent destruction only after its child list becomes empty. The source inactive multi-child branch can make self-linked nodes and loop forever; native code reports the unsupported corruption instead of corrupting the graph.
- Dispatch snapshots next before the callback and preserves non-restoring context semantics. A reused saved slot resolves to its new live native generation. An unreused freed snapshot is rejected; a slot reused as detached level7 is rejected at the unsupported free-list next read (`0x40b0e0` top-level, `0x40b433` child). There is no fabricated successor.
- Host append remains an explicit host composition operation. Host retirement cannot discard a source pool task or a mixed subtree containing one. Successful source retirement requires the reconstructed destructor and required cleanup ownership.

## Original instruction comparisons

- [original-task-work-comparison.json](original-task-work-comparison.json): 16 sequences, 4,890 operations, all source return/head traces and arena/graph/work bytes match for its declared fixtures. Whole arena span: 16 MiB+32. Report pins target and function-byte SHA256, fixture blob digest and actual native probe binary digest.
- [original-task-lifecycle-comparison.json](original-task-lifecycle-comparison.json): 17 sequences, 2,302 operations, all source return/head traces and arena/graph/work bytes match for its declared fixtures. Whole arena span: 16 MiB+32. Report pins target and function-byte SHA256, fixture blob digest and actual native probe binary digest.
- Inactive DestroyTask is a separate boundary fixture: exact original instructions stop on entry to401000; original arena remains unchanged, native arena/graph serialization matches, native failure points to401000, and task/work are retained. `original_return_observed=false`. This does not count as a complete cleanup success.

The work verifier executes the original initialization body from40b1ce with an explicit zeroed heap fixture and executes original40b220/40b310 without overlays. The lifecycle verifier executes the actual post-initializer pool body and actual creation, dispatch, FreeTask, FreeChildren and destruction. Only the retained default51f680 RET callback is used. One collision/object fixture seeds owned chain data explicitly, then executes all real cleanup instructions; those resource initialization producers are not claimed. Native pool objects contain owned references, so the test probe encodes their graph into its declared canonical source identity scheme; this serialization is test-only, not a runtime guest memory/interpreter layer.

The first arena comparison found an omitted source write to the removed next header at40b35e. Fixing that producing write made both header metadata and stale payload bytes match; no downstream byte substitution was added.

Unit checks include4096-slot exhaustion, real arena exhaustion, detached rollback with already-allocated primary work retained, raw child copy, real nested cleanup, partial chain cleanup continuation, actual ground-independent callback mutation, source slot reuse, deferred destruction, unsupported detached snapshot, explicit default-global omission/null, native ownership and invalid buffers. Strict optimized programs pass; task_work and task_lifecycle also pass ASan+UBSan with all their owned sources instrumented.

## Integration contract and remaining bounds

- Production module: `task_work` alongside existing `task`. Unit programs: existing `task`, plus `task_work` and `task_lifecycle`; no arguments/assets. Probes: `native_task_work_probe.cpp`, `native_task_lifecycle_probe.cpp`; reusable tools: `verify_original_task_work.py`, `verify_original_task_lifecycle.py`. Root owns Makefile/archive changes.
- No guest pointer cast, duplicate scheduler, runtime x86 interpreter, host math approximation or callback identity fallback is present. The original emulator is only an instruction comparison oracle.
- Unlifted51a740 initialization, CRT64fc70 initialization, OS heap acquisition/failure policy, diagnostic401000/CRT formatting, arbitrary game/mod callbacks, destructive callback reentrancy/corrupted graphs, real startup and actual hardware execution remain explicit limits. Native tagged failures for source hang/crash cases are a deliberate host boundary, not proof of original successful cleanup.
- Numeric FP context/precision is untouched by these integer/pointer routines; source callbacks may have their own explicit numeric owner. No hardware x87 equality claim is inferred from these tests. Full native PC gameplay/mod loading/Metal rendering/controller integration remains unfinished.

## Frozen producer source digests

| Source | SHA256 |
|---|---|
| `sadx-native/src/task.hpp` | `773d55499fb84a35e5dc745e15c8a1e210ced8521305b7eaeb0e9196d71f953b` |
| `sadx-native/src/task.cpp` | `71862253332ecad9fa8251f2dc5af474a9ae94b601bd54b63b64c813ee930d44` |
| `sadx-native/src/task_work.hpp` | `734036331df7c64f130c7482d1f593fdba7858407b02cc029246c5c93fdb134e` |
| `sadx-native/src/task_work.cpp` | `cae92a17ad3836e0666705cc148f635f434a3197f83d381012ebf785c269608c` |
| `sadx-native/tests/task_lifecycle.cpp` | `b5c31f8a55226ff11cc6e9298916779375cad13a776dfc782800b14e0b85abcb` |
| `sadx-native/tests/task_work.cpp` | `fbc1e0212f1fef7f14d756472a32643bb6078cf9d21c767795768e7e399a3b7c` |
| `sadx-native/tests/native_task_work_probe.cpp` | `9bd57dbde678a6c31843692f05a7f758bf935280bcc09586a1590fcda4fb84b7` |
| `sadx-native/tests/native_task_lifecycle_probe.cpp` | `256c37e79df9ef40a39d99eafbd5e2eaf9da3d59301e5e0728bf03cf45ac2e88` |
| `sadx-native/tools/verify_original_task_work.py` | `b1057b2ff375b54bb21848f5a557f5c1ff447a70fef6bfde04ae292435c36329` |
| `sadx-native/tools/verify_original_task_lifecycle.py` | `892844dee039506495577396fe77d1fea50490d75004e9cd7d6ca18cbb899aa1` |
