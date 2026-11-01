# Exact input-axis readability candidate

Branch `packet/input-axes-readability`, base `ec06b2d`.
Context `cf690f7`, default baseline `a9956ab`, native baseline `ef46d41`,
complete two-file source rewrite `c77091c`. This is not whole input/pad/CPU closure.

Both public APIs, struct layouts and five direct consumers retain their contracts.
The implementation names coefficient decoding, symmetric signed16 axes, length
measurement, direction, deadzone scaling and signed-low-word conversion. Ordered
SSE operations remain explicit because caller rounding, DAZ/FTZ, masks and fault
publication are part of the API. No host float substitution, guessed deadzone,
new owner, guest dependency or callback framework was introduced.

InputModAxesOutput optionals represent reached stores. They are not missing
fixture facts and are deliberately retained. Previous engaged values survive
unreached stores; the left Y/magnitude/X and right X/magnitude/Y sequences retain
their different conversion order, including aliased destinations. Integer
indefinite is reduced to signed low16 before controller clamping. COMISS/CMOVBE
selection remains distinct from MINSS. Singular deadzones preserve NaN/flags.

Before source edits, the unchanged actual-DLL judge froze all 109,056 native and
source records. Current full-stream hashes, every value, control, flag and all
93,104 FP-partial identities match. There are zero value failures; full-state
`passed` remains false. Source/native flag disagreement is not promoted to pass.

A separate before-source native characterization froze 327,680 cases and
24,903,752 output bytes across signed/raw edges, seeded and empty destinations,
rounding/DAZ/FTZ/masks, coefficient substitutions, aliasing and thread ownership.
Every byte remains equal. These are maintained-model contracts, not unobserved
Intel/Windows guarantees. Only third-party numeric archives are linked; both
maintained axes and SseContext translation units are compiled directly.

Eight copied semantic faults were attempted. Seven were initially caught; early
right-X publication survived. That receipt is retained in faults-initial.json.
An additional exact-power-of-two/half-integer case reaches a Y CVTT precision
fault after X converts successfully. It was replayed against immutable old source
and header blobs from `ec06b2d`, after the rewrite, without inventing a pre-edit
run. All old matrix and thread records remain unchanged. The augmented 327,681
cases / 24,903,828 bytes match old/current exactly, FNV9340279443059284959.
All eight final copied faults compile and are caught; faults-final.json records
source/test hashes and exact compiler commands. No invalid compile is counted.

Five direct consumer TUs compile: input, input_mod_pad, native_input and both Mac
controller diagnostics. Ordinary native source/product closure, affected pad and
runtime judges, final holdout and integrated gate remain Root work. The inherited
pad staging/publication concern and all SSE/CRT failures remain separate debt.

The helper entry now has an explicit descriptive INPUT symbol override, backed
by the retained body/provenance and unchanged instruction oracle. The keyboard
normalization is an interior source slice and is NOTSA, not an invented function
entry. Numeric coefficient provenance remains in symbol-decision.json; the value
record owns no DLL storage or PE reads. Full `make symbols` was attempted but the
ignored pinned sa-tools checkout is absent locally. No SDK download or fabricated
symbol database was used. Complete provenance validation must use available pinned
inputs before final integration.

The new native contract is registered with direct-source/header/third-party Make
dependencies. Initial linking with the base SoftFloat archive alone failed because
float32 members are in the SSE archive; the corrected two-archive build succeeds.
Source modification and judge additions are separate commits. Baselines, current
libraries, full streams and fault copies remain under ignored build/input-axes.
Do not remove pinned proof. Fresh independent STYLE12/spec review and final cohort
remain pending; no gameplay, physical controller,63FPS or campaign claim.
