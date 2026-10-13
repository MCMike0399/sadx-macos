0x43acd0: sub esp, 0xc
0x43acd3: push ebx
0x43acd4: push esi
0x43acd5: push edi
0x43acd6: xor ecx, ecx
0x43acd8: mov edi, 0x3b2f720
0x43acdd: mov word [word_3b32724], 0x0
0x43ace6: call sub_7850f0
0x43aceb: mov eax, dword [dword_3b36d38]
0x43acf0: lea ebx, dword [eax+eax*2]
0x43acf3: lea ebx, dword [dword_3b2f718+12292+ebx*4]
0x43acfa: cmp ebx, 0x3b32728
0x43ad00: jb loc_43ae10
0x43ad06: jmp loc_43ad10
0x43ad10: test dword [ebx], 0x10000000
0x43ad16: mov esi, dword [ebx+4]
0x43ad19: jne loc_43ad37
0x43ad1b: mov eax, dword [esi+4]
0x43ad1e: fld dword [eax+0x18]
0x43ad21: fadd dword [esi+8]
0x43ad24: fsubr dword [esp+0x18+arg_0]
0x43ad28: fld dword [eax+0x1c]
0x43ad2b: fadd dword [esi+0xc]
0x43ad2e: fsubr dword [esp+0x18+arg_4]
0x43ad32: fld dword [eax+0x20]
0x43ad35: jmp loc_43ad9d
0x43ad37: xor ecx, ecx
0x43ad39: call sub_7875f0
0x43ad3e: mov eax, dword [esi+0x1c]
0x43ad41: test eax, eax
0x43ad43: je loc_43ad4f
0x43ad45: movzx edx, ax
0x43ad48: xor ecx, ecx
0x43ad4a: call sub_784fb0
0x43ad4f: mov eax, dword [esi+0x14]
0x43ad52: test eax, eax
0x43ad54: je loc_43ad60
0x43ad56: movzx edx, ax
0x43ad59: xor ecx, ecx
0x43ad5b: call sub_784ee0
0x43ad60: mov eax, dword [esi+0x18]
0x43ad63: test eax, eax
0x43ad65: je loc_43ad71
0x43ad67: movzx edx, ax
0x43ad6a: xor ecx, ecx
0x43ad6c: call sub_784e20
0x43ad71: mov edx, dword [esi+4]
0x43ad74: lea eax, dword [esp+0x18+var_C]
0x43ad78: push eax
0x43ad79: add edx, 0x18
0x43ad7c: xor ecx, ecx
0x43ad7e: call sub_781670
0x43ad83: fld dword [esp+0x18+var_C]
0x43ad87: fadd dword [esi+8]
0x43ad8a: fsubr dword [esp+0x18+arg_0]
0x43ad8e: fld dword [esp+0x18+var_8]
0x43ad92: fadd dword [esi+0xc]
0x43ad95: fsubr dword [esp+0x18+arg_4]
0x43ad99: fld dword [esp+0x18+var_4]
0x43ad9d: fadd dword [esi+0x10]
0x43ada0: mov ecx, dword [esi+4]
0x43ada3: fsubr dword [esp+0x18+arg_8]
0x43ada7: fld dword [esp+0x18+arg_C]
0x43adab: fadd dword [ecx+0x24]
0x43adae: fadd dword [qword_value_4636033603912859648+8]
0x43adb4: fld st1
0x43adb6: fmul st2
0x43adb8: fld st3
0x43adba: fmul st4
0x43adbc: faddp st1
0x43adbe: fld st4
0x43adc0: fmul st5
0x43adc2: faddp st1
0x43adc4: fld st1
0x43adc6: fmul st2
0x43adc8: fcompp
0x43adca: fstp st0
0x43adcc: fstp st0
0x43adce: fnstsw ax
0x43add0: fstp st0
0x43add2: test ah, 0x41
0x43add5: fstp st0
0x43add7: jne loc_43ae01
0x43add9: mov eax, dword [ebx]
0x43addb: mov edx, dword [ebx+8]
0x43adde: mov dword [edi], eax
0x43ade0: mov ax, word [word_3b32724]
0x43ade6: mov dword [edi+4], esi
0x43ade9: mov dword [edi+8], edx
0x43adec: add edi, 0xc
0x43adef: inc ax
0x43adf1: cmp ax, 0x400
0x43adf5: mov word [word_3b32724], ax
0x43adfb: jge loc_43aed7
0x43ae01: sub ebx, 0xc
0x43ae04: cmp ebx, 0x3b32728
0x43ae0a: jae loc_43ad10
0x43ae10: mov ecx, 0x1
0x43ae15: call sub_785140
0x43ae1a: cmp dword [dword_915460], 0x1
0x43ae21: jne loc_43aed0
0x43ae27: mov eax, dword [dword_3b2f718]
0x43ae2c: test eax, eax
0x43ae2e: je loc_43aed0
0x43ae34: mov eax, dword [dword_3b36d3c]
0x43ae39: test ax, ax
0x43ae3c: mov esi, 0x3b2e518
0x43ae41: mov ebx, eax
0x43ae43: jle loc_43aed0
0x43ae49: lea esp, dword [esp+0x18+var_18]
0x43ae50: test dword [esi+0x20], 0x400003
0x43ae57: je loc_43aec7
0x43ae59: fld dword [esi]
0x43ae5b: push ecx
0x43ae5c: fsub dword [esp+0x1c+arg_0]
0x43ae60: fld dword [esi+4]
0x43ae63: fsub dword [esp+0x1c+arg_4]
0x43ae67: fld dword [esi+8]
0x43ae6a: fsub dword [esp+0x1c+arg_8]
0x43ae6e: fld st0
0x43ae70: fmul st1
0x43ae72: fld st2
0x43ae74: fmul st3
0x43ae76: faddp st1
0x43ae78: fld st3
0x43ae7a: fmul st4
0x43ae7c: faddp st1
0x43ae7e: fstp dword [esp+0x1c+var_1C]
0x43ae81: fstp st0
0x43ae83: fstp st0
0x43ae85: fstp st0
0x43ae87: call sub_7871f0
0x43ae8c: fsub dword [esp+0x18+arg_C]
0x43ae90: fld dword [esi+0xc]
0x43ae93: fxch st0, st1
0x43ae95: fcompp
0x43ae97: fnstsw ax
0x43ae99: test ah, 0x5
0x43ae9c: jp loc_43aec7
0x43ae9e: mov ecx, dword [esi+0x20]
0x43aea1: mov edx, dword [esi+0x18]
0x43aea4: mov ax, word [word_3b32724]
0x43aeaa: mov dword [edi], ecx
0x43aeac: mov dword [edi+8], 0x0
0x43aeb3: mov dword [edi+4], edx
0x43aeb6: add edi, 0xc
0x43aeb9: inc ax
0x43aebb: cmp ax, 0x400
0x43aebf: mov word [word_3b32724], ax
0x43aec5: jge loc_43aed0
0x43aec7: add esi, 0x24
0x43aeca: dec ebx
0x43aecb: test bx, bx
0x43aece: jg loc_43ae50
0x43aed0: pop edi
0x43aed1: pop esi
0x43aed2: pop ebx
0x43aed3: add esp, 0xc
0x43aed6: ret
0x43aed7: mov ecx, 0x1
0x43aedc: call sub_785140
0x43aee1: pop edi
0x43aee2: pop esi
0x43aee3: pop ebx
0x43aee4: add esp, 0xc
0x43aee7: ret
