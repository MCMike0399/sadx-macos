0x450c10: push ebp
0x450c11: mov ebp, esp
0x450c13: sub esp, 0x68
0x450c16: mov dword [dword_3b42ff0], 0x0
0x450c20: mov dword [ebp+var_8], 0x3b47f50
0x450c27: mov ecx, dword [ebp+arg_0]
0x450c2a: add ecx, 0x10
0x450c2d: call sub_787cc0
0x450c32: mov eax, dword [ebp+arg_0]
0x450c35: fadd dword [eax]
0x450c37: fst dword [ebp+var_4]
0x450c3a: fmul dword [ebp+var_4]
0x450c3d: fstp dword [ebp+var_C]
0x450c40: mov ecx, dword [ebp+arg_0]
0x450c43: mov edx, dword [ecx]
0x450c45: push edx
0x450c46: mov eax, dword [ebp+arg_0]
0x450c49: mov ecx, dword [eax+0xc]
0x450c4c: push ecx
0x450c4d: mov edx, dword [ebp+arg_0]
0x450c50: mov eax, dword [edx+8]
0x450c53: push eax
0x450c54: mov ecx, dword [ebp+arg_0]
0x450c57: mov edx, dword [ecx+4]
0x450c5a: push edx
0x450c5b: call sub_43acd0
0x450c60: add esp, 0x10
0x450c63: mov eax, dword [dword_3c48550]
0x450c68: mov dword [ebp+var_10], eax
0x450c6b: jmp loc_450c75
0x450c6d: mov ecx, dword [ebp+var_10]
0x450c70: mov edx, dword [ecx]
0x450c72: mov dword [ebp+var_10], edx
0x450c75: cmp dword [ebp+var_10], 0x0
0x450c79: je loc_45101d
0x450c7f: mov eax, dword [ebp+var_10]
0x450c82: mov dword [eax+0x1c], 0x0
0x450c89: mov ecx, dword [ebp+var_10]
0x450c8c: mov dword [ecx+0x24], 0x0
0x450c93: mov dword [ebp+var_1C], 0x0
0x450c9a: movsx edx, word [word_3b32724]
0x450ca1: mov dword [ebp+var_14], edx
0x450ca4: mov dword [ebp+var_18], 0x3b2f720
0x450cab: jmp loc_450cbf
0x450cad: mov eax, dword [ebp+var_14]
0x450cb0: sub eax, 0x1
0x450cb3: mov dword [ebp+var_14], eax
0x450cb6: mov ecx, dword [ebp+var_18]
0x450cb9: add ecx, 0xc
0x450cbc: mov dword [ebp+var_18], ecx
0x450cbf: cmp dword [ebp+var_14], 0x0
0x450cc3: jle loc_450cde
0x450cc5: mov edx, dword [ebp+var_18]
0x450cc8: mov eax, dword [ebp+var_10]
0x450ccb: mov ecx, dword [edx+4]
0x450cce: cmp ecx, dword [eax+8]
0x450cd1: jne loc_450cdc
0x450cd3: mov dword [ebp+var_1C], 0x1
0x450cda: jmp loc_450cde
0x450cdc: jmp loc_450cad
0x450cde: cmp dword [ebp+var_1C], 0x0
0x450ce2: jne loc_450ce6
0x450ce4: jmp loc_450c6d
0x450ce6: mov edx, dword [ebp+var_10]
0x450ce9: mov eax, dword [edx+0x10]
0x450cec: mov dword [ebp+var_24], eax
0x450cef: mov ecx, dword [ebp+var_10]
0x450cf2: mov edx, dword [ecx+0x14]
0x450cf5: mov dword [ebp+var_2C], edx
0x450cf8: mov eax, dword [ebp+arg_0]
0x450cfb: mov ecx, dword [eax+0xc]
0x450cfe: mov dword [ebp+var_20], ecx
0x450d01: mov edx, dword [ebp+var_10]
0x450d04: fld dword [ebp+var_20]
0x450d07: fsub dword [edx+0x28]
0x450d0a: fsub dword [ebp+var_4]
0x450d0d: fstp dword [ebp+var_30]
0x450d10: mov eax, dword [ebp+var_10]
0x450d13: fld dword [ebp+var_20]
0x450d16: fadd dword [eax+0x28]
0x450d19: fadd dword [ebp+var_4]
0x450d1c: fstp dword [ebp+var_28]
0x450d1f: cmp dword [ebp+var_24], 0x0
0x450d23: je loc_450d6d
0x450d25: cmp dword [ebp+var_2C], 0x0
0x450d29: je loc_450d6d
0x450d2b: mov ecx, dword [ebp+var_24]
0x450d2e: fld dword [ecx+0x10]
0x450d31: fcomp dword [ebp+var_30]
0x450d34: fnstsw ax
0x450d36: test ah, 0x5
0x450d39: jp loc_450d6d
0x450d3b: mov edx, dword [ebp+var_2C]
0x450d3e: fld dword [ebp+var_28]
0x450d41: fcomp dword [edx+0x10]
0x450d44: fnstsw ax
0x450d46: test ah, 0x5
0x450d49: jp loc_450d6d
0x450d4b: mov eax, dword [ebp+var_24]
0x450d4e: fld dword [eax+0x10]
0x450d51: fcomp dword [ebp+var_30]
0x450d54: fnstsw ax
0x450d56: test ah, 0x41
0x450d59: jp loc_450d6d
0x450d5b: mov ecx, dword [ebp+var_24]
0x450d5e: mov edx, dword [ecx]
0x450d60: mov dword [ebp+var_24], edx
0x450d63: cmp dword [ebp+var_24], 0x0
0x450d67: jne loc_450d6b
0x450d69: jmp loc_450d6d
0x450d6b: jmp loc_450d4b
0x450d6d: cmp dword [ebp+var_24], 0x0
0x450d71: je loc_450ed2
0x450d77: mov eax, dword [ebp+var_24]
0x450d7a: fld dword [eax+0x10]
0x450d7d: fcomp dword [ebp+var_30]
0x450d80: fnstsw ax
0x450d82: test ah, 0x41
0x450d85: jne loc_450ed2
0x450d8b: jmp loc_450d95
0x450d8d: mov ecx, dword [ebp+var_24]
0x450d90: mov edx, dword [ecx]
0x450d92: mov dword [ebp+var_24], edx
0x450d95: cmp dword [ebp+var_24], 0x0
0x450d99: je loc_450ecd
0x450d9f: mov eax, dword [ebp+var_24]
0x450da2: fld dword [eax+0x10]
0x450da5: fcomp dword [ebp+var_28]
0x450da8: fnstsw ax
0x450daa: test ah, 0x5
0x450dad: jp loc_450ecd
0x450db3: mov ecx, dword [ebp+var_24]
0x450db6: fld dword [ebp+var_C]
0x450db9: fadd dword [ecx+0x14]
0x450dbc: fmul dword [dword_value_1067869798]
0x450dc2: fstp dword [ebp+var_40]
0x450dc5: mov edx, dword [ebp+var_24]
0x450dc8: add edx, 0x8
0x450dcb: mov dword [ebp+var_38], edx
0x450dce: mov eax, dword [ebp+arg_0]
0x450dd1: add eax, 0x4
0x450dd4: mov dword [ebp+var_4C], eax
0x450dd7: mov ecx, dword [ebp+var_38]
0x450dda: mov edx, dword [ebp+var_4C]
0x450ddd: fld dword [ecx]
0x450ddf: fsub dword [edx]
0x450de1: fstp dword [ebp+var_34]
0x450de4: mov eax, dword [ebp+var_4C]
0x450de7: add eax, 0x4
0x450dea: mov dword [ebp+var_4C], eax
0x450ded: mov ecx, dword [ebp+var_38]
0x450df0: add ecx, 0x4
0x450df3: mov dword [ebp+var_38], ecx
0x450df6: fld dword [ebp+var_34]
0x450df9: fmul dword [ebp+var_34]
0x450dfc: fstp dword [ebp+var_3C]
0x450dff: fld dword [ebp+var_3C]
0x450e02: fcomp dword [ebp+var_40]
0x450e05: fnstsw ax
0x450e07: test ah, 0x41
0x450e0a: jne loc_450e11
0x450e0c: jmp loc_450d8d
0x450e11: mov edx, dword [ebp+var_38]
0x450e14: mov eax, dword [ebp+var_4C]
0x450e17: fld dword [edx]
0x450e19: fsub dword [eax]
0x450e1b: fstp dword [ebp+var_44]
0x450e1e: mov ecx, dword [ebp+var_4C]
0x450e21: add ecx, 0x4
0x450e24: mov dword [ebp+var_4C], ecx
0x450e27: mov edx, dword [ebp+var_38]
0x450e2a: add edx, 0x4
0x450e2d: mov dword [ebp+var_38], edx
0x450e30: fld dword [ebp+var_44]
0x450e33: fmul dword [ebp+var_44]
0x450e36: fadd dword [ebp+var_3C]
0x450e39: fstp dword [ebp+var_3C]
0x450e3c: fld dword [ebp+var_3C]
0x450e3f: fcomp dword [ebp+var_40]
0x450e42: fnstsw ax
0x450e44: test ah, 0x41
0x450e47: jne loc_450e4e
0x450e49: jmp loc_450d8d
0x450e4e: mov eax, dword [ebp+var_38]
0x450e51: mov ecx, dword [ebp+var_4C]
0x450e54: fld dword [eax]
0x450e56: fsub dword [ecx]
0x450e58: fst dword [ebp+var_48]
0x450e5b: fmul dword [ebp+var_48]
0x450e5e: fadd dword [ebp+var_3C]
0x450e61: fstp dword [ebp+var_3C]
0x450e64: fld dword [ebp+var_3C]
0x450e67: fcomp dword [ebp+var_40]
0x450e6a: fnstsw ax
0x450e6c: test ah, 0x41
0x450e6f: jne loc_450e76
0x450e71: jmp loc_450d8d
0x450e76: mov edx, dword [ebp+var_10]
0x450e79: cmp dword [edx+0x1c], 0x0
0x450e7d: jne loc_450e88
0x450e7f: mov eax, dword [ebp+var_10]
0x450e82: mov ecx, dword [ebp+var_8]
0x450e85: mov dword [eax+0x1c], ecx
0x450e88: mov edx, dword [ebp+var_10]
0x450e8b: mov eax, dword [edx+0x24]
0x450e8e: add eax, 0x1
0x450e91: mov ecx, dword [ebp+var_10]
0x450e94: mov dword [ecx+0x24], eax
0x450e97: mov edx, dword [ebp+var_8]
0x450e9a: mov eax, dword [ebp+var_24]
0x450e9d: mov dword [edx], eax
0x450e9f: mov ecx, dword [ebp+var_8]
0x450ea2: add ecx, 0x4
0x450ea5: mov dword [ebp+var_8], ecx
0x450ea8: mov edx, dword [dword_3b42ff0]
0x450eae: add edx, 0x1
0x450eb1: mov dword [dword_3b42ff0], edx
0x450eb7: cmp dword [dword_3b42ff0], 0x80
0x450ec1: jne loc_450ec8
0x450ec3: jmp loc_45101d
0x450ec8: jmp loc_450d8d
0x450ecd: jmp loc_451018
0x450ed2: mov eax, dword [ebp+var_2C]
0x450ed5: mov dword [ebp+var_24], eax
0x450ed8: jmp loc_450ee3
0x450eda: mov ecx, dword [ebp+var_24]
0x450edd: mov edx, dword [ecx+4]
0x450ee0: mov dword [ebp+var_24], edx
0x450ee3: cmp dword [ebp+var_24], 0x0
0x450ee7: je loc_451018
0x450eed: mov eax, dword [ebp+var_24]
0x450ef0: fld dword [eax+0x10]
0x450ef3: fcomp dword [ebp+var_30]
0x450ef6: fnstsw ax
0x450ef8: test ah, 0x41
0x450efb: jne loc_451018
0x450f01: mov ecx, dword [ebp+var_24]
0x450f04: fld dword [ebp+var_C]
0x450f07: fadd dword [ecx+0x14]
0x450f0a: fmul dword [dword_value_1067869798]
0x450f10: fstp dword [ebp+var_5C]
0x450f13: mov edx, dword [ebp+var_24]
0x450f16: add edx, 0x8
0x450f19: mov dword [ebp+var_54], edx
0x450f1c: mov eax, dword [ebp+arg_0]
0x450f1f: add eax, 0x4
0x450f22: mov dword [ebp+var_68], eax
0x450f25: mov ecx, dword [ebp+var_54]
0x450f28: mov edx, dword [ebp+var_68]
0x450f2b: fld dword [ecx]
0x450f2d: fsub dword [edx]
0x450f2f: fstp dword [ebp+var_50]
0x450f32: mov eax, dword [ebp+var_68]
0x450f35: add eax, 0x4
0x450f38: mov dword [ebp+var_68], eax
0x450f3b: mov ecx, dword [ebp+var_54]
0x450f3e: add ecx, 0x4
0x450f41: mov dword [ebp+var_54], ecx
0x450f44: fld dword [ebp+var_50]
0x450f47: fmul dword [ebp+var_50]
0x450f4a: fstp dword [ebp+var_58]
0x450f4d: fld dword [ebp+var_58]
0x450f50: fcomp dword [ebp+var_5C]
0x450f53: fnstsw ax
0x450f55: test ah, 0x41
0x450f58: jne loc_450f5f
0x450f5a: jmp loc_450eda
0x450f5f: mov edx, dword [ebp+var_54]
0x450f62: mov eax, dword [ebp+var_68]
0x450f65: fld dword [edx]
0x450f67: fsub dword [eax]
0x450f69: fstp dword [ebp+var_60]
0x450f6c: mov ecx, dword [ebp+var_68]
0x450f6f: add ecx, 0x4
0x450f72: mov dword [ebp+var_68], ecx
0x450f75: mov edx, dword [ebp+var_54]
0x450f78: add edx, 0x4
0x450f7b: mov dword [ebp+var_54], edx
0x450f7e: fld dword [ebp+var_60]
0x450f81: fmul dword [ebp+var_60]
0x450f84: fadd dword [ebp+var_58]
0x450f87: fstp dword [ebp+var_58]
0x450f8a: fld dword [ebp+var_58]
0x450f8d: fcomp dword [ebp+var_5C]
0x450f90: fnstsw ax
0x450f92: test ah, 0x41
0x450f95: jne loc_450f9c
0x450f97: jmp loc_450eda
0x450f9c: mov eax, dword [ebp+var_54]
0x450f9f: mov ecx, dword [ebp+var_68]
0x450fa2: fld dword [eax]
0x450fa4: fsub dword [ecx]
0x450fa6: fst dword [ebp+var_64]
0x450fa9: fmul dword [ebp+var_64]
0x450fac: fadd dword [ebp+var_58]
0x450faf: fstp dword [ebp+var_58]
0x450fb2: fld dword [ebp+var_58]
0x450fb5: fcomp dword [ebp+var_5C]
0x450fb8: fnstsw ax
0x450fba: test ah, 0x41
0x450fbd: jne loc_450fc4
0x450fbf: jmp loc_450eda
0x450fc4: mov edx, dword [ebp+var_10]
0x450fc7: cmp dword [edx+0x1c], 0x0
0x450fcb: jne loc_450fd6
0x450fcd: mov eax, dword [ebp+var_10]
0x450fd0: mov ecx, dword [ebp+var_8]
0x450fd3: mov dword [eax+0x1c], ecx
0x450fd6: mov edx, dword [ebp+var_10]
0x450fd9: mov eax, dword [edx+0x24]
0x450fdc: add eax, 0x1
0x450fdf: mov ecx, dword [ebp+var_10]
0x450fe2: mov dword [ecx+0x24], eax
0x450fe5: mov edx, dword [ebp+var_8]
0x450fe8: mov eax, dword [ebp+var_24]
0x450feb: mov dword [edx], eax
0x450fed: mov ecx, dword [ebp+var_8]
0x450ff0: add ecx, 0x4
0x450ff3: mov dword [ebp+var_8], ecx
0x450ff6: mov edx, dword [dword_3b42ff0]
0x450ffc: add edx, 0x1
0x450fff: mov dword [dword_3b42ff0], edx
0x451005: cmp dword [dword_3b42ff0], 0x80
0x45100f: jne loc_451013
0x451011: jmp loc_45101d
0x451013: jmp loc_450eda
0x451018: jmp loc_450c6d
0x45101d: mov eax, dword [dword_3b42ff0]
0x451022: mov esp, ebp
0x451024: pop ebp
0x451025: ret
