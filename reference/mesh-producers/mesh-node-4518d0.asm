0x4518d0: push ebp
0x4518d1: mov ebp, esp
0x4518d3: sub esp, 0x8
0x4518d6: mov dword [ebp+var_8], 0x3b48150
0x4518dd: mov dword [ebp+var_4], 0x0
0x4518e4: jmp loc_4518f8
0x4518e6: mov eax, dword [ebp+var_4]
0x4518e9: add eax, 0x1
0x4518ec: mov dword [ebp+var_4], eax
0x4518ef: mov ecx, dword [ebp+var_8]
0x4518f2: add ecx, 0x4
0x4518f5: mov dword [ebp+var_8], ecx
0x4518f8: cmp dword [ebp+var_4], 0x100
0x4518ff: jge loc_45190c
0x451901: mov edx, dword [ebp+var_8]
0x451904: mov dword [edx], 0x0
0x45190a: jmp loc_4518e6
0x45190c: mov esp, ebp
0x45190e: pop ebp
0x45190f: ret
