0x43b700: mov eax, dword [dword_3b36d30]
0x43b705: test eax, eax
0x43b707: je loc_43b728
0x43b709: cmp byte [eax], 0x0
0x43b70c: jne loc_43b728
0x43b70e: xor eax, eax
0x43b710: mov ah, byte [word_3b22dcc]
0x43b716: or ax, word [byte_3b22dec]
0x43b71d: mov byte [byte_3b36d40], ah
0x43b723: mov byte [byte_3b36d41], al
0x43b728: movsx ecx, byte [byte_3b36d41]
0x43b72f: mov eax, 0x1
0x43b734: mov dword [dword_91545c], eax
0x43b739: mov dword [dword_915460], eax
0x43b73e: movsx eax, byte [byte_3b36d40]
0x43b745: lea edx, dword [ecx+eax*8]
0x43b748: mov edx, dword [dword_9759c8+32832+edx*4]
0x43b74f: test edx, edx
0x43b751: mov dword [dword_3b2f718], edx
0x43b757: jne loc_43b766
0x43b759: mov dword [dword_91545c], edx
0x43b75f: mov dword [dword_915460], edx
0x43b765: ret
0x43b766: push ecx
0x43b767: push eax
0x43b768: call sub_43a260
0x43b76d: add esp, 0x8
0x43b770: ret
