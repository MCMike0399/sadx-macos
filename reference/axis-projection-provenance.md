# Axis rotation and floor-input matrix projection

Source SHA-256:
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
This independently recovered slice extends the existing native PlayerCoordinates
owner. It creates no second sine table/matrix stack and executes no guest
instructions at runtime. Hopper remained root-owned; retained REA dossiers and
Xcode LLVM disassembly supplied the source evidence.

Full `njRotateV` `0x788df0..0x78931d` is 1,326 bytes, SHA-256
`9bd39b09fb449b85431a823d7fc5fa44f5f112fae033648f9490bf55eca08fd7`;
genuine Evidence `ev_3e34ee7ba8d1e22e8da8703b7989eac99f2b65de38e86b063583f89b7f5591df`
from `physics-dependencies/axis-rotation.json` structuredContent. Full assembly
is retained in that directory's `axis-rotation.asm`. The source matrix portion
of alternate `0x44b0e0`, `0x44b453..0x44b50c`, is 186 bytes, SHA-256
`0765fad99f1880c70c99ed7e3b18169e31d8a5d65e288e4a352f19c779a86c1e`.
Its source assembly and main Evidence are retained under ground-dependencies.

`rotate_axis(matrix,axis,angle,constants,fp)` copies axis words first, computes
the source Y/Z squared spill and signed square root, and recovers the original
FUCOM equality/special-axis branches. The first alignment stores ratios from
the retained reciprocal; the second uses raw axis.x and the stored Y/Z length.
There is no full-axis normalization or zero-axis identity substitute. Source
literal ±1/zero stores, including zero-axis special behavior, are preserved.
The remaining matrix alignment, table rotation and inverse alignment are
statically lifted to native ext80 expressions with source-VA comments, original
register retention, m32 operand forms, binary32 spills and partial row stores.
No generic Rodrigues formula replaces the source's different intermediate
rounding. The fourth matrix row is untouched.

Static recovery explicitly decoded x87 DE E9 subtraction destination ordering;
LLVM's AT&T `fsubrp` lexical rendering alone was insufficient. The initial
reversed lift failed original matrix goldens and was corrected at the producing
expressions. No output-bit substitution was added.

Source-required quiet comparison is a new `X87Context::compare_quiet` operation.
FUCOM/FUCOMPP accepts quiet NaNs without invalid and rejects signaling NaNs;
ordinary FCOM remains signaling for either kind. The unchanged pinned SoftFloat
`extF80_eq.c` supplies this rule; finite ordering reuses existing `extF80_lt`
only after NaN classification. Exactly one source was added (53 C sources,
61 upstream files); original 8086 specialization/TLS ownership and SSE symbols
were unchanged. Canonical unsupported CPU encodings remain explicitly rejected.

`project_input_to_floor` owns the actual matrix path, accepts the caller's raw
stick-angle conversion result and retained pre-ftol ST0, and stores that ST0
to input X/Y through source binary32 stores; input Z is literal −1. It pushes
the shared stack, copies the real unit matrix, applies camera Y then X, and
calculates camera direction. It then pushes/copies unit again, applies source
RotateV about the supplied floor normal, and calculates projected direction.
Finally it runs the real task-to-world owner on literal forward `(1,0,0)`.
Result fields preserve independently optional camera/projected/forward vectors,
source failure boundary and nested CoordinateResult facts. A failed callee does
not fabricate a pop or erase the actual shared stack/frame mutations. Source
push-capacity no-op followed by pop can reduce prior depth; this is retained.

`CoordinateAxisConstants` is the new eight-byte artifact: binary32 zero at
`0x7dcb2c`, then binary32 one at `0x7dcb10`; SHA-256
`22b6f43bd8d27738d3213f29e96b62d01d9d6c0ab4f9732aaae803186f51eab7`.
Runtime facts/constants are caller-owned inputs, including nonfinite source
values; no host sin/atan or identity provider fills an unavailable boundary.

`tests/axis_projection.cpp` takes table, unit and eight-byte axis constants.
Optimized and ASan/UBSan runs pass quarter-axis direction, untouched fourth
row, non-unit-axis source policy, caller retained ST0, forward direction,
normalization-unavailable partial stores/stack state and capacity behavior.
The existing numeric archive was linked without sanitizer instrumentation.
Quiet-compare tests separately pass QNaN/SNaN semantics, both operand positions,
finite order, signed zero, NaN versus denormal priority and unmasked failures.

Reusable probe: `tests/native_axis_projection_probe.cpp`; runner:
`tools/verify_original_axis_projection.py`. Each of twelve PC/RC profiles has
1,274 raw-bit fixtures: 15,288 comparisons match all vectors, presence flags,
depth and all six frame matrices. Complete original `0x788df0` executes for
primitive fixtures. Projection fixtures run actual `0x44b453..0x44b50d` plus
real camera/matrix/Ninja/task transform callees with explicitly seeded prior
raw-stick angle and retained ST0. No instruction overlays are used, but this
is a partial original body, not a complete alternate-acceleration comparison.
Systematic axes (including zero/non-unit), all angle quadrants/wraps, random
finite axes/matrices, varied depth/capacity/normalization, signed zero,
subnormals, infinities and quiet NaNs are included. Initial signaling-NaN FLD
remains a separately retained emulator limitation.

Flags differ in 1,169 cases per profile and remain separately reported.
Hardware exceptions, actual running-game/mod-selected control state, preceding
stick producer, full alternate-body composition and gameplay remain unverified.
The final signed angle at `0x438410` is outside this passing projection scope;
it requires root's real CRT acos `0x6450f0` and the actual cross-product owner.
Direction-reset path resources also remain open.

Fixture SHA-256:
`755b00f20f0985884513f3032c17cf226d7a2a7412b4c55106eabea7d4784f09`.
Temporary probe SHA-256:
`7b6fc50746443527ef7211c62cd271218fb28596c95d7735d2036d0ba014c13a`.
Report SHA-256:
`73993f3f4833b3cbf926b0e72f23bda4f6d5ab5e17da87b0eae1f281e6726701`.
Root owns source/probe/test integration and the full native gate; no Makefile
or shared build artifacts were written by this worker.

## Subsequent signed-vector-angle closure

The leaf `player_projection.hpp/.cpp` now reconstructs complete `0x438410` and
uses root's actual `crt_acos` owner with the shared explicit CRT runtime. It
stores dot to binary32 while retaining the original extended dot for the lower
−1 clamp; the +1 clamp reloads the stored value. The nonclamped path reloads
the spill, calls CRT acos, applies both actual binary64 scale factors, and
converts using source `_ftol`. It computes the actual cross, then optionally
uses the supplied sign-direction dot to complement the angle. No input vector
normalization or host acos is introduced. Missing/error CRT boundaries retain
outer/inner source addresses, stored dot, optional pending ext80 value or
already-stored binary64 result, and do not run the later cross operation.

Root's caller-register audit clarified canonical cross ownership. Ground's
old helper accepted `(speed,collision)` but the actual source sets
ECX=collision, EDX=speed. Its result was therefore a caller-order adapter to
the same source `0x7889f0`, not a distinct cross convention. The one canonical
`cross_product_length_extended(firstECX,secondEDX,output,fp)` now lives in
`ninja_math`; ground supplies `(collision,speed)`, and projection supplies its
actual `(forward,projected)` order. Full-call entry tracing corrected an earlier
manual stack-offset derivation which omitted the still-pending task-to-world
arguments. No aliases or output sign patches remain.
The helper loads second-vector words and multiplies first-vector memory words,
computes all X/Y/Z components before any store, then squares retained extended
components and spills the summed square to binary32 before signed sqrt. Output
may alias either input because every source product precedes the first store.
Direct tests for output aliasing either the first or second input pass.

Source `0x788d40`, initially labeled normalize-cross, is a plane
projection/distance helper and remains distinct. It consumes six-float plane
data through EDX; treating it as a cross operation was a disproved lexical
inference. Direction reset still needs that owner and its path-resource chain.

Signed-angle Evidence:
`ev_d32c3f5d0dc1ffd5d0e97eead86ec00880bf7ccbac9b650064fe444745e9d293`;
168-byte body SHA-256
`f895b77609e50ecd88bbbe47bbd417a6838b74b893ebf19806283e5ce057f37b`.
Canonical cross Evidence from ground-dependencies/cross-product-length.json:
`ev_19c8b8b3311dfe3d65dc7bdb3f742a9e393e185a623fb5904e0afcf3813632aa`;
88-byte body `0x7889f0..0x788a47` SHA-256
`7f9777641eba60ec6d12de166a4d2dbc5ee1d76f482a717ec45e75eb677c17da`.

Additional constants are float negative-one at `0x7dcb0c`, then doubles
`0x7dccd0` and `0x7dccc8` (20 bytes, SHA-256
`8e208a883f2c7993aeda6ec61fd4e0b12323c4bc08b25e32cbf83ae2e2ac81a3`).
The signed-angle test/probe use a 28-byte combined packet: existing zero/one,
then those 20 bytes; SHA-256
`443ae9201e62abf25de902664499600a37b668497f43fa6b232b74f719db9094`,
FNV64 `6584ca27c26f97be`.

`tests/player_projection.cpp` passed optimized and sanitizer runs for source
clamp short circuits before unavailable CRT, optional sign direction,
canonical cross orientation/aliasing, complement policy, all twelve selected
control profiles, restored control words, unknown policy's unstored extended
value versus missing dispatcher's already-stored binary64 result. The numeric
archive linked into the sanitizer run was not sanitizer-instrumented.

The sole signed-angle producer is `tests/native_projection_angle_probe.cpp`,
with two distinct entries for angle and canonical cross. Reusable runner
`tools/verify_original_projection_angle.py` executes the complete original
angle body and independently executes canonical cross without overlays.
Each of twelve CW profiles has 998 angle and 998 cross fixtures: 11,976 angle/CW
returns and 11,976 cross-vector stores/full ext80 lengths match. Flags differ
in 920 angle and 678 cross fixtures per profile and remain separate. Axis
combinations, optional sign direction, random finite vectors, dot endpoint
neighbors, signed zero, subnormal, infinity and quiet NaN cases are included.
Policy1 explicitly bypasses source CRT error dispatch; original hardware,
actual runtime/mod control state and complete gameplay remain unverified.

Fixture SHA-256:
`948fa685760fb278d80defddda82c8a3ba98823ebc6bb69b4266d80049bac43b`.
Temporary native-probe SHA-256:
`c2dee227188575a262fda92e1a45777dbaf97b3a06ec9937755b3868e5fa0f5b`.
Report SHA-256:
`43f1ed0bb5fa81b09f74b5b282c9efda1d25b9a754b840bd8f2050c7cca69bc6`.
This closes the matrix/signed-angle dependencies locally. The peer-owned
alternate-acceleration original-body comparison must additionally establish
its producer/control/store composition; the earlier seeded-tail record does
not become a full-body claim retroactively.
