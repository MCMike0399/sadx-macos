# Native source CRT square root

Target sonic.exe SHA256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.

The binary64 entry0x6452a4 calls classifier0x648bd5 and body0x6452ad. The
special classifier constructs an ext80 NaN directly, preserving its signaling
bit instead of first executing FLD64. Finite inputs use the source internal
control-word setup; negative zero and positive infinity bypass FSQRT. Negative
nonzero inputs load the source indefinite encoding. Common return/error policy
preserves operation identity5, partial results and missing runtime facts.

`crt_sqrt_extended` models the separate retained-ST0 wrapper0x645290, including
its binary64 classification spill. That wrapper is not covered by the binary64
comparison profile and must not inherit its verification claim.

REA dossiers are retained in mesh-math-dependencies/crt-square-root-body.json
and mesh-resource-dependencies/sqrt-classifier.json; exact LLVM disassembly
corroborates instruction boundaries. Owned Hopper sessions were exported and
closed with verified launcher cleanup. Public API body-range coverage remains
limited as recorded in each Evidence.

Private native probe comparison:1019 binary64 inputs×12 maskedCW profiles=
12228 executions, zero ext80-return/CW mismatches,6144 low-six exception-status
differences. Matherr bypass1 is an explicit test seed, not an observed Windows
process fact. Source-policy/unknown-handler/signed-zero/NaN payload unit tests
pass. Actual Intel flags/delivery, full CRT error handlers and full gameplay
remain unverified. Production Makefile integration has been added; the full
combined rebuild waits for concurrently edited task/geometry owners to freeze.

Production integration update: the combined arm64 gate now passes44 native programs and4 extraction tests. Current production probe/report digests are pinned in continuation.json; older private-library measurements above remain historical. Full game/hardware equivalence is still unverified.
