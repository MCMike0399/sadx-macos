0x451a00: push ebp
0x451a01: mov ebp, esp
0x451a03: push ecx
0x451a04: xor ecx, ecx
0x451a06: call sub_7850f0
0x451a0b: mov edx, dword [ebp+arg_0]
0x451a0e: add edx, 0x8
0x451a11: xor ecx, ecx
0x451a13: call sub_785070
0x451a18: mov eax, dword [ebp+arg_0]
0x451a1b: cmp dword [eax+0x1c], 0x0
0x451a1f: je loc_451a2f
0x451a21: mov ecx, dword [ebp+arg_0]
0x451a24: movzx edx, word [ecx+0x1c]
0x451a28: xor ecx, ecx
0x451a2a: call sub_784fb0
0x451a2f: mov edx, dword [ebp+arg_0]
0x451a32: cmp dword [edx+0x18], 0x0
0x451a36: je loc_451a46
0x451a38: mov eax, dword [ebp+arg_0]
0x451a3b: movzx edx, word [eax+0x18]
0x451a3f: xor ecx, ecx
0x451a41: call sub_784e20
0x451a46: mov ecx, dword [ebp+arg_0]
0x451a49: cmp dword [ecx+0x14], 0x0
0x451a4d: je loc_451a5d
0x451a4f: mov edx, dword [ebp+arg_0]
0x451a52: movzx edx, word [edx+0x14]
0x451a56: xor ecx, ecx
0x451a58: call sub_784ee0
0x451a5d: mov edx, dword [ebp+arg_0]
0x451a60: add edx, 0x20
0x451a63: xor ecx, ecx
0x451a65: call sub_788a50
0x451a6a: mov eax, dword [ebp+arg_0]
0x451a6d: mov ecx, dword [eax+0x2c]
0x451a70: mov dword [ebp+var_4], ecx
0x451a73: cmp dword [ebp+var_4], 0x0
0x451a77: je loc_451a92
0x451a79: mov edx, dword [ebp+arg_8]
0x451a7c: push edx
0x451a7d: mov eax, dword [ebp+arg_4]
0x451a80: push eax
0x451a81: mov ecx, dword [ebp+var_4]
0x451a84: push ecx
0x451a85: call sub_451a00
0x451a8a: add esp, 0xc
0x451a8d: mov dword [ebp+var_4], eax
0x451a90: jmp loc_451a73
0x451a92: mov edx, dword [ebp+arg_8]
0x451a95: push edx
0x451a96: mov eax, dword [ebp+arg_4]
0x451a99: push eax
0x451a9a: mov ecx, dword [ebp+arg_0]
0x451a9d: push ecx
0x451a9e: call sub_451ad0
0x451aa3: add esp, 0xc
0x451aa6: test eax, eax
0x451aa8: jne loc_451ab8
0x451aaa: mov ecx, 0x1
0x451aaf: call sub_785140
0x451ab4: xor eax, eax
0x451ab6: jmp loc_451ac8
0x451ab8: mov ecx, 0x1
0x451abd: call sub_785140
0x451ac2: mov edx, dword [ebp+arg_0]
0x451ac5: mov eax, dword [edx+0x30]
0x451ac8: mov esp, ebp
0x451aca: pop ebp
0x451acb: ret
