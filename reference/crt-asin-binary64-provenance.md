# Binary64 CRT asin entry

The native `crt_asin_binary64` models source entry 0x645034, classifier
0x648bd5 and shared body 0x64503d from sonic.exe SHA256
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
The previously implemented retained-ST0 entry 0x645020 remains distinct: it
spills ST0 to binary64 before classification. Binary64 entry 0x645034 constructs
the ext80 nonfinite operand directly, retaining signaling NaNs until arithmetic.
This is the entry used by the actual polygon query 0x453820.

`crt_argument` now owns the shared binary64 loader used by asin and sqrt.
Tests distinguish quiet-NaN code7 from signaling-NaN code1, preserve raw argument
bits in explicit callbacks, and verify finite body sharing and control restore.
The square-root comparison rerun after this extraction matched all 12,228
returned ext80 values/control words; 6,144 exception-status differences remain.

The new original-instruction asin profile covers 692 fixtures × 12 masked
control words = 8,304 executions. It runs original classifier/body/return
instructions, or stops at the actual 0x648b6b matherr call under explicit policy0.
No FPATAN, handler or return instruction is replaced. Its strict result is
false: Unicorn's FPATAN retains only a host-double result (seen in zero low
11 significand bits), while the native model retains the Bochs P5/P6 ext80
approximation. Missing emulator precision flags also change policy0 dispatch
boundaries. Every disagreement is retained in
`original-crt-asin-binary64-comparison.json`; none has been filtered or used to
weaken native arithmetic. Finite results, Intel hardware FPATAN and real CRT
startup/error handlers are not proven equivalent by this profile.

REA evidence is retained in crt-dependencies/crt-asin-body.json,
crt-dependencies/crt-asin-classify.json and
mesh-resource-dependencies/sqrt-classifier.json; exact binary entry instructions
corroborate the different loading paths. The private comparison library is
pinned in the report. Production archive integration awaits the task/mesh
source freeze and combined gate.

Production integration update: the combined arm64 gate now passes44 native programs and4 extraction tests. Current production probe/report digests are pinned in continuation.json; older private-library measurements above remain historical. Full game/hardware equivalence is still unverified.
