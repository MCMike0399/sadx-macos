# Recovered SADX task dispatch prerequisite

Date: 2026-10-11. Implemented files are `sadx-native/src/task.hpp`,
`sadx-native/src/task.cpp`, and `sadx-native/tests/task.cpp`. No Makefile or
other project files were changed by this task.

The input remains `Sonic Adventure DX/sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
This is reconstructed native source for dispatch semantics, not original-source
recovery and not a functioning game.

## Instruction observations

Primary local instruction records supplied by the parent investigation:

- `_reference/sadx/task-loop.asm`: function VA `0x40b0c0`, RunObjectIndex
  candidate name. The loader declaration provides a name candidate, while the
  shipped bytes determine behavior.
- `_reference/sadx/task-children.asm`: function VA `0x40b420`, LoopTaskC candidate.
- `_reference/sadx/sonic-main.evidence.json`: parent reports actual Hopper engine
  recovery of Sonic_Main. This implementation uses the above two instruction
  records directly; it does not claim that its scheduler tests execute Hopper
  pseudocode or prove the larger Sonic_Main implementation.

Current genuine REA records have now been independently inspected from their
`structuredContent` rather than inferred from filenames:

| Record | Procedure | Evidence ID |
| --- | --- | --- |
| `_reference/sadx/gameplay/task-list.json` | `0x40b0c0` | `ev_1dea238db9632dee9c5b8e539f25da2748078df75ae5969d53ba9a1d2ca77288` |
| `_reference/sadx/gameplay/task-children.json` | `0x40b420` | `ev_63b2ea8c4785069aa788d88cb6d66b417cfbe901eb319b78ce962e0767981628` |
| `_reference/sadx/gameplay/task-free.json` | `0x40b6c0` | `ev_35502b256d89e3fdbc93c24fb1d2711375f57423f282bf6b4d7455e4cef9263a` |

All three records bind the executable SHA-256 above, operation
`analyze_function`, provider `hopper`, and analysis-profile digest
`8c226db6a8bd36b662dc1cb83ab0ad0af698230fc322b58c6c4870878bf6ae70`.
Hopper launcher identity is
`launcher-sha256:dad6f886261fad0d4cdd01aee6fcf57439c60d7b5c54e62fc97f0180116690e0`.
Their pseudocode/assembly corroborate the list alias, callback/next ordering,
global context handling, unconditional child indirect call and FreeTask exec
replacement described below. These are static observations, not runtime
execution evidence. Each dossier explicitly limits complete function body
range reporting, detailed reference flags, external/thunk classification and
unresolved-call coverage through Hopper's public Python API. Pseudocode and
assembly are provider representations, not original source.

RunObjectIndex changes input index 8 to 7 and otherwise indexes the eight list
heads at guest global `0x3abdbc4`. It reads `next` at node offset zero and
`exec` at offset `0x10` before callback invocation. It stores exec into
`0x3abdbf4`. If exec is nonzero it stores offset `0x30` into `0x3abdbf8`, calls
exec, then clears the latter global. It clears current exec after every node,
including a null-exec node. Empty lists return without changing either global.
The self-linked-next branch logs a diagnostic and continues; it does not
provide a termination guard. This host API constructs acyclic native lists and
does not accept corrupted guest list links or claim to reproduce that path.

LoopTaskC begins at parent offset `0x0c`, walks immediate child siblings and
captures each next before invoking exec. It sets and clears `0x3abdbf8` around
each callback. It does not set or restore `0x3abdbf4` and does not automatically
walk grandchildren. It invokes exec without a null check, so a zero child exec
is an invalid indirect call in x86; the native API models it as the explicit
tagged `null_child_callback` failure, with the failing task retained.

Neither dispatcher restores the caller's context after nested dispatch. A
child sees its top-level parent's current-function context. If that child runs
a nonempty top-level list, that nested loop clears current_function and the
outer child walker leaves it cleared. Nonempty child walks also clear field30
instead of restoring the parent resource. Empty nested walks preserve the
existing context because no loop body executes. These cases have distinct
tests.

## Ownership evidence and native boundary

Additional read-only disassembly was justified by self-deletion/lifetime needs:

```sh
xcrun llvm-objdump --disassemble --start-address=0x40afb0 \
  --stop-address=0x40b030 "Sonic Adventure DX/sonic.exe"
xcrun llvm-objdump --disassemble --start-address=0x40af70 \
  --stop-address=0x40afb0 "Sonic Adventure DX/sonic.exe"
xcrun llvm-objdump --disassemble --start-address=0x40b570 \
  --stop-address=0x40b880 "Sonic Adventure DX/sonic.exe"
```

FreeTask candidate `0x40b6c0` calls active-list membership search `0x40afb0`,
then replaces the task's exec with `0x40b570`. It does not immediately unlink or
reclaim the task. Membership search traverses the eight lists and child trees,
using recursive helper `0x40af70`. The native `free_task()` preserves acceptance
only for a live member of this scheduler and schedules a native retirement
callback for that member's next dispatch. Repeated marking of a live task is
accepted. No additional cancellation or replacement mode was invented.

The original `0x40b570` calls destruction callback at offset `0x18`, clears
exec/display/destructor, processes children, unlinks sibling/parent/list links,
cleans work resources through game functions and puts the task node on a
freelist. Complete destruction-callback, work-allocator, thread/field30 and
FreeTaskC semantics are NOT reconstructed here. The native scheduled callback
performs only the explicitly documented host lifetime operation; full guest
DeleteObjectMaster equivalence is unverified.

Native ownership is deliberately separate: scheduler storage owns shared task
handles; links are private native pointers and cannot be populated with guest32
addresses. Callback arguments are shared native handles. Each field30 is an
opaque owned `std::shared_ptr<void>` resource, without a guessed guest layout.
Host composition helpers append linked tasks/children; original task creation
and insertion semantics are not claimed. Retirement unlinks a native subtree.
Retired scheduler ownership is released after the outermost walker exits;
retained caller handles keep node storage/resources alive until they drop them.
This makes callback self-retirement and pending-next captures memory-safe.

If a callback directly retires the saved next sibling, the dispatcher returns
`deleted_snapshot_next` with a retained handle. It does not silently skip the
node, dereference freed memory, or model the original freelist's unsafe/recycled
node traversal as established behavior. This is an explicit native failure
boundary. Native callback exceptions propagate after releasing callback context
and unwinding deferred reclamation; x86 exception equivalence is not claimed.

Expected failures are represented by `TaskFailure`/`TaskResult`: invalid index,
foreign/null handle, retired handle, null child exec and deleted saved next.
Only native function-pointer callbacks are accepted by the typed API; it does
not resolve unknown guest callbacks or infer that an unmapped address is null.

## Verification performed

Optimized C++17 arm64 compilation and execution passed. AddressSanitizer and
UndefinedBehaviorSanitizer compilation/execution also passed, including the
final empty-nested-walker cases:

```sh
xcrun clang++ -std=c++17 -Wall -Wextra -Werror -O2 -Isadx-native/src \
  sadx-native/src/task.cpp sadx-native/tests/task.cpp \
  -o /private/tmp/sadx-port-sources/task_test
/private/tmp/sadx-port-sources/task_test
xcrun clang++ -std=c++17 -Wall -Wextra -Werror -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer -Isadx-native/src \
  sadx-native/src/task.cpp sadx-native/tests/task.cpp \
  -o /private/tmp/sadx-port-sources/task_test_sanitized
/private/tmp/sadx-port-sources/task_test_sanitized
```

Tests cover eight independent lists and index-8 alias; invalid indexes;
top-level null skip; self-retirement and captured-next execution; delayed
FreeTask scheduling/repeated marking; next-exec mutation; tail insertion after
an already captured null versus insertion visible to a later sibling; explicit
failure when saved next is retired; child self-retirement; no automatic
grandchild traversal; parent/current-function context in child callbacks;
nested nonrestore and empty-walk context cases; null child failure;
foreign/retired ownership rejection; native callback exception cleanup;
resource lifetime retained by handles and released by last ownership;
native subtree unlinking.

These source-level tests do not compare traces against running original x86
task functions and do not establish real-provider gameplay behavior.

## Remaining requirements

This provides only the dispatch prerequisite for the requested native PC SADX
port. Complete task allocation/destruction, actual gameplay callback recovery,
startup loop integration, character physics/collision, stage lifecycle, native
GPU rendering, sound, saves, speedrun mod integration and Xbox/Switch 2 Pro
controller verification remain unfinished. No native game readiness or
speedrun acceptance claim follows from passing dispatcher tests.
