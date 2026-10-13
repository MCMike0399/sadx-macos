# Native CRTfloor and collision matrix continuation

Original input SHA-256: `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Native deliverable remains macOS ARM64 with Metal. Windows is only a later behavior reference; no PC setup or execution occurred.

## Observed source and native implementation

`crt_floor` reconstructs `0x6449f0` and the `0x64917a` fallback. Runtime CPU feature at `0x3d0f400`, MXCSR and raw fallback CW at `0x2bcc034` are explicit caller facts. The static file word `0x173f` is a file observation, not proven runtime initialization.
SSE uses the source packed integer masking, signaling negative comparisons and SUBSD. x87 uses native SoftFloat FRNDINT independent of PC; saved control word restoration and binary64 stores/loads follow the source. Missing real CRT error handlers stop at the actual call with pending stored result and modified CW preserved.

Imported unchanged pinned SoftFloat source: `extF80_roundToInt.c`, plus SSE `f64_sub.c`, `s_addMagsF64.c`, `s_subMagsF64.c`, `s_normRoundPackToF64.c`. Upstream digest manifests and original license notices retained.

Collision matrix owners distinguish push-ex/unit-copy from success-only supplied-matrix push. Ordinary pop guards target zero; PopEx permits depth1 to zero. Zero-depth current pointer is outside the owned slot array and becomes an explicit native failure; supplied-unit push can recover without reading outside storage. No unsafe before-base matrix is fabricated.

## REA evidence

| Artifact | Procedure | Evidence ID |
| --- | --- | --- |
| `_reference/sadx/path-dependencies/adjust-angle2.json` | `0x4383a0` | `ev_890ae318dd0bb1e3b2f3bd76a82f75b7cadbc596d25ce7c3db9fa39b06a8f8b8` |
| `_reference/sadx/path-dependencies/path-angle.json` | `0x4393c0` | `ev_2ee75f8e54acd1c0c7d281a7a1ddeaa26d5db1e644f04383602e22052278200f` |
| `_reference/sadx/path-dependencies/path-segment.json` | `0x49c210` | `ev_11b97464ad75773ee3f17c6af84522dee2e60336c89978467ea83bdde4447599` |
| `_reference/sadx/path-dependencies/crt-floor.json` | `0x6449f0` | `ev_33d785cb67223dda1009129a18faef198b5a8a3eb627b2e4ba2e63b3fe0aa057` |
| `_reference/sadx/floor-dependencies/floor-error2.json` | `0x64afe5` | `ev_592a076c62a7f05706e13ecfefe617ca5d6ace13fe1325e0f94744acafba5a3c` |
| `_reference/sadx/floor-dependencies/round-integer.json` | `0x64dbc9` | `ev_ceca57471d435b07282d8ec41a83b2ff7c53fa86e653fe60489166c6c2bbc3eb` |
| `_reference/sadx/floor-dependencies/floor-fallback.json` | `0x64917a` | `ev_417c04092d8a3968b2e7b7d05ac51fe09127fab96b146a795bd510a220c2a041` |
| `_reference/sadx/floor-dependencies/control-fpu.json` | `0x64b1fd` | `ev_78a1660a72da31dd4b3e2e5f59841ec715975642b55d3811f24fb31db0c2947b` |
| `_reference/sadx/floor-dependencies/fp-classify.json` | `0x64b0cd` | `ev_4e992072e322c000ae596239e6749e23ef9182ed8c5495798b5a46f710d4a7eb` |
| `_reference/sadx/floor-dependencies/floor-error.json` | `0x64af93` | `ev_5a90c5a68dc352251c68fab64e98d9d2bc830b22f11df0cef7366d3e7269a416` |
| `_reference/sadx/floor-dependencies/floor-error-return.json` | `0x648ee8` | `ev_a871bf6d75dc6b9323186f3417d753af585b376d1ab305fa75a38110a3ea25ca` |

The exact complete artifacts are retained; Hopper explicitly cannot report complete function body ranges through its public Python API. Exact LLVM byte disassembly corroborates boundaries. All owned Hopper sessions were exported and closed with verified launcher cleanup.

## Verification and open precision boundaries

`original-crt-floor-comparison.json` retains 100,744 explicit-profile evaluations (1,028 values × 96 main profiles plus two control-mask fallback profiles). All 49,344 SSE-profile returned ext80 values, pending arguments, error call boundaries and CW match the original instructions. Overall strict result remains FAILED: 150 fallback signaling-NaN call-boundary differences and 24 fallback negative-subnormal returned-value differences under FTZ. Exception flags are reported separately.
The retained fallback instruction traces show Unicorn storing signaling NaNs unchanged before `0x64b0cd` classification (class4), while native FLD64 quiets them (class3). FTZ traces show the initial x87 binary64 spill turning negative subnormals into negative zero; the isolated original `0x64dbc9` FRNDINT helper returns -1 with either MXCSR. These are scoped emulator observations; actual Intel hardware validation remains unperformed. Native semantics were not weakened and no fixture was removed to turn this report green.

Unit tests cover raw source constants, signed zero, DAZ effects, PC-independent FRNDINT, required runtime facts, retained fallback CW at unresolved math errors, and collision-stack capacity/zero-depth ownership. These do not prove a complete game or actively loaded mod parity.

New normal/shape/contact/path/landscape owners and their own ledgers are integrated separately. Full scene contact resolution, startup, character state machines, native presentation/audio/saves and physical input remain required.

The extra raw-CW gate profile clears bit6. Intel labels control-word bits6/7
reserved; this experiment does not establish a capturable hardware readback or
justify assuming that reserved bits survive FLDCW unchanged on the user’s CPU.
See [Intel SDM, x87 control word](https://cdrdv2-public.intel.com/868137/325462-089-sdm-vol-1-2abcd-3abcd-4.pdf)
and [Intel floating-point reference sheet](https://www.intel.com/content/www/us/en/developer/articles/technical/floating-point-reference-sheet-for-intel-architecture.html).
The current software API preserves caller-selected raw words; actual Intel
reserved-bit readback remains a separate open hardware requirement.
