0x4521c0: push ebp
0x4521c1: mov ebp, esp
0x4521c3: sub esp, 0x8
0x4521c6: mov eax, dword [dword_3c48558]
0x4521cb: mov dword [ebp+var_4], eax
0x4521ce: mov ecx, dword [ebp+var_4]
0x4521d1: mov edx, dword [ecx]
0x4521d3: mov dword [ebp+var_8], edx
0x4521d6: cmp dword [ebp+var_8], 0x0
0x4521da: jne loc_4521e0
0x4521dc: xor eax, eax
0x4521de: jmp loc_452227
0x4521e0: mov eax, dword [ebp+var_8]
0x4521e3: mov dword [dword_3c48558], eax
0x4521e8: mov ecx, dword [ebp+arg_0]
0x4521eb: mov edx, dword [ecx]
0x4521ed: mov dword [ebp+var_8], edx
0x4521f0: mov eax, dword [ebp+var_4]
0x4521f3: mov ecx, dword [ebp+var_8]
0x4521f6: mov dword [eax], ecx
0x4521f8: cmp dword [ebp+var_8], 0x0
0x4521fc: je loc_452209
0x4521fe: mov edx, dword [ebp+var_8]
0x452201: mov eax, dword [ebp+var_4]
0x452204: mov dword [edx+4], eax
0x452207: jmp loc_452212
0x452209: mov ecx, dword [ebp+arg_0]
0x45220c: mov edx, dword [ebp+var_4]
0x45220f: mov dword [ecx+4], edx
0x452212: mov eax, dword [ebp+arg_0]
0x452215: mov ecx, dword [ebp+var_4]
0x452218: mov dword [eax], ecx
0x45221a: mov edx, dword [ebp+var_4]
0x45221d: mov dword [edx+4], 0x0
0x452224: mov eax, dword [ebp+var_4]
0x452227: mov esp, ebp
0x452229: pop ebp
0x45222a: ret
