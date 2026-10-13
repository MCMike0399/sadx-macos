# Native FPATAN math dependency

This is a **derivative math-only build** of source files from
[Bochs revision `0a93954eb5debb16dd43b3f02337ee4e6c088b92`](https://github.com/bochs-emu/Bochs/tree/0a93954eb5debb16dd43b3f02337ee4e6c088b92).
It compiles native integer C++ on Apple Silicon. It does not run an x86 CPU,
execute `sonic.exe`, or substitute host `atan2`, `double`, or `long double`.

`sources.txt` contains the translation units required by FPATAN (26) and
`fpu/fsincos.cc` (FSINCOS/FPTAN, added for the 791170 projection and D3DX
7716df), including
the P5/P6 table, internal 67-bit chopped / 64-bit nearest arithmetic, exact
remainder reduction, and the Bochs-modified SoftFloat3e binary128 operations.
The modified binary128 rounding code is required; unmodified Berkeley
SoftFloat3e is not a drop-in replacement. `upstream.json` records revision
and SHA-256 of every retained upstream source/header. One documented patch to
`s_normRoundPackToExtF80.cc` returns canonical signed zero before normalization,
avoiding a 64-bit shift passed to a helper restricted to shifts 1..63. Its
upstream and patched digests are recorded separately. Other source files remain
unchanged.
`config.h`, `math.hpp`, and `build.sh` are local build adapters. The generated
translation-unit wrappers place all dependency functions and types inside
`sadx::bochs_fpatan`, preventing collisions with the separate Berkeley
SoftFloat basic arithmetic library.

Build the archive into ignored output:

```sh
sh third_party/bochs_fpatan/build.sh build/bochs_fpatan
```

Compile `src/x87_transcendental.cpp` with include paths for this directory and
`source/`; link its archive alongside the basic SoftFloat archive.
`X87Transcendental::fpatan_bochs_p5_p6(context, y, x)` explicitly selects this
candidate model. It preserves the context's control word, follows RC, forces
64 significant bits independently of PC as FPATAN does, records architectural
low-six exception flags, and rejects foreign-thread/unmasked contexts.
Stack/tag/TOP/C1 and deferred trap delivery are outside the current context API.

Bochs upstream describes this implementation as matching hardware. That
description is upstream intent, not proof that this port matches the user's
Intel Ultra Windows reference. Hardware comparison remains required before
claiming speedrun equivalence. See `_reference/sadx/transcendental-provenance.md`.

Retained licensing is mixed: upstream `COPYING` is included as
`COPYING-bochs.txt` (LGPL 2.1 text); `fpu_trans.h` carries its LGPL
version-2-or-later notice. SoftFloat3e's BSD notice is included as
`COPYING-softfloat3e.txt` and retained in its files. Several older FPU files
carry the historical SoftFloat2b permission, indemnification, and derivative
notice paragraphs; those paragraphs remain intact. New Bochs helper files
without independent notices remain accompanied by upstream's `COPYING`.
Retain the complete source, notices, local adapter source, and license files
when distributing this component; any later executable packaging must account
for the library's relinking/source terms.
