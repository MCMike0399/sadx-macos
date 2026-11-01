# Input policy semantic inventory

Reviewed baseline `353d528bbb1a50e8cb0e4cdeaf7657dc9ad8607d` in the supplied
checkout on `packet/input-policy-review`. This is an exhaustive body review of
exactly eight files, **724 lines and four complete modules**, not a diff review.
The fingerprinted [fragment](input-policy.json) contains every member, file SHA,
owner, actual consumer, accepted historical judge and remaining limit.
`input_initial_state` was inspected only as a dependency; its review is not repeated.

| Module | Header lines | Implementation lines | File dispositions | Module disposition |
| --- | ---: | ---: | --- | --- |
| `input` | 106 | 214 | Both already conforming | Already conforming |
| `input_mod_axes` | 32 | 68 | Both need migration | Needs migration |
| `input_mod_pad` | 143 | 136 | Both need migration | Needs migration |
| `input_mod_constants` | 7 | 18 | Both already conforming | Already conforming |

Totals: **4 conforming and 4 migration files; 2 conforming and 2 migration
modules; 0 open-review/reference-purpose/obsolete dispositions.** Qualification
gaps remain explicit even for conforming maintained source.

The action sampler is a bounded native policy: genuine unbound bindings, atomic
rebinding, selected keyboard ramp, per-stick arbitration and delayed edge commit.
Production uses `native::Input`, imported Windows key settings and raw SDL-domain
samples instead. Apple's normalized snapshots and explicit immediate keyboard
policy make no raw-controller fidelity claim. Numeric masks, scales and SSE input
words are legitimate encodings; they are not executable-address debt.

Two migration areas remain. Axes need readable named calculations and canonical
provenance, retaining exact operation widths, rounding/DAZ/FTZ, singular deadzones,
low16 conversion and partial stores. Their output optionals describe which stores
completed before a fault; they are **not unavailable fixture facts**. The pad owner
has actual fixture knownness, byte offsets, source-address diagnostics and reference
task identities in the game tier. Its complete native overload still forwards
through that representation. Migrate this owner with native input, reference
startup/frame composition and scene/player consumers, preserving raw-record aliases,
per-pad publication, Enable8's real overflow alias and live rumble callback order.

Before rewriting the pad, qualify its failure prefixes: `convert()` stages axis
outputs locally, so a later component SSE exception can prevent an earlier completed
component from reaching the pad. The original helper stores X before converting Y.
This is a static gap/potential inherited mismatch, not a newly executed failure.
The masked poll judge does not settle it. Arbitrary overlapping record copies and
callback mutation also lack discriminating qualification.

Historical axes evidence has 109,056 value comparisons with zero value mismatches
and 93,104 MXCSR differences. Pad has 1,044 cases with zero value mismatches and
300 MXCSR differences. Both remain **FP partials**. Pad source pins are stale;
axes lacks source pins. The native input report also has stale runtime pins.
All eleven coefficient-report pins match, and its seven words are exact; four
numeric mutants were recorded killed. Three boot mutants were recorded killed
against older reference composition. No dedicated axes/pad mutant or holdout
receipt was located. This checkout has no build directory or raw run libraries.

The historical MPEG gate pins all eight current source files. It does not refresh
all function oracles or certify this review. Existing SSE value failures and
transitive producer partials remain unchanged. Pinned deadzones/settings, Input then
Speedrun ordering, 63 FPS cadence, digital button versus analog/raw representations,
and CCEF/EHCF boundaries are traced in the fragment; gameplay callers and physical
hardware are not newly qualified.

No builds, tests, providers, downloads, network/host/storage operations, delegates,
source/judge edits, global/generated inventory writes, STATUS/HANDOFF edits or pushes
were performed. Main and its active gate/build were untouched. Root owns reconciliation
and subsequent judge/source work. Whole input, player, Session and campaign closure
is not implied.
