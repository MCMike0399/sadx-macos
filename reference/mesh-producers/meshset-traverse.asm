0x451ad0: push ebp
0x451ad1: mov ebp, esp
0x451ad3: mov eax, 0x308c
0x451ad8: call sub_644c40
0x451add: mov eax, dword [ebp+arg_0]
0x451ae0: mov ecx, dword [eax+4]
0x451ae3: mov dword [ebp+var_300C], ecx
0x451ae9: mov edx, dword [ebp+var_300C]
0x451aef: movzx eax, word [edx+0x14]
0x451af3: mov dword [ebp+var_4], eax
0x451af6: mov ecx, dword [ebp+var_300C]
0x451afc: mov edx, dword [ecx+0xc]
0x451aff: mov dword [ebp+var_301C], edx
0x451b05: jmp loc_451b1f
0x451b07: mov eax, dword [ebp+var_4]
0x451b0a: sub eax, 0x1
0x451b0d: mov dword [ebp+var_4], eax
0x451b10: mov ecx, dword [ebp+var_301C]
0x451b16: add ecx, 0x1c
0x451b19: mov dword [ebp+var_301C], ecx
0x451b1f: cmp dword [ebp+var_4], 0x0
0x451b23: jle loc_452155
0x451b29: mov edx, dword [ebp+var_301C]
0x451b2f: movzx eax, word [edx]
0x451b32: and eax, 0xc000
0x451b37: mov dword [ebp+var_3018], eax
0x451b3d: mov ecx, dword [ebp+var_3018]
0x451b43: mov dword [ebp+var_3080], ecx
0x451b49: cmp dword [ebp+var_3080], 0x8000
0x451b53: jg loc_451b78
0x451b55: cmp dword [ebp+var_3080], 0x8000
0x451b5f: je loc_451b9e
0x451b61: cmp dword [ebp+var_3080], 0x0
0x451b68: je loc_451b86
0x451b6a: cmp dword [ebp+var_3080], 0x4000
0x451b74: je loc_451b92
0x451b76: jmp loc_451ba0
0x451b78: cmp dword [ebp+var_3080], 0xc000
0x451b82: je loc_451b9e
0x451b84: jmp loc_451ba0
0x451b86: mov dword [ebp+var_3014], 0x3
0x451b90: jmp loc_451ba5
0x451b92: mov dword [ebp+var_3014], 0x4
0x451b9c: jmp loc_451ba5
0x451b9e: jmp loc_451ba5
0x451ba0: jmp loc_451b07
0x451ba5: mov edx, dword [ebp+var_301C]
0x451bab: movzx eax, word [edx+2]
0x451baf: mov dword [ebp+var_3020], eax
0x451bb5: mov ecx, dword [ebp+var_301C]
0x451bbb: mov edx, dword [ecx+4]
0x451bbe: mov dword [ebp+var_3010], edx
0x451bc4: jmp loc_451bd5
0x451bc6: mov eax, dword [ebp+var_3020]
0x451bcc: sub eax, 0x1
0x451bcf: mov dword [ebp+var_3020], eax
0x451bd5: cmp dword [ebp+var_3020], 0x0
0x451bdc: jle loc_452150
0x451be2: lea ecx, dword [ebp+var_3018]
0x451be8: push ecx
0x451be9: call sub_452230
0x451bee: add esp, 0x4
0x451bf1: mov dword [ebp+var_302C], eax
0x451bf7: mov dword [ebp+var_3024], 0x0
0x451c01: lea edx, dword [ebp+var_3008]
0x451c07: mov dword [ebp+var_3028], edx
0x451c0d: jmp loc_451c2d
0x451c0f: mov eax, dword [ebp+var_3024]
0x451c15: add eax, 0x1
0x451c18: mov dword [ebp+var_3024], eax
0x451c1e: mov ecx, dword [ebp+var_3028]
0x451c24: add ecx, 0xc
0x451c27: mov dword [ebp+var_3028], ecx
0x451c2d: mov edx, dword [ebp+var_3014]
0x451c33: sub edx, 0x2
0x451c36: cmp dword [ebp+var_3024], edx
0x451c3c: jge loc_45214b
0x451c42: mov eax, dword [ebp+var_3028]
0x451c48: mov ecx, dword [eax]
0x451c4a: mov dword [ebp+var_3068], ecx
0x451c50: mov edx, dword [ebp+var_3028]
0x451c56: mov eax, dword [edx+0xc]
0x451c59: mov dword [ebp+var_3054], eax
0x451c5f: mov ecx, dword [ebp+var_3028]
0x451c65: mov edx, dword [ecx+0x18]
0x451c68: mov dword [ebp+var_303C], edx
0x451c6e: mov eax, dword [ebp+var_3028]
0x451c74: mov ecx, dword [eax+4]
0x451c77: mov dword [ebp+var_3038], ecx
0x451c7d: mov edx, dword [ebp+var_3028]
0x451c83: mov eax, dword [edx+0x10]
0x451c86: mov dword [ebp+var_3048], eax
0x451c8c: mov ecx, dword [ebp+var_3028]
0x451c92: mov edx, dword [ecx+0x1c]
0x451c95: mov dword [ebp+var_3034], edx
0x451c9b: mov eax, dword [ebp+var_3028]
0x451ca1: mov ecx, dword [eax+8]
0x451ca4: mov dword [ebp+var_305C], ecx
0x451caa: mov edx, dword [ebp+var_3028]
0x451cb0: mov eax, dword [edx+0x14]
0x451cb3: mov dword [ebp+var_306C], eax
0x451cb9: mov ecx, dword [ebp+var_3028]
0x451cbf: mov edx, dword [ecx+0x20]
0x451cc2: mov dword [ebp+var_304C], edx
0x451cc8: mov dword [ebp+var_3044], 0x3eaaa64c
0x451cd2: fld dword [ebp+var_3068]
0x451cd8: fadd dword [ebp+var_3054]
0x451cde: fadd dword [ebp+var_303C]
0x451ce4: fmul dword [ebp+var_3044]
0x451cea: fstp dword [ebp+var_3040]
0x451cf0: fld dword [ebp+var_3038]
0x451cf6: fadd dword [ebp+var_3048]
0x451cfc: fadd dword [ebp+var_3034]
0x451d02: fmul dword [ebp+var_3044]
0x451d08: fstp dword [ebp+var_3058]
0x451d0e: fld dword [ebp+var_305C]
0x451d14: fadd dword [ebp+var_306C]
0x451d1a: fadd dword [ebp+var_304C]
0x451d20: fmul dword [ebp+var_3044]
0x451d26: fstp dword [ebp+var_3064]
0x451d2c: fld dword [ebp+var_3064]
0x451d32: fsub dword [dword_3b4758c]
0x451d38: push ecx
0x451d39: fstp dword [esp+0x4+var_4]
0x451d3c: call sub_452160
0x451d41: add esp, 0x4
0x451d44: mov dword [ebp+var_3030], eax
0x451d4a: cmp dword [ebp+var_3030], 0x0
0x451d51: je loc_452142
0x451d57: mov eax, dword [ebp+var_3024]
0x451d5d: add eax, dword [ebp+var_302C]
0x451d63: and eax, 0x1
0x451d66: test eax, eax
0x451d68: jne loc_451dfa
0x451d6e: mov ecx, dword [ebp+var_3030]
0x451d74: mov edx, dword [ebp+var_3068]
0x451d7a: mov dword [ecx+0x18], edx
0x451d7d: mov eax, dword [ebp+var_3030]
0x451d83: mov ecx, dword [ebp+var_3038]
0x451d89: mov dword [eax+0x1c], ecx
0x451d8c: mov edx, dword [ebp+var_3030]
0x451d92: mov eax, dword [ebp+var_305C]
0x451d98: mov dword [edx+0x20], eax
0x451d9b: mov ecx, dword [ebp+var_3030]
0x451da1: mov edx, dword [ebp+var_3054]
0x451da7: mov dword [ecx+0x24], edx
0x451daa: mov eax, dword [ebp+var_3030]
0x451db0: mov ecx, dword [ebp+var_3048]
0x451db6: mov dword [eax+0x28], ecx
0x451db9: mov edx, dword [ebp+var_3030]
0x451dbf: mov eax, dword [ebp+var_306C]
0x451dc5: mov dword [edx+0x2c], eax
0x451dc8: mov ecx, dword [ebp+var_3030]
0x451dce: mov edx, dword [ebp+var_303C]
0x451dd4: mov dword [ecx+0x30], edx
0x451dd7: mov eax, dword [ebp+var_3030]
0x451ddd: mov ecx, dword [ebp+var_3034]
0x451de3: mov dword [eax+0x34], ecx
0x451de6: mov edx, dword [ebp+var_3030]
0x451dec: mov eax, dword [ebp+var_304C]
0x451df2: mov dword [edx+0x38], eax
0x451df5: jmp loc_451e81
0x451dfa: mov ecx, dword [ebp+var_3030]
0x451e00: mov edx, dword [ebp+var_3068]
0x451e06: mov dword [ecx+0x18], edx
0x451e09: mov eax, dword [ebp+var_3030]
0x451e0f: mov ecx, dword [ebp+var_3038]
0x451e15: mov dword [eax+0x1c], ecx
0x451e18: mov edx, dword [ebp+var_3030]
0x451e1e: mov eax, dword [ebp+var_305C]
0x451e24: mov dword [edx+0x20], eax
0x451e27: mov ecx, dword [ebp+var_3030]
0x451e2d: mov edx, dword [ebp+var_303C]
0x451e33: mov dword [ecx+0x24], edx
0x451e36: mov eax, dword [ebp+var_3030]
0x451e3c: mov ecx, dword [ebp+var_3034]
0x451e42: mov dword [eax+0x28], ecx
0x451e45: mov edx, dword [ebp+var_3030]
0x451e4b: mov eax, dword [ebp+var_304C]
0x451e51: mov dword [edx+0x2c], eax
0x451e54: mov ecx, dword [ebp+var_3030]
0x451e5a: mov edx, dword [ebp+var_3054]
0x451e60: mov dword [ecx+0x30], edx
0x451e63: mov eax, dword [ebp+var_3030]
0x451e69: mov ecx, dword [ebp+var_3048]
0x451e6f: mov dword [eax+0x34], ecx
0x451e72: mov edx, dword [ebp+var_3030]
0x451e78: mov eax, dword [ebp+var_306C]
0x451e7e: mov dword [edx+0x38], eax
0x451e81: fld dword [ebp+var_306C]
0x451e87: fsub dword [ebp+var_305C]
0x451e8d: sub esp, 0x8
0x451e90: fstp qword [esp+0x8+var_8]
0x451e93: call sub_6451bb
0x451e98: add esp, 0x8
0x451e9b: fst dword [ebp+var_3084]
0x451ea1: fstp dword [ebp+var_3070]
0x451ea7: fld dword [ebp+var_304C]
0x451ead: fsub dword [ebp+var_306C]
0x451eb3: sub esp, 0x8
0x451eb6: fstp qword [esp+0x8+var_8]
0x451eb9: call sub_6451bb
0x451ebe: add esp, 0x8
0x451ec1: fst dword [ebp+var_3088]
0x451ec7: fstp dword [ebp+var_3074]
0x451ecd: fld dword [ebp+var_305C]
0x451ed3: fsub dword [ebp+var_304C]
0x451ed9: sub esp, 0x8
0x451edc: fstp qword [esp+0x8+var_8]
0x451edf: call sub_6451bb
0x451ee4: add esp, 0x8
0x451ee7: fst dword [ebp+var_308C]
0x451eed: fstp dword [ebp+var_3078]
0x451ef3: fld dword [ebp+var_3074]
0x451ef9: fcomp dword [ebp+var_3070]
0x451eff: fnstsw ax
0x451f01: test ah, 0x41
0x451f04: jne loc_451f12
0x451f06: mov ecx, dword [ebp+var_3074]
0x451f0c: mov dword [ebp+var_3070], ecx
0x451f12: fld dword [ebp+var_3078]
0x451f18: fcomp dword [ebp+var_3070]
0x451f1e: fnstsw ax
0x451f20: test ah, 0x41
0x451f23: jne loc_451f31
0x451f25: mov edx, dword [ebp+var_3078]
0x451f2b: mov dword [ebp+var_3070], edx
0x451f31: fld dword [ebp+var_3070]
0x451f37: fcomp dword [dword_3b47588]
0x451f3d: fnstsw ax
0x451f3f: test ah, 0x41
0x451f42: jne loc_451f4f
0x451f44: mov eax, dword [ebp+var_3070]
0x451f4a: mov dword [dword_3b47588], eax
0x451f4f: fld dword [ebp+var_3068]
0x451f55: fsub dword [ebp+var_3040]
0x451f5b: fstp dword [ebp+var_3068]
0x451f61: fld dword [ebp+var_3038]
0x451f67: fsub dword [ebp+var_3058]
0x451f6d: fstp dword [ebp+var_3038]
0x451f73: fld dword [ebp+var_305C]
0x451f79: fsub dword [ebp+var_3064]
0x451f7f: fstp dword [ebp+var_305C]
0x451f85: fld dword [ebp+var_3054]
0x451f8b: fsub dword [ebp+var_3040]
0x451f91: fstp dword [ebp+var_3054]
0x451f97: fld dword [ebp+var_3048]
0x451f9d: fsub dword [ebp+var_3058]
0x451fa3: fstp dword [ebp+var_3048]
0x451fa9: fld dword [ebp+var_306C]
0x451faf: fsub dword [ebp+var_3064]
0x451fb5: fstp dword [ebp+var_306C]
0x451fbb: fld dword [ebp+var_303C]
0x451fc1: fsub dword [ebp+var_3040]
0x451fc7: fstp dword [ebp+var_303C]
0x451fcd: fld dword [ebp+var_3034]
0x451fd3: fsub dword [ebp+var_3058]
0x451fd9: fstp dword [ebp+var_3034]
0x451fdf: fld dword [ebp+var_304C]
0x451fe5: fsub dword [ebp+var_3064]
0x451feb: fstp dword [ebp+var_304C]
0x451ff1: mov ecx, dword [ebp+var_3030]
0x451ff7: mov edx, dword [ebp+var_3040]
0x451ffd: mov dword [ecx+8], edx
0x452000: mov eax, dword [ebp+var_3030]
0x452006: mov ecx, dword [ebp+var_3058]
0x45200c: mov dword [eax+0xc], ecx
0x45200f: mov edx, dword [ebp+var_3030]
0x452015: mov eax, dword [ebp+var_3064]
0x45201b: mov dword [edx+0x10], eax
0x45201e: fld dword [ebp+var_305C]
0x452024: fmul dword [ebp+var_305C]
0x45202a: fld dword [ebp+var_3038]
0x452030: fmul dword [ebp+var_3038]
0x452036: fld dword [ebp+var_3068]
0x45203c: fmul dword [ebp+var_3068]
0x452042: faddp st1
0x452044: faddp st1
0x452046: fstp dword [ebp+var_3050]
0x45204c: fld dword [ebp+var_306C]
0x452052: fmul dword [ebp+var_306C]
0x452058: fld dword [ebp+var_3048]
0x45205e: fmul dword [ebp+var_3048]
0x452064: fld dword [ebp+var_3054]
0x45206a: fmul dword [ebp+var_3054]
0x452070: faddp st1
0x452072: faddp st1
0x452074: fstp dword [ebp+var_3060]
0x45207a: fld dword [ebp+var_3050]
0x452080: fcomp dword [ebp+var_3060]
0x452086: fnstsw ax
0x452088: test ah, 0x5
0x45208b: jp loc_452099
0x45208d: mov ecx, dword [ebp+var_3060]
0x452093: mov dword [ebp+var_3050], ecx
0x452099: fld dword [ebp+var_304C]
0x45209f: fmul dword [ebp+var_304C]
0x4520a5: fld dword [ebp+var_3034]
0x4520ab: fmul dword [ebp+var_3034]
0x4520b1: fld dword [ebp+var_303C]
0x4520b7: fmul dword [ebp+var_303C]
0x4520bd: faddp st1
0x4520bf: faddp st1
0x4520c1: fstp dword [ebp+var_3060]
0x4520c7: fld dword [ebp+var_3050]
0x4520cd: fcomp dword [ebp+var_3060]
0x4520d3: fnstsw ax
0x4520d5: test ah, 0x5
0x4520d8: jp loc_4520e6
0x4520da: mov edx, dword [ebp+var_3060]
0x4520e0: mov dword [ebp+var_3050], edx
0x4520e6: mov eax, dword [ebp+var_3030]
0x4520ec: mov ecx, dword [ebp+var_3050]
0x4520f2: mov dword [eax+0x14], ecx
0x4520f5: mov edx, dword [ebp+arg_0]
0x4520f8: mov eax, dword [edx+4]
0x4520fb: mov ecx, dword [eax+0x10]
0x4520fe: mov dword [ebp+var_307C], ecx
0x452104: cmp dword [ebp+var_307C], 0x0
0x45210b: je loc_452133
0x45210d: mov edx, dword [ebp+var_301C]
0x452113: movzx eax, word [edx]
0x452116: and eax, 0xffff3fff
0x45211b: imul eax, eax, 0x14
0x45211e: mov ecx, dword [ebp+var_3030]
0x452124: mov edx, dword [ebp+var_307C]
0x45212a: mov eax, dword [edx+eax+0x10]
0x45212e: mov dword [ecx+0x3c], eax
0x452131: jmp loc_452140
0x452133: mov ecx, dword [ebp+var_3030]
0x452139: mov dword [ecx+0x3c], 0x0
0x452140: jmp loc_452146
0x452142: xor eax, eax
0x452144: jmp loc_45215a
0x452146: jmp loc_451c0f
0x45214b: jmp loc_451bc6
0x452150: jmp loc_451b07
0x452155: mov eax, 0x1
0x45215a: mov esp, ebp
0x45215c: pop ebp
0x45215d: ret
