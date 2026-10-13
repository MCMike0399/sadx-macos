0x450920: push ebp
0x450921: mov ebp, esp
0x450923: sub esp, 0xc
0x450926: mov eax, dword [ebp+arg_0]
0x450929: mov ecx, dword [ebp+arg_0]
0x45092c: fld dword [eax+8]
0x45092f: fmul dword [ecx+8]
0x450932: mov edx, dword [ebp+arg_0]
0x450935: mov eax, dword [ebp+arg_0]
0x450938: fld dword [edx+4]
0x45093b: fmul dword [eax+4]
0x45093e: mov ecx, dword [ebp+arg_0]
0x450941: mov edx, dword [ebp+arg_0]
0x450944: fld dword [ecx]
0x450946: fmul dword [edx]
0x450948: faddp st1
0x45094a: faddp st1
0x45094c: fst dword [ebp+var_8]
0x45094f: sub esp, 0x8
0x450952: fstp qword [esp+0x14+var_14]
0x450955: call sub_6452a4
0x45095a: add esp, 0x8
0x45095d: fst dword [ebp+var_C]
0x450960: fstp dword [ebp+var_4]
0x450963: fld dword [ebp+var_4]
0x450966: fcomp dword [dword_value_1142947840+4]
0x45096c: fnstsw ax
0x45096e: test ah, 0x44
0x450971: jp loc_450992
0x450973: mov eax, dword [ebp+arg_0]
0x450976: mov dword [eax+8], 0x0
0x45097d: mov ecx, dword [ebp+arg_0]
0x450980: mov dword [ecx+4], 0x0
0x450987: mov edx, dword [ebp+arg_0]
0x45098a: mov dword [edx], 0x0
0x450990: jmp loc_4509c9
0x450992: fld dword [aImm32dll+16]
0x450998: fdiv dword [ebp+var_4]
0x45099b: fstp dword [ebp+var_4]
0x45099e: mov eax, dword [ebp+arg_0]
0x4509a1: fld dword [ebp+var_4]
0x4509a4: fmul dword [eax]
0x4509a6: mov ecx, dword [ebp+arg_0]
0x4509a9: fstp dword [ecx]
0x4509ab: mov edx, dword [ebp+arg_0]
0x4509ae: fld dword [ebp+var_4]
0x4509b1: fmul dword [edx+4]
0x4509b4: mov eax, dword [ebp+arg_0]
0x4509b7: fstp dword [eax+4]
0x4509ba: mov ecx, dword [ebp+arg_0]
0x4509bd: fld dword [ebp+var_4]
0x4509c0: fmul dword [ecx+8]
0x4509c3: mov edx, dword [ebp+arg_0]
0x4509c6: fstp dword [edx+8]
0x4509c9: mov esp, ebp
0x4509cb: pop ebp
0x4509cc: ret
