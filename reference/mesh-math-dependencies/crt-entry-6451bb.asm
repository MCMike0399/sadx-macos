0x6451bb: push ebp
0x6451bc: mov ebp, esp
0x6451be: push ecx
0x6451bf: push ecx
0x6451c0: push esi
0x6451c1: push edi
0x6451c2: mov esi, 0xffff
0x6451c7: push esi
0x6451c8: push 0x133f
0x6451cd: call sub_64b1fd
0x6451d2: mov edi, eax
0x6451d4: mov eax, dword [ebp+arg_6]
0x6451d7: and ax, 0x7ff0
0x6451db: cmp ax, 0x7ff0
0x6451df: pop ecx
0x6451e0: pop ecx
0x6451e1: jne loc_64524b
0x6451e3: fld qword [ebp+arg_0]
0x6451e6: push ecx
0x6451e7: push ecx
0x6451e8: fstp qword [esp+0x18+var_18]
0x6451eb: call sub_64b0cd
0x6451f0: dec eax
0x6451f1: pop ecx
0x6451f2: pop ecx
0x6451f3: je loc_64523f
0x6451f5: dec eax
0x6451f6: je loc_645231
0x6451f8: dec eax
0x6451f9: fld qword [ebp+arg_0]
0x6451fc: push edi
0x6451fd: je loc_645220
0x6451ff: fadd qword [dword_value_3233808384+8]
0x645205: sub esp, 0x10
0x645208: fstp qword [esp+0x24+var_1C]
0x64520c: fld qword [ebp+arg_0]
0x64520f: fstp qword [esp+0x24+var_24]
0x645212: push 0x15
0x645214: push 0x8
0x645216: call sub_64afe5
0x64521b: add esp, 0x1c
0x64521e: jmp loc_645268
0x645220: push ecx
0x645221: push ecx
0x645222: fstp qword [esp+0x1c+var_1C]
0x645225: push 0x15
0x645227: call sub_64af93
0x64522c: add esp, 0x10
0x64522f: jmp loc_645268
0x645231: push esi
0x645232: push edi
0x645233: call sub_64b1fd
0x645238: fld qword [ebp+arg_0]
0x64523b: fchs
0x64523d: jmp loc_645266
0x64523f: push esi
0x645240: push edi
0x645241: call sub_64b1fd
0x645246: fld qword [ebp+arg_0]
0x645249: jmp loc_645266
0x64524b: mov eax, dword [ebp+arg_4]
0x64524e: and eax, 0x7fffffff
0x645253: mov dword [ebp+var_4], eax
0x645256: mov eax, dword [ebp+arg_0]
0x645259: push esi
0x64525a: push edi
0x64525b: mov dword [ebp+var_8], eax
0x64525e: call sub_64b1fd
0x645263: fld qword [ebp+var_8]
0x645266: pop ecx
0x645267: pop ecx
0x645268: pop edi
0x645269: pop esi
0x64526a: leave
0x64526b: ret
