0x451910: push ebp
0x451911: mov ebp, esp
0x451913: sub esp, 0x10
0x451916: mov dword [ebp+var_10], 0x3b48150
0x45191d: mov dword [ebp+var_8], 0x80
0x451924: jmp loc_45192f
0x451926: mov eax, dword [ebp+var_8]
0x451929: sub eax, 0x1
0x45192c: mov dword [ebp+var_8], eax
0x45192f: cmp dword [ebp+var_8], 0x0
0x451933: jle loc_451950
0x451935: mov ecx, dword [ebp+var_10]
0x451938: mov edx, dword [ecx]
0x45193a: mov dword [ebp+var_4], edx
0x45193d: cmp dword [ebp+var_4], 0x0
0x451941: je loc_451945
0x451943: jmp loc_451950
0x451945: mov eax, dword [ebp+var_10]
0x451948: add eax, 0x8
0x45194b: mov dword [ebp+var_10], eax
0x45194e: jmp loc_451926
0x451950: cmp dword [ebp+var_8], 0x0
0x451954: jg loc_451971
0x451956: mov ecx, dword [ebp+arg_0]
0x451959: mov dword [ecx+0x10], 0x0
0x451960: mov edx, dword [ebp+arg_0]
0x451963: mov dword [edx+0x14], 0x0
0x45196a: xor eax, eax
0x45196c: jmp loc_4519f9
0x451971: mov eax, dword [ebp+arg_0]
0x451974: mov ecx, dword [ebp+var_4]
0x451977: mov dword [eax+0x10], ecx
0x45197a: mov edx, dword [ebp+var_10]
0x45197d: mov eax, dword [edx+4]
0x451980: mov dword [ebp+var_C], eax
0x451983: mov ecx, dword [ebp+var_10]
0x451986: add ecx, 0x8
0x451989: mov dword [ebp+var_10], ecx
0x45198c: mov edx, dword [ebp+var_8]
0x45198f: sub edx, 0x1
0x451992: mov dword [ebp+var_8], edx
0x451995: cmp dword [ebp+var_8], 0x0
0x451999: jg loc_4519a9
0x45199b: mov eax, dword [ebp+arg_0]
0x45199e: mov ecx, dword [ebp+var_C]
0x4519a1: mov dword [eax+0x14], ecx
0x4519a4: mov eax, dword [ebp+var_4]
0x4519a7: jmp loc_4519f9
0x4519a9: jmp loc_4519b4
0x4519ab: mov edx, dword [ebp+var_8]
0x4519ae: sub edx, 0x1
0x4519b1: mov dword [ebp+var_8], edx
0x4519b4: cmp dword [ebp+var_8], 0x0
0x4519b8: jle loc_4519ed
0x4519ba: mov eax, dword [ebp+var_10]
0x4519bd: mov ecx, dword [eax]
0x4519bf: mov dword [ebp+var_4], ecx
0x4519c2: cmp dword [ebp+var_4], 0x0
0x4519c6: je loc_4519e2
0x4519c8: mov edx, dword [ebp+var_C]
0x4519cb: mov eax, dword [ebp+var_4]
0x4519ce: mov dword [edx], eax
0x4519d0: mov ecx, dword [ebp+var_4]
0x4519d3: mov edx, dword [ebp+var_C]
0x4519d6: mov dword [ecx+4], edx
0x4519d9: mov eax, dword [ebp+var_10]
0x4519dc: mov ecx, dword [eax+4]
0x4519df: mov dword [ebp+var_C], ecx
0x4519e2: mov edx, dword [ebp+var_10]
0x4519e5: add edx, 0x8
0x4519e8: mov dword [ebp+var_10], edx
0x4519eb: jmp loc_4519ab
0x4519ed: mov eax, dword [ebp+arg_0]
0x4519f0: mov ecx, dword [ebp+var_C]
0x4519f3: mov dword [eax+0x14], ecx
0x4519f6: mov eax, dword [ebp+var_4]
0x4519f9: mov esp, ebp
0x4519fb: pop ebp
0x4519fc: ret
