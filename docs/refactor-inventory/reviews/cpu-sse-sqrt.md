# CPU scalar SSE and CRT square-root inventory

Reviewed Main `bda836fc8c14fc83458703fb64b408f6e2ad8f36` in the supplied isolated
checkout. The [fingerprinted fragment](cpu-sse-sqrt.json) reviews every body in
exactly **four files, 220 lines and two complete modules**.

| Module | Header / implementation lines | File dispositions | Module disposition |
| --- | ---: | --- | --- |
| `src/cpu/sse_math` | 51 / 129 | Header conforms; implementation needs migration | Needs migration |
| `src/cpu/crt_sqrt` | 9 / 31 | Both need migration | Needs migration |

Totals: **1 conforming file, 3 migration files; 2 migration modules**. No open,
reference-purpose or obsolete classifications. Two actionable findings:

- **CPU-SSE-1:** expand compressed statements and name local operation/status
  roles throughout the scalar implementation. The 32-bit finishing helper and
  double-subtraction tail repeat width-specific underflow/FTZ/overflow policy;
  preserve their ordering and factor only where the same comparison supports it.
  Keep the typed API and narrow numeric boundary. Hex encodings, binary words,
  selected rounding and explicit status are legitimate; host float replacement
  has no proof here.
- **CPU-CRT-1:** clean up the complete CRT root body and entry provenance while
  separating source-PC diagnostics and unavailable policy facts from its native
  contract. The shared `finish_crt_inverse` owner lives in `crt_asin.cpp` and also
  serves acos. Any contract migration must include those clients and mesh
  normalization, preserving pending stores, handler reload and failure prefixes.
  Resolve entry names before adding canonical annotations. Keep recovered
  classification/error behavior in CPU; introduce no generic framework or default
  matherr policy.

`SseContext` owns MXCSR and its construction thread identity. Each operation saves
and restores SoftFloat TLS rounding, tininess and flags, including on an unmasked
exception. SSE leaves ext80 precision untouched; x87 saves/restores that fourth
TLS field separately. The SSE archive supplies distinct NaN propagation and reuses
the sibling archive's sole TLS owner. All 20 SSE and 62 sibling upstream pin
entries match available bytes. This is provenance identity, not source approval.

Signed zero, NaN selection, DAZ, FTZ, all selected rounding modes, sticky status,
early denormal fault priority and destination suppression are part of the scalar
contract. This does not establish packed-lane atomicity. Display owns production
SSE state; native input borrows it. Reference NativeGame and invocation-local
D3DX/HLE contexts have separate lifetimes. The inspected D3DX overrides do not
publish local SSE sticky flags back to guest MXCSR; that adapter limit remains
outside these four dispositions.

CRT square root retains separate binary64 and retained-ext80 entry contracts.
The latter spills for classification while retaining the original arithmetic
operand. Missing policy/handler boundaries deliberately preserve partial values,
stores and the source CW instead of rolling back. Lower arithmetic TLS restoration
is separate from CRT CW restoration. SSE sqrt and NinjaLength's signed-root return
are different contracts; neither is a duplicate CRT implementation. No maintained
source caller of `crt_sqrt_extended` was located, but it remains a supported
compatibility entry. The actual binary64 caller is mesh normalization, outside
the current native source list; exclusion does not remove maintained scope.

Historical SSE remains **FAIL: 5,076 value mismatches** in 312,112 evaluations,
plus 232,720 MXCSR differences. The retained report attributes value differences
to emulator NaN/FTZ/MINSS behavior and reports zero unclassified differences;
this does not establish Intel parity or change the verdict. Historical CRT remains
**FP partial: 12,228 binary64 calls, zero value/CW mismatches, 6,144 flag differences**.
Its seeded bypass policy and masked CWs do not qualify retained-entry/error-handler
behavior. Axes retain 93,104 MXCSR differences; the producer retains 108,240
exception differences. Existing failures and partials remain explicit.

The tracked task-foundation gate at `353d528` pins all four reviewed files exactly
and records their registered units. Dedicated SSE/CRT reports lack C++ transitive
source pins; binary hashes cannot fill that gap. The available full historical
reports differ from the raw hashes named by current summaries: timing differs,
and CRT also names a different library. Aggregate results agree, but exact raw
receipt identity is unavailable. The fragment fingerprints
current reports, probes, scripts and dependencies. Tracked historical CRT dossiers
are available, with null raw results and incomplete Hopper body-range coverage;
external binaries, raw libraries/streams and hardware captures were not inspected.
No dedicated SSE/CRT mutant or retained-entry holdout receipt was located.
NinjaLength's historical mutations and holdout belong to its separate primitive.

Before source work, freeze complete inherited failure identities and outputs and
qualify omitted scalar operations, TLS restoration through failure, retained CRT
classification and actual consumer publication. No builds, tests, oracles,
mutations, providers, network/host/storage actions, delegates, source/judge edits,
Main/build writes, generated inventory/STATUS/HANDOFF edits or push were performed.
Only this note and fragment are committed. Root owns reconciliation; whole CPU,
math, Input, player and campaign closure remain open.
