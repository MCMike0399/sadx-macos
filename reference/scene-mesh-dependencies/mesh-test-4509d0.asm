0x4509d0: push ebp
0x4509d1: mov ebp, esp
0x4509d3: sub esp, 0xc
0x4509d6: mov eax, dword [ebp+arg_0]
0x4509d9: mov ecx, dword [ebp+arg_0]
0x4509dc: fld dword [eax+8]
0x4509df: fmul dword [ecx+8]
0x4509e2: mov edx, dword [ebp+arg_0]
0x4509e5: mov eax, dword [ebp+arg_0]
0x4509e8: fld dword [edx+4]
0x4509eb: fmul dword [eax+4]
0x4509ee: mov ecx, dword [ebp+arg_0]
0x4509f1: mov edx, dword [ebp+arg_0]
0x4509f4: fld dword [ecx]
0x4509f6: fmul dword [edx]
0x4509f8: faddp st1
0x4509fa: faddp st1
0x4509fc: fst dword [ebp+var_8]
0x4509ff: sub esp, 0x8
0x450a02: fstp qword [esp+0x14+var_14]
0x450a05: call sub_6452a4
0x450a0a: add esp, 0x8
0x450a0d: fst dword [ebp+var_C]
0x450a10: fstp dword [ebp+var_4]
0x450a13: fld dword [ebp+var_4]
0x450a16: fcomp dword [dword_value_1142947840+4]
0x450a1c: fnstsw ax
0x450a1e: test ah, 0x44
0x450a21: jp loc_450a42
0x450a23: mov eax, dword [ebp+arg_0]
0x450a26: mov dword [eax+8], 0x0
0x450a2d: mov ecx, dword [ebp+arg_0]
0x450a30: mov dword [ecx], 0x0
0x450a36: mov edx, dword [ebp+arg_0]
0x450a39: mov dword [edx+4], 0x3f800000
0x450a40: jmp loc_450a79
0x450a42: fld dword [aImm32dll+16]
0x450a48: fdiv dword [ebp+var_4]
0x450a4b: fstp dword [ebp+var_4]
0x450a4e: mov eax, dword [ebp+arg_0]
0x450a51: fld dword [ebp+var_4]
0x450a54: fmul dword [eax]
0x450a56: mov ecx, dword [ebp+arg_0]
0x450a59: fstp dword [ecx]
0x450a5b: mov edx, dword [ebp+arg_0]
0x450a5e: fld dword [ebp+var_4]
0x450a61: fmul dword [edx+4]
0x450a64: mov eax, dword [ebp+arg_0]
0x450a67: fstp dword [eax+4]
0x450a6a: mov ecx, dword [ebp+arg_0]
0x450a6d: fld dword [ebp+var_4]
0x450a70: fmul dword [ecx+8]
0x450a73: mov edx, dword [ebp+arg_0]
0x450a76: fstp dword [edx+8]
0x450a79: mov esp, ebp
0x450a7b: pop ebp
0x450a7c: ret
