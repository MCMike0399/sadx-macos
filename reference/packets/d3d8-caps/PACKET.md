# Complete D3D8 capability module readability

Source `d1a7b42`, baseline `349de60` / Main `6ed0ebe`. Both complete files keep the
same API and type identity. Named wire offsets and capability masks replace opaque
writes; one little-endian writer and zero initialization preserve the exact record.
The full diagnostic remains unchanged and explicitly says the GPU was not captured.

There are exactly two maintained consumers: HLE adapter/device GetDeviceCaps and
NativeGame reference device construction/reporting. Neither is a new native GPU
policy. The current shipping archive excludes this module. It has no source-global
owner, callback, heap state or dependency on the Task migration.

The unchanged exhaustive wire unit passes before and after. Both independently
compiled probes match all seven capacity/null records, every one of 212 wire bytes,
output sentinels and the complete 214-byte diagnostic. Three valid compiled faults
are caught by the unchanged unit: WFOG mask, unspecified zero bytes and hidden
unknown-GPU disclosure. `BEFORE*.json`, `AFTER-OUTPUT.json`, `MUTATIONS.json` and
`QUALIFICATION.json` bind the exact observations and Source.

The actual unchanged guest consumer unit passes using the fresh caps implementation
before pinned dependency archives. Its link map identifies the new object as the
sole definition provider. It checks both COM methods, bounds, one assumption receipt
and real original mipmap-format behavior with and without MIPMAP. Raw artifacts stay
under this worktree's ignored `build/d3d8-caps`. No mutable Main archive is used as
the before/after model. No source data, tolerance or existing unit changed.

Layers, global owners, readability, constexpr classifications and profile checks
pass. The standalone library has only standard system dependencies. Root still owes
fresh independent complete-module review, protection for the new probe, inventory
reconciliation and the coherent Main integration gate (including game-runtime).
No Windows GPU parity, full gate, new journey, native gameplay or campaign closure.
