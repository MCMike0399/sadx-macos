# SADX PC native-port evidence and first recovered math slice

Investigation date: 2026-10-11. This report preserves observations and unfinished
requirements; no playable native SADX port or speedrun compatibility is claimed.

## Artifact identity

- Supplied input: `/Users/burbujamc/Developer/rea/Sonic Adventure DX/sonic.exe`.
- Executable SHA-256: `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
- Quarter sine table: VA `0x88af78`, 1025 little-endian IEEE binary32 entries,
  4100 bytes. Exact bytes extracted into
  `/private/tmp/sadx-port-sources/ninja-trig-table.bin`.
- Table SHA-256: `3acbb71a623df2d12dc44e34a48caf049d0e50560593ed003ca15d827947feed`.
- Test table FNV-1a64: `b047ecf880b993c2`. The test rejects differing bytes using
  this pinned digest; the cryptographic SHA-256 above was calculated directly
  from the extracted table and is not implemented by the standalone C++ test.
- Table first and final values are `+0.0f` and `1.0f`; entries are monotonic.

## Public source investigation

Revisions were queried from GitHub HEAD with `git ls-remote`; complete recursive
trees were fetched through GitHub's API. Local tree JSONs reside beside this
report. Search coverage included named repositories and public searches for PC
SADX decompilation, native source ports and recompilation. This is a bounded
negative search, not proof that no unpublished project exists.

| Repository/revision | Observed scope | Native PC SADX reuse limitation |
| --- | --- | --- |
| [FraGag/sadx_decomp](https://github.com/FraGag/sadx_decomp/tree/ca40cd33e71e04fd21d3a4ff159ca80d5b5780dd), `ca40cd33e71e04fd21d3a4ff159ca80d5b5780dd` | Archived; 2359 files, 2356 under CHRMODELS. [Solution](https://github.com/FraGag/sadx_decomp/blob/ca40cd33e71e04fd21d3a4ff159ca80d5b5780dd/SonicAdventureDXPC.sln) contains only CHRMODELS, Win32 Debug/Release. | Model/animation data reconstruction; no sonic.exe gameplay source project. [README](https://github.com/FraGag/sadx_decomp/blob/ca40cd33e71e04fd21d3a4ff159ca80d5b5780dd/README.txt) has an educational/noncommercial disclaimer. |
| [doldecomp/sadx](https://github.com/doldecomp/sadx/tree/4c2b833be9b4b63d57d761beb647cb8d9acbedb6), `4c2b833be9b4b63d57d761beb647cb8d9acbedb6` | GameCube decompilation. Complete tree has seven src files, all C runtime/exception/errno or debugger assembly. [Configuration](https://github.com/doldecomp/sadx/blob/4c2b833be9b4b63d57d761beb647cb8d9acbedb6/configure.py) disables REL rebuilding and leaves RELs TODO. | No recovered game engine sufficient for a port. Host macOS build instructions emit GameCube binaries, not an arm64 Mac game. No project-wide LICENSE observed in complete tree. |
| [sonicfreak1337/SARecomp](https://github.com/sonicfreak1337/SARecomp/tree/d5288fe64e68711dd9f0e6718c1d0fb2eeca92f2), `d5288fe64e68711dd9f0e6718c1d0fb2eeca92f2` | Author reports playable beta for Dreamcast PAL v1.003 on Windows/Linux/Steam Deck. [Development docs](https://github.com/sonicfreak1337/SARecomp/blob/d5288fe64e68711dd9f0e6718c1d0fb2eeca92f2/docs/DEVELOPMENT.md) say a clone lacks pinned SDK and generated AOT inputs and no tested clean-machine bootstrap exists. | Does not consume PC SADX executable. [Linux CMake](https://github.com/sonicfreak1337/SARecomp/blob/d5288fe64e68711dd9f0e6718c1d0fb2eeca92f2/cmake/SonicLinux.cmake) requires retained local SDK and generated guest sources plus AVX2/FMA. No advertised macOS arm64 target or project-wide LICENSE observed; third-party notices are present. |
| [sonicretro/sad_disasm](https://github.com/sonicretro/sad_disasm/tree/78386b53816129ff8d37ef1bce756613fdb5de0d), `78386b53816129ff8d37ef1bce756613fdb5de0d` | Dreamcast SH-4 disassemblies made primarily to locate LandTable data. | Structural reference only; no PC engine source. |

Local [X-Hax loader](https://github.com/X-Hax/sadx-mod-loader/tree/eb4cdcca9637d611a98b910fcfed6ce18580451c)
revision is `eb4cdcca9637d611a98b910fcfed6ce18580451c`.
Its public SADXModLoader.h rejects every architecture other than 32-bit x86.
Function declarations reference absolute PC addresses and x86 calling
conventions/assembly. Source types/address candidates can guide reconstruction;
the DLL ABI cannot be directly loaded by an arm64 process. README describes
2004 US PC support; supplied artifact identity remains authoritative when
resolving the user's 2003 wording.

## Reconstructed math implementation

Files added only within assigned scope:

- `sadx-native/src/ninja_math.hpp`
- `sadx-native/src/ninja_math.cpp`
- `sadx-native/tests/ninja_math.cpp`

These are reconstructed C++17 implementations, not recovered original source.
The table remains caller supplied through
`sadx::NinjaMath::SineTable = std::array<float, 1025>`.

Observed instruction behavior:

- `njSin`, candidate VA `0x788290`, and `njCos`, candidate VA `0x7883c0`:
  16-bit input wrap; high 12 bits choose a knot/quadrant, low four bits choose
  linear interpolation fraction. Exact table knots take a separate branch.
  Quadrants reflect/sign the same quarter table. Cosine corresponds to the same
  table after a quarter-turn phase shift. Original signed cardinal zeros are
  preserved. Constant VA `0x7e0120` contains `0.0625f`.
- `njInnerProduct`, candidate VA `0x787cf0`: multiplies corresponding vector
  components and adds `(z product + y product) + x product`.
- `njScalor2`, candidate VA `0x787e10`: adds `(x squared + y squared) + z squared`.
  It must not be replaced with the dot-product implementation because the
  observed summation order differs.
- Signed square root VA `0x7871f0`: negative inputs use multiplication by
  `-1.0f`, `fsqrt`, `fchs`; nonnegative and unordered inputs use plain `fsqrt`.
  Comparison constant VA `0x7e0048` is double zero; negative multiplier VA
  `0x7dcb0c` is float minus one. Negative zero is preserved.

Trig and root instruction evidence was supplied in
`_reference/sadx/ninja-trig.asm` and `_reference/sadx/ninja-sqrt.asm`.
Dot and squared-magnitude bytes were inspected directly with Xcode's
`llvm-objdump --disassemble --start-address=... --stop-address=...` against the
digest-bound executable. No runtime observation or REA Evidence ID is claimed
for these standalone disassembly records.

## Precision boundary and verification

The recovered x86 routines return an x87 register, not an explicitly stored
binary32 value. The arm64 implementation uses binary64 intermediate arithmetic
and returns binary32. This is an explicit approximation boundary: x87 precision
control, rounding mode, exceptional-value handling and caller use of unrounded
register values remain unverified. In particular, dot-product addition can lose
bits at binary64 precision that an extended x87 register retains. Do not claim
full game-physics or speedrun equivalence from these tests.

The code rejects fast-math at preprocessing time. Build with
`-ffp-contract=off` and no fast-math to retain the reconstructed operation order.

The extracted table itself differs from analytic sine by up to
`5.305380790798786e-7` at its knots. After interpolation and float projection,
the measured first-quadrant maximum is `7.535787632795277e-7` at angle 14552.
Replacing this table with host `sin()` changes observed source data.

Validation performed successfully on a native arm64 Mach-O executable:

```sh
xcrun clang++ -std=c++17 -Wall -Wextra -Werror -ffp-contract=off -O2 \
  -Isadx-native/src sadx-native/src/ninja_math.cpp \
  sadx-native/tests/ninja_math.cpp \
  -o /private/tmp/sadx-port-sources/ninja_math_test
/private/tmp/sadx-port-sources/ninja_math_test \
  /private/tmp/sadx-port-sources/ninja-trig-table.bin
```

Tests pin the table digest, check all 1025 first-quadrant knots bit-for-bit,
exercise every one of 65536 integer angles for both trigonometric functions,
check positive/negative wraps, INT32 extrema, half/quarter-turn relations,
odd/even/reflection symmetries, table-chord interpolation and analytic error
under `1e-6`. Additional cases verify vector results, cancellation-sensitive
dot-product order, signed-root behavior, signed zero, NaN, infinities and a
subnormal input. They do not execute the original Windows functions.

## Remaining work toward the original objective

Native math is only the first recovered engine slice. A playable PC SADX port
still needs startup/task scheduling, Sonic and other character behavior,
collision, stages, cameras, asset loaders, native GPU rendering, sound/video,
saves/menu flow, selected speedrun fixes, mod integration, and real controller
verification. Current math tests do not prove any of those requirements.

Next central implementation work should bind and reconstruct a closed gameplay
slice from this PC executable (task loop, Sonic update/input, collision), retain
its data/float semantics, and compare its state transitions with the original.
The planned source integration of selected mods preserves the native target;
changing the foundation to Dreamcast does not establish PC SADX speedrun parity.
