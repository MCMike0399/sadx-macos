0x43aef0: mov eax, dword [dword_3b2cbb0]
0x43aef5: sub esp, 0x3c
0x43aef8: xor ecx, ecx
0x43aefa: cmp eax, ecx
0x43aefc: push ebp
0x43aefd: mov ebp, 0x3b2f720
0x43af02: je loc_43b2db
0x43af08: add eax, 0x20
0x43af0b: mov edx, dword [eax]
0x43af0d: mov dword [esp+0x40+var_18], edx
0x43af11: mov edx, dword [eax+4]
0x43af14: mov eax, dword [eax+8]
0x43af17: mov dword [esp+0x40+var_10], eax
0x43af1b: mov eax, dword [dword_3b42e10]
0x43af20: cmp eax, ecx
0x43af22: mov dword [esp+0x40+var_14], edx
0x43af26: mov dword [esp+0x40+var_3C], 0x1
0x43af2e: je loc_43af71
0x43af30: add eax, 0x20
0x43af33: mov edx, dword [eax]
0x43af35: mov dword [esp+0x40+var_18], edx
0x43af39: mov edx, dword [eax+4]
0x43af3c: mov eax, dword [eax+8]
0x43af3f: mov dword [esp+0x40+var_10], eax
0x43af43: mov eax, dword [dword_3b42e14]
0x43af48: cmp eax, ecx
0x43af4a: mov dword [esp+0x40+var_14], edx
0x43af4e: je loc_43af91
0x43af50: add eax, 0x20
0x43af53: mov edx, dword [eax]
0x43af55: mov dword [esp+0x40+var_C], edx
0x43af59: mov edx, dword [eax+4]
0x43af5c: mov eax, dword [eax+8]
0x43af5f: mov dword [esp+0x40+var_8], edx
0x43af63: mov dword [esp+0x40+var_4], eax
0x43af67: mov dword [esp+0x40+var_3C], 0x2
0x43af6f: jmp loc_43af91
0x43af71: mov eax, dword [dword_3b42e14]
0x43af76: cmp eax, ecx
0x43af78: je loc_43af91
0x43af7a: add eax, 0x20
0x43af7d: mov edx, dword [eax]
0x43af7f: mov dword [esp+0x40+var_18], edx
0x43af83: mov edx, dword [eax+4]
0x43af86: mov eax, dword [eax+8]
0x43af89: mov dword [esp+0x40+var_14], edx
0x43af8d: mov dword [esp+0x40+var_10], eax
0x43af91: push ebx
0x43af92: push esi
0x43af93: mov word [word_3b32724], cx
0x43af9a: mov dword [dword_3b36d38], ecx
0x43afa0: push edi
0x43afa1: xor ecx, ecx
0x43afa3: call sub_7850f0
0x43afa8: movsx eax, word [word_3b2d510]
0x43afaf: lea ebx, dword [eax+eax*2]
0x43afb2: lea ebx, dword [word_3b32724+1536+ebx*4]
0x43afb9: cmp ebx, 0x3b32d30
0x43afbf: jb loc_43b104
0x43afc5: mov eax, 0x3b2f720
0x43afca: neg eax
0x43afcc: mov dword [esp+0x4c+var_2C], eax
0x43afd0: mov ecx, dword [ebx+8]
0x43afd3: mov edx, dword [ecx+0x20]
0x43afd6: mov esi, dword [ebx+4]
0x43afd9: xor eax, eax
0x43afdb: mov ax, word [edx+4]
0x43afdf: and eax, 0x100
0x43afe4: test ax, ax
0x43afe7: je loc_43b0f5
0x43afed: test dword [ebx], 0x10000000
0x43aff3: jne loc_43b018
0x43aff5: mov eax, dword [esi+4]
0x43aff8: fld dword [eax+0x18]
0x43affb: add eax, 0x18
0x43affe: fadd dword [esi+8]
0x43b001: lea ecx, dword [esi+8]
0x43b004: fld dword [eax+4]
0x43b007: add eax, 0x4
0x43b00a: fadd dword [ecx+4]
0x43b00d: add ecx, 0x4
0x43b010: fld dword [eax+4]
0x43b013: fadd dword [ecx+4]
0x43b016: jmp loc_43b07e
0x43b018: xor ecx, ecx
0x43b01a: call sub_7875f0
0x43b01f: mov eax, dword [esi+0x1c]
0x43b022: test eax, eax
0x43b024: je loc_43b030
0x43b026: movzx edx, ax
0x43b029: xor ecx, ecx
0x43b02b: call sub_784fb0
0x43b030: mov eax, dword [esi+0x14]
0x43b033: test eax, eax
0x43b035: je loc_43b041
0x43b037: movzx edx, ax
0x43b03a: xor ecx, ecx
0x43b03c: call sub_784ee0
0x43b041: mov eax, dword [esi+0x18]
0x43b044: test eax, eax
0x43b046: je loc_43b052
0x43b048: movzx edx, ax
0x43b04b: xor ecx, ecx
0x43b04d: call sub_784e20
0x43b052: mov edx, dword [esi+4]
0x43b055: lea ecx, dword [esp+0x4c+var_24]
0x43b059: push ecx
0x43b05a: add edx, 0x18
0x43b05d: xor ecx, ecx
0x43b05f: call sub_781670
0x43b064: fld dword [esp+0x4c+var_24]
0x43b068: fadd dword [esi+8]
0x43b06b: lea eax, dword [esi+8]
0x43b06e: fld dword [esp+0x4c+var_20]
0x43b072: add eax, 0x4
0x43b075: fadd dword [eax]
0x43b077: fld dword [esp+0x4c+var_1C]
0x43b07b: fadd dword [eax+4]
0x43b07e: mov edx, dword [esp+0x4c+var_3C]
0x43b082: fld dword [dword_915458]
0x43b088: test edx, edx
0x43b08a: jle loc_43b0ed
0x43b08c: mov edi, dword [esi+4]
0x43b08f: lea ecx, dword [esp+0x4c+var_14]
0x43b093: fld dword [ecx-4]
0x43b096: fsub st4
0x43b098: fstp dword [esp+0x4c+var_30]
0x43b09c: fld dword [ecx]
0x43b09e: fsub st3
0x43b0a0: fstp dword [esp+0x4c+var_34]
0x43b0a4: fld dword [ecx+4]
0x43b0a7: fsub st2
0x43b0a9: fstp dword [esp+0x4c+var_38]
0x43b0ad: fadd dword [edi+0x24]
0x43b0b0: fld dword [esp+0x4c+var_38]
0x43b0b4: fmul dword [esp+0x4c+var_38]
0x43b0b8: fld dword [esp+0x4c+var_34]
0x43b0bc: fmul dword [esp+0x4c+var_34]
0x43b0c0: faddp st1
0x43b0c2: fld dword [esp+0x4c+var_30]
0x43b0c6: fmul dword [esp+0x4c+var_30]
0x43b0ca: faddp st1
0x43b0cc: fld st1
0x43b0ce: fmul st2
0x43b0d0: fcompp
0x43b0d2: fnstsw ax
0x43b0d4: fstp st0
0x43b0d6: test ah, 0x41
0x43b0d9: je loc_43b1e6
0x43b0df: fld dword [dword_value_1109393408]
0x43b0e5: dec edx
0x43b0e6: add ecx, 0xc
0x43b0e9: test edx, edx
0x43b0eb: jg loc_43b093
0x43b0ed: fstp st0
0x43b0ef: fstp st0
0x43b0f1: fstp st0
0x43b0f3: fstp st0
0x43b0f5: sub ebx, 0xc
0x43b0f8: cmp ebx, 0x3b32d30
0x43b0fe: jae loc_43afd0
0x43b104: mov ecx, 0x1
0x43b109: call sub_785140
0x43b10e: cmp dword [dword_915460], 0x1
0x43b115: mov dword [dword_3b36d3c], 0x0
0x43b11f: mov dword [esp+0x4c+var_38], 0x3b2e518
0x43b127: jne loc_43b2d8
0x43b12d: mov eax, dword [dword_3b2f718]
0x43b132: test eax, eax
0x43b134: je loc_43b2d8
0x43b13a: mov ebx, dword [eax+0xc]
0x43b13d: movsx eax, word [eax]
0x43b140: test ax, ax
0x43b143: mov dword [esp+0x4c+var_2C], eax
0x43b147: jle loc_43b2d8
0x43b14d: lea ecx, dword [ecx]
0x43b150: test dword [ebx+0x20], 0x400003
0x43b157: je loc_43b2c7
0x43b15d: mov edi, dword [esp+0x4c+var_3C]
0x43b161: mov ecx, dword [dword_915458]
0x43b167: mov eax, edi
0x43b169: test eax, eax
0x43b16b: mov dword [esp+0x4c+var_34], ecx
0x43b16f: jle loc_43b2c3
0x43b175: lea esi, dword [esp+0x4c+var_14]
0x43b179: lea esp, dword [esp+0x4c+var_4C]
0x43b180: fld dword [ebx]
0x43b182: mov edx, dword [ebx+8]
0x43b185: fld dword [ebx+4]
0x43b188: mov dword [esp+0x4c+var_28], edx
0x43b18c: fxch st0, st1
0x43b18e: push ecx
0x43b18f: fsub dword [esi-4]
0x43b192: fstp dword [esp+0x50+var_30]
0x43b196: fsub dword [esi]
0x43b198: fld dword [esp+0x50+var_28]
0x43b19c: fsub dword [esi+4]
0x43b19f: fld st0
0x43b1a1: fmul st1
0x43b1a3: fld st2
0x43b1a5: fmul st3
0x43b1a7: faddp st1
0x43b1a9: fld dword [esp+0x50+var_30]
0x43b1ad: fmul dword [esp+0x50+var_30]
0x43b1b1: faddp st1
0x43b1b3: fstp dword [esp+0x50+var_50]
0x43b1b6: fstp st0
0x43b1b8: fstp st0
0x43b1ba: call sub_7871f0
0x43b1bf: fsub dword [esp+0x4c+var_34]
0x43b1c3: fcomp dword [ebx+0xc]
0x43b1c6: fnstsw ax
0x43b1c8: test ah, 0x5
0x43b1cb: jnp loc_43b271
0x43b1d1: dec edi
0x43b1d2: add esi, 0xc
0x43b1d5: test edi, edi
0x43b1d7: mov dword [esp+0x4c+var_34], 0x42200000
0x43b1df: jg loc_43b180
0x43b1e1: jmp loc_43b2c3
0x43b1e6: mov eax, dword [esp+0x4c+var_2C]
0x43b1ea: fstp st0
0x43b1ec: lea edx, dword [eax+ebp+0x3b32728]
0x43b1f3: fstp st0
0x43b1f5: mov ecx, ebx
0x43b1f7: fstp st0
0x43b1f9: mov edi, dword [ecx]
0x43b1fb: mov dword [edx], edi
0x43b1fd: mov edi, dword [ecx+4]
0x43b200: mov ecx, dword [ecx+8]
0x43b203: mov dword [edx+4], edi
0x43b206: mov dword [edx+8], ecx
0x43b209: mov edx, dword [ebx+8]
0x43b20c: mov ecx, dword [dword_3b36d38]
0x43b212: mov dword [eax+ebp+0x3b2f724], esi
0x43b219: mov dword [eax+ebp+0x3b2f728], edx
0x43b220: mov eax, dword [ebx]
0x43b222: inc ecx
0x43b223: mov dword [ebp], eax
0x43b226: add ebp, 0xc
0x43b229: inc word [word_3b32724]
0x43b230: cmp ecx, 0x80
0x43b236: mov dword [dword_3b36d38], ecx
0x43b23c: jge loc_43b25f
0x43b23e: cmp word [word_3b32724], 0x400
0x43b247: jl loc_43b0f5
0x43b24d: mov ecx, 0x1
0x43b252: call sub_785140
0x43b257: pop edi
0x43b258: pop esi
0x43b259: pop ebx
0x43b25a: pop ebp
0x43b25b: add esp, 0x3c
0x43b25e: ret
0x43b25f: push 0x7e0018
0x43b264: call sub_401000
0x43b269: add esp, 0x4
0x43b26c: jmp loc_43b104
0x43b271: mov edi, dword [esp+0x4c+var_38]
0x43b275: mov eax, dword [esp+0x4c+var_38]
0x43b279: mov ecx, 0x9
0x43b27e: mov esi, ebx
0x43b280: rep movsd dword [edi], dword [esi]
0x43b282: mov ecx, dword [ebx+0x20]
0x43b285: add eax, 0x24
0x43b288: mov dword [ebp], ecx
0x43b28b: mov dword [ebp+8], 0x0
0x43b292: mov edx, dword [ebx+0x18]
0x43b295: mov dword [esp+0x4c+var_38], eax
0x43b299: mov eax, dword [dword_3b36d3c]
0x43b29e: inc eax
0x43b29f: mov dword [ebp+4], edx
0x43b2a2: add ebp, 0xc
0x43b2a5: inc word [word_3b32724]
0x43b2ac: cmp eax, 0x80
0x43b2b1: mov dword [dword_3b36d3c], eax
0x43b2b6: jge loc_43b2d8
0x43b2b8: cmp word [word_3b32724], 0x400
0x43b2c1: jge loc_43b2d8
0x43b2c3: mov eax, dword [esp+0x4c+var_2C]
0x43b2c7: add ebx, 0x24
0x43b2ca: dec eax
0x43b2cb: test ax, ax
0x43b2ce: mov dword [esp+0x4c+var_2C], eax
0x43b2d2: jg loc_43b150
0x43b2d8: pop edi
0x43b2d9: pop esi
0x43b2da: pop ebx
0x43b2db: pop ebp
0x43b2dc: add esp, 0x3c
0x43b2df: ret
