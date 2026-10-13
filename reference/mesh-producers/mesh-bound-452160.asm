0x452160: push ebp
0x452161: mov ebp, esp
0x452163: sub esp, 0x10
0x452166: fld dword [ebp+arg_0]
0x452169: call sub_644340
0x45216e: mov dword [ebp+var_8], eax
0x452171: mov eax, dword [ebp+var_8]
0x452174: sar eax, 0x4
0x452177: add eax, 0x40
0x45217a: mov dword [ebp+var_8], eax
0x45217d: cmp dword [ebp+var_8], 0x0
0x452181: jge loc_45218a
0x452183: mov dword [ebp+var_8], 0x0
0x45218a: cmp dword [ebp+var_8], 0x7f
0x45218e: jle loc_452197
0x452190: mov dword [ebp+var_8], 0x7f
0x452197: mov ecx, dword [ebp+var_8]
0x45219a: shl ecx, 0x1
0x45219c: mov dword [ebp+var_8], ecx
0x45219f: mov edx, dword [ebp+var_8]
0x4521a2: lea eax, dword [dword_3b47628+2856+edx*4]
0x4521a9: mov dword [ebp+var_4], eax
0x4521ac: mov ecx, dword [ebp+var_4]
0x4521af: push ecx
0x4521b0: call sub_4521c0
0x4521b5: add esp, 0x4
0x4521b8: mov esp, ebp
0x4521ba: pop ebp
0x4521bb: ret
