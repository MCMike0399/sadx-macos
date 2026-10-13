0x43ca40: sub esp, 0xc
0x43ca43: push ebx
0x43ca44: push ebp
0x43ca45: mov ebp, dword [esp+0x14+arg_0]
0x43ca49: mov ebx, ecx
0x43ca4b: mov ecx, dword [eax+0x6c]
0x43ca4e: push edi
0x43ca4f: mov edi, dword [ecx+0x28]
0x43ca52: test edi, edi
0x43ca54: je loc_43cb38
0x43ca5a: mov cl, byte [esi+8]
0x43ca5d: test cl, cl
0x43ca5f: je loc_43ca64
0x43ca61: add edi, 0x1c
0x43ca64: lea ecx, dword [eax+0x38]
0x43ca67: call sub_787cc0
0x43ca6c: fcomp dword [dword_value_1142947840+4]
0x43ca72: fnstsw ax
0x43ca74: test ah, 0x44
0x43ca77: jp loc_43ca93
0x43ca79: fld dword [esi+0x20]
0x43ca7c: fadd dword [ebx]
0x43ca7e: fstp dword [ebp+4]
0x43ca81: fld dword [esi+0x24]
0x43ca84: fadd dword [ebx+4]
0x43ca87: fstp dword [ebp+8]
0x43ca8a: fld dword [esi+0x28]
0x43ca8d: fadd dword [ebx+8]
0x43ca90: fstp dword [ebp+0xc]
0x43ca93: cmp dword [edi], 0x0
0x43ca96: je loc_43caed
0x43ca98: lea ebx, dword [esi+0x20]
0x43ca9b: mov edx, ebx
0x43ca9d: mov eax, dword [edx]
0x43ca9f: mov ecx, dword [edx+4]
0x43caa2: mov edx, dword [edx+8]
0x43caa5: mov dword [esp+0x18+var_C], eax
0x43caa9: mov dword [esp+0x18+var_8], ecx
0x43caad: mov dword [esp+0x18+var_4], edx
0x43cab1: lea eax, dword [ebp+4]
0x43cab4: mov edx, dword [eax]
0x43cab6: mov ecx, ebx
0x43cab8: mov dword [ecx], edx
0x43caba: mov edx, dword [eax+4]
0x43cabd: mov dword [ecx+4], edx
0x43cac0: mov eax, dword [eax+8]
0x43cac3: mov dword [ecx+8], eax
0x43cac6: movzx ecx, byte [esi+8]
0x43caca: mov edx, dword [dword_3b42e30+ecx*4]
0x43cad1: push edi
0x43cad2: push esi
0x43cad3: push edx
0x43cad4: call dword [edi]
0x43cad6: mov eax, dword [esp+0x18+ret_addr]
0x43cada: mov ecx, dword [esp+0x18+arg_0]
0x43cade: mov edx, dword [esp+0x18+arg_4]
0x43cae2: mov dword [ebx], eax
0x43cae4: mov dword [ebx+4], ecx
0x43cae7: add esp, 0xc
0x43caea: mov dword [ebx+8], edx
0x43caed: fld dword [edi+0x10]
0x43caf0: fadd dword [ebp+4]
0x43caf3: fstp dword [ebp+4]
0x43caf6: fld dword [edi+0x14]
0x43caf9: fadd dword [ebp+8]
0x43cafc: fstp dword [ebp+8]
0x43caff: fld dword [edi+0x18]
0x43cb02: fadd dword [ebp+0xc]
0x43cb05: fstp dword [ebp+0xc]
0x43cb08: mov eax, dword [edi+4]
0x43cb0b: mov ecx, dword [esi+0x14]
0x43cb0e: mov ebx, dword [esi+0x18]
0x43cb11: add eax, ecx
0x43cb13: and eax, 0xffff
0x43cb18: mov dword [esi+0x14], eax
0x43cb1b: mov ecx, dword [edi+8]
0x43cb1e: add ecx, ebx
0x43cb20: and ecx, 0xffff
0x43cb26: mov dword [esi+0x18], ecx
0x43cb29: mov edx, dword [edi+0xc]
0x43cb2c: add edx, dword [esi+0x1c]
0x43cb2f: and edx, 0xffff
0x43cb35: mov dword [esi+0x1c], edx
0x43cb38: pop edi
0x43cb39: pop ebp
0x43cb3a: pop ebx
0x43cb3b: add esp, 0xc
0x43cb3e: ret
