0x43c7d0: sub esp, 0x10
0x43c7d3: push ebx
0x43c7d4: mov ebx, eax
0x43c7d6: mov eax, dword [esi+0xa0]
0x43c7dc: mov dword [edi+8], 0x0
0x43c7e3: mov dword [edi], 0x0
0x43c7e9: mov edx, dword [esi+0x60]
0x43c7ec: mov ecx, edx
0x43c7ee: and ecx, 0x4000
0x43c7f4: mov dword [esp+0x14+var_10], eax
0x43c7f8: je loc_43c81f
0x43c7fa: fld dword [ebx+8]
0x43c7fd: fcomp dword [dword_value_1142947840+4]
0x43c803: fnstsw ax
0x43c805: test ah, 0x41
0x43c808: jp loc_43c81f
0x43c80a: fld dword [esp+0x14+var_10]
0x43c80e: pop ebx
0x43c80f: fmul dword [dword_value_1065437102+4]
0x43c815: fsubr dword [edi+4]
0x43c818: fstp dword [edi+4]
0x43c81b: add esp, 0x10
0x43c81e: ret
0x43c81f: test ecx, ecx
0x43c821: je loc_43c848
0x43c823: fld dword [ebx+8]
0x43c826: fcomp dword [dword_value_1142947840+4]
0x43c82c: fnstsw ax
0x43c82e: test ah, 0x41
0x43c831: jne loc_43c848
0x43c833: fld dword [esp+0x14+var_10]
0x43c837: pop ebx
0x43c838: fmul dword [dword_value_1055286886+4]
0x43c83e: fsubr dword [edi+4]
0x43c841: fstp dword [edi+4]
0x43c844: add esp, 0x10
0x43c847: ret
0x43c848: test edx, 0x20000
0x43c84e: jne loc_43ca2d
0x43c854: fld dword [esi+0x3c]
0x43c857: fcomp dword [dword_value_1142947840+4]
0x43c85d: fnstsw ax
0x43c85f: test ah, 0x41
0x43c862: jp loc_43ca1c
0x43c868: test dl, 0x8
0x43c86b: je loc_43c889
0x43c86d: mov al, byte [esi+0x10]
0x43c870: fld dword [esp+0x14+var_10]
0x43c874: fmul dword [dword_value_1050253722]
0x43c87a: pop ebx
0x43c87b: fld dword [edi+4]
0x43c87e: fsub st1
0x43c880: fstp dword [edi+4]
0x43c883: fstp st0
0x43c885: add esp, 0x10
0x43c888: ret
0x43c889: mov edx, dword [esp+0x14+arg_0]
0x43c88d: add edx, 0x1c
0x43c890: mov ecx, dword_3b0f0f8
0x43c895: call sub_787cf0
0x43c89a: fchs
0x43c89c: xor ecx, ecx
0x43c89e: mov cx, word [esi+0x10]
0x43c8a2: mov edx, ecx
0x43c8a4: and edx, 0x2000
0x43c8aa: test dx, dx
0x43c8ad: je loc_43c912
0x43c8af: fcom dword [dword_value_3206125978]
0x43c8b5: fnstsw ax
0x43c8b7: test ah, 0x41
0x43c8ba: jp loc_43c8d7
0x43c8bc: fstp st0
0x43c8be: pop ebx
0x43c8bf: fld dword [esp+0x10+var_10]
0x43c8c3: fmul dword [dword_value_1072064102]
0x43c8c9: fld dword [edi+4]
0x43c8cc: fsub st1
0x43c8ce: fstp dword [edi+4]
0x43c8d1: fstp st0
0x43c8d3: add esp, 0x10
0x43c8d6: ret
0x43c8d7: fcomp dword [dword_value_1070805811+4]
0x43c8dd: fld dword [esp+0x14+var_10]
0x43c8e1: fnstsw ax
0x43c8e3: test ah, 0x41
0x43c8e6: jp loc_43c8fd
0x43c8e8: fmul dword [dword_value_1070805811]
0x43c8ee: pop ebx
0x43c8ef: fld dword [edi+4]
0x43c8f2: fsub st1
0x43c8f4: fstp dword [edi+4]
0x43c8f7: fstp st0
0x43c8f9: add esp, 0x10
0x43c8fc: ret
0x43c8fd: fmul dword [dword_value_1068708659]
0x43c903: pop ebx
0x43c904: fld dword [edi+4]
0x43c907: fsub st1
0x43c909: fstp dword [edi+4]
0x43c90c: fstp st0
0x43c90e: add esp, 0x10
0x43c911: ret
0x43c912: fld dword [esi+0x38]
0x43c915: fcomp dword [esi+0xd0]
0x43c91b: fnstsw ax
0x43c91d: test ah, 0x1
0x43c920: jne loc_43c92b
0x43c922: test cl, 0x3
0x43c925: jne loc_43ca07
0x43c92b: fcom dword [dword_value_3184315597]
0x43c931: fnstsw ax
0x43c933: test ah, 0x41
0x43c936: jnp loc_43ca07
0x43c93c: fcomp dword [dword_value_1055286886+4]
0x43c942: fnstsw ax
0x43c944: test ah, 0x5
0x43c947: jp loc_43c9bd
0x43c949: add ebx, 0x4
0x43c94c: mov eax, dword [ebx]
0x43c94e: mov ecx, dword [ebx+4]
0x43c951: mov edx, dword [ebx+8]
0x43c954: mov dword [esp+0x14+var_8], ecx
0x43c958: lea ecx, dword [esp+0x14+var_C]
0x43c95c: mov dword [esp+0x14+var_C], eax
0x43c960: mov dword [esp+0x14+var_4], edx
0x43c964: call sub_787cc0
0x43c969: fcomp dword [dword_value_1142947840+4]
0x43c96f: fnstsw ax
0x43c971: test ah, 0x44
0x43c974: jp loc_43c97e
0x43c976: fld dword [dword_value_1142947840+4]
0x43c97c: jmp loc_43c991
0x43c97e: lea eax, dword [esp+0x14+var_C]
0x43c982: push eax
0x43c983: call sub_418ac0
0x43c988: fstp st0
0x43c98a: fld dword [esp+0x18+var_8]
0x43c98e: add esp, 0x4
0x43c991: fabs
0x43c993: fcomp dword [dword_value_1048576000]
0x43c999: fld dword [esp+0x14+var_10]
0x43c99d: fnstsw ax
0x43c99f: test ah, 0x5
0x43c9a2: jp loc_43c874
0x43c9a8: fmul dword [dword_value_1069547520]
0x43c9ae: pop ebx
0x43c9af: fld dword [edi+4]
0x43c9b2: fsub st1
0x43c9b4: fstp dword [edi+4]
0x43c9b7: fstp st0
0x43c9b9: add esp, 0x10
0x43c9bc: ret
0x43c9bd: fld dword [esi+0x38]
0x43c9c0: fcomp dword [esi+0xc4]
0x43c9c6: fnstsw ax
0x43c9c8: test ah, 0x41
0x43c9cb: jp loc_43c9e6
0x43c9cd: fld dword [esp+0x14+var_10]
0x43c9d1: pop ebx
0x43c9d2: fmul dword [dword_value_1056964608]
0x43c9d8: fld dword [edi+4]
0x43c9db: fsub st1
0x43c9dd: fstp dword [edi+4]
0x43c9e0: fstp st0
0x43c9e2: add esp, 0x10
0x43c9e5: ret
0x43c9e6: test cl, 0x3
0x43c9e9: je loc_43ca09
0x43c9eb: fld dword [esi+0x38]
0x43c9ee: pop ebx
0x43c9ef: fdiv dword [esi+0xd0]
0x43c9f5: fmul dword [esp+0x10+var_10]
0x43c9f9: fld dword [edi+4]
0x43c9fc: fsub st1
0x43c9fe: fstp dword [edi+4]
0x43ca01: fstp st0
0x43ca03: add esp, 0x10
0x43ca06: ret
0x43ca07: fstp st0
0x43ca09: fld dword [esp+0x14+var_10]
0x43ca0d: pop ebx
0x43ca0e: fld dword [edi+4]
0x43ca11: fsub st1
0x43ca13: fstp dword [edi+4]
0x43ca16: fstp st0
0x43ca18: add esp, 0x10
0x43ca1b: ret
0x43ca1c: xor ecx, ecx
0x43ca1e: mov cx, word [esi+0x10]
0x43ca22: and ecx, 0x2000
0x43ca28: test cx, cx
0x43ca2b: je loc_43ca37
0x43ca2d: fld dword [edi+4]
0x43ca30: fsub dword [esp+0x14+var_10]
0x43ca34: fstp dword [edi+4]
0x43ca37: pop ebx
0x43ca38: add esp, 0x10
0x43ca3b: ret
