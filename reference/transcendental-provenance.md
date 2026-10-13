# Original angle transcendental recovery and native model

Observed source executable: `Sonic Adventure DX/sonic.exe`, SHA-256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
This note records static instruction recovery, source-bound constants, a native
software candidate, and its verification limits. It does not claim complete
game recovery or Windows hardware equivalence.

## Original dependencies

`floor-normal-angles.asm` shows normal→angles at `0x4b8e80`: CRT asin
`0x645020`, then binary64 multiplications at `0x7dccd0` and `0x7dccc8`, then
CRT integer conversion `0x644340`; the second angle uses FPATAN at `0x4b8f38`
with normal.x as ST(1), normal.y as ST(0), then the same scale and the negative
factor `0x7dcce8`. The stack, comparison flags, early-axis branches, and
integer conversion are separate recovered semantics, not host math.

`crt-asin.asm` and `crt-asin-helpers.asm` were generated from those exact local
bytes with Xcode's LLVM objdump. For finite |x|<1, `0x645058..0x645064` computes:

1. `FLD1; FADD ST(1)` gives 1+x while retaining the original x.
2. `FLD1; FSUB ST(2); FMULP` gives (1-x)*(1+x).
3. `FSQRT; FPATAN` returns atan2(x, sqrt((1-x)*(1+x))).

The entry FST binary64 at `0x645023` feeds classification; it does not replace
ST(0) with its rounded binary64 projection. `0x64503f` saves CW.
`0x648ba5..0x648bb7` constructs `(savedCW & 0x0300) | 0x007f`: preserves PC,
forces RC nearest, and masks the arithmetic exceptions while inside CRT.
Helpers restore saved CW on the ordinary return path. The precision/error
return helper and callback machinery may store/reload a binary64 result;
implementing only the formula is insufficient for the full CRT contract.
Runtime CRT switch `0x3c837a8` is not file-backed, so its live value must be
captured or explicitly supplied; a source scan does not establish it.

Source-backed constant bytes from `PEImage.read`:

| VA | File offset | Little-endian bytes | Meaning |
|---|---|---|---|
| `0x2bcc000` | `0x27cc000` | `00000000000000c0ffff` | Negative quiet NaN, ext80 |
| `0x2bcc00a` | `0x27cc00a` | `35c26821a2da0fc9ff3f` | +π/2 literal, ext80 `3fff:c90fdaa22168c235` |
| `0x7dccd0` | `0x3dccd0` | `000000000000f040` | 65536, binary64 |
| `0x7dccc8` | `0x3dccc8` | `0ae3e4b4305fc43f` | Positive radians conversion factor, binary64 |
| `0x7dcce8` | `0x3dcce8` | `0ae3e4b4305fc4bf` | Negative radians conversion factor, binary64 |

## Model choice and primary evidence

Chosen implementable native candidate: math-only
[Bochs FPATAN](https://github.com/bochs-emu/Bochs/blob/0a93954eb5debb16dd43b3f02337ee4e6c088b92/bochs/cpu/fpu/fpatan.cc),
pinned to `0a93954eb5debb16dd43b3f02337ee4e6c088b92` from `git ls-remote`.
The upstream algorithm uses 33 arc-tangent lookup entries, small/large-ratio
polynomial branches, and
[67-bit chopped / 64-bit nearest intermediates](https://github.com/bochs-emu/Bochs/blob/0a93954eb5debb16dd43b3f02337ee4e6c088b92/bochs/cpu/fpu/f128_67.cc).
Its [CPU FPATAN wrapper](https://github.com/bochs-emu/Bochs/blob/0a93954eb5debb16dd43b3f02337ee4e6c088b92/bochs/cpu/fpu/fpu_trans.cc)
forces PC extended, retaining RC/masks. The native port follows that wrapper;
the main game remains native C++ and no CPU emulator is linked.

Intel documents that PC affects basic arithmetic and square root, whereas
transcendental results use extended precision. Intel's stated error bounds
allow FPATAN to differ from ideal correctly-rounded mathematics; equality
with a general libm is therefore insufficient. See
[Intel SDM Volume 1 §§8.1.5.2 and 8.3.10](https://cdrdv2-public.intel.com/874241/253665-090-sdm-vol-1.pdf).

[MPFR](https://www.mpfr.org/mpfr-current/mpfr.html) provides precise arbitrary
precision and correctly-rounded atan2 with explicit rounding. It would be a
useful mathematical test oracle, but its ideal result does not reproduce the
CPU's internal approximation automatically. MPFR/GMP is also a larger runtime
closure than the selected 26 math translation units. MPFR current licensing
is LGPL 3-or-later. No MPFR runtime dependency or unrelated installation was
added. A host-libm or direct C long-double polynomial port is not selected:
it would still need a software representation and exact intermediate rounding
to satisfy this project's x87 requirement.

The retrieved primary musl [atanl source](https://git.musl-libc.org/cgit/musl/tree/src/math/atanl.c)
uses host `long double` arithmetic with branches for 53/64/113-bit significands;
its 53-bit branch calls `atan`. The [asinl source](https://git.musl-libc.org/cgit/musl/tree/src/math/asinl.c)
is also a host-type polynomial implementation. The sources retain their
per-file Sun/FreeBSD notices alongside musl's
[copyright inventory](https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT).
These are viable general libm algorithms, but porting them to software ext80
would be additional work and would still not model Intel's internal tables.
No musl code was added to the runtime.

## Implementation and checks

The retained dependency has 26 compiled sources and 13 headers, with upstream
per-file SHA-256 and one explicit native safety patch in
`sadx-native/third_party/bochs_fpatan/upstream.json`. The exact link closure
was resolved by symbols from compiled upstream math objects: no unresolved
dependency remains. Source/wrappers are native arm64 integer code; dependency
symbols use `sadx::bochs_fpatan` and hidden visibility. Licensing notices and
both upstream license files are retained. Runtime use requires explicit
`X87Transcendental::fpatan_bochs_p5_p6(ctx,y,x)` selection.

Built `libbochs_fpatan.a`, compiled and linked the native context plus candidate,
and ran `sadx-native/build/x87_transcendental_test` successfully. Checks cover
all 12 PC/RC combinations, the forced input angle (-127,+0), both zero signs,
all finite/infinite quadrants, quiet-NaN payloads in either operand, tiny finite
ratios, unmasked precision/pending exceptions, cross-thread context rejection,
true ext80 subnormal input, and every table interval via 8192 monotonically increasing ratios plus odd
symmetry. These are model/contract checks, not recovered gameplay captures.

UndefinedBehaviorSanitizer found an exact-zero remainder normalization defect
in upstream `s_normRoundPackToExtF80.cc`: zero produced a leading-zero count
of 64 and reached `softfloat_shortShiftLeft128`, whose domain is 1..63. The
native derivative returns canonical signed zero at that producer before
normalizing. The patch retains notices and records both upstream/local digests;
it avoids depending on compiler/CPU shift behavior at equal operands and table
knots. All model tests subsequently passed with the complete vendor closure
instrumented by UndefinedBehaviorSanitizer.

Remaining unknowns: actual Intel Ultra FPATAN bits/C1 under each RC, complete
CRT asin error/callback state, full x87 deferred exception delivery, actual
modded gameplay call inputs, and end-to-end integer angle comparison on the
Windows reference. Current Unicorn binary32 instruction comparisons cannot
establish transcendental ext80 hardware fidelity. Performance of the whole
game is unmeasured; this bounded math closure alone does not establish frame
rate or Metal/controller readiness.

## Function-local original floor-angle comparison

`tools/verify_original_floor_angles.py` and
`tests/native_floor_angles_probe.cpp` now execute/compare the actual original
GetNormalAngles `0x4b8e80`, CRT asin `0x645020`, control/error return helpers,
and `_ftol` `0x644340`. The oracle supplies actual source-backed constants and
caller-selected runtime word `0x3c837a8`; it never replaces FPATAN, asin,
integer conversion, or an error callback. The native probe explicitly uses
the Bochs P5/P6 model through the reconstructed floor owner.

The retained `_reference/sadx/original-floor-angles-comparison.json` covers
1,915 cases for each of 12 masked PC/RC control-word combinations: 1,656
floor cases and 233 standalone CRT cases with explicit policy1 bypass,
plus 13 floor and 13 CRT cases with explicit policy0. Inputs include source
threshold neighbors, axis cases, zero signs, subnormals, NaN payloads,
infinities, values beyond the normalized domain, 1,200 generated normalized
vectors, 300 unnormalized vectors, and 200 finite CRT arguments. Distinct
output sentinels remain visible when native evaluation stops at a missing
runtime boundary. Unknown-policy native cases are recorded separately rather
than supplying an invented memory word to the original.

Observation: all 19,872 policy1 floor outputs match, with matching restored CW
and no dispatcher boundary differences. The original standalone CRT ext80
results differ in 2,048 policy1 comparisons. Example, CW `0x007f`, input bits
`0x3f7ffd5f`: original significand `0xc7ea599eee818800` versus native
`0xc7ea599eee818780`. These differences remain evidence;
the native model was not weakened to reproduce host-double transcendental
emulation. Exception flags differ on many inputs.

For policy0, the harness stops immediately before the real dispatcher call
`0x648b6b`, after the original FSTP64 spill, instead of inventing a callback.
Unicorn's missing precision flag causes 44 boundary disagreements across
the 11 non-`0x027f` control profiles for ±0.5: native evaluation correctly
exposes a missing handler and retains its source-site CW and angle sentinels,
whereas the emulator takes an ordinary return. These conditions are distinct
from final-angle arithmetic differences. The comparison exits 1 because
extended-register/boundary equality is not established, while explicitly
recording `bypass_floor_output_control_and_boundary_match: true`.

The report preserves every discrepancy, exact hex register/store bits,
case-blob SHA-256, source routine and native source/header identities,
library SHA-256, embedded compiler identity, declared build profile, and
dependency metadata identities. The weaker emulator is not an original Intel
Ultra hardware oracle; initialized CRT policy, callbacks, complete exception
delivery, and modded gameplay input traces still need their own evidence.
