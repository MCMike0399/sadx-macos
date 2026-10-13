# Independent retrospective audit: contact attributes

Result: **no discrepancy found** between current `sadx-native/src/player_contact_attributes.cpp`, retained Ghidra's complete 335-byte `0x43cb90..0x43ccde` body, and exact LLVM instructions. No source change was proposed or applied.

Ghidra evidence: `_reference/sadx/ghidra-engine-recovery/contact-attributes.json`, EvidenceID **ev_955076befa99f7756550288839c7afcaff262d0735bfc0f15690c18213a79c43**. Original executable SHA-256 **6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74**. This audit opened no provider/session and edited no build or source files.

Checked source fields and order:

1. The initial store ORs the entire incoming EBX into playerwk+0x5c before parameter availability checks or friction writes. Native `player.attributes |= incoming` preserves this mutation on later typed failure.
2. Source shadow top flags at playerwk+0x1e4 select mask0x2 first, then mask0x400000, then the copy branch. The current owner preserves that priority, including when both are set.
3. Character ID is the full unsigned byte at taskwk+9; parameter stride is0x84, with no source row clamp. The supplied native table has256 optional rows; unavailable defaults fail at the corresponding first source read. Character5's fixed weight address0x9157f4 equals `0x915560 + 5*0x84`, so the native row5 lookup matches the special source address.
4. Stores occur in order: weight at playerwk+0x114, air_resist_y at+0xf4, run_accel at+0xd8, then friction at motionwk2+0x3c. The current owner preserves earlier stores on a missing later default or motion resource.
5. Source arithmetic is actual FLD32 -> FMUL m32 -> FST32. Native uses `from_float`, `multiply_float_memory`, and `to_float`, preserving the loaded-versus-memory operand distinction. In the alternate branch, air resistance uses one FLD32 followed by FADD ST0/ST0 and FST32; native adds the same retained value to itself. The default branch uses pure MOV stores, preserving signaling encodings without introducing FP arithmetic or flags.
6. Friction tests **BL**, not all32 incoming bits: mask0x08, then0x04, then0x10, then JNS on the low byte's sign. Native casts to uint8 before selecting mask0x80. High incoming flags do not select the signed-byte branch. The five source immediate stores are respectively `3e19999a`, `3dcccccd`, `3f000000`, `3fc00000`, `3f800000`.

Checked arithmetic constant addresses: half `0x7dcc98` (0.5), ordinary weight/run factor `0x7dd588` (binary32 bits3ee66666), air-y factor `0x7e0060` (1.5), run factor `0x7e007c` (bits3f266666), alternate weight factor `0x7dd584` (0.6). Exact bytes, rather than Ghidra's rounded decimal rendering, remain the authority. Default source row fields are weight `0x915560`, air-y `0x915540`, run-accel `0x915524`, each plus the character stride.

Ghidra corroborates control flow and dataflow, but its C-like floats do not establish x87 precision/rounding or source operation widths. This audit therefore used actual opcodes for ordering and the byte-friction decision. It assumes the established single native owner supplies live task/player/default-table values for the call; it does not establish initialized Windows FP/handler state, actual hardware flags, updated mods loaded, or full-game behavior. Existing source comparison `original-player-contact-attributes-comparison.json` retains its separate state/boundary/control and full-emulator results. No new test or hardware execution was claimed by this static audit.
