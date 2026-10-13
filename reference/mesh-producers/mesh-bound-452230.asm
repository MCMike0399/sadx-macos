0x452230: push ebp
0x452231: mov ebp, esp
0x452233: sub esp, 0x18
0x452236: mov dword [ebp+var_4], 0x0
0x45223d: mov eax, dword [ebp+arg_0]
0x452240: mov ecx, dword [eax]
0x452242: mov dword [ebp+var_18], ecx
0x452245: cmp dword [ebp+var_18], 0x8000
0x45224c: jg loc_45226f
0x45224e: cmp dword [ebp+var_18], 0x8000
0x452255: je loc_4522de
0x45225b: cmp dword [ebp+var_18], 0x0
0x45225f: je loc_45227d
0x452261: cmp dword [ebp+var_18], 0x4000
0x452268: je loc_45227d
0x45226a: jmp loc_45238d
0x45226f: cmp dword [ebp+var_18], 0xc000
0x452276: je loc_4522de
0x452278: jmp loc_45238d
0x45227d: mov edx, dword [ebp+arg_0]
0x452280: mov eax, dword [edx+4]
0x452283: mov dword [ebp+var_C], eax
0x452286: mov ecx, dword [ebp+arg_0]
0x452289: add ecx, 0x10
0x45228c: mov dword [ebp+var_8], ecx
0x45228f: jmp loc_4522b2
0x452291: mov edx, dword [ebp+var_C]
0x452294: sub edx, 0x1
0x452297: mov dword [ebp+var_C], edx
0x45229a: mov eax, dword [ebp+arg_0]
0x45229d: mov ecx, dword [eax+8]
0x4522a0: add ecx, 0x2
0x4522a3: mov edx, dword [ebp+arg_0]
0x4522a6: mov dword [edx+8], ecx
0x4522a9: mov eax, dword [ebp+var_8]
0x4522ac: add eax, 0xc
0x4522af: mov dword [ebp+var_8], eax
0x4522b2: cmp dword [ebp+var_C], 0x0
0x4522b6: jle loc_4522d9
0x4522b8: mov ecx, dword [ebp+var_8]
0x4522bb: push ecx
0x4522bc: mov edx, dword [ebp+arg_0]
0x4522bf: mov eax, dword [edx+0xc]
0x4522c2: mov ecx, dword [ebp+arg_0]
0x4522c5: mov edx, dword [ecx+8]
0x4522c8: movsx edx, word [edx]
0x4522cb: imul edx, edx, 0xc
0x4522ce: add edx, dword [eax]
0x4522d0: xor ecx, ecx
0x4522d2: call sub_781670
0x4522d7: jmp loc_452291
0x4522d9: jmp loc_45238d
0x4522de: mov eax, dword [ebp+arg_0]
0x4522e1: mov ecx, dword [eax+8]
0x4522e4: movsx edx, word [ecx]
0x4522e7: and edx, 0x7fff
0x4522ed: mov eax, dword [ebp+arg_0]
0x4522f0: mov dword [eax+4], edx
0x4522f3: mov ecx, dword [ebp+arg_0]
0x4522f6: mov edx, dword [ecx+8]
0x4522f9: movsx eax, word [edx]
0x4522fc: and eax, 0x8000
0x452301: test eax, eax
0x452303: je loc_45230c
0x452305: mov dword [ebp+var_4], 0x1
0x45230c: mov ecx, dword [ebp+arg_0]
0x45230f: cmp dword [ecx+4], 0x400
0x452316: jle loc_452322
0x452318: mov edx, dword [ebp+arg_0]
0x45231b: mov dword [edx+4], 0x400
0x452322: mov eax, dword [ebp+arg_0]
0x452325: mov ecx, dword [eax+8]
0x452328: add ecx, 0x2
0x45232b: mov edx, dword [ebp+arg_0]
0x45232e: mov dword [edx+8], ecx
0x452331: mov eax, dword [ebp+arg_0]
0x452334: mov ecx, dword [eax+4]
0x452337: mov dword [ebp+var_14], ecx
0x45233a: mov edx, dword [ebp+arg_0]
0x45233d: add edx, 0x10
0x452340: mov dword [ebp+var_10], edx
0x452343: jmp loc_452366
0x452345: mov eax, dword [ebp+var_14]
0x452348: sub eax, 0x1
0x45234b: mov dword [ebp+var_14], eax
0x45234e: mov ecx, dword [ebp+arg_0]
0x452351: mov edx, dword [ecx+8]
0x452354: add edx, 0x2
0x452357: mov eax, dword [ebp+arg_0]
0x45235a: mov dword [eax+8], edx
0x45235d: mov ecx, dword [ebp+var_10]
0x452360: add ecx, 0xc
0x452363: mov dword [ebp+var_10], ecx
0x452366: cmp dword [ebp+var_14], 0x0
0x45236a: jle loc_45238d
0x45236c: mov edx, dword [ebp+var_10]
0x45236f: push edx
0x452370: mov eax, dword [ebp+arg_0]
0x452373: mov ecx, dword [eax+0xc]
0x452376: mov edx, dword [ebp+arg_0]
0x452379: mov eax, dword [edx+8]
0x45237c: movsx edx, word [eax]
0x45237f: imul edx, edx, 0xc
0x452382: add edx, dword [ecx]
0x452384: xor ecx, ecx
0x452386: call sub_781670
0x45238b: jmp loc_452345
0x45238d: mov eax, dword [ebp+var_4]
0x452390: mov esp, ebp
0x452392: pop ebp
0x452393: ret
