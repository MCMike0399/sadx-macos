# Late model and the missing Gamma dossiers

Ghidra 12.1.4 decompiled these from the pinned sonic.exe. The pseudocode is evidence. The oracles compare the executable.

`lateDrawModel` (`0x4048e0`) allocates a 0x70 node with type 1. The flags argument is the third allocator argument, not the type. Flags 6 therefore still file a type-1 node. The node stores the model at +0x2c and the matrix at +0x30, then queues unless the negated depth is not greater than the limit and flag bit 0 is set.

`0x4089b5` and `0x408c56` are not their own procedures. Both requests decompiled the containing `late_exec`. Its switch on the header nibble is the consumer:

* case 1 installs the stored matrix and draws the model when the texture list is present and the model word is at least `0x100000`. A clear simple-model byte and a lighting palette at or below zero take `dsSimpleDrawModel`; otherwise `njDrawModel`.
* case 6 reads the node at +0x80 and +0x88 and draws a 2D polygon. That is past the 0x70 model node, so it is not the shadow blob.

`Shadow` (`0x49ee30`) ends in `late_DrawShadowObject` on `object_shadow`.

`analyze_function` rejected `0x4c44a0` and `0x4c2f50` because Ghidra has no procedure at either address. A later name search returned no procedures, and `resolve_containing_procedure` returned null. The bytes are still code: `0x4c44a0` is `sub esp, 8` and `0x4c2f50` is `sub esp, 0x3c`. The empty land-candidate path is the `jle` to `mov eax, ebx` with ebx cleared, so a count at or below zero returns 0. Gun effects and that empty return were recovered from the executable with Unicorn, not from a decompiler body.
