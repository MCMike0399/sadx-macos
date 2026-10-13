# Native air acceleration

The source owner is `sadx-native/src/player_acceleration_air.hpp/.cpp`, recovered
from original `sonic.exe` 0x44b9c0..0x44bb54 (405 bytes), bound to executable
SHA256 6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
REA/Hopper dossier `movement/air-acceleration.json` has Evidence
`ev_5c22e29ff700ce9aae9871c73023cd116d85181212151bcac72faed4a1f42fc0`.
LLVM instruction decoding corroborates the full local body; Hopper body-range
coverage remains unavailable rather than claimed complete by that provider.

The native implementation preserves flag4000 event priority, weighted world
acceleration, actual world-to-player matrix conversion, distinct airborne X
resistance, requested GetAnalog outputs, wrapped heading thresholds, descending
control doubling, target-yaw stores, real regular-yaw mutation, and final X/Y/Z
acceleration stores. It shares actual task, player and motion owners. Missing
facts/callees retain earlier stores. GetAnalog's output-store mask distinguishes
active event input from initialized angle/magnitude scratch.

The original-instruction harness executes the real input query, regular yaw,
Ninja matrix/trig and angle-distance callees with no instruction overlays.
Across 2,496 cases for each of twelve explicitly selected CW profiles (29,952
executions), stored player/world state, task angles/flags, target yaw, yaw
call counts, depth, all three matrix frames and CW match. Exception flags
differ; the report records them without weakening native arithmetic.

Runtime globals, physics parameters and matrix state are explicit fixtures.
Flag4000 event behavior is covered by the separate alternate-event owner, not
this parent profile. Native hardware, loaded Windows mods, actual keyboard and
controller gameplay, Sonic state-machine/collision ordering and whole-game
speedrun equivalence remain unverified. This is source-native ARM64 code; the
i386 instruction engine is confined to the development comparison harness.
