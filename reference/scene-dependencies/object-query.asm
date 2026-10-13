0x453820: push ebp
0x453821: mov ebp, esp
0x453823: sub esp, 0x18c
0x453829: push esi
0x45382a: push edi
0x45382b: mov eax, dword [ebp+arg_0]
0x45382e: fld dword [eax]
0x453830: fcomp dword [dword_value_1142947840+4]
0x453836: fnstsw ax
0x453838: test ah, 0x41
0x45383b: jp loc_453844
0x45383d: xor eax, eax
0x45383f: jmp loc_4546d3
0x453844: mov dword [dword_3b42ff4], 0x0
0x45384e: mov ecx, dword [ebp+arg_0]
0x453851: add ecx, 0x10
0x453854: call sub_787cc0
0x453859: fstp dword [ebp+var_10]
0x45385c: mov ecx, dword [ebp+arg_0]
0x45385f: fld dword [ecx]
0x453861: fmul dword [dword_value_1056964608]
0x453867: fstp dword [ebp+var_8]
0x45386a: mov dword [ebp+var_14], 0x3b47590
0x453871: mov edx, dword [ebp+arg_4]
0x453874: mov dword [ebp+var_18], edx
0x453877: mov dword [ebp+var_20], 0x0
0x45387e: mov eax, dword [dword_3c48550]
0x453883: mov dword [ebp+var_1C], eax
0x453886: jmp loc_453890
0x453888: mov ecx, dword [ebp+var_1C]
0x45388b: mov edx, dword [ecx]
0x45388d: mov dword [ebp+var_1C], edx
0x453890: cmp dword [ebp+var_1C], 0x0
0x453894: je loc_4538ac
0x453896: mov eax, dword [ebp+var_1C]
0x453899: mov ecx, dword [eax+8]
0x45389c: cmp ecx, dword [ebp+var_18]
0x45389f: jne loc_4538aa
0x4538a1: mov dword [ebp+var_20], 0x1
0x4538a8: jmp loc_4538ac
0x4538aa: jmp loc_453888
0x4538ac: cmp dword [ebp+var_20], 0x0
0x4538b0: jne loc_4538b9
0x4538b2: xor eax, eax
0x4538b4: jmp loc_4546d3
0x4538b9: mov edx, dword [ebp+var_1C]
0x4538bc: mov eax, dword [edx+0x24]
0x4538bf: mov dword [ebp+var_C], eax
0x4538c2: mov ecx, dword [ebp+var_1C]
0x4538c5: mov edx, dword [ecx+0x1c]
0x4538c8: mov dword [ebp+var_4], edx
0x4538cb: jmp loc_4538df
0x4538cd: mov eax, dword [ebp+var_C]
0x4538d0: sub eax, 0x1
0x4538d3: mov dword [ebp+var_C], eax
0x4538d6: mov ecx, dword [ebp+var_4]
0x4538d9: add ecx, 0x4
0x4538dc: mov dword [ebp+var_4], ecx
0x4538df: cmp dword [ebp+var_C], 0x0
0x4538e3: jle loc_45434b
0x4538e9: mov dword [ebp+var_48], 0x0
0x4538f0: mov edx, dword [ebp+var_48]
0x4538f3: mov dword [ebp+var_4C], edx
0x4538f6: mov eax, dword [ebp+var_4C]
0x4538f9: mov dword [ebp+var_50], eax
0x4538fc: mov ecx, dword [ebp+var_4]
0x4538ff: mov edx, dword [ecx]
0x453901: add edx, 0x18
0x453904: mov dword [ebp+var_84], edx
0x45390a: mov eax, dword [ebp+var_4]
0x45390d: mov ecx, dword [eax]
0x45390f: add ecx, 0x24
0x453912: mov dword [ebp+var_7C], ecx
0x453915: mov edx, dword [ebp+var_4]
0x453918: mov eax, dword [edx]
0x45391a: add eax, 0x30
0x45391d: mov dword [ebp+var_8C], eax
0x453923: mov ecx, dword [ebp+var_84]
0x453929: mov edx, dword [ecx]
0x45392b: mov dword [ebp+var_90], edx
0x453931: mov eax, dword [ebp+var_84]
0x453937: add eax, 0x4
0x45393a: mov dword [ebp+var_84], eax
0x453940: mov ecx, dword [ebp+var_84]
0x453946: mov edx, dword [ecx]
0x453948: mov dword [ebp+var_74], edx
0x45394b: mov eax, dword [ebp+var_84]
0x453951: add eax, 0x4
0x453954: mov dword [ebp+var_84], eax
0x45395a: mov ecx, dword [ebp+var_84]
0x453960: mov edx, dword [ecx]
0x453962: mov dword [ebp+var_88], edx
0x453968: mov eax, dword [ebp+var_84]
0x45396e: add eax, 0x4
0x453971: mov dword [ebp+var_84], eax
0x453977: mov ecx, dword [ebp+var_84]
0x45397d: sub ecx, 0xc
0x453980: mov dword [ebp+var_84], ecx
0x453986: mov edx, dword [ebp+var_7C]
0x453989: mov eax, dword [edx]
0x45398b: mov dword [ebp+var_80], eax
0x45398e: mov ecx, dword [ebp+var_7C]
0x453991: add ecx, 0x4
0x453994: mov dword [ebp+var_7C], ecx
0x453997: mov edx, dword [ebp+var_7C]
0x45399a: mov eax, dword [edx]
0x45399c: mov dword [ebp+var_78], eax
0x45399f: mov ecx, dword [ebp+var_7C]
0x4539a2: add ecx, 0x4
0x4539a5: mov dword [ebp+var_7C], ecx
0x4539a8: mov edx, dword [ebp+var_7C]
0x4539ab: mov eax, dword [edx]
0x4539ad: mov dword [ebp+var_94], eax
0x4539b3: mov ecx, dword [ebp+var_7C]
0x4539b6: add ecx, 0x4
0x4539b9: mov dword [ebp+var_7C], ecx
0x4539bc: mov edx, dword [ebp+var_7C]
0x4539bf: sub edx, 0xc
0x4539c2: mov dword [ebp+var_7C], edx
0x4539c5: fld dword [ebp+var_80]
0x4539c8: fsub dword [ebp+var_90]
0x4539ce: fstp dword [ebp+var_80]
0x4539d1: fld dword [ebp+var_78]
0x4539d4: fsub dword [ebp+var_74]
0x4539d7: fstp dword [ebp+var_78]
0x4539da: fld dword [ebp+var_94]
0x4539e0: fsub dword [ebp+var_88]
0x4539e6: fstp dword [ebp+var_94]
0x4539ec: lea eax, dword [ebp+var_44]
0x4539ef: mov dword [ebp+var_7C], eax
0x4539f2: mov ecx, dword [ebp+var_7C]
0x4539f5: add ecx, 0xc
0x4539f8: mov dword [ebp+var_7C], ecx
0x4539fb: mov edx, dword [ebp+var_7C]
0x4539fe: sub edx, 0x4
0x453a01: mov dword [ebp+var_7C], edx
0x453a04: mov eax, dword [ebp+var_7C]
0x453a07: mov ecx, dword [ebp+var_94]
0x453a0d: mov dword [eax], ecx
0x453a0f: mov edx, dword [ebp+var_7C]
0x453a12: sub edx, 0x4
0x453a15: mov dword [ebp+var_7C], edx
0x453a18: mov eax, dword [ebp+var_7C]
0x453a1b: mov ecx, dword [ebp+var_78]
0x453a1e: mov dword [eax], ecx
0x453a20: mov edx, dword [ebp+var_7C]
0x453a23: sub edx, 0x4
0x453a26: mov dword [ebp+var_7C], edx
0x453a29: mov eax, dword [ebp+var_7C]
0x453a2c: mov ecx, dword [ebp+var_80]
0x453a2f: mov dword [eax], ecx
0x453a31: mov edx, dword [ebp+var_8C]
0x453a37: mov eax, dword [edx]
0x453a39: mov dword [ebp+var_80], eax
0x453a3c: mov ecx, dword [ebp+var_8C]
0x453a42: add ecx, 0x4
0x453a45: mov dword [ebp+var_8C], ecx
0x453a4b: mov edx, dword [ebp+var_8C]
0x453a51: mov eax, dword [edx]
0x453a53: mov dword [ebp+var_78], eax
0x453a56: mov ecx, dword [ebp+var_8C]
0x453a5c: add ecx, 0x4
0x453a5f: mov dword [ebp+var_8C], ecx
0x453a65: mov edx, dword [ebp+var_8C]
0x453a6b: mov eax, dword [edx]
0x453a6d: mov dword [ebp+var_94], eax
0x453a73: mov ecx, dword [ebp+var_8C]
0x453a79: add ecx, 0x4
0x453a7c: mov dword [ebp+var_8C], ecx
0x453a82: mov edx, dword [ebp+var_8C]
0x453a88: sub edx, 0xc
0x453a8b: mov dword [ebp+var_8C], edx
0x453a91: fld dword [ebp+var_80]
0x453a94: fsub dword [ebp+var_90]
0x453a9a: fstp dword [ebp+var_80]
0x453a9d: fld dword [ebp+var_78]
0x453aa0: fsub dword [ebp+var_74]
0x453aa3: fstp dword [ebp+var_78]
0x453aa6: fld dword [ebp+var_94]
0x453aac: fsub dword [ebp+var_88]
0x453ab2: fstp dword [ebp+var_94]
0x453ab8: lea eax, dword [ebp+var_38]
0x453abb: mov dword [ebp+var_8C], eax
0x453ac1: mov ecx, dword [ebp+var_8C]
0x453ac7: add ecx, 0xc
0x453aca: mov dword [ebp+var_8C], ecx
0x453ad0: mov edx, dword [ebp+var_8C]
0x453ad6: sub edx, 0x4
0x453ad9: mov dword [ebp+var_8C], edx
0x453adf: mov eax, dword [ebp+var_8C]
0x453ae5: mov ecx, dword [ebp+var_94]
0x453aeb: mov dword [eax], ecx
0x453aed: mov edx, dword [ebp+var_8C]
0x453af3: sub edx, 0x4
0x453af6: mov dword [ebp+var_8C], edx
0x453afc: mov eax, dword [ebp+var_8C]
0x453b02: mov ecx, dword [ebp+var_78]
0x453b05: mov dword [eax], ecx
0x453b07: mov edx, dword [ebp+var_8C]
0x453b0d: sub edx, 0x4
0x453b10: mov dword [ebp+var_8C], edx
0x453b16: mov eax, dword [ebp+var_8C]
0x453b1c: mov ecx, dword [ebp+var_80]
0x453b1f: mov dword [eax], ecx
0x453b21: mov edx, dword [ebp+arg_0]
0x453b24: fld dword [edx+4]
0x453b27: fsub dword [ebp+var_90]
0x453b2d: fstp dword [ebp+var_70]
0x453b30: mov eax, dword [ebp+arg_0]
0x453b33: fld dword [eax+8]
0x453b36: fsub dword [ebp+var_74]
0x453b39: fstp dword [ebp+var_6C]
0x453b3c: mov ecx, dword [ebp+arg_0]
0x453b3f: fld dword [ecx+0xc]
0x453b42: fsub dword [ebp+var_88]
0x453b48: fstp dword [ebp+var_68]
0x453b4b: mov edx, dword [ebp+arg_0]
0x453b4e: add edx, 0x10
0x453b51: mov eax, dword [edx]
0x453b53: mov dword [ebp+var_2C], eax
0x453b56: mov ecx, dword [edx+4]
0x453b59: mov dword [ebp+var_28], ecx
0x453b5c: mov edx, dword [edx+8]
0x453b5f: mov dword [ebp+var_24], edx
0x453b62: fld dword [ebp+var_50]
0x453b65: fadd dword [ebp+var_44]
0x453b68: fadd dword [ebp+var_38]
0x453b6b: fdiv dword [dword_value_1077936128]
0x453b71: fstp dword [ebp+var_A0]
0x453b77: fld dword [ebp+var_4C]
0x453b7a: fadd dword [ebp+var_40]
0x453b7d: fadd dword [ebp+var_34]
0x453b80: fdiv dword [dword_value_1077936128]
0x453b86: fstp dword [ebp+var_9C]
0x453b8c: fld dword [ebp+var_48]
0x453b8f: fadd dword [ebp+var_3C]
0x453b92: fadd dword [ebp+var_30]
0x453b95: fdiv dword [dword_value_1077936128]
0x453b9b: fstp dword [ebp+var_98]
0x453ba1: fld dword [ebp+var_50]
0x453ba4: fsub dword [ebp+var_A0]
0x453baa: fstp dword [ebp+var_50]
0x453bad: fld dword [ebp+var_4C]
0x453bb0: fsub dword [ebp+var_9C]
0x453bb6: fstp dword [ebp+var_4C]
0x453bb9: fld dword [ebp+var_48]
0x453bbc: fsub dword [ebp+var_98]
0x453bc2: fstp dword [ebp+var_48]
0x453bc5: fld dword [ebp+var_44]
0x453bc8: fsub dword [ebp+var_A0]
0x453bce: fstp dword [ebp+var_44]
0x453bd1: fld dword [ebp+var_40]
0x453bd4: fsub dword [ebp+var_9C]
0x453bda: fstp dword [ebp+var_40]
0x453bdd: fld dword [ebp+var_3C]
0x453be0: fsub dword [ebp+var_98]
0x453be6: fstp dword [ebp+var_3C]
0x453be9: fld dword [ebp+var_38]
0x453bec: fsub dword [ebp+var_A0]
0x453bf2: fstp dword [ebp+var_38]
0x453bf5: fld dword [ebp+var_34]
0x453bf8: fsub dword [ebp+var_9C]
0x453bfe: fstp dword [ebp+var_34]
0x453c01: fld dword [ebp+var_30]
0x453c04: fsub dword [ebp+var_98]
0x453c0a: fstp dword [ebp+var_30]
0x453c0d: mov ecx, 0x389d650
0x453c12: call sub_7850f0
0x453c17: push 0x3f851eb8
0x453c1c: push 0x3f851eb8
0x453c21: push 0x3f851eb8
0x453c26: xor ecx, ecx
0x453c28: call sub_784c70
0x453c2d: lea eax, dword [ebp+var_50]
0x453c30: push eax
0x453c31: lea edx, dword [ebp+var_50]
0x453c34: xor ecx, ecx
0x453c36: call sub_781670
0x453c3b: lea ecx, dword [ebp+var_44]
0x453c3e: push ecx
0x453c3f: lea edx, dword [ebp+var_44]
0x453c42: xor ecx, ecx
0x453c44: call sub_781670
0x453c49: lea edx, dword [ebp+var_38]
0x453c4c: push edx
0x453c4d: lea edx, dword [ebp+var_38]
0x453c50: xor ecx, ecx
0x453c52: call sub_781670
0x453c57: fld dword [ebp+var_50]
0x453c5a: fadd dword [ebp+var_A0]
0x453c60: fstp dword [ebp+var_50]
0x453c63: fld dword [ebp+var_4C]
0x453c66: fadd dword [ebp+var_9C]
0x453c6c: fstp dword [ebp+var_4C]
0x453c6f: fld dword [ebp+var_48]
0x453c72: fadd dword [ebp+var_98]
0x453c78: fstp dword [ebp+var_48]
0x453c7b: fld dword [ebp+var_44]
0x453c7e: fadd dword [ebp+var_A0]
0x453c84: fstp dword [ebp+var_44]
0x453c87: fld dword [ebp+var_40]
0x453c8a: fadd dword [ebp+var_9C]
0x453c90: fstp dword [ebp+var_40]
0x453c93: fld dword [ebp+var_3C]
0x453c96: fadd dword [ebp+var_98]
0x453c9c: fstp dword [ebp+var_3C]
0x453c9f: fld dword [ebp+var_38]
0x453ca2: fadd dword [ebp+var_A0]
0x453ca8: fstp dword [ebp+var_38]
0x453cab: fld dword [ebp+var_34]
0x453cae: fadd dword [ebp+var_9C]
0x453cb4: fstp dword [ebp+var_34]
0x453cb7: fld dword [ebp+var_30]
0x453cba: fadd dword [ebp+var_98]
0x453cc0: fstp dword [ebp+var_30]
0x453cc3: mov ecx, 0x1
0x453cc8: call sub_785140
0x453ccd: mov ecx, 0x389d650
0x453cd2: call sub_7850f0
0x453cd7: lea eax, dword [ebp+var_64]
0x453cda: push eax
0x453cdb: lea ecx, dword [ebp+var_38]
0x453cde: push ecx
0x453cdf: lea edx, dword [ebp+var_44]
0x453ce2: push edx
0x453ce3: lea eax, dword [ebp+var_50]
0x453ce6: push eax
0x453ce7: call sub_450810
0x453cec: add esp, 0x10
0x453cef: lea ecx, dword [ebp+var_64]
0x453cf2: push ecx
0x453cf3: call sub_4509d0
0x453cf8: add esp, 0x4
0x453cfb: lea edx, dword [ebp+var_A8]
0x453d01: push edx
0x453d02: lea eax, dword [ebp+var_A4]
0x453d08: push eax
0x453d09: mov ecx, dword [ebp+var_5C]
0x453d0c: push ecx
0x453d0d: mov edx, dword [ebp+var_60]
0x453d10: push edx
0x453d11: mov eax, dword [ebp+var_64]
0x453d14: push eax
0x453d15: call sub_4b8e80
0x453d1a: add esp, 0x14
0x453d1d: mov ecx, dword [ebp+var_A4]
0x453d23: neg ecx
0x453d25: test ecx, ecx
0x453d27: je loc_453d3b
0x453d29: mov edx, dword [ebp+var_A4]
0x453d2f: neg edx
0x453d31: movzx edx, dx
0x453d34: xor ecx, ecx
0x453d36: call sub_784ee0
0x453d3b: mov eax, dword [ebp+var_A8]
0x453d41: neg eax
0x453d43: test eax, eax
0x453d45: je loc_453d59
0x453d47: mov ecx, dword [ebp+var_A8]
0x453d4d: neg ecx
0x453d4f: movzx edx, cx
0x453d52: xor ecx, ecx
0x453d54: call sub_784fb0
0x453d59: lea edx, dword [ebp+var_44]
0x453d5c: push edx
0x453d5d: lea edx, dword [ebp+var_44]
0x453d60: xor ecx, ecx
0x453d62: call sub_781670
0x453d67: lea eax, dword [ebp+var_38]
0x453d6a: push eax
0x453d6b: lea edx, dword [ebp+var_38]
0x453d6e: xor ecx, ecx
0x453d70: call sub_781670
0x453d75: lea ecx, dword [ebp+var_70]
0x453d78: push ecx
0x453d79: lea edx, dword [ebp+var_70]
0x453d7c: xor ecx, ecx
0x453d7e: call sub_781670
0x453d83: lea edx, dword [ebp+var_2C]
0x453d86: push edx
0x453d87: lea edx, dword [ebp+var_2C]
0x453d8a: xor ecx, ecx
0x453d8c: call sub_781670
0x453d91: mov eax, dword [ebp+arg_0]
0x453d94: mov ecx, dword [eax]
0x453d96: mov dword [ebp+var_54], ecx
0x453d99: mov edx, dword [ebp+var_4]
0x453d9c: mov eax, dword [edx]
0x453d9e: mov ecx, dword [eax+0x3c]
0x453da1: and ecx, aLastwater003
0x453da7: mov dword [ebp+var_58], ecx
0x453daa: cmp dword [ebp+var_58], 0x0
0x453dae: je loc_453dcc
0x453db0: fld dword [ebp+var_6C]
0x453db3: fcomp dword [dword_value_1142947840+4]
0x453db9: fnstsw ax
0x453dbb: test ah, 0x5
0x453dbe: jp loc_453dcc
0x453dc0: fld dword [ebp+var_54]
0x453dc3: fmul dword [aImm32dll+12]
0x453dc9: fstp dword [ebp+var_54]
0x453dcc: mov dword [ebp+var_AC], 0x0
0x453dd6: mov edx, dword [ebp+var_2C]
0x453dd9: mov dword [ebp+var_C8], edx
0x453ddf: mov eax, dword [ebp+var_28]
0x453de2: mov dword [ebp+var_C4], eax
0x453de8: mov ecx, dword [ebp+var_24]
0x453deb: mov dword [ebp+var_C0], ecx
0x453df1: lea edx, dword [ebp+var_C8]
0x453df7: push edx
0x453df8: call sub_450920
0x453dfd: add esp, 0x4
0x453e00: fld dword [ebp+var_C8]
0x453e06: fmul dword [ebp+var_8]
0x453e09: fstp dword [ebp+var_C8]
0x453e0f: fld dword [ebp+var_C4]
0x453e15: fmul dword [ebp+var_8]
0x453e18: fstp dword [ebp+var_C4]
0x453e1e: fld dword [ebp+var_C0]
0x453e24: fmul dword [ebp+var_8]
0x453e27: fstp dword [ebp+var_C0]
0x453e2d: mov eax, dword [ebp+var_70]
0x453e30: mov dword [ebp+var_BC], eax
0x453e36: mov ecx, dword [ebp+var_6C]
0x453e39: mov dword [ebp+var_B8], ecx
0x453e3f: mov edx, dword [ebp+var_68]
0x453e42: mov dword [ebp+var_B4], edx
0x453e48: mov dword [ebp+var_B0], 0x0
0x453e52: jmp loc_453e99
0x453e54: fld dword [ebp+var_BC]
0x453e5a: fadd dword [ebp+var_C8]
0x453e60: fstp dword [ebp+var_BC]
0x453e66: fld dword [ebp+var_B8]
0x453e6c: fadd dword [ebp+var_C4]
0x453e72: fstp dword [ebp+var_B8]
0x453e78: fld dword [ebp+var_B4]
0x453e7e: fadd dword [ebp+var_C0]
0x453e84: fstp dword [ebp+var_B4]
0x453e8a: fld dword [ebp+var_B0]
0x453e90: fadd dword [ebp+var_8]
0x453e93: fstp dword [ebp+var_B0]
0x453e99: mov dword [ebp+var_CC], 0x1
0x453ea3: fld dword [ebp+var_B0]
0x453ea9: fcomp dword [ebp+var_10]
0x453eac: fnstsw ax
0x453eae: test ah, 0x1
0x453eb1: jne loc_453efc
0x453eb3: mov eax, dword [ebp+var_70]
0x453eb6: mov dword [ebp+var_BC], eax
0x453ebc: mov ecx, dword [ebp+var_6C]
0x453ebf: mov dword [ebp+var_B8], ecx
0x453ec5: mov edx, dword [ebp+var_68]
0x453ec8: mov dword [ebp+var_B4], edx
0x453ece: fld dword [ebp+var_70]
0x453ed1: fadd dword [ebp+var_2C]
0x453ed4: fstp dword [ebp+var_BC]
0x453eda: fld dword [ebp+var_6C]
0x453edd: fadd dword [ebp+var_28]
0x453ee0: fstp dword [ebp+var_B8]
0x453ee6: fld dword [ebp+var_68]
0x453ee9: fadd dword [ebp+var_24]
0x453eec: fstp dword [ebp+var_B4]
0x453ef2: mov dword [ebp+var_CC], 0x0
0x453efc: fld dword [ebp+var_B8]
0x453f02: sub esp, 0x8
0x453f05: fstp qword [esp+0x19c+var_19C]
0x453f08: call sub_6451bb
0x453f0d: add esp, 0x8
0x453f10: fst dword [ebp+var_18C]
0x453f16: mov eax, dword [ebp+arg_0]
0x453f19: fcomp dword [eax]
0x453f1b: fnstsw ax
0x453f1d: test ah, 0x41
0x453f20: jp loc_453ffb
0x453f26: fld dword [ebp+var_3C]
0x453f29: fsub dword [ebp+var_48]
0x453f2c: fld dword [ebp+var_BC]
0x453f32: fsub dword [ebp+var_50]
0x453f35: fmulp st1
0x453f37: fld dword [ebp+var_44]
0x453f3a: fsub dword [ebp+var_50]
0x453f3d: fld dword [ebp+var_B4]
0x453f43: fsub dword [ebp+var_48]
0x453f46: fmulp st1
0x453f48: fsubp st1
0x453f4a: fstp dword [ebp+var_D0]
0x453f50: fld dword [ebp+var_30]
0x453f53: fsub dword [ebp+var_3C]
0x453f56: fld dword [ebp+var_BC]
0x453f5c: fsub dword [ebp+var_44]
0x453f5f: fmulp st1
0x453f61: fld dword [ebp+var_38]
0x453f64: fsub dword [ebp+var_44]
0x453f67: fld dword [ebp+var_B4]
0x453f6d: fsub dword [ebp+var_3C]
0x453f70: fmulp st1
0x453f72: fsubp st1
0x453f74: fstp dword [ebp+var_D8]
0x453f7a: fld dword [ebp+var_48]
0x453f7d: fsub dword [ebp+var_30]
0x453f80: fld dword [ebp+var_BC]
0x453f86: fsub dword [ebp+var_38]
0x453f89: fmulp st1
0x453f8b: fld dword [ebp+var_50]
0x453f8e: fsub dword [ebp+var_38]
0x453f91: fld dword [ebp+var_B4]
0x453f97: fsub dword [ebp+var_30]
0x453f9a: fmulp st1
0x453f9c: fsubp st1
0x453f9e: fstp dword [ebp+var_D4]
0x453fa4: fld dword [ebp+var_D0]
0x453faa: fmul dword [ebp+var_D8]
0x453fb0: fcomp dword [dword_value_1142947840+4]
0x453fb6: fnstsw ax
0x453fb8: test ah, 0x1
0x453fbb: jne loc_453ffb
0x453fbd: fld dword [ebp+var_D8]
0x453fc3: fmul dword [ebp+var_D4]
0x453fc9: fcomp dword [dword_value_1142947840+4]
0x453fcf: fnstsw ax
0x453fd1: test ah, 0x1
0x453fd4: jne loc_453ffb
0x453fd6: fld dword [ebp+var_D4]
0x453fdc: fmul dword [ebp+var_D0]
0x453fe2: fcomp dword [dword_value_1142947840+4]
0x453fe8: fnstsw ax
0x453fea: test ah, 0x1
0x453fed: jne loc_453ffb
0x453fef: mov dword [ebp+var_AC], 0x1
0x453ff9: jmp loc_45400b
0x453ffb: cmp dword [ebp+var_CC], 0x0
0x454002: jne loc_454006
0x454004: jmp loc_45400b
0x454006: jmp loc_453e54
0x45400b: cmp dword [ebp+var_AC], 0x0
0x454012: je loc_45433c
0x454018: mov ecx, dword [ebp+var_6C]
0x45401b: mov dword [ebp+var_F0], ecx
0x454021: fld dword [ebp+var_6C]
0x454024: fadd dword [ebp+var_28]
0x454027: fstp dword [ebp+var_EC]
0x45402d: mov edx, dword [ebp+var_64]
0x454030: mov dword [ebp+var_138], edx
0x454036: mov eax, dword [ebp+var_60]
0x454039: mov dword [ebp+var_134], eax
0x45403f: mov ecx, dword [ebp+var_5C]
0x454042: mov dword [ebp+var_130], ecx
0x454048: cmp dword [ebp+var_58], 0x0
0x45404c: je loc_45409e
0x45404e: fld dword [ebp+var_6C]
0x454051: fcomp dword [dword_value_1142947840+4]
0x454057: fnstsw ax
0x454059: test ah, 0x1
0x45405c: jne loc_45407b
0x45405e: mov edx, dword [ebp+var_64]
0x454061: mov dword [ebp+var_12C], edx
0x454067: mov eax, dword [ebp+var_60]
0x45406a: mov dword [ebp+var_128], eax
0x454070: mov ecx, dword [ebp+var_5C]
0x454073: mov dword [ebp+var_124], ecx
0x454079: jmp loc_45409c
0x45407b: fld dword [ebp+var_64]
0x45407e: fchs
0x454080: fstp dword [ebp+var_12C]
0x454086: fld dword [ebp+var_60]
0x454089: fchs
0x45408b: fstp dword [ebp+var_128]
0x454091: fld dword [ebp+var_5C]
0x454094: fchs
0x454096: fstp dword [ebp+var_124]
0x45409c: jmp loc_4540b9
0x45409e: mov edx, dword [ebp+var_64]
0x4540a1: mov dword [ebp+var_12C], edx
0x4540a7: mov eax, dword [ebp+var_60]
0x4540aa: mov dword [ebp+var_128], eax
0x4540b0: mov ecx, dword [ebp+var_5C]
0x4540b3: mov dword [ebp+var_124], ecx
0x4540b9: fld dword [ebp+var_128]
0x4540bf: sub esp, 0x8
0x4540c2: fstp qword [esp+0x19c+var_19C]
0x4540c5: fld dword [ebp+var_12C]
0x4540cb: sub esp, 0x8
0x4540ce: fstp qword [esp+0x1a4+var_1A4]
0x4540d1: call sub_645270
0x4540d6: add esp, 0x10
0x4540d9: fmul qword [qword_value_4679240012837945344]
0x4540df: fdiv qword [qword_value_4618760254707662970]
0x4540e5: call sub_644340
0x4540ea: neg eax
0x4540ec: mov dword [ebp+var_13C], eax
0x4540f2: fld dword [ebp+var_124]
0x4540f8: sub esp, 0x8
0x4540fb: fstp qword [esp+0x19c+var_19C]
0x4540fe: call sub_645034
0x454103: add esp, 0x8
0x454106: fmul qword [qword_value_4679240012837945344]
0x45410c: fdiv qword [qword_value_4618760254707662970]
0x454112: call sub_644340
0x454117: mov dword [ebp+var_140], eax
0x45411d: xor ecx, ecx
0x45411f: call sub_787630
0x454124: mov edx, dword [ebp+var_70]
0x454127: mov dword [ebp+var_E4], edx
0x45412d: mov dword [ebp+var_E0], 0x0
0x454137: mov eax, dword [ebp+var_68]
0x45413a: mov dword [ebp+var_DC], eax
0x454140: lea ecx, dword [ebp+var_14C]
0x454146: push ecx
0x454147: lea edx, dword [ebp+var_E4]
0x45414d: xor ecx, ecx
0x45414f: call sub_781670
0x454154: mov edx, dword [ebp+var_4]
0x454157: mov eax, dword [edx]
0x454159: fld dword [ebp+var_14C]
0x45415f: fadd dword [eax+0x18]
0x454162: fstp dword [ebp+var_120]
0x454168: mov ecx, dword [ebp+var_4]
0x45416b: mov edx, dword [ecx]
0x45416d: fld dword [ebp+var_148]
0x454173: fadd dword [edx+0x1c]
0x454176: fstp dword [ebp+var_11C]
0x45417c: mov eax, dword [ebp+var_4]
0x45417f: mov ecx, dword [eax]
0x454181: fld dword [ebp+var_144]
0x454187: fadd dword [ecx+0x20]
0x45418a: fstp dword [ebp+var_118]
0x454190: mov edx, dword [ebp+var_54]
0x454193: mov dword [ebp+var_E0], edx
0x454199: lea eax, dword [ebp+var_14C]
0x45419f: push eax
0x4541a0: lea edx, dword [ebp+var_E4]
0x4541a6: xor ecx, ecx
0x4541a8: call sub_781670
0x4541ad: mov ecx, dword [ebp+var_4]
0x4541b0: mov edx, dword [ecx]
0x4541b2: fld dword [ebp+var_14C]
0x4541b8: fadd dword [edx+0x18]
0x4541bb: fstp dword [ebp+var_114]
0x4541c1: mov eax, dword [ebp+var_4]
0x4541c4: mov ecx, dword [eax]
0x4541c6: fld dword [ebp+var_148]
0x4541cc: fadd dword [ecx+0x1c]
0x4541cf: fstp dword [ebp+var_110]
0x4541d5: mov edx, dword [ebp+var_4]
0x4541d8: mov eax, dword [edx]
0x4541da: fld dword [ebp+var_144]
0x4541e0: fadd dword [eax+0x20]
0x4541e3: fstp dword [ebp+var_10C]
0x4541e9: mov ecx, dword [ebp+var_2C]
0x4541ec: mov dword [ebp+var_158], ecx
0x4541f2: mov edx, dword [ebp+var_28]
0x4541f5: mov dword [ebp+var_154], edx
0x4541fb: mov eax, dword [ebp+var_24]
0x4541fe: mov dword [ebp+var_150], eax
0x454204: cmp dword [ebp+var_58], 0x0
0x454208: je loc_45422a
0x45420a: fld dword [ebp+var_6C]
0x45420d: fmul dword [ebp+var_28]
0x454210: fcomp dword [dword_value_1142947840+4]
0x454216: fnstsw ax
0x454218: test ah, 0x5
0x45421b: jp loc_454228
0x45421d: fld dword [ebp+var_28]
0x454220: fchs
0x454222: fstp dword [ebp+var_154]
0x454228: jmp loc_454248
0x45422a: fld dword [ebp+var_54]
0x45422d: fmul dword [ebp+var_28]
0x454230: fcomp dword [dword_value_1142947840+4]
0x454236: fnstsw ax
0x454238: test ah, 0x5
0x45423b: jp loc_454248
0x45423d: fld dword [ebp+var_28]
0x454240: fchs
0x454242: fstp dword [ebp+var_154]
0x454248: mov ecx, dword [ebp+var_2C]
0x45424b: mov dword [ebp+var_158], ecx
0x454251: mov dword [ebp+var_154], 0x0
0x45425b: mov edx, dword [ebp+var_24]
0x45425e: mov dword [ebp+var_150], edx
0x454264: lea eax, dword [ebp+var_14C]
0x45426a: push eax
0x45426b: lea edx, dword [ebp+var_158]
0x454271: xor ecx, ecx
0x454273: call sub_781670
0x454278: mov ecx, dword [ebp+var_14C]
0x45427e: mov dword [ebp+var_108], ecx
0x454284: mov edx, dword [ebp+var_148]
0x45428a: mov dword [ebp+var_104], edx
0x454290: mov eax, dword [ebp+var_144]
0x454296: mov dword [ebp+var_100], eax
0x45429c: mov dword [ebp+var_158], 0x0
0x4542a6: mov ecx, dword [ebp+var_28]
0x4542a9: mov dword [ebp+var_154], ecx
0x4542af: mov dword [ebp+var_150], 0x0
0x4542b9: lea edx, dword [ebp+var_14C]
0x4542bf: push edx
0x4542c0: lea edx, dword [ebp+var_158]
0x4542c6: xor ecx, ecx
0x4542c8: call sub_781670
0x4542cd: mov eax, dword [ebp+var_14C]
0x4542d3: mov dword [ebp+var_FC], eax
0x4542d9: mov ecx, dword [ebp+var_148]
0x4542df: mov dword [ebp+var_F8], ecx
0x4542e5: mov edx, dword [ebp+var_144]
0x4542eb: mov dword [ebp+var_F4], edx
0x4542f1: mov eax, dword [ebp+var_4]
0x4542f4: mov esi, dword [eax]
0x4542f6: mov edi, dword [ebp+var_14]
0x4542f9: add edi, 0x4
0x4542fc: mov ecx, 0x10
0x454301: rep movsd dword [edi], dword [esi]
0x454303: mov edi, dword [ebp+var_14]
0x454306: add edi, 0x44
0x454309: mov ecx, 0x16
0x45430e: lea esi, dword [ebp+var_140]
0x454314: rep movsd dword [edi], dword [esi]
0x454316: mov ecx, dword [ebp+var_14]
0x454319: add ecx, 0x9c
0x45431f: mov dword [ebp+var_14], ecx
0x454322: mov edx, dword [dword_3b42ff4]
0x454328: add edx, 0x1
0x45432b: mov dword [dword_3b42ff4], edx
0x454331: cmp dword [dword_3b42ff4], 0x10
0x454338: jne loc_45433c
0x45433a: jmp loc_45434b
0x45433c: mov ecx, 0x1
0x454341: call sub_785140
0x454346: jmp loc_4538cd
0x45434b: cmp dword [dword_3b42ff4], 0x1
0x454352: jge loc_45435b
0x454354: xor eax, eax
0x454356: jmp loc_4546d3
0x45435b: mov dword [ebp+var_15C], 0x0
0x454365: mov eax, dword [dword_3b47628]
0x45436a: mov dword [ebp+var_160], eax
0x454370: mov dword [ebp+var_164], 0x1
0x45437a: mov dword [ebp+var_14], 0x3b4762c
0x454381: jmp loc_45439e
0x454383: mov ecx, dword [ebp+var_164]
0x454389: add ecx, 0x1
0x45438c: mov dword [ebp+var_164], ecx
0x454392: mov edx, dword [ebp+var_14]
0x454395: add edx, 0x9c
0x45439b: mov dword [ebp+var_14], edx
0x45439e: mov eax, dword [ebp+var_164]
0x4543a4: cmp eax, dword [dword_3b42ff4]
0x4543aa: jge loc_4543df
0x4543ac: mov ecx, dword [ebp+var_14]
0x4543af: fld dword [ecx+0x98]
0x4543b5: fcomp dword [ebp+var_160]
0x4543bb: fnstsw ax
0x4543bd: test ah, 0x5
0x4543c0: jp loc_4543dd
0x4543c2: mov edx, dword [ebp+var_14]
0x4543c5: mov eax, dword [edx+0x98]
0x4543cb: mov dword [ebp+var_160], eax
0x4543d1: mov ecx, dword [ebp+var_164]
0x4543d7: mov dword [ebp+var_15C], ecx
0x4543dd: jmp loc_454383
0x4543df: mov edx, dword [dword_3b42ff4]
0x4543e5: mov dword [ebp+var_168], edx
0x4543eb: mov dword [ebp+var_14], 0x3b47590
0x4543f2: mov dword [ebp+var_16C], 0x3b47008
0x4543fc: jmp loc_454428
0x4543fe: mov eax, dword [ebp+var_168]
0x454404: sub eax, 0x1
0x454407: mov dword [ebp+var_168], eax
0x45440d: mov ecx, dword [ebp+var_14]
0x454410: add ecx, 0x9c
0x454416: mov dword [ebp+var_14], ecx
0x454419: mov edx, dword [ebp+var_16C]
0x45441f: add edx, 0x58
0x454422: mov dword [ebp+var_16C], edx
0x454428: cmp dword [ebp+var_168], 0x0
0x45442f: jle loc_454446
0x454431: mov esi, dword [ebp+var_14]
0x454434: add esi, 0x44
0x454437: mov ecx, 0x16
0x45443c: mov edi, dword [ebp+var_16C]
0x454442: rep movsd dword [edi], dword [esi]
0x454444: jmp loc_4543fe
0x454446: mov eax, dword [ebp+arg_0]
0x454449: mov cx, word [dword_3b42ff4]
0x454450: mov word [eax+0x28], cx
0x454454: mov edx, dword [ebp+arg_0]
0x454457: mov ax, word [ebp+var_15C]
0x45445e: mov word [edx+0x2a], ax
0x454462: mov ecx, dword [ebp+var_15C]
0x454468: imul ecx, ecx, 0x9c
0x45446e: add ecx, 0x3b475d4
0x454474: mov dword [ebp+var_170], ecx
0x45447a: mov edx, dword [ebp+arg_0]
0x45447d: mov eax, dword [ebp+var_170]
0x454483: mov ecx, dword [eax]
0x454485: mov dword [edx+0x34], ecx
0x454488: mov edx, dword [ebp+arg_0]
0x45448b: mov eax, dword [ebp+var_170]
0x454491: mov ecx, dword [eax+4]
0x454494: mov dword [edx+0x38], ecx
0x454497: mov edx, dword [ebp+var_170]
0x45449d: add edx, 0x8
0x4544a0: mov eax, dword [ebp+arg_0]
0x4544a3: add eax, 0x3c
0x4544a6: mov ecx, dword [edx]
0x4544a8: mov dword [eax], ecx
0x4544aa: mov ecx, dword [edx+4]
0x4544ad: mov dword [eax+4], ecx
0x4544b0: mov edx, dword [edx+8]
0x4544b3: mov dword [eax+8], edx
0x4544b6: mov eax, dword [ebp+var_170]
0x4544bc: add eax, 0x14
0x4544bf: mov ecx, dword [ebp+arg_0]
0x4544c2: add ecx, 0x48
0x4544c5: mov edx, dword [eax]
0x4544c7: mov dword [ecx], edx
0x4544c9: mov edx, dword [eax+4]
0x4544cc: mov dword [ecx+4], edx
0x4544cf: mov eax, dword [eax+8]
0x4544d2: mov dword [ecx+8], eax
0x4544d5: mov ecx, dword [ebp+var_170]
0x4544db: add ecx, 0x38
0x4544de: mov edx, dword [ebp+arg_0]
0x4544e1: add edx, 0x6c
0x4544e4: mov eax, dword [ecx]
0x4544e6: mov dword [edx], eax
0x4544e8: mov eax, dword [ecx+4]
0x4544eb: mov dword [edx+4], eax
0x4544ee: mov ecx, dword [ecx+8]
0x4544f1: mov dword [edx+8], ecx
0x4544f4: mov edx, dword [ebp+var_170]
0x4544fa: add edx, 0x44
0x4544fd: mov eax, dword [ebp+arg_0]
0x454500: add eax, 0x78
0x454503: mov ecx, dword [edx]
0x454505: mov dword [eax], ecx
0x454507: mov ecx, dword [edx+4]
0x45450a: mov dword [eax+4], ecx
0x45450d: mov edx, dword [edx+8]
0x454510: mov dword [eax+8], edx
0x454513: cmp dword [dword_3b42ff4], 0x1
0x45451a: jne loc_45455f
0x45451c: mov eax, dword [ebp+var_170]
0x454522: add eax, 0x20
0x454525: mov ecx, dword [ebp+arg_0]
0x454528: add ecx, 0x54
0x45452b: mov edx, dword [eax]
0x45452d: mov dword [ecx], edx
0x45452f: mov edx, dword [eax+4]
0x454532: mov dword [ecx+4], edx
0x454535: mov eax, dword [eax+8]
0x454538: mov dword [ecx+8], eax
0x45453b: mov ecx, dword [ebp+var_170]
0x454541: add ecx, 0x2c
0x454544: mov edx, dword [ebp+arg_0]
0x454547: add edx, 0x60
0x45454a: mov eax, dword [ecx]
0x45454c: mov dword [edx], eax
0x45454e: mov eax, dword [ecx+4]
0x454551: mov dword [edx+4], eax
0x454554: mov ecx, dword [ecx+8]
0x454557: mov dword [edx+8], ecx
0x45455a: jmp loc_4546ce
0x45455f: mov dword [ebp+var_14], 0x3b47590
0x454566: mov edx, dword [ebp+var_14]
0x454569: add edx, 0x64
0x45456c: mov eax, dword [ebp+arg_0]
0x45456f: add eax, 0x54
0x454572: mov ecx, dword [edx]
0x454574: mov dword [eax], ecx
0x454576: mov ecx, dword [edx+4]
0x454579: mov dword [eax+4], ecx
0x45457c: mov edx, dword [edx+8]
0x45457f: mov dword [eax+8], edx
0x454582: mov eax, dword [ebp+var_14]
0x454585: add eax, 0x70
0x454588: mov ecx, dword [ebp+arg_0]
0x45458b: add ecx, 0x60
0x45458e: mov edx, dword [eax]
0x454590: mov dword [ecx], edx
0x454592: mov edx, dword [eax+4]
0x454595: mov dword [ecx+4], edx
0x454598: mov eax, dword [eax+8]
0x45459b: mov dword [ecx+8], eax
0x45459e: mov ecx, dword [ebp+var_14]
0x4545a1: add ecx, 0x9c
0x4545a7: mov dword [ebp+var_14], ecx
0x4545aa: mov edx, dword [dword_3b42ff4]
0x4545b0: sub edx, 0x1
0x4545b3: mov dword [ebp+var_174], edx
0x4545b9: jmp loc_4545d6
0x4545bb: mov eax, dword [ebp+var_174]
0x4545c1: sub eax, 0x1
0x4545c4: mov dword [ebp+var_174], eax
0x4545ca: mov ecx, dword [ebp+var_14]
0x4545cd: add ecx, 0x9c
0x4545d3: mov dword [ebp+var_14], ecx
0x4545d6: cmp dword [ebp+var_174], 0x0
0x4545dd: jle loc_454650
0x4545df: mov edx, dword [ebp+arg_0]
0x4545e2: mov eax, dword [ebp+var_14]
0x4545e5: fld dword [edx+0x54]
0x4545e8: fadd dword [eax+0x64]
0x4545eb: mov ecx, dword [ebp+arg_0]
0x4545ee: fstp dword [ecx+0x54]
0x4545f1: mov edx, dword [ebp+arg_0]
0x4545f4: mov eax, dword [ebp+var_14]
0x4545f7: fld dword [edx+0x58]
0x4545fa: fadd dword [eax+0x68]
0x4545fd: mov ecx, dword [ebp+arg_0]
0x454600: fstp dword [ecx+0x58]
0x454603: mov edx, dword [ebp+arg_0]
0x454606: mov eax, dword [ebp+var_14]
0x454609: fld dword [edx+0x5c]
0x45460c: fadd dword [eax+0x6c]
0x45460f: mov ecx, dword [ebp+arg_0]
0x454612: fstp dword [ecx+0x5c]
0x454615: mov edx, dword [ebp+arg_0]
0x454618: mov eax, dword [ebp+var_14]
0x45461b: fld dword [edx+0x60]
0x45461e: fadd dword [eax+0x70]
0x454621: mov ecx, dword [ebp+arg_0]
0x454624: fstp dword [ecx+0x60]
0x454627: mov edx, dword [ebp+arg_0]
0x45462a: mov eax, dword [ebp+var_14]
0x45462d: fld dword [edx+0x64]
0x454630: fadd dword [eax+0x74]
0x454633: mov ecx, dword [ebp+arg_0]
0x454636: fstp dword [ecx+0x64]
0x454639: mov edx, dword [ebp+arg_0]
0x45463c: mov eax, dword [ebp+var_14]
0x45463f: fld dword [edx+0x68]
0x454642: fadd dword [eax+0x78]
0x454645: mov ecx, dword [ebp+arg_0]
0x454648: fstp dword [ecx+0x68]
0x45464b: jmp loc_4545bb
0x454650: fild dword [dword_3b42ff4]
0x454656: fdivr dword [aImm32dll+16]
0x45465c: fstp dword [ebp+var_178]
0x454662: mov edx, dword [ebp+arg_0]
0x454665: fld dword [ebp+var_178]
0x45466b: fmul dword [edx+0x54]
0x45466e: mov eax, dword [ebp+arg_0]
0x454671: fstp dword [eax+0x54]
0x454674: mov ecx, dword [ebp+arg_0]
0x454677: fld dword [ebp+var_178]
0x45467d: fmul dword [ecx+0x58]
0x454680: mov edx, dword [ebp+arg_0]
0x454683: fstp dword [edx+0x58]
0x454686: mov eax, dword [ebp+arg_0]
0x454689: fld dword [ebp+var_178]
0x45468f: fmul dword [eax+0x5c]
0x454692: mov ecx, dword [ebp+arg_0]
0x454695: fstp dword [ecx+0x5c]
0x454698: mov edx, dword [ebp+arg_0]
0x45469b: fld dword [ebp+var_178]
0x4546a1: fmul dword [edx+0x60]
0x4546a4: mov eax, dword [ebp+arg_0]
0x4546a7: fstp dword [eax+0x60]
0x4546aa: mov ecx, dword [ebp+arg_0]
0x4546ad: fld dword [ebp+var_178]
0x4546b3: fmul dword [ecx+0x64]
0x4546b6: mov edx, dword [ebp+arg_0]
0x4546b9: fstp dword [edx+0x64]
0x4546bc: mov eax, dword [ebp+arg_0]
0x4546bf: fld dword [ebp+var_178]
0x4546c5: fmul dword [eax+0x68]
0x4546c8: mov ecx, dword [ebp+arg_0]
0x4546cb: fstp dword [ecx+0x68]
0x4546ce: mov eax, 0x1
0x4546d3: pop edi
0x4546d4: pop esi
0x4546d5: mov esp, ebp
0x4546d7: pop ebp
0x4546d8: ret
