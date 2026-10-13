0x43a260: mov eax, dword [esp+arg_0]
0x43a264: mov ecx, dword [esp+arg_4]
0x43a268: lea edx, dword [ecx+eax*8]
0x43a26b: push esi
0x43a26c: mov esi, dword [dword_9759c8+32832+edx*4]
0x43a273: test esi, esi
0x43a275: je loc_43a34c
0x43a27b: movsx eax, word [esi]
0x43a27e: test eax, eax
0x43a280: mov ecx, dword [esi+0xc]
0x43a283: push ebx
0x43a284: push edi
0x43a285: jle loc_43a2b0
0x43a287: lea edi, dword [ecx+0x18]
0x43a28a: mov ebx, eax
0x43a28c: lea esp, dword [esp+0xc+var_C]
0x43a290: mov eax, dword [edi+8]
0x43a293: and eax, 0x84000000
0x43a298: cmp eax, 0x80000000
0x43a29d: jne loc_43a2aa
0x43a29f: mov ecx, dword [edi]
0x43a2a1: push ecx
0x43a2a2: call sub_787140
0x43a2a7: add esp, 0x4
0x43a2aa: add edi, 0x24
0x43a2ad: dec ebx
0x43a2ae: jne loc_43a290
0x43a2b0: cmp dword [dword_3abdcf0], 0x2
0x43a2b7: jge loc_43a2d9
0x43a2b9: mov ebx, dword [esi+0x10]
0x43a2bc: xor edi, edi
0x43a2be: cmp word [esi+2], di
0x43a2c2: jle loc_43a2d9
0x43a2c4: mov edx, dword [ebx+0xc]
0x43a2c7: push edx
0x43a2c8: call sub_787140
0x43a2cd: movsx eax, word [esi+2]
0x43a2d1: add esp, 0x4
0x43a2d4: inc edi
0x43a2d5: cmp edi, eax
0x43a2d7: jl loc_43a2c4
0x43a2d9: mov ax, word [esi+4]
0x43a2dd: test al, 0x2
0x43a2df: je loc_43a2fb
0x43a2e1: test byte [esi+6], 0x3
0x43a2e5: jne loc_43a2f3
0x43a2e7: mov ecx, dword [esi+0x18]
0x43a2ea: push ecx
0x43a2eb: call sub_4227e0+256
0x43a2f0: add esp, 0x4
0x43a2f3: inc word [esi+6]
0x43a2f7: pop edi
0x43a2f8: pop ebx
0x43a2f9: pop esi
0x43a2fa: ret
0x43a2fb: test al, 0x8
0x43a2fd: je loc_43a321
0x43a2ff: mov eax, dword [esi+0x14]
0x43a302: test eax, eax
0x43a304: je loc_43a34a
0x43a306: test byte [esi+6], 0x3
0x43a30a: jne loc_43a319
0x43a30c: mov edx, dword [esi+0x18]
0x43a30f: push edx
0x43a310: push eax
0x43a311: call sub_421180
0x43a316: add esp, 0x8
0x43a319: inc word [esi+6]
0x43a31d: pop edi
0x43a31e: pop ebx
0x43a31f: pop esi
0x43a320: ret
0x43a321: test al, 0x1
0x43a323: je loc_43a34a
0x43a325: movsx eax, word [esi+2]
0x43a329: test eax, eax
0x43a32b: mov ecx, dword [esi+0x10]
0x43a32e: jle loc_43a34a
0x43a330: lea esi, dword [ecx+0x14]
0x43a333: mov edi, eax
0x43a335: mov eax, dword [esi]
0x43a337: test eax, eax
0x43a339: je loc_43a344
0x43a33b: push eax
0x43a33c: call sub_4227e0+256
0x43a341: add esp, 0x4
0x43a344: add esi, 0x18
0x43a347: dec edi
0x43a348: jne loc_43a335
0x43a34a: pop edi
0x43a34b: pop ebx
0x43a34c: pop esi
0x43a34d: ret
