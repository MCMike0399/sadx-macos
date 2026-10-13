0x43d8b0: sub esp, 0x104
0x43d8b6: push ebx
0x43d8b7: xor ebx, ebx
0x43d8b9: cmp word [esi+0x28], bx
0x43d8bd: push ebp
0x43d8be: push edi
0x43d8bf: mov dword [esp+0x110+var_F0], ebx
0x43d8c3: mov dword [esp+0x110+var_D8], 0xffffffff
0x43d8cb: mov dword [esp+0x110+var_E0], 0x447a0000
0x43d8d3: jbe loc_43da92
0x43d8d9: mov edi, 0x3b47038
0x43d8de: mov edi, edi
0x43d8e0: fld dword [edi+4]
0x43d8e3: push ecx
0x43d8e4: fsub dword [esi+0xc]
0x43d8e7: fld dword [edi]
0x43d8e9: fsub dword [esi+8]
0x43d8ec: fld dword [edi-4]
0x43d8ef: fsub dword [esi+4]
0x43d8f2: fld st0
0x43d8f4: fmul st1
0x43d8f6: fld st3
0x43d8f8: fmul st4
0x43d8fa: faddp st1
0x43d8fc: fld st2
0x43d8fe: fmul st3
0x43d900: faddp st1
0x43d902: fstp dword [esp+0x114+var_114]
0x43d905: fstp st0
0x43d907: fstp st0
0x43d909: fstp st0
0x43d90b: call sub_7871f0
0x43d910: push 0x0
0x43d912: fst dword [esp+0x114+var_DC]
0x43d916: lea ebp, dword [edi-0x1c]
0x43d919: fstp dword [esp+ebx*4+0x114+var_40]
0x43d920: lea eax, dword [esi+0x1c]
0x43d923: push ebp
0x43d924: push eax
0x43d925: mov dword [esp+ebx*4+0x11c+var_80], 0x0
0x43d930: call sub_438410
0x43d935: add esp, 0xc
0x43d938: cmp eax, 0x2c00
0x43d93d: mov dword [esp+0x110+var_104], eax
0x43d941: jle loc_43da25
0x43d947: fld dword [esi+0x20]
0x43d94a: fcomp dword [edi-0x18]
0x43d94d: fnstsw ax
0x43d94f: test ah, 0x5
0x43d952: jnp loc_43da25
0x43d958: lea eax, dword [esi+0x10]
0x43d95b: mov ecx, dword [eax]
0x43d95d: mov edx, dword [eax+4]
0x43d960: mov eax, dword [eax+8]
0x43d963: mov dword [esp+0x110+var_FC], ecx
0x43d967: lea ecx, dword [esp+0x110+var_FC]
0x43d96b: mov dword [esp+0x110+var_F8], edx
0x43d96f: mov dword [esp+0x110+var_F4], eax
0x43d973: call sub_787cc0
0x43d978: fcomp dword [dword_value_1142947840+4]
0x43d97e: fnstsw ax
0x43d980: test ah, 0x44
0x43d983: jnp loc_43d994
0x43d985: lea ecx, dword [esp+0x110+var_FC]
0x43d989: push ecx
0x43d98a: call sub_418ac0
0x43d98f: fstp st0
0x43d991: add esp, 0x4
0x43d994: cmp dword [esp+0x110+var_104], 0x7000
0x43d99c: jge loc_43da30
0x43d9a2: mov edx, ebp
0x43d9a4: lea ecx, dword [esp+0x110+var_FC]
0x43d9a8: call sub_787cf0
0x43d9ad: fst dword [esp+0x110+var_104]
0x43d9b1: fcomp dword [dword_value_1142947840+4]
0x43d9b7: fnstsw ax
0x43d9b9: test ah, 0x5
0x43d9bc: jp loc_43da30
0x43d9be: mov edx, ebp
0x43d9c0: lea ecx, dword [esi+0x10]
0x43d9c3: call sub_787cf0
0x43d9c8: fstp dword [esp+0x110+var_100]
0x43d9cc: mov ecx, 0x600
0x43d9d1: call sub_7883c0
0x43d9d6: fchs
0x43d9d8: fcomp dword [esp+0x110+var_104]
0x43d9dc: fnstsw ax
0x43d9de: test ah, 0x41
0x43d9e1: je loc_43d9f4
0x43d9e3: fld dword [esp+0x110+var_100]
0x43d9e7: fcomp dword [dword_value_3230662656]
0x43d9ed: fnstsw ax
0x43d9ef: test ah, 0x5
0x43d9f2: jp loc_43da30
0x43d9f4: fld dword [esp+0x110+var_E0]
0x43d9f8: mov eax, dword [esp+0x110+var_F0]
0x43d9fc: fcomp dword [esp+0x110+var_DC]
0x43da00: inc eax
0x43da01: mov dword [esp+0x110+var_F0], eax
0x43da05: mov dword [esp+ebx*4+0x110+var_80], 0x8
0x43da10: fnstsw ax
0x43da12: test ah, 0x41
0x43da15: jne loc_43da30
0x43da17: mov edx, dword [esp+0x110+var_DC]
0x43da1b: mov dword [esp+0x110+var_E0], edx
0x43da1f: mov dword [esp+0x110+var_D8], ebx
0x43da23: jmp loc_43da30
0x43da25: mov dword [esp+ebx*4+0x110+var_80], 0x2
0x43da30: movzx eax, word [esi+0x28]
0x43da34: inc ebx
0x43da35: add edi, 0x58
0x43da38: cmp ebx, eax
0x43da3a: jl loc_43d8e0
0x43da40: mov eax, dword [esp+0x110+var_F0]
0x43da44: test eax, eax
0x43da46: jle loc_43da92
0x43da48: lea ecx, dword [esi+0x10]
0x43da4b: call sub_787cc0
0x43da50: mov ebx, dword [esp+0x110+var_D8]
0x43da54: fcomp dword [esp+ebx*4+0x110+var_40]
0x43da5b: fnstsw ax
0x43da5d: test ah, 0x41
0x43da60: jne loc_43da92
0x43da62: mov ebp, dword [esp+0x110+arg_8]
0x43da69: mov eax, dword [ebp+0x60]
0x43da6c: test ah, 0x40
0x43da6f: jne loc_43da92
0x43da71: lea edi, dword [esi+0x6c]
0x43da74: mov ecx, edi
0x43da76: call sub_787cc0
0x43da7b: fcomp dword [dword_value_1142947840+4]
0x43da81: fnstsw ax
0x43da83: test ah, 0x44
0x43da86: jp loc_43da9e
0x43da88: xor eax, eax
0x43da8a: mov dword [esi+0x74], eax
0x43da8d: mov dword [esi+0x70], eax
0x43da90: mov dword [edi], eax
0x43da92: pop edi
0x43da93: pop ebp
0x43da94: xor eax, eax
0x43da96: pop ebx
0x43da97: add esp, 0x104
0x43da9d: ret
0x43da9e: fld dword [esi+0x78]
0x43daa1: imul ebx, ebx, 0x58
0x43daa4: fadd dword [edi]
0x43daa6: fstp dword [edi]
0x43daa8: fld dword [esi+0x7c]
0x43daab: fadd dword [esi+0x70]
0x43daae: add ebx, 0x3b47008
0x43dab4: fstp dword [esi+0x70]
0x43dab7: lea ecx, dword [ebp+0x44]
0x43daba: fld dword [esi+0x80]
0x43dac0: lea eax, dword [ebx+0x14]
0x43dac3: fadd dword [esi+0x74]
0x43dac6: mov dword [esp+0x110+var_100], ecx
0x43daca: fstp dword [esi+0x74]
0x43dacd: mov edx, dword [eax]
0x43dacf: mov dword [ecx], edx
0x43dad1: mov edx, dword [eax+4]
0x43dad4: mov dword [ecx+4], edx
0x43dad7: mov eax, dword [eax+8]
0x43dada: mov dword [ecx+8], eax
0x43dadd: mov eax, dword [esp+0x110+arg_4]
0x43dae4: fld dword [eax+4]
0x43dae7: lea ebp, dword [eax+4]
0x43daea: fchs
0x43daec: lea ecx, dword [esp+0x110+var_BC]
0x43daf0: fstp dword [esp+0x110+var_BC]
0x43daf4: fld dword [eax+8]
0x43daf7: fchs
0x43daf9: fstp dword [esp+0x110+var_B8]
0x43dafd: fld dword [eax+0xc]
0x43db00: fchs
0x43db02: fstp dword [esp+0x110+var_B4]
0x43db06: call sub_787cc0
0x43db0b: fcomp dword [dword_value_1142947840+4]
0x43db11: fnstsw ax
0x43db13: test ah, 0x44
0x43db16: jnp loc_43da92
0x43db1c: mov edx, dword [esp+0x110+var_100]
0x43db20: lea ecx, dword [esp+0x110+var_EC]
0x43db24: push ecx
0x43db25: mov ecx, edi
0x43db27: call sub_7889f0
0x43db2c: fstp st0
0x43db2e: lea ecx, dword [esp+0x110+var_EC]
0x43db32: call sub_787cc0
0x43db37: fcomp dword [dword_value_1142947840+4]
0x43db3d: fnstsw ax
0x43db3f: test ah, 0x44
0x43db42: jp loc_43db5b
0x43db44: mov edx, dword [esi+0x1c]
0x43db47: mov eax, dword [esi+0x20]
0x43db4a: mov ecx, dword [esi+0x24]
0x43db4d: mov dword [esp+0x110+var_EC], edx
0x43db51: mov dword [esp+0x110+var_E8], eax
0x43db55: mov dword [esp+0x110+var_E4], ecx
0x43db59: jmp loc_43db6a
0x43db5b: lea edx, dword [esp+0x110+var_EC]
0x43db5f: push edx
0x43db60: call sub_418ac0
0x43db65: fstp st0
0x43db67: add esp, 0x4
0x43db6a: mov eax, dword [ebx+0x20]
0x43db6d: mov ecx, dword [ebx+0x24]
0x43db70: mov edx, dword [ebx+0x28]
0x43db73: mov dword [esp+0x110+var_B0], eax
0x43db77: mov eax, dword [ebx+0x14]
0x43db7a: mov dword [esp+0x110+var_AC], ecx
0x43db7e: mov ecx, dword [ebx+0x18]
0x43db81: mov dword [esp+0x110+var_A8], edx
0x43db85: mov edx, dword [ebx+0x1c]
0x43db88: mov dword [esp+0x110+var_A4], eax
0x43db8c: mov eax, dword [esi+0x60]
0x43db8f: mov dword [esp+0x110+var_A0], ecx
0x43db93: mov ecx, dword [esi+0x64]
0x43db96: mov dword [esp+0x110+var_9C], edx
0x43db9a: mov edx, dword [esi+0x68]
0x43db9d: mov dword [esp+0x110+var_98], eax
0x43dba1: mov eax, dword [esp+0x110+var_EC]
0x43dba5: mov dword [esp+0x110+var_94], ecx
0x43dba9: mov ecx, dword [esp+0x110+var_E8]
0x43dbad: mov dword [esp+0x110+var_90], edx
0x43dbb4: mov edx, dword [esp+0x110+var_E4]
0x43dbb8: mov dword [esp+0x110+var_8C], eax
0x43dbbf: lea eax, dword [esp+0x110+var_D4]
0x43dbc3: mov dword [esp+0x110+var_88], ecx
0x43dbca: mov dword [esp+0x110+var_84], edx
0x43dbd1: push eax
0x43dbd2: lea edx, dword [esp+0x114+var_98]
0x43dbd6: lea ecx, dword [esp+0x114+var_B0]
0x43dbda: call sub_7887d0
0x43dbdf: fstp st0
0x43dbe1: mov ecx, edi
0x43dbe3: mov edx, dword [ecx]
0x43dbe5: mov eax, dword [ecx+4]
0x43dbe8: mov ecx, dword [ecx+8]
0x43dbeb: mov dword [esp+0x110+var_F4], ecx
0x43dbef: lea ecx, dword [esp+0x110+var_FC]
0x43dbf3: mov dword [esp+0x110+var_FC], edx
0x43dbf7: mov dword [esp+0x110+var_F8], eax
0x43dbfb: call sub_787cc0
0x43dc00: fcomp dword [dword_value_1142947840+4]
0x43dc06: fnstsw ax
0x43dc08: test ah, 0x44
0x43dc0b: jnp loc_43dc1c
0x43dc0d: lea edx, dword [esp+0x110+var_FC]
0x43dc11: push edx
0x43dc12: call sub_418ac0
0x43dc17: fstp st0
0x43dc19: add esp, 0x4
0x43dc1c: lea edx, dword [esp+0x110+var_FC]
0x43dc20: lea ecx, dword [esp+0x110+var_C8]
0x43dc24: call sub_787cf0
0x43dc29: fcomp dword [dword_value_1142947840+4]
0x43dc2f: fnstsw ax
0x43dc31: test ah, 0x5
0x43dc34: jp loc_43dc54
0x43dc36: fld dword [esp+0x110+var_C8]
0x43dc3a: fchs
0x43dc3c: fstp dword [esp+0x110+var_C8]
0x43dc40: fld dword [esp+0x110+var_C4]
0x43dc44: fchs
0x43dc46: fstp dword [esp+0x110+var_C4]
0x43dc4a: fld dword [esp+0x110+var_C0]
0x43dc4e: fchs
0x43dc50: fstp dword [esp+0x110+var_C0]
0x43dc54: mov ecx, ebp
0x43dc56: call sub_787cc0
0x43dc5b: fld dword [esp+0x110+var_C8]
0x43dc5f: mov eax, dword [esp+0x110+arg_4]
0x43dc66: fmul st1
0x43dc68: fstp dword [ebp]
0x43dc6b: fld dword [esp+0x110+var_C4]
0x43dc6f: fmul st1
0x43dc71: fstp dword [eax+8]
0x43dc74: fmul dword [esp+0x110+var_C0]
0x43dc78: fstp dword [eax+0xc]
0x43dc7b: mov eax, dword [ebp]
0x43dc7e: mov dword [edi], eax
0x43dc80: mov ecx, dword [ebp+4]
0x43dc83: mov dword [edi+4], ecx
0x43dc86: mov edx, dword [ebp+8]
0x43dc89: mov dword [edi+8], edx
0x43dc8c: pop edi
0x43dc8d: pop ebp
0x43dc8e: or eax, 0xffffffff
0x43dc91: pop ebx
0x43dc92: add esp, 0x104
0x43dc98: ret
