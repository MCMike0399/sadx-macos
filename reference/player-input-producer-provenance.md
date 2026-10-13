# WriteAnalogs and updated Input Mod magnitude owner

Implemented native `player_input_producer.hpp/.cpp`, semantic tests, production-archive probe and reusable instruction verifier. The owning paths are now separate: original x87 **WriteAnalogs**, updated Input2.6.1's literal **integer/MOV magnitude override**, and the still-pending **SSE keyboard/controller preparation**. No original game file or existing shared InputState/mac_input/player_input source was changed in this work.

## Pinned evidence

* Supplied sonic.exe SHA-256 **6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74**. Original WriteAnalogs **0x40f170..0x40f343**, **468 bytes**, SHA-256 **96d1eb2d7a2c4b8370e3f96c6249ec1b50278c318f36d1fce33143c425e92bfa**. Offline LLVM disassembly: `player-input-producer.asm`. REA dossier: `ground-dependencies/input-producer.json`, Evidence **ev_c16d7fed83c5b40993d9ebf9d9f17614d82005aaaeba4723a641f8858c940e9f**.
* Actual shipped Input2.6.1 DLL SHA-256 **c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5**, image base **0x10000000**. Retained archive/profile under `updated-pc-reference/`. Exported Jumps pointer at **0x1000e0b8** refers to array **0x1000e070**; its actual fourth pair is **0x40f343 ->0x10004b70**. File-backed Count is zero before CRT initialization; it is not proof a live initialized Jumps list has zero entries.
* Hook entry **0x10004b70** calls literal body **0x10004b00..0x10004b6b**, **108 bytes**, SHA-256 **e5da4315822365342ec3a22414b7237353b6e9c1cce28f83bd4ad0602cf6488e**. `input-mod-magnitude-hook.asm` preserves actual instructions, not only corresponding upstream C++.

## Native owning inputs/state

The executable/constants owner supplies the **40-byte** constants record (four f32 fields and three f64 fields). The extractor emits `build/assets/input-producer-constants.bin` with source addresses, offsets and digest in its metadata. SHA-256 **8cf6c0c178d7e4f766401104b755a528282f0d59b6a00feb8f8022aea2d20e22**. Native decoding retains original bits rather than substituting rounded literals:

| Field | Source VA | Value |
| --- | --- | --- |
| positive deadzone | 0x7dccf4 | 3072 f32 |
| negative deadzone | 0x7dccf0 | -3072 f32 |
| magnitude scale | 0x7dcce0 | bits0x29307b91, approx3.918702723968749e-14 f32 |
| magnitude cap | 0x7dcb10 | 1 f32 |
| angle units | 0x7dccd0 | 65536 f64 |
| positive angle factor | 0x7dccc8 | bits0x3fc45f30b4e4e30a f64 |
| negative angle factor | 0x7dcce8 | bits0xbfc45f30b4e4e30a f64 |

Runtime supplies decoded ControllerPointers axes for original slots0/1, effective controller-enabled bytes, global control byte, observed camera absence/presence/yaw, and independently prepared Input Mod pad fields (connection, keyboard allowance, axes, normalized magnitude). An unknown camera differs from an explicitly observed null pointer. Original pointer-pad fields and DreamPad's internal fields are separate owners: physical input/demo/redirection can make them differ. Values are not inferred from filenames or defaults.

Output state owns the original **UnusedNormalizedAnalogs** array at **0x3b0e368** and actual **NormalizedAnalogs** array at **0x3b0e7a0**, eight records each. Missing output facts remain optional. Earlier slots and earlier raw stores survive a later missing-field/exception boundary; callers must not assume transactional failure or safely rerun partially updated state without restoring their frame inputs.

## Recovered original math/store order

WriteAnalogs reads signed16 left axes only for slots **0 and1**, shifts each by8 (native multiply256 avoids C++ negative-shift UB), converts via FILD32 and binary32 stores. A component within **[-3072,+3072]** is dead; boundaries are inclusive, matching raw signed SDK **[-12,+12]**. Outside components subtract/add positive deadzone3072; inside components become positive zero.

It evaluates **FPATAN(adjustedY,adjustedX)**, scales with the actual binary64 angle units/factor, then runs recovered CRT **0x644340**. With camera present it uses negative factor and **-cameraYaw-converted**; with known null camera it uses positive factor directly. Both paths mask direction to16bits. The selected native arithmetic is explicitly named **Bochs P5/P6 FPATAN** plus software x87/real CRT conversion, not host atan2 and not a claim that it matches the user's Intel Ultra yet.

Squared magnitude order is **Y² then X²**, sum with source x87 operation order, **FSTP binary32**, call **SignedSqrt0x7871f0**. That helper returns **RET4**, consuming its pushed argument, so the caller's following `FMUL [ESP+0x14]` reads the same stored **squared sum**, not the prior X/Y variable. The resulting original strength is:

`sqrt(storedSquaredSum) * storedSquaredSum * sourceMagnitudeScale`, capped at1.

This cubic response must not be replaced with linear stick length. The source-specific **FSUB/FADD/FMUL/FCOM m32** APIs are used at actual memory-operand sites, preserving signaling NaN instruction form instead of inserting extra FLDs. Binary64 source constants are ordinary finite retained values. Actual PC CW/MXCSR/exception timing remains unobserved.

All8 slots receive raw magnitude/direction stores; slots2–7 have original zero direction/strength without reading ControllerPointers. Raw FST retains its extended magnitude for a second normalized FSTP. Original global and per-controller bytes must each be **exactly1** to copy normalized input; otherwise normalized angle/magnitude are explicitly zeroed. Raw state is stored before testing the per-controller gate, preserving partial write order.

## Actual Input2.6.1 after-hook

The binary hook is integer tests and raw MOV, with no x87/SSE arithmetic:

1. Global control byte **0x909fb0 !=0**.
2. Effective controller-enabled byte **0x1000e73c+slot !=0**.
3. DreamPad connected byte **0x1000e98c+96*slot !=0**, or keyboard-allowed byte **0x1000e99c+96*slot !=0** and **slot0**.
4. Signed16 axis at **0x1000e96c/0x1000e96e+96*slot** has absolute X or Y **strictly greater than12**.
5. Raw copy of magnitude word from **0x1000e994+96*slot** into normalized magnitude.

No direction is recomputed. Negative, signed-zero, NaN/signaling payload words are preserved by MOV; no invented floating normalization or x87 exception is introduced. The original **==1** byte gates and hook **!=0** gates remain distinct, even for noncanonical byte2 test seeds. Slots2–7 can thus gain magnitude while retaining original zero direction. A disconnected keyboard allowance for a nonzero slot does not activate the hook. Unknown connection/keyboard/axes/magnitude fails only when that source field is actually consumed.

The convenience owner `produce_input_mod_261_bochs_p5_p6` completes original WriteAnalogs first and runs the literal updated override only after successful return. The combined test routes an actual shared InputState keyboard press/ramp sample into this producer and then actual GetAnalog, retaining direction and prepared ramp magnitude. This closes the **producer/query interface**, not the complete physical game loop or exact SSE preparation backend.

## Verification

Production `make` builds `build/player-input-producer-test` and `build/native-player-input-producer-probe.dylib` against current `build/libsadx-core.a`. Semantic and UBSan tests print **player input producer passed**. Checks cover deadzone edges, cubic magnitude, camera adjustment, unknown camera vs null, slot order/partial stores, control omission, strict hook threshold, disconnected/keyboard slot eligibility, distinct byte gates, raw signaling-NaN copy and actual keyboard-ramp→GetAnalog integration.

Reusable verifier `tools/verify_original_player_input_producer.py` uses unchanged original instructions and the **actual retained Input2.6.1 hook code**, with explicit test-owned sparse globals/pads. Updated mode applies only the observed **0x40f30c enabled-table operand redirection** and **0x40f343 return JMP** before execution. No Windows DLL startup/imports or physical input run occurs, and the hook is not a high-level test double.

The production-built probe comparison covers **5,050 seeds ×12 CWs ×2 modes =121,200 evaluations**: **0 output-word mismatches**. It retains **108,240 exception mismatches** from the known Unicorn exception-state limitations, so **overall passed=false**, **value_outputs_passed=true**. Artifact: `original-input-producer-comparison.json` and hashed `.cases.bin`. The source GetAnalog exception-oracle analysis in `player-input-provenance.md` remains applicable. Actual Intel Ultra/NVIDIA Windows11+AtlasOS modified-process floating state still needs native Windows evidence.

## Next-stage SSE controller/keyboard anchors

Retained `input-mod-controller-normalize.asm` identifies actual **convert_axes0x10004400..0x100045df**, **480 bytes**, SHA-256 **0842dfed02318b34dbad4e9973d4bc6c31e2eb4756a10a7b2e4b23ba4f3168c0**. Optimized ABI: **ECX destination**, **EDX pointer to two signed16 axes**, stack **deadzone** and **radial flag**, **XMM0 float return**. It first tests integer absolute-axis deadzone, clamps -32768 to-32767, converts to binary32, computes **Y²+X² in SSE f32**, promotes that sum to f64, uses **SQRTSD**, converts back f32, computes radial direction and strength, then uses **CVTTSS2SI**/low16/clamps127 for output axes. The denominator is `32767-deadzone`; source setting deadzone32767 is singular and must not be silently normalized away.

Actual keyboard normalization portions at **0x10004e59..0x10004ff8** likewise use binary32 multiply/add/divide, binary64 square root and CVTT conversion, then keyboard half-press handling. `input-mod-keyboard-normalize.asm` retains that closure's main body; controller poll and all side effects are not claimed recovered. Constants include32767,127/128, float epsilon and half0.5 at the retained DLL's ordinary rdata addresses.

These **SSE** paths obey **MXCSR**, not x87 CW. Shared InputState's existing host sqrt/ramp profile is a semantic model, not bit-level proof against this DLL. Controller axes from GameController have already undergone Apple's deadzone, while original Input Mod consumes SDL signed16 raw axes; applying a second default deadzone would be a behavior change. Keyboard half-press, camera recenter and D-pad/menu support remain unimplemented in the shared layer. No guessed raw-axis reconstruction, default MXCSR or host atan2 is added here. Root explicitly deferred this distinct SSE owner until the current source foundation was finalized; these exact anchors support the next implementation stage.
