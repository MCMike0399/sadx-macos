# Source heading and angle adjustment

The native owner is `sadx-native/src/player_heading.hpp/.cpp`. It implements
`AdjustAngle` at 0x438350 and the fast yaw helper at 0x443df0 against the supplied
`sonic.exe` SHA-256 6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.

REA/Hopper evidence:

- `heading-dependencies/adjust-angle.json`,
  ev_b6a242ec35d15a55a4d2e7821f3527a6a4a20b89cbc96ce979b94b4edaa41e77.
- `ground-dependencies/ground-yaw-443df0.json`,
  ev_205ebe95a72e1d966f3438f8b21b0edf577c51400aa63c2b8354f43c09afca1a.

Complete function ranges are not exposed by Hopper's public API. LLVM decoded
exact source bytes corroborate the inspected boundaries 0x438350..0x438391 and
0x443df0..0x443e56; the verifier records their hashes.

`adjust_angle` masks both input angles, sign-extends the wrapped 16-bit
subtraction, tests the signed step and its original wrapped NEG, then adds or
subtracts modulo 65536. A half-turn difference takes the negative direction.
Negative steps and INT_MIN remain source behavior rather than normalized input
or undefined C++ overflow.

`turn_player_heading` copies local speed, executes the real task-to-world
matrix owner, changes task yaw by the source 0x2000 bound, executes the real
world-to-task owner, then commits speed. Source yaw changes, temporary vector
stores and partial matrix state are retained when a required boundary fails.
It does not alter acceleration or select a character/game state.

The original helper's custom ABI uses EAX=player work, EDI=task work and target
at entry ESP+8; the entry ESP+4 word is unused by this body. The first development
harness put the target at ESP+4 and failed 918 cases/profile. Reading the actual
MOV at 0x443e1a corrected that harness; no native angle/physics adjustment was
made to fit those invalid ABI seeds.

`original-player-heading-comparison.json` and both hashed case blobs retain
4,960 integer angle-policy cases and 1,315 complete heading/matrix cases for
each of 12 experimental control words (15,780 heading executions). Integer
returns, task yaw, local velocity, depth and all 96 matrix words match. No
instruction overlays or identity transforms are used. Exception flags differ
in 1,211 cases/profile; original Intel hardware, real startup/mod state and
hardware exception delivery remain unverified.

This closes one ground-turning dependency. Blended/regular yaw, passive/special
rotation, direction reset, collision and complete character ticks remain
separate work. The target remains native macOS arm64; these i386 instructions
run only in a development comparison harness, not the game's runtime.
