# Original normal/UV triangle strip core

Branch `packet/normal-uv-strip-core`, based on `04c40ae0b0840b9982cda95452057c1d678eeec1`.
Root owns runtime attachment and fresh independent review. No GPU, complete model,
CharacterSelect/N3b or full-gate claim is made by this packet.

## Ownership and source

The packet implements original `7835f0 polybuff_normal_uv_strip`, `794600
PolyBuff_SetCurrent`, `794630 PolyBuff_LockTriangleStrip`, `794750 PolyBuff_Unlock`,
`794760 PolyBuff_DrawTriangleStrip`, and the per-buffer initializer/free/discard
`794540`, `7945c0`, `794800`. It does not implement `78e720 InitPolyBuffers`;
that complete dossier supplies initialization facts for Root's composition.
`CorrectMaterial`, model/material/world composition, runtime, Display,
LogoRenderer, CharacterPack, object drawing and the Metal binding remain Root owned.
No loader replacement algorithm was used. The pinned vertex-color and E102 fixes
are off; the original mask, degenerates, signed UVs and order are preserved.

All authoritative full source paths, dossier SHA-256 hashes, evidence IDs and exact
body ranges are in
`reference/production-integration-20261026/normal-uv-strip-source.json`.
The immutable original executable SHA-256 is
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Dossiers are retained in Root's durable `sadx-wt-normal-uv-strip` worktree, not `/tmp`.
Semantic PolyBuff helper names come from pinned `SADXModLoader/direct3d.h`;
new symbol overrides record that declaration plus the full original assembly.
The classic strip header reverses points/normals; `77eda0` dispatch assembly
establishes `(mesh, points, normals)`, which this engine interface follows.

## Native integration contract

`src/engine/normal_uv_strip.hpp` exposes ordinary functions in `sadx::polybuff`.
Runtime defines its own opaque borrowed `Context`, `DeviceBuffer`, `BufferMemory`
and the declared link-time seams. It owns one `BufferState` for the original
normal/UV record (index 8), rather than synthesizing one per model draw.
The original record is 36 bytes at `3d0ff20`; native `BufferState` has native
pointers and a bounded borrowed argument view, and is deliberately not ABI packed.
Root must register the single global owner when binding it. Color and cull mode
are entry snapshots of the existing model renderer owners, passed by value.

Mesh and vector payloads borrow Root's existing imported pack. The mesh view carries
its actual strip count and signed16 index/UV ranges. Vectors expose the original
binary32 component words, so copies preserve NaN bits and signed zero without FP
loads. Range origins can be negative; no index is silently clamped or made unsigned.
An unavailable imported word throws `DataFault(kind,index)` exactly when consumed;
the reference judge maps it to the actual source read and retains all earlier stores.
Output storage is a separate `BufferMemory` owner; `VertexCursor` names that storage
plus a byte offset. A device resource alone does not imply readable/writable bytes.
The runtime's `store_vertex_word` must check and write the actual bytes, failing
at the unavailable store instead of truncating or silently skipping output.

Device seams bind the existing shared `graphics::Draw`/`Frame` owners. They must
snapshot actual current device state when `draw_primitive` runs. SetCurrent sets
FVF first, then stream 0/resource/stride. Lock records remaining bytes and argument
words in source order, increments lock count, then calls the device. The returned
cursor starts as `{nullptr,stride}`: a failed Lock that leaves its output untouched
must keep this stale value, and the first attempted vertex write must fail. A
failing HRESULT that *does* write the output pointer is ignored by the caller.
Unlock returns its device HRESULT; strip/discard ignore it. Draw snapshots the
argument-array identity and lock count, sets cull state 22, draws D3D primitive 5,
and clears lock count only after every call returns. It does not restore cull mode.

`observe_buffer_store` and `observe_argument_store` are reference observations of
stores already decided by the engine; native bindings may be empty functions.
They must not become alternate algorithms. API or byte-write failures propagate,
leaving the source store/API prefix intact. Allocation and release are explicit
seams: BufferState owns the resource and argument allocation across calls, while
the runtime supplies their actual storage/lifetimes. Free clears only resource and
argument identities; capacity, stride, counters and other fields retain source values.

`exact(Context&,float)` returns existing `fidelity::ExactFloat` backed by the
runtime's borrowed arithmetic context. No X87Context alias or emulator operation
enters the engine. Signed16 FILD inputs convert exactly to binary32; UV products
retain caller precision/rounding and explicitly store binary32. Scale is the exact
m32 source constant `0x1.010102p-8f` (bytes `81 80 80 3b`), not guessed division.
There is no Question-entry FP control-word receipt; level 3 is retained honestly.

## Initialization facts

All source PolyBuff records begin in zero-filled source storage. `794540` requires
caller-provided count, stride, FVF and debug name; BufferState declares no invented
capacity or rendering defaults. It writes stride, capacity, remaining and FVF,
requests `CreateVertexBuffer(bytes,0x208,FVF,pool0,&resource)`, allocates
`count*16` argument bytes, clears lock count, initializes only start/count in each
16-byte argument row, then writes debug name and zero to the final field. Cull and
the fourth argument word remain untouched. A null state is an actual no-op.
Create HRESULT and allocation failure are not converted to success guards.

`78e720` stores its third argument, allocates giant scratch as its second argument
multiplied by 36, initializes nine records from four inline tables, then clears 128
words of source scratch. It supplies record 8 with stride 36 and FVF `0x152`.
The PE count table initially contains 12000 there, but maintained
`LibraryConfigurationDefaults.buffer_counts[8]` is 32000; library configuration
changes this source owner before initialization. Root must pass its *actual*
configured count. Neither 350 Question vertices nor either observed table value
is used to fabricate buffer capacity. Discard reserves one stride, locks that
first stride with DISCARD, then unlocks; it leaves lock count and render args alone.

## ABI and judge

The strip is cdecl with three stack operands and caller cleanup. SetCurrent,
Unlock, DrawTriangleStrip, Free and Discard take ECX as the record pointer.
LockTriangleStrip takes ECX record, EDX requested vertex count and one cull stack
word, returns its actual output pointer in EAX, and uses `ret 4`. Init takes ECX
record, EDX count and three stack words (stride, FVF, debug name), with `ret 12`.
The source D3D8 slots are SetVertexShader +304, SetStreamSource +332,
SetRenderState +200, DrawPrimitive +280, CreateVertexBuffer +92; resource Lock
+44, Unlock +48 and Release +8. COM calls retain their original stdcall operands.

The new original-instruction oracle executes all eight complete original game
bodies; only COM and malloc/free are receipts. It compares state, ordered stores
and API operands, successful final vertex bytes, actual Lock return and Unlock
HRESULT. Explicit imported-data bounds report the first missing source fact;
source instruction/address receipts are retained in private reports. Masks,
zero/bad counts, signed indices and UVs, initial/trailing degenerates, equality
exhaustion, overflow, zero stride, device/allocation/output failures and API stops
are exercised. No existing oracle, corpus, fixture, tolerance or mutant is edited.

Run the private checker only after Root's execution slot is free:

```sh
python3 tools/oracles/check_normal_uv_strip_packet.py \
  --python /Users/burbujamc/Developer/sadx-macos/reference/python/bin/python \
  --exe '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe'
```

It builds sequentially under this worktree's `build/normal-uv-strip`, runs the unit,
development judge, seven semantic mutants, then a distinct seeded holdout.
Makefile additions register the isolated unit/probe; the unit manifest only gains
its new row. Root must separately add the engine source to the native product
closure when binding the real runtime seams. No apps, shared outputs, full gate
or Ghidra workers are launched by the checker.

Frozen proof: `reference/production-integration-20261026/normal-uv-strip-proof.json`.
The unit passes 12 checks. Development has 1,014 exact value/prefix cases,
15,720 ordered receipts and 9,309 vertex-word stores; holdout has 2,662 exact
value/prefix cases, 29,088 receipts and 14,934 stores. All seven mutants are killed
(value mismatches respectively 4, 68, 127, 218, 237, 157, 318).
Both original instruction judges exit **2, FP partial**, with 154/198 flag rows:
native inexact `0x20`, source clear `0x00`. Neither is a strict pass.
Reference/native state/API/final-vertex receipt hashes are identical:
development `ac3cc34770941cdeae705a26110ab2fb870cb45eb56185cae4752d6a3417f707`,
holdout `55781d1ef9d10ca261fb0ce49cc8463ba6a982766fddc501e96b9823ad420b10`.
Those receipts explicitly exclude the separately compared low-six flags; FPCW,
all other compared state, returns, vertex and owned argument snapshots, and ordered API/store prefixes
remain included. The small proof records both input hashes and full report hashes.

Full declared ranges are recognized by the unchanged profile checker: eight per
oracle report, zero unparsed/opaque ranges, zero UNHANDLED overlaps. Provenance,
layer, one-owner, readability and diff checks pass. Only additions to the judge
are registered; old protected inputs retain their prior bytes apart from allowed
appended unit/probe registrations. Root's gate rc 0 at base 04c40ae was externally
reported as terminal before focused execution resumed; no full gate ran here.
Full FPSW/tag/stack state, unmasked/deferred exception delivery, real D3D/Metal
resource errors, full model rendering and actual Question pixels remain unverified.
