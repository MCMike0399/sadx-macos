# Source CRT asin / floor-angle review and native integration tests

2026-10-11. Exact supplied PC executable SHA-256:
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Reviewed source owners: `crt_asin.hpp/.cpp`, `floor_angles.hpp/.cpp`.
Retained instruction records: `crt-asin.asm`, `crt-asin-helpers.asm`,
`floor-normal-angles.asm`. No Makefile changes by this worker.

## Genuine static records used

The records below were read from structured REA contents, not inferred from
filenames. They share the source digest and Hopper analysis-profile digest
`8c226db6a8bd36b662dc1cb83ab0ad0af698230fc322b58c6c4870878bf6ae70`.

| Function | Evidence ID |
| --- | --- |
| CRT asin entry `0x645020` | `ev_ccd0f5e6db8243bc51071fa5da1e6d06ea8dedc8653bd890b9c71cfdfa79ffb5` |
| CRT body `0x64503d` | `ev_f8ca953e98680992008f1357259af82809cfeb439d5c36e1897c6a51a89794a6` |
| Input classifier `0x648c18` | `ev_6f0a3bf2adc3a2b124ad27eb42846e82e26ef1c2d88df094629327777cdda989` |
| Temporary control setup `0x648ba5` | `ev_0efb0550b71bac9236e47c3a5c0954466e97d6b4723c593f6a01bf42c4322204` |
| Return / precision dispatch `0x648c3b` | `ev_3a18ebdde6be070a5be7df8d188638f433a829d63d603f240c66d05d26a55a00` |
| Matherr dispatch wrapper `0x648b47` | `ev_9148a72919ac38c386959b85f1efe52ba23947be95bca9718ca2c91648389133` |
| Normal angles `0x4b8e80` | `ev_406d833fc9e52ca16d6a8951f8e7e10e585092b06b6021229888cb05cb7b17c9` |
| CRT integer conversion `0x644340` | `ev_e5cbb45a2dffc49e226e29af4afc29c5bbe91f467f3cf420f4120fe6167be7b7` |

These remain static reconstructed representations, not execution of a running
Windows game or proof of hardware FPATAN/exception behavior.

## Reviewed semantics and evidence-driven fixes

CRT entry FST64 classifies its binary64 spill while retaining extended ST0.
Finite inputs change the internal control word to `(saved & 0x0300)|0x007f`
unless the incoming word is exactly `0x027f`. Nonfinite entry bypasses that
temporary setup. In-range arithmetic order is `(1+x)*(1-x)`, square root,
FPATAN; exact endpoint classification loads the source signed PI/2 constant.
Classification is deliberately based on the spill, so unrounded values just
below/above one can still take the source endpoint path.

Matherr runtime state remains explicit: nonzero source word bypasses dispatch;
unknown state is tagged rather than assumed. With zero policy, domain/nonfinite
cases need the real dispatcher; ordinary cases enter precision dispatch only
under the source saved-CW/precision-mask/sticky-PE conditions. Exactly default
`0x027f` bypasses the precision branch even with prior sticky PE. Dispatcher
first stores binary64 result, invokes the handler, then reloads result BEFORE
restoring the saved control word. Missing/unavailable handler retains the call
site's control state and pending stored result; there is no invented rollback.

Fixes made from exact instruction/data evidence:

- Quieting FADD at `0x648bc9` uses source double `1.0` at `0x844df0`, rather
  than an assumed zero operand. It only affects the source NaN branch, but the
  producing representation now preserves the exact operand.
- Unknown policy boundary is `0x645066` for ordinary/endpoint returns and
  `0x6450cc` for domain/nonfinite error returns, matching their distinct reads.
- Floor FPATAN inputs explicitly load source normal X before Y; argument
  evaluation order is not left to the C++ compiler.
- Source `0x4b8f3a` stores inclination after FPATAN returns and before roll
  scaling/FIST conversion. `FloorAngleArithmeticError`, derived from X87Error,
  retains that known stored inclination and the subsequent failing source
  boundary if native arithmetic stops. It does not invent an unknown roll or
  claim equivalence with hardware's deferred exception delivery. Existing
  function/result ABI and X87Error catches remain compatible.

Floor branches match exact FCOM/status-parity meaning: only strict greater/less
take cutoff/near-axis shortcuts. Equality and unordered inputs follow their
actual paths; normals are not normalized, clamped or replaced. Exact cutoff
floats are ±`0x3f7ffd61` in binary32 magnitude; epsilon is `0x3b03126f`.
Angle-unit/factor doubles are `0x40f0000000000000`,
`0x3fc45f30b4e4e30a`, `0xbfc45f30b4e4e30a`, directly checked at
`0x7dccd0/0x7dccc8/0x7dcce8`. PI/2 and negative indefinite constants were
checked at `0x2bcc00a/0x2bcc000`.

## Native tests and supported integration

Added `tests/crt_asin.cpp` (no arguments) and `tests/floor_angles.cpp`
(source quarter-table and source unit-matrix paths). Optimized native arm64 and
AddressSanitizer/UndefinedBehaviorSanitizer wrapper/source executions pass.
The linked upstream math archives in these checks were ordinary optimized
builds; the Bochs producer also has its separate sanitizer verification.

Meaningful cases include all twelve precision/rounding control words; zero
signs and exact source endpoint constants; binary64 classifier boundaries;
unknown/zero/bypass policies; sticky PE and exact default-CW behavior; domain
dispatcher arguments; nonfinite bypass of temporary CW setup; handler-result
load-before-restore demonstrated with an explicit signaling result/unmasked
invalid; exact cutoff/epsilon equality versus next representable float;
near-axis shortcuts without normal normalization; partial first-angle store
when roll FISTP fails; and general floor inclination under source factors/CRT
integer correction.

The integration test binds PlayerCoordinates::floor_to_world to the ACTUAL
floor_normal_angles owner under explicitly selected bypass runtime policy.
General up-floor and slope paths traverse native CRT asin, selected Bochs
FPATAN, source integer conversion, source Ninja table and real matrix
operations, then restore the previous matrix owner. It contains no identity
normal-angle test double. An explicit zero-policy/missing-dispatch case fails
before matrix push instead of producing fake successful coordinates.

## Limits and remaining goal

Bochs P5/P6 is an explicitly selected software FPATAN model. These tests do not
establish mathematical correctly-rounded behavior as Intel hardware behavior,
nor verify the user's actual runtime matherr word/handler/control state. The
independent original-instruction floor-angle comparison belongs to the
transcendental worker and reports its own mismatches/flags. Real default CRT
error dispatch remains unresolved; supported integration requires explicit
runtime policy/handler inputs. Full native PC gameplay, updated required mods,
native GPU performance, controllers and speedrun fidelity remain unfinished.
