# Native original texture assets

`ninja_texture` parses owned PVMH or standalone GBIX/PVRT bytes and decodes the
formats actually observed in EFF_REGULAR and the selected ECoast archives.
The parser retains the exact chunk/table ranges, raw28-byte names, raw metadata
fields, reserved bytes, ordinal/table-index/global-index identities, GBIX chunks
and uninterpreted payload tails. It does not manufacture a runtime texaddr,
texture-list binding, GPU allocation or cache lifetime.

The executable's actual77f2f0 selects pixel data at PVRT+16 and records the
lengthword−8 payload byte count. Bubble ordinal130 in EFF_REGULAR is a64×64
ARGB1555 rectangle, chunk8224bytes at0xda730, pixels8192bytes at0xda740, followed
by16 retained tail bytes. Moving pixel data to a guessed32-byte boundary would
be incorrect. The original77f3f0 table reader assigns low masks0x1/2/4/8 to
filename/formats/dimensions/global-index respectively. These masks differ from
the pinned primary library's filename/global enum labels; original bytes govern.

All532 records and540 levels from EFF_REGULAR, BEACH01/02/03, OBJ_BEACH,
BEACH_SEA, BG_BEACH and EC_CLOUDS match actual77f2f0/77f3f0 metadata and
78bc30/78bd10/78bf50/78c570 packed16 converter instructions. No source overlay
or GPU vtable reply is used. The format2 texture retains all nine mip levels,
including the original two-byte prefix before the1×1 level. Sixteen synthetic
metadata mask profiles separately verify filename/global-index disambiguation.
Optimized and UBSan tests also check GBIX, partial parsed prefixes, zero dimensions,
short pixel payloads and explicitly unsupported encodings.

RGBA8 uses straight alpha and integer floor(component×255/component_max), based
on the primary MIT PuyoTools pixel codecs pinned at commit
62118d22d06ca19c66e8d97cb14358ffd2be1df1. Its BGRA result is expressed as RGBA.
The exact packed16 texels are retained alongside this declared expansion model;
RGBA8 does not establish the original GPU's packed-UNORM/filter behavior.
Upstream snapshots and SHA identities are in texture-primary/source.json, with
license in sadx-native/third_party/puyotools-format/LICENSE.md. Primary source:
https://github.com/nickworonekin/puyotools/tree/62118d22d06ca19c66e8d97cb14358ffd2be1df1/src/PuyoTools.Core/Textures/Pvr

Genuine Ghidra Evidence IDs and source/compiler/asset fingerprints are in
ninja-texture-ledger.json and original-ninja-texture-comparison.json. Ghidra
78c570 was undefined in this analysis profile; the original29-byte body was
instead disassembled and executed directly. The metadata and pixel observations
do not complete original77f7f0 registry/refcount or78cf20 cache/Metal lifetimes.
Bubble raw RGBA, packed16 and lossless preview PNG are in texture-decoded/.
Actual modified-loader texture overrides and physical GPU behavior remain
unobserved. No complete rendering or game-runtime claim follows from this leaf.
