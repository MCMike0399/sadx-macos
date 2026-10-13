# Native texture list and cache selection

The new owner reuses the existing NinjaTextureListOwner and
NinjaTextureMemoryReference. Header0/4/20 facts and raw surface-word provenance
are explicit optional values; native runtime ownership now distinguishes
unobserved, observednull and a real owned upload. Source pointer words are never
cast to GPU handles. Palette revision42 is mutable cache metadata on that same
resource, preserving archive/ordinal/runtime identity.

Actual77f3d0 and77f440 retain current/requested references separately from the
active memory cached by78cf20. The getter never reads advertised count; it uses
modulo32 12-byte index arithmetic. Native bounds/field coverage failures remain
explicit instead of guessing outside the owned array. Equal global IDs can skip
binding a new reference and retain the previously active owner.

The78cdc0 eager header reads and palette lookup/no-bank/notfound/unchanged-version
paths are closed. Dirty palettes commit revision42 before the color-pointer read
and unimplemented surface lock78ce5a. No lock, palette conversion or update
success is fabricated. Non-paletted supplied asset paths close completely.
78b840 is exactly three render-state commands, through the existing render owner.
Original ignored HRESULTs are preserved independently of native call completion.

The683-profile actual-instruction comparison matches all declared state,
reference-normalization, call operands/order and stop boundaries. Source bytes
are unchanged. External D3D calls use explicitly synthetic receipts or stops;
this does not establish GPU/runtime cache equivalence. Optimized and UBSan unit
checks pass. Genuine Ghidra Evidence and source/compiler/case identities are in
ninja-texture-selection-ledger.json and the original comparison report.

The separate Metal sprite consumer validates a real typed upload with the same
archive/ordinal and runtime owner, then captures the selected surface owner with
the draw. It does not cast arbitrary opaque ownership, substitute bubble130, or
infer original77f7f0 registry/refcount/lifecycle behavior. Physical modified-loader
cache state, sampler/filter/blend equivalence and dirty palette updates remain
separate unverified work.
