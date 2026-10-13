0x451790: push ebp
0x451791: mov ebp, esp
0x451793: sub esp, 0x8
0x451796: mov eax, dword [dword_3c48554]
0x45179b: mov dword [ebp+var_4], eax
0x45179e: mov ecx, dword [ebp+var_4]
0x4517a1: mov edx, dword [ecx]
0x4517a3: mov dword [ebp+var_8], edx
0x4517a6: cmp dword [ebp+var_8], 0x0
0x4517aa: jne loc_4517b0
0x4517ac: xor eax, eax
0x4517ae: jmp loc_4517f3
0x4517b0: mov eax, dword [ebp+var_8]
0x4517b3: mov dword [dword_3c48554], eax
0x4517b8: mov ecx, dword [dword_3c48550]
0x4517be: mov dword [ebp+var_8], ecx
0x4517c1: mov edx, dword [ebp+var_4]
0x4517c4: mov eax, dword [ebp+var_8]
0x4517c7: mov dword [edx], eax
0x4517c9: mov ecx, dword [ebp+var_4]
0x4517cc: cmp dword [ecx], 0x0
0x4517cf: je loc_4517da
0x4517d1: mov edx, dword [ebp+var_8]
0x4517d4: mov eax, dword [ebp+var_4]
0x4517d7: mov dword [edx+4], eax
0x4517da: mov ecx, dword [ebp+var_4]
0x4517dd: mov dword [dword_3c48550], ecx
0x4517e3: mov edx, dword_3c48550
0x4517e8: not edx
0x4517ea: mov eax, dword [ebp+var_4]
0x4517ed: mov dword [eax+4], edx
0x4517f0: mov eax, dword [ebp+var_4]
0x4517f3: mov esp, ebp
0x4517f5: pop ebp
0x4517f6: ret
