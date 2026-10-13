# Native player-motion helper reconstruction

2026-10-11. Input: `Sonic Adventure DX/sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Added source: `sadx-native/src/player_motion.hpp`, `.cpp`, and
`sadx-native/tests/player_motion.cpp`. No Makefile changes.

These are reconstructed native helpers, not original source or a functioning
player/game loop. Full PGetSpeed is deliberately not replaced by a simplified
integrator. Unknown rotation/input/global effects remain explicit.

## Evidence and exact bytes

Genuine REA `structuredContent` was inspected independently:

| Record | VA | Evidence ID |
| --- | --- | --- |
| `movement/gravity.json` | `0x443800` | `ev_5a125baa211bc2641a4a4e54718ca61b3cba08072d3a86577089216b008d7a24` |
| `movement/clear-speed.json` | `0x43c550` | `ev_c5b4fe4415a586aade4ac3fff65e57b7308657e79a620b7659daf189fd746483` |
| `movement/get-speed.json` | `0x443f50` | `ev_cf65f09daeadac3322ee8d88a2168dbbd8521dad577e137edd685c0dc30d1dbb` |

All bind the executable digest above, operation `analyze_function`, Hopper
launcher identity
`launcher-sha256:dad6f886261fad0d4cdd01aee6fcf57439c60d7b5c54e62fc97f0180116690e0`,
and analysis-profile digest
`8c226db6a8bd36b662dc1cb83ab0ad0af698230fc322b58c6c4870878bf6ae70`.
They explicitly limit complete body-range reporting, reference flags,
external/thunk classification and unresolved-call coverage through Hopper's
public Python API. Pseudocode/assembly are provider representations, not original
source; this is static evidence rather than gameplay execution observation.

Direct Xcode `llvm-objdump` inspection corroborated control/instruction order,
including the additional limiter and transform callees. Exact byte ranges use
inclusive start/exclusive end, mapped through PE section raw offsets:

| Function | Range | Bytes | SHA-256 of bytes |
| --- | --- | --- | --- |
| Gravity | `0x443800..0x44389e` | 158 | `01117d81b3465510c57e9f104d252b86bc05c08b816102934c91466bdc66fe50` |
| Clear speed | `0x43c550..0x43c575` | 37 | `a63c6d795896d69baef4c7f9522ca63c99d75ddf96c14fdd7fa35e9b49a64103` |
| Speed limiter | `0x43c380..0x43c4b4` | 308 | `ed2a073798a1f65eadf1c47c6eea05f80d06c15292b2f311b531f242def33d20` |
| Full get speed, not implemented | `0x443f50..0x4441ab` | 603 | `e60e611ea247b5f1aedb3d91c3889242dd956f9ab3668981015211b4a9cf45ea` |
| World-to-player transform, unresolved | `0x43ec00..0x43ec8a` | 138 | `9cbbc83cf285c6f9ecf2b8d4ad96af398044dea18d3da0118f095732cf1ca9d2` |

Names/types were candidate guidance from [X-Hax SDK revision
eb4cdcca9637d611a98b910fcfed6ce18580451c](https://github.com/X-Hax/sadx-mod-loader/tree/eb4cdcca9637d611a98b910fcfed6ce18580451c),
`SADXStructsNew.h` (`motionwk2`, `playerwk`, `player_parameter`, `taskwk`) and
`SADXFunctionsNew.h`. Actual instruction offsets govern implementation.

## Reconstructed semantics and minimal native state

- `MotionWork`: speed at guest `motionwk2+0x04`; acceleration at `+0x10`.
- `PlayerMotion`: acceleration at guest `playerwk+0x2c`; speed at `+0x38`.
- `PlayerOrientation`: raw flags at `taskwk+0x04`; raw x/y/z angle integers at
  `+0x14/+0x18/+0x1c`. Values are forwarded without normalization.
- `MotionLimits`: horizontal and vertical source parameters at
  `playerwk+0xa4/+0xa8` (`lim_h_spd/lim_v_spd`). Weight is separately supplied,
  corresponding to `playerwk+0x114`.

These are native value objects, not packed guest-memory views. Caller-owned
state supplies all values; no guessed defaults, ambient globals or guest-pointer
casts are introduced.

PClearSpeed stores integer zero bits to player speed z/y/x if player owner is
nonnull, then motion speed z/y/x if motion owner is nonnull. Either owner can be
absent independently. It does not clear either acceleration vector.

PGetGravity copies motion acceleration into a local vector. If task flag
`0x4000` is set, it returns without updating player state. Otherwise each world
gravity component (globals `0x3b0f0f8/+4/+8`) is multiplied by weight, added to
the corresponding local acceleration component, then stored as binary32.
The local vector goes through `0x43ec00`, and only the resulting transformed
vector is copied to player acceleration. Motion acceleration and both speed
vectors remain unchanged.

The native implementation requires a caller-supplied, owned
`WorldToPlayerTransform`. It does not pretend that identity rotation is correct.
Missing/unavailable transform results are tagged and preserve output state;
native exceptions also preserve output and propagate. Flag `0x4000` remains a
successful no-op even when the transform is unavailable. The original transform
pushes/initializes matrix state, rotates by the task angles, transforms the
vector and pops matrix state; exact Ninja matrix operations/order remain
unreconstructed. A future owner must supply that verified implementation.

The complete local speed-limiter semantics at `0x43c380` clamp motion speed X,
then Z against horizontal limits, then Y against vertical limits. Each performs
strict ordered upper-bound then negative lower-bound comparison. Two branches
selected by task flag `0x800` have identical operations. Negative limits, NaN
values/limits, infinities and signed zero are not normalized or rejected. The
sequential comparison behavior is retained, including the counterintuitive
positive result for negative finite limits.

## Numeric and execution boundary

Gravity uses binary64 multiplication/addition followed by the observed binary32
stores, compiled with `-ffp-contract=off` and without fast-math. Premature float
rounding after multiplication is avoided. Original x87 precision/control-word,
rounding and exceptional-value behavior remain unverified; binary64 may differ
from extended-register arithmetic. Speed clear uses exact zero stores and the
limiter uses ordered float comparisons, but signaling-NaN/FPU-exception runtime
equivalence is still not claimed.

Optimized and AddressSanitizer/UndefinedBehaviorSanitizer builds/tests passed on
native Mach-O arm64:

```sh
xcrun clang++ -std=c++17 -Wall -Wextra -Werror -ffp-contract=off -O2 \
  -Isadx-native/src sadx-native/src/player_motion.cpp \
  sadx-native/tests/player_motion.cpp \
  -o /private/tmp/sadx-port-sources/player_motion_test
/private/tmp/sadx-port-sources/player_motion_test
```

Sanitized validation used `-O1 -g -fsanitize=address,undefined
-fno-omit-frame-pointer` instead of `-O2`.
Tests cover independent nullable owners, exact positive zero, untouched fields,
component arithmetic and required transform call order, raw orientation
forwarding, disabled gravity, tagged missing/unavailable transforms, exception
output preservation, multiplication precision followed by binary32 storage,
NaN propagation, axis-specific bounds, equality, signed zero, negative limits,
NaN limits/components and infinite speeds/bounds. Test-supplied transforms
verify the boundary, not original transform behavior. These tests do not execute
original game functions or prove physics/speedrun equivalence.

## Full PGetSpeed remains unresolved

`0x443f50` has sign-change/input-dependent stopping behavior, acceleration
integration, special attribute/animation conditions, player-to-world rotation,
world effect-vector addition, limiter and small-speed cleanup. It calls:

- `0x43bdc0`: input/context check, itself dependent on flags, player index,
  stage/global state and controller data.
- `0x43ec90` or `0x43edd0`: task-angle or floor-normal rotation, selected by
  attribute bit `0x4000`.
- `0x785170`: vector addition into motion speed from player effect vector at
  `playerwk+0x20`, independently verified from its `fld/fadd/fstp` instructions
  (SDK candidate `njAddVector`), not a magnitude calculation.
- `0x43c380`: recovered limiter above.
- `0x43c4c0`: input-dependent small-speed cleanup (original constants at
  `0x7dfaf4/0x7e005c` are approximately `+0.02/-0.02`).

No incomplete PGetSpeed implementation was added. Resolving these dependencies
and preserving x87 intermediates across its comparisons are required before
claiming the function has been ported. Character physics, startup/task
integration, world collision, native game rendering/audio/save flows,
speedrun-fidelity verification and real Xbox/Switch 2 Pro controller verification
remain unfinished under the original full native PC SADX goal.
