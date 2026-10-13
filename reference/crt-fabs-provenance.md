# Native CRT fabs6451bb

Target sonic.exe SHA2566b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
REA mesh-math-dependencies/crt-entry-6451bb.json plus retained floor-dependencies
classify/control/error helpers establish source code; exact LLVM disassembly
corroborates0x6451bb..0x64526c and64af93..64afe5.

Finite sign bits clear in integer memory before saved CW restoration and final
FLD64. Internal CW is133f. Nonfinite classification performs sourceFLD64/FST64;
negative infinity changes sign after restoration, while NaN sign is retained.
NaN policy2bcc6f0 is explicit and mutable errno3c837d4 becomes0x21 underpolicy1.
Policy0's real lower handler remains required; missing policy/handler stops at
64af96/64afbd with source pending argument and control state preserved.

Native unit tests pass. Private original instruction profile has1019 values×12
CW=12228 evaluations, zero completed-value or errno mismatches.24 signalingNaN
routes stop at source645216 while native FLD64 quieting chooses the qNaN branch;
CW differs at those unresolved boundaries.72 status differences remain. The
strict overall report is false; raw values were not removed and native semantics
were not weakened. Actual Intel flags/NaN loads and policy0 handler behavior
remain unverified. Report:original-crt-fabs-comparison.json, probe identity pinned.
Full production archive integration waits for task/mesh source freeze.

Production integration update: the combined arm64 gate now passes44 native programs and4 extraction tests. Current production probe/report digests are pinned in continuation.json; older private-library measurements above remain historical. Full game/hardware equivalence is still unverified.
