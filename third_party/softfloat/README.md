# Berkeley SoftFloat 3e, native x87 arithmetic subset

Pinned upstream: https://github.com/ucb-bar/berkeley-softfloat-3/tree/a0c6494cdc11865811dec815d5c0049fba9d82a8

`source/` and `COPYING.txt` are unchanged upstream files. `upstream.json`
records their digests. `sources.txt` lists the exact required C closure for
binary32/ext80 conversion, add/subtract/multiply/square-root and flags. This is
the x87 `8086` behavior specialization, implemented as native integer C on
Apple Silicon; it does not execute guest machine code.

`platform.h` is REA/SADX-native build configuration, with little-endian,
fast 64-bit integers and per-thread mode/flag variables. C and C++ use Clang's
`__thread` for matching Darwin TLS linkage. Include this before upstream
`softfloat.h`.
Compile sources as C11 with these three include directories:

- `third_party/softfloat`
- `third_party/softfloat/source/include`
- `third_party/softfloat/source/8086`

No host SIMD or floating-point instruction behavior is substituted for ext80
arithmetic. `sh third_party/softfloat/build.sh build/softfloat` produces an
archive; callers can instead compile the manifest's sources with their build
system. All archive/objects belong in ignored build output, not this source tree.

Retain `COPYING.txt` and the upstream file notices in source redistribution;
include the license with any redistributed executable using this code.
