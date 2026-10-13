# Native software x87 arithmetic: precision repair

2026-10-11. Objective remains native PC SADX with maximum fidelity to the
supplied executable and required updated mods. Software extended arithmetic is
compiled to native arm64; it does not run a guest CPU or Windows runtime.

## Producing evidence and primary sources

The pre-repair instruction comparison in `original-math-comparison.json`
executed selected exact i386 function bytes from supplied `sonic.exe` under
Unicorn 2.1.4 and compared stored binary32 results against native helpers. Input
SHA-256 is
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`;
baseline native probe library SHA-256 was
`f446984554587f61f7c6b1aa95d8b768328e4c1113d941fe187a82edf62d04e6`.
Under experimental CW `0x027f` the selected cases passed. Under CW `0x037f`,
two dot-product cancellation cases returned `1.0f` from original instructions
and `0.0f` from binary64 native arithmetic. One case uses vector
`{-2^60, 1, 2^60}` dot `{1,1,1}` with observed summation `(z+y)+x`.
This evidence changes the arithmetic representation: host double is inadequate
for extended-register precision. The harness does not observe actual game
startup/control state, execute the full game or prove original x86 hardware
equivalence. Integrated post-repair comparison is owned by the root worker.

Implementation uses [Berkeley SoftFloat 3e](https://github.com/ucb-bar/berkeley-softfloat-3/tree/a0c6494cdc11865811dec815d5c0049fba9d82a8)
pinned at revision `a0c6494cdc11865811dec815d5c0049fba9d82a8` (GitHub HEAD
queried directly). Initially a 36-file C dependency closure, now expanded to 39
files for signaling comparison, then 52 for coordinate/CRT conversions,
plus headers and license is
vendored under `sadx-native/third_party/softfloat`. Upstream files are unchanged;
`upstream.json` binds all 60 current upstream file digests (44 in the initial
arithmetic subset), and local verification passed.
The repository snapshot calls itself 3e and includes subsequent upstream
additions; revision pinning identifies the actual consumed implementation.

The [official interface documentation](https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat.html#6.3)
defines ext80 arithmetic precision controls and canonical representations.
Its arithmetic precision settings 32/64/80 select 24/53/64 significand bits;
conversion to binary32 uses rounding control independently of that precision.
The [official porting documentation](https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat-source.html#5.2)
defines the `8086` specialization, implementation-specific NaN handling and
thread-local build variables. Native integer implementations are portable
across the supported host architectures; specialization selects source x87
semantics, not the hardware CPU that executes the C code.

The [Intel architecture manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
and [Volume 1](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf)
are the primary control-word/exception references, particularly sections 8.1.5
and 8.5. Denormal operand is an additional x87 exception beyond SoftFloat's five
IEEE exceptions, and is explicitly detected at binary32 load/ext80 operand
boundaries here. Actual complete x87 status and hardware exception delivery
remain outside this wrapper.

License is [upstream COPYING.txt](https://github.com/ucb-bar/berkeley-softfloat-3/blob/a0c6494cdc11865811dec815d5c0049fba9d82a8/COPYING.txt),
the Berkeley three-clause BSD-style license. The full notice and every vendored
file's notice are retained. Source redistribution must keep them; binary
redistribution must include the notice in documentation/materials. No purchased
dependency is required.

## Owning representation and integration contract

Added `sadx-native/src/x87_math.hpp/.cpp` and `tests/x87_math.cpp`. Existing
Ninja helpers and Makefile are not edited by this worker.

```cpp
sadx::X87Context context(captured_or_explicit_control_word);
sadx::X87Scope scope(context);
auto& fp = sadx::X87Context::current();
auto sum = fp.add(fp.from_float(first), fp.from_float(second));
float stored = fp.to_float(sum);
```

`X87Value` owns canonical sign/exponent plus a 64-bit integer significand, with
no host `long double` assumptions or arbitrary guest-memory constructor.
Operations are `from_float`, `add`, `subtract`, `multiply`, `sqrt`, `to_float`;
`negated()` performs an exact sign-bit flip. Keep intermediate values as
`X87Value` until an actual binary32 store in the source instructions.

Control words are explicit, retained without normalization and map PC 00/10/11
to 24/53/64 significand bits. Reserved PC 01 yields tagged `X87Error` rather
than silently selecting a different representation. All four RC settings map
to nearest-even, downward, upward and toward-zero arithmetic/stores. Extended
exponent range remains in force even for reduced significand precision.
No default game control word is inferred. Missing active scope is an explicit
failure; a newly created native context starts with clear software flags, which
does not assert original-process initial flags.

Each `X87Context` belongs to the constructing thread; arithmetic, control/status
access and updates reject cross-thread use. A thread-local active-context
pointer owns scoped selection, with nested scopes restoring the previous native
owner. Each operation saves/restores SoftFloat's thread-local rounding,
precision, tininess and exception variables. Different contexts on one thread
and independent contexts on different threads therefore cannot contaminate
one another's environments.

SoftFloat uses the `8086` specialization and post-rounding tininess. Its five
IEEE exception flags map to architectural low status bits; binary32/ext80
denormal inputs contribute DE. Context flags are sticky and can be explicitly
cleared. Supported unmasked events produce `X87Error(unmasked_exception)` with
the exception bits and preserve them. Pending unmasked flags block subsequent
evaluation until explicitly cleared/masked. This is a fail-fast native
boundary, not an implementation of x87 deferred interrupt/trap timing, stack
faults, register tags or condition codes.

## Build contract

`third_party/softfloat/sources.txt` is the exact current 52-file C source list. Compile
it as C11 with include directories `third_party/softfloat`,
`third_party/softfloat/source/include`, and
`third_party/softfloat/source/8086`. Every source includes `platform.h`, which
selects little-endian, `SOFTFLOAT_FAST_INT64`, no optional inlining, and Clang
`__thread` in BOTH C and C++ for matching Darwin TLS linkage. Using standard
C++ `thread_local` declarations with C `_Thread_local` definitions caused
unresolved C++ wrapper symbols on Darwin, so that mixed build was corrected.
Client inclusion uses the same platform configuration before `softfloat.h`.

```sh
sh sadx-native/third_party/softfloat/build.sh /private/tmp/sadx-port-sources/softfloat-build
xcrun clang++ -std=c++17 -Wall -Wextra -Werror -ffp-contract=off -O2 \
  -Isadx-native/src sadx-native/src/x87_math.cpp sadx-native/tests/x87_math.cpp \
  /private/tmp/sadx-port-sources/softfloat-build/softfloat.a \
  -o /private/tmp/sadx-port-sources/x87_math_test
/private/tmp/sadx-port-sources/x87_math_test \
  /private/tmp/sadx-port-sources/ninja-trig-table.bin
```

The build script compiles only manifest sources and rewrites the archive from
their exact object list; stale build members cannot enter it. Outputs belong
in ignored build space, not vendored source. Link `softfloat.a` after C++ callers.

## Verification and remaining fidelity limits

Optimized native arm64 execution passed. The wrapper/test sources also passed
AddressSanitizer and UndefinedBehaviorSanitizer with `-O1 -g
-fsanitize=address,undefined -fno-omit-frame-pointer`; the upstream archive in
that check was the ordinary C11 optimized build, not separately instrumented.

Cases include:

- Demonstrated cancellation repair: 53-bit returns zero; 64-bit returns one.
- Distinct 24-bit significand behavior and extended exponent range at all PCs.
- Positive/negative halfway results for all RC modes and changed store rounding.
- Sticky invalid/overflow/underflow/inexact/denormal flags; explicit unmasked
  invalid/denormal failures and pending-error/clear behavior.
- Bit-preserved finite values, signed zeros, infinities and quiet NaN payloads;
  signaling NaN quieting/payload preservation under the 8086 specialization.
- Nested scope restoration, alternating same-thread contexts and 10,000
  cancellation operations per concurrently running thread, plus foreign-owner
  rejection.
- 65,536 original-table interpolation projections across the two tested PCs
  (1024 chords x 16 fractions x two signs x two precision modes), matching the
  prior binary64 projection where it was already observed to be sufficient.

The original quarter-table SHA-256 is
`3acbb71a623df2d12dc44e34a48caf049d0e50560593ed003ca15d827947feed`;
the test additionally pins its 4100 bytes with FNV-1a64 `b047ecf880b993c2`.

These tests validate this representation and its integration contract, not full
SoftFloat conformance or a functioning game. Original NaN priority/payloads,
exception flag/timing equivalence, noncanonical ext80 input, wider instruction
sets and actual startup/mod-modified control state require further evidence.
The integration must preserve source operand order and actual spill/store
locations. Full updated-mod gameplay fidelity, native GPU operation, controller
verification and speedrun readiness remain unfinished.

## Subsequent integration and signaling FCOM stage

The current `original-math-comparison.json` was inspected after root integration:
it now reports passing stored-value comparisons for all twelve experimental
control words (24/53/64 significand bits crossed with four rounding modes).
Each mode compares 131,072 sine/cosine results, 1,009 dot and squared-magnitude
pairs (including cancellation, NaN and subnormal fixtures), and twelve signed
roots. Native library digest in that passing record is
`e141f503d1c04717a196c7e67f3f249f7d52b6e266e92ab9dd8edbe64821014b`.
This strengthens function-local stored-value evidence under the emulator;
its original runtime/hardware/control-state limits still apply.

`X87Context::compare` now models signaling FCOM, not FUCOM. Upstream
`extF80_eq_signaling`, `extF80_lt`, and their primitive `s_lt128` are included
unchanged, expanding the pinned closure to 39 C sources. Both quiet and signaling
NaN pipelines raise invalid; masked invalid returns `unordered`. The helper
`x87_comparison_status()` returns C0/C2/C3 status bits: less `0x0100`, equal
`0x4000`, greater zero, unordered `0x4500`. No host float comparison substitutes
for the source signaling comparison. Original hardware NaN/exception delivery
equivalence is not inferred from this implementation choice.

Comparison tests passed for signed-zero equality, negative/positive order,
subnormal inputs, opposing infinities, quiet/signaling NaN load/compare pipelines
with masked and unmasked invalid, condition-code encoding, and concurrent
contexts where one thread accrues invalid while the other does not. Canonical
signaling ext80 register values were not directly accepted at this stage;
binary32 signaling NaNs quiet during the load conversion, with invalid retained.
The subsequent validated source-constant import below now permits canonical
signaling NaN data without pretending to decode arbitrary CPU register state.

The full local PGetSpeed reconstruction uses these comparisons and preserves
its x87 register/store order. Its separate report,
`player-speed-provenance.md`, records 37,380 isolated original-instruction
comparisons with explicit input/rotation test doubles: stored values, untouched
acceleration and call counts match; emulator/native exception flags differ and
are reported separately. That flag discrepancy remains unverified against
original hardware and is not hidden by the passing value result.

## Coordinate/CRT arithmetic additions

The coordinate owner required native ext80 division for actual vector
normalization. The root also requested exact binary64 and signed-int64 loads,
current-rounding FISTP64 conversion, and binary64 stores for source CRT asin /
integer conversion. Added public methods are `divide`, `from_double`,
`from_int64`, `to_int64_current_rounding`, `to_double`, and
`from_canonical_ext80`.

The latter validates source CONSTANT encodings; it is not arbitrary CPU-state
decoding or FLD80 instruction execution. Exponentzero with an explicit integer
bit, and nonzero exponent without it, are tagged unsupported/noncanonical.
Canonical signedzeros/subnormals/infinities and both quiet/signaling NaN
encodings are retained exactly. Data import itself does not quiet payloads or
invent arithmetic flags; subsequent arithmetic applies its own exception rules.
Original PI/2 `0x3fff:c90fdaa22168c235` and negative qNaN
`0xffff:c000000000000000` constants are supported as requested. Source register
state with unsupported encodings remains outside this API.

Focused tests pass for exact FILD64 minimum/maximum values independently of PC,
binary64 loads, four FISTP64 rounding directions, masked invalid integer
indefinite, division/zero-denominator flags, PI/2 binary64 store, exact imported
NaN payload bits, noncanonical rejection, canonical ext80 subnormal comparison
and actual signaling-ext80 FCOM masked/unmasked invalid. The unchanged pinned
SoftFloat closure now contains 52 C sources and 60 upstream files including
headers/license; their manifest digests were revalidated.

`player-coordinates-provenance.md` records 23,724 coordinate/primitive original
instruction comparisons with no rotation/normalization overlays. Stored
vectors, complete frame bytes and stack depths match; emulator flags differ
and remain separately unverified. General floor-normal transcendental/CRT
integration is a separate owner; no host atan/asin approximation is introduced.

## Memory arithmetic instruction forms and NaN producer repair (2026-10-11)

The actual source `njAddVector` at `0x785170` loads world X with FLD m32,
then adds effect X directly with FADD m32. Treating its second operand as
another FLD incorrectly quieted a signaling memory NaN before the combined
instruction selected its result. Source 25 bytes are
`d901d802d919d94204d84104d95904d94208d84108d95908c3`; SHA-256 is
`6062ff68472aa60aea2d73c7cabbd34a40ff4ba2bdf10473cf48d7b33a730fb2`.
Y/Z load effect first and add world memory; this operand order is preserved.

This repair follows architectural documentation, rather than changing stored
outputs to satisfy the emulator. Intel's masked x87 NaN rules select a quiet
source when paired with a signaling source; two quiet sources select the
larger significand. Exception priority puts NaN handling and zero division
ahead of denormal processing. [Intel SDM Volume 1, revision 090,
§4.8.3.5/Table 4-8 and §4.9.2](https://cdrdv2-public.intel.com/874241/253665-090-sdm-vol-1.pdf)
documents these rules. FLD m32/m64 converts to extended precision and reports
invalid for signaling NaNs; FLD m80/register has a different boundary.
[Intel SDM Volume 2, FLD entry, printed page 3-402](https://cdrdv2-public.intel.com/774492/325383-sdm-vol-2abcd.pdf)
establishes this instruction distinction. Combined with the single-source NaN
conversion rule in Table 4-8, the retained quieting behavior of `from_float`
and `from_double` is derived from these primary sources.

The arithmetic owner now has explicit m32/m64 methods for add, multiply,
subtract/reverse subtract, divide/reverse divide and signaling FCOM. Their
memory decoder widens NaN sign/fraction bits without quieting; unchanged
SoftFloat 8086 propagation owns the actual arithmetic result and invalid flag.
Finite widening is exact. No vendored source was changed and no C sources
were added. All context/environment/exception ownership remains scoped to the
calling thread. Reverse methods mean memory minus/divided by the register.

The audit changed actual memory-operand sites in Ninja interpolation/dot/signed
sqrt, gravity and axis limits, player shadow radius, PGetSpeed and vector-add
tail, matrix coefficients/normalization, floor thresholds/scales and CRT asin's
quieting addition. Register-register instructions stay register operations.
CRT integer conversion has register subtraction and needed no memory method.
The input producer adopted the same methods through its separate owner.
The audit also retained original cached FLD operands and relevant store order;
there is no downstream NaN-payload substitution.

`make -C sadx-native all test` passed. Focused tests cover all twelve explicit
PC/RC profiles, memory SNaN versus separate-FLD payload selection, both widths,
all operation families, comparison codes, finite denormals, reverse orientation
and masked/unmasked invalid/denormal priority. Additional unmasked priority
tests passed independently after the broad gate. The complete arithmetic owner
and its tests also passed AddressSanitizer/UndefinedBehaviorSanitizer (the
unchanged SoftFloat archive was linked without sanitizer instrumentation).

Reusable test-only producer `tests/native_x87_memory_probe.cpp` and verifier
`tools/verify_original_x87_memory.py` retain raw integer input bits across the
native boundary. Immutable wrappers execute FLD m32/m64 plus each actual memory
arithmetic instruction form and store ext80. These wrappers are explicit test
instruction fixtures, not captured game execution. The verifier independently
pins the original njAddVector bytes above. Command:

```sh
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -arch arm64 -dynamiclib -Isadx-native/src sadx-native/tests/native_x87_memory_probe.cpp sadx-native/build/libsadx-core.a -o sadx-native/build/native-x87-memory-probe.dylib
_reference/sadx/python/bin/python sadx-native/tools/verify_original_x87_memory.py "Sonic Adventure DX/sonic.exe" sadx-native/build/native-x87-memory-probe.dylib --output _reference/sadx/original-x87-memory-comparison.json
```

Each of twelve CW profiles has 4,536 fixtures. The 4,032 fixtures whose initial
FLD source is not signaling all match full ext80 results and FCOM conditions,
including signaling NaNs in the memory arithmetic operand. The 504 initial
signaling-NaN FLD fixtures retain 120 strict differences per profile: Unicorn
leaves the loaded NaN signaling, contrary to the documented conversion rule,
affecting unchanged registers during FCOM and subsequent NaN selection.
All 1,440 value/condition differences are retained in the mismatch artifact.
The strict report remains false and the verifier exits 1; the separately
reported non-signaling-register-load profile passes. Flags differ in
2,445/2,399/2,379 cases per profile at PC24/53/64 respectively. For example,
Unicorn reports zero/zero division as ZE while native reports architectural IE.
These emulator differences are not repaired in the native arithmetic owner.

Focused fixture blob SHA-256:
`dfa27fbf3d01632b5e305e90d80ecfcada68df7ba313766f9a41cbefb7e37e5c`.
Mismatch artifact SHA-256:
`6e561432a63bf0876bc68848c4d829d1e914911c6f02fa7ad750e691cd9aef95`.
Focused native library SHA-256:
`e672a90ea605265992f8e0162e1c387f23a6324fc21848e3d9884730798580ba`.
Report SHA-256:
`fbd8e500a70db0f46f470afda30219b0b6fc3e3eeb85d959ca10e85b5fc1ddc2`.

The newly regenerated real movement comparison has 4,115 fixtures per each
twelve CW profiles (49,380 comparisons), with no instruction overlays. Stored
values, untouched fields, call counts and complete matrix/frame state now
match. Cases 3103..3105's original X payload discrepancy is resolved at the
memory arithmetic producer. Fixture SHA-256:
`62af8ef670e3b88e54311c38e96f65dd720615c04599674918f26e92bbeec321`;
native-library SHA-256:
`ea547e712f9e80e438b3d211c77db3373509b55907f7b408f94cd3822e0e5d09`;
report SHA-256:
`a5c99e6cf4bca4e5d7183096bb57be90b0511f13e54e337398499c3bcc39ec8e`.
This is source-call composition with explicit normalized-input/runtime/matrix
fixtures and selected CRT matherr bypass policy, not startup/mod execution.
Exception flags remain separately mismatched. Hardware NaN results, full x87
exception delivery/C1/status state and actual mod-selected runtime FP state
remain unverified. A complete playable native game remains unfinished.

Root's final source-foundation gate subsequently passed all nineteen native
test programs and four Python tests. All eleven existing original-instruction
comparison library digests were checked against the freshly built probes.
The focused memory comparison is the separately retained strict-failure
record described above; it is not an additional complete-pass assertion.

## FUCOM required by axis rotation

The actual `0x788df0` matrix owner uses FUCOMPP, so a distinct
`compare_quiet(X87Value,X87Value)` was added. Quiet NaNs return unordered
without invalid; canonical signaling NaNs raise invalid. FCOM and memory
arithmetic APIs retain their existing behavior. The pinned unchanged
`extF80_eq.c` was added to the manifest and build closure; finite ordering
reuses extF80_lt after NaN classification. The closure now has 53 C sources
and 61 pinned upstream files. No SSE backend or shared TLS globals changed.

Focused tests passed both QNaN operand positions, SNaN/QNaN competition,
signed zeros, finite order, QNaN versus denormal priority and unmasked invalid
behavior. This describes canonical supported register values; arbitrary
noncanonical CPU-state decoding and hardware deferred delivery remain outside
the API. `axis-projection-provenance.md` records the actual complete axis and
seeded projection-tail instruction comparisons, including separately retained
emulator flag differences and incomplete gameplay/producer bounds.
