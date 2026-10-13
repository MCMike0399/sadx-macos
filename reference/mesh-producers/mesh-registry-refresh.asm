0x4511d0: push ebp
0x4511d1: mov ebp, esp
0x4511d3: sub esp, 0x860
0x4511d9: cmp dword [dword_3c4855c], 0x0
0x4511e0: jne loc_4511e9
0x4511e2: call sub_43aef0
0x4511e7: jmp loc_4511f8
0x4511e9: call sub_43b580
0x4511ee: mov dword [dword_3c4855c], 0x0
0x4511f8: mov dword [ebp+var_804], 0x0
0x451202: lea eax, dword [ebp+var_400]
0x451208: mov dword [ebp+var_814], eax
0x45120e: lea ecx, dword [ebp+var_800]
0x451214: mov dword [ebp+var_818], ecx
0x45121a: movsx edx, word [word_3b32724]
0x451221: mov dword [ebp+var_808], edx
0x451227: cmp dword [ebp+var_808], 0x100
0x451231: jle loc_45123d
0x451233: mov dword [ebp+var_808], 0x100
0x45123d: mov dword [ebp+var_810], 0x3b2f720
0x451247: jmp loc_451267
0x451249: mov eax, dword [ebp+var_808]
0x45124f: sub eax, 0x1
0x451252: mov dword [ebp+var_808], eax
0x451258: mov ecx, dword [ebp+var_810]
0x45125e: add ecx, 0xc
0x451261: mov dword [ebp+var_810], ecx
0x451267: cmp dword [ebp+var_808], 0x0
0x45126e: jle loc_451400
0x451274: mov edx, dword [ebp+var_810]
0x45127a: mov eax, dword [edx]
0x45127c: and eax, 0x8000000
0x451281: test eax, eax
0x451283: je loc_4512d8
0x451285: mov ecx, dword [ebp+var_814]
0x45128b: mov edx, dword [ebp+var_810]
0x451291: mov eax, dword [edx+4]
0x451294: mov dword [ecx], eax
0x451296: mov ecx, dword [ebp+var_814]
0x45129c: add ecx, 0x4
0x45129f: mov dword [ebp+var_814], ecx
0x4512a5: mov edx, dword [ebp+var_804]
0x4512ab: add edx, 0x1
0x4512ae: mov dword [ebp+var_804], edx
0x4512b4: mov eax, dword [ebp+var_818]
0x4512ba: mov ecx, dword [ebp+var_810]
0x4512c0: mov edx, dword [ecx]
0x4512c2: mov dword [eax], edx
0x4512c4: mov eax, dword [ebp+var_818]
0x4512ca: add eax, 0x4
0x4512cd: mov dword [ebp+var_818], eax
0x4512d3: jmp loc_451249
0x4512d8: mov ecx, dword [ebp+var_810]
0x4512de: mov edx, dword [dword_3b47000]
0x4512e4: cmp edx, dword [ecx+4]
0x4512e7: jne loc_451346
0x4512e9: mov dword [dword_3b47000], 0x0
0x4512f3: mov eax, dword [ebp+var_814]
0x4512f9: mov ecx, dword [ebp+var_810]
0x4512ff: mov edx, dword [ecx+4]
0x451302: mov dword [eax], edx
0x451304: mov eax, dword [ebp+var_814]
0x45130a: add eax, 0x4
0x45130d: mov dword [ebp+var_814], eax
0x451313: mov ecx, dword [ebp+var_804]
0x451319: add ecx, 0x1
0x45131c: mov dword [ebp+var_804], ecx
0x451322: mov edx, dword [ebp+var_818]
0x451328: mov eax, dword [ebp+var_810]
0x45132e: mov ecx, dword [eax]
0x451330: mov dword [edx], ecx
0x451332: mov edx, dword [ebp+var_818]
0x451338: add edx, 0x4
0x45133b: mov dword [ebp+var_818], edx
0x451341: jmp loc_451249
0x451346: mov dword [ebp+var_81C], 0x0
0x451350: mov eax, dword [dword_3c48550]
0x451355: mov dword [ebp+var_80C], eax
0x45135b: jmp loc_45136b
0x45135d: mov ecx, dword [ebp+var_80C]
0x451363: mov edx, dword [ecx]
0x451365: mov dword [ebp+var_80C], edx
0x45136b: cmp dword [ebp+var_80C], 0x0
0x451372: je loc_451396
0x451374: mov eax, dword [ebp+var_80C]
0x45137a: mov ecx, dword [ebp+var_810]
0x451380: mov edx, dword [eax+8]
0x451383: cmp edx, dword [ecx+4]
0x451386: jne loc_451394
0x451388: mov dword [ebp+var_81C], 0x1
0x451392: jmp loc_451396
0x451394: jmp loc_45135d
0x451396: cmp dword [ebp+var_81C], 0x0
0x45139d: jne loc_4513fb
0x45139f: mov eax, dword [ebp+var_814]
0x4513a5: mov ecx, dword [ebp+var_810]
0x4513ab: mov edx, dword [ecx+4]
0x4513ae: mov dword [eax], edx
0x4513b0: mov eax, dword [ebp+var_814]
0x4513b6: add eax, 0x4
0x4513b9: mov dword [ebp+var_814], eax
0x4513bf: mov ecx, dword [ebp+var_804]
0x4513c5: add ecx, 0x1
0x4513c8: mov dword [ebp+var_804], ecx
0x4513ce: mov edx, dword [ebp+var_818]
0x4513d4: mov eax, dword [ebp+var_810]
0x4513da: mov ecx, dword [eax]
0x4513dc: mov dword [edx], ecx
0x4513de: mov edx, dword [ebp+var_818]
0x4513e4: add edx, 0x4
0x4513e7: mov dword [ebp+var_818], edx
0x4513ed: cmp dword [ebp+var_804], 0x100
0x4513f7: jne loc_4513fb
0x4513f9: jmp loc_451400
0x4513fb: jmp loc_451249
0x451400: cmp dword [ebp+var_804], 0x100
0x45140a: jne loc_451411
0x45140c: jmp loc_451505
0x451411: mov eax, dword [dword_3c48550]
0x451416: mov dword [ebp+var_80C], eax
0x45141c: cmp dword [ebp+var_80C], 0x0
0x451423: je loc_4514fb
0x451429: mov dword [ebp+var_824], 0x0
0x451433: movsx ecx, word [word_3b32724]
0x45143a: mov dword [ebp+var_808], ecx
0x451440: mov dword [ebp+var_810], 0x3b2f720
0x45144a: jmp loc_45146a
0x45144c: mov edx, dword [ebp+var_808]
0x451452: sub edx, 0x1
0x451455: mov dword [ebp+var_808], edx
0x45145b: mov eax, dword [ebp+var_810]
0x451461: add eax, 0xc
0x451464: mov dword [ebp+var_810], eax
0x45146a: cmp dword [ebp+var_808], 0x0
0x451471: jle loc_4514c4
0x451473: mov ecx, dword [ebp+var_80C]
0x451479: mov edx, dword [ebp+var_810]
0x45147f: mov eax, dword [ecx+8]
0x451482: cmp eax, dword [edx+4]
0x451485: jne loc_4514c2
0x451487: mov ecx, dword [ebp+var_810]
0x45148d: mov edx, dword [ecx]
0x45148f: and edx, 0x8000000
0x451495: test edx, edx
0x451497: jne loc_4514aa
0x451499: mov eax, dword [ebp+var_810]
0x45149f: mov ecx, dword [dword_3b47000]
0x4514a5: cmp ecx, dword [eax+4]
0x4514a8: jne loc_4514b6
0x4514aa: mov dword [ebp+var_824], 0x0
0x4514b4: jmp loc_4514c0
0x4514b6: mov dword [ebp+var_824], 0x1
0x4514c0: jmp loc_4514c4
0x4514c2: jmp loc_45144c
0x4514c4: mov edx, dword [ebp+var_80C]
0x4514ca: mov eax, dword [edx]
0x4514cc: mov dword [ebp+var_820], eax
0x4514d2: cmp dword [ebp+var_824], 0x0
0x4514d9: jne loc_4514ea
0x4514db: mov ecx, dword [ebp+var_80C]
0x4514e1: push ecx
0x4514e2: call sub_451800
0x4514e7: add esp, 0x4
0x4514ea: mov edx, dword [ebp+var_820]
0x4514f0: mov dword [ebp+var_80C], edx
0x4514f6: jmp loc_45141c
0x4514fb: xor eax, eax
0x4514fd: test eax, eax
0x4514ff: jne loc_4511f8
0x451505: mov ecx, 0x389d650
0x45150a: call sub_7850f0
0x45150f: mov dword [ebp+var_82C], 0x100
0x451519: mov dword [ebp+var_828], 0x3b43000
0x451523: mov ecx, dword [ebp+var_804]
0x451529: mov dword [ebp+var_830], ecx
0x45152f: lea edx, dword [ebp+var_800]
0x451535: mov dword [ebp+var_83C], edx
0x45153b: lea eax, dword [ebp+var_400]
0x451541: mov dword [ebp+var_838], eax
0x451547: jmp loc_451576
0x451549: mov ecx, dword [ebp+var_830]
0x45154f: sub ecx, 0x1
0x451552: mov dword [ebp+var_830], ecx
0x451558: mov edx, dword [ebp+var_83C]
0x45155e: add edx, 0x4
0x451561: mov dword [ebp+var_83C], edx
0x451567: mov eax, dword [ebp+var_838]
0x45156d: add eax, 0x4
0x451570: mov dword [ebp+var_838], eax
0x451576: cmp dword [ebp+var_830], 0x0
0x45157d: jle loc_451779
0x451583: call sub_451790
0x451588: mov dword [ebp+var_828], eax
0x45158e: cmp dword [ebp+var_828], 0x0
0x451595: jne loc_45159c
0x451597: jmp loc_451779
0x45159c: call sub_4518d0
0x4515a1: mov ecx, 0x389d650
0x4515a6: call sub_7850f0
0x4515ab: mov ecx, dword [ebp+var_838]
0x4515b1: mov edx, dword [ecx]
0x4515b3: mov dword [ebp+var_834], edx
0x4515b9: mov edx, dword [ebp+var_834]
0x4515bf: add edx, 0x8
0x4515c2: xor ecx, ecx
0x4515c4: call sub_785070
0x4515c9: mov eax, dword [ebp+var_834]
0x4515cf: cmp dword [eax+0x1c], 0x0
0x4515d3: je loc_4515e6
0x4515d5: mov ecx, dword [ebp+var_834]
0x4515db: movzx edx, word [ecx+0x1c]
0x4515df: xor ecx, ecx
0x4515e1: call sub_784fb0
0x4515e6: mov edx, dword [ebp+var_834]
0x4515ec: cmp dword [edx+0x18], 0x0
0x4515f0: je loc_451603
0x4515f2: mov eax, dword [ebp+var_834]
0x4515f8: movzx edx, word [eax+0x18]
0x4515fc: xor ecx, ecx
0x4515fe: call sub_784e20
0x451603: mov ecx, dword [ebp+var_834]
0x451609: cmp dword [ecx+0x14], 0x0
0x45160d: je loc_451620
0x45160f: mov edx, dword [ebp+var_834]
0x451615: movzx edx, word [edx+0x14]
0x451619: xor ecx, ecx
0x45161b: call sub_784ee0
0x451620: mov edx, dword [ebp+var_834]
0x451626: add edx, 0x20
0x451629: xor ecx, ecx
0x45162b: call sub_788a50
0x451630: mov eax, dword [ebp+var_834]
0x451636: mov ecx, dword [eax+4]
0x451639: mov edx, dword [ecx+0x18]
0x45163c: mov dword [ebp+var_848], edx
0x451642: mov eax, dword [ebp+var_834]
0x451648: mov ecx, dword [eax+4]
0x45164b: mov edx, dword [ecx+0x1c]
0x45164e: mov dword [ebp+var_844], edx
0x451654: mov eax, dword [ebp+var_834]
0x45165a: mov ecx, dword [eax+4]
0x45165d: mov edx, dword [ecx+0x20]
0x451660: mov dword [ebp+var_840], edx
0x451666: lea eax, dword [ebp+var_848]
0x45166c: push eax
0x45166d: lea edx, dword [ebp+var_848]
0x451673: xor ecx, ecx
0x451675: call sub_781670
0x45167a: mov ecx, dword [ebp+var_840]
0x451680: mov dword [dword_3b4758c], ecx
0x451686: mov edx, dword [ebp+var_828]
0x45168c: mov eax, dword [ebp+var_848]
0x451692: mov dword [edx+0x30], eax
0x451695: mov ecx, dword [ebp+var_828]
0x45169b: mov edx, dword [ebp+var_844]
0x4516a1: mov dword [ecx+0x34], edx
0x4516a4: mov eax, dword [ebp+var_828]
0x4516aa: mov ecx, dword [ebp+var_840]
0x4516b0: mov dword [eax+0x38], ecx
0x4516b3: mov ecx, 0x1
0x4516b8: call sub_785140
0x4516bd: mov dword [dword_3b47588], 0x0
0x4516c7: mov dword [dword_3b42ff8], 0x0
0x4516d1: mov edx, dword [ebp+var_828]
0x4516d7: mov eax, dword [ebp+var_838]
0x4516dd: mov ecx, dword [eax]
0x4516df: mov dword [edx+8], ecx
0x4516e2: mov edx, dword [ebp+var_828]
0x4516e8: mov eax, dword [ebp+var_83C]
0x4516ee: mov ecx, dword [eax]
0x4516f0: mov dword [edx+0xc], ecx
0x4516f3: mov edx, dword [ebp+var_828]
0x4516f9: mov dword [edx+0x10], 0x0
0x451700: mov eax, dword [ebp+var_828]
0x451706: mov dword [eax+0x18], 0x0
0x45170d: mov ecx, dword [ebp+var_828]
0x451713: mov dword [ecx+0x1c], 0x0
0x45171a: mov edx, dword [ebp+var_828]
0x451720: mov dword [edx+0x24], 0x0
0x451727: mov eax, dword [ebp+var_828]
0x45172d: push eax
0x45172e: mov ecx, dword [ebp+var_83C]
0x451734: mov edx, dword [ecx]
0x451736: push edx
0x451737: mov eax, dword [ebp+var_838]
0x45173d: mov ecx, dword [eax]
0x45173f: push ecx
0x451740: call sub_451a00
0x451745: add esp, 0xc
0x451748: mov edx, dword [ebp+var_828]
0x45174e: push edx
0x45174f: call sub_451910
0x451754: add esp, 0x4
0x451757: mov eax, dword [ebp+var_828]
0x45175d: mov ecx, dword [dword_3b47588]
0x451763: mov dword [eax+0x28], ecx
0x451766: mov edx, dword [ebp+var_828]
0x45176c: mov eax, dword [dword_3b42ff8]
0x451771: mov dword [edx+0x2c], eax
0x451774: jmp loc_451549
0x451779: mov ecx, 0x1
0x45177e: call sub_785140
0x451783: mov eax, 0x1
0x451788: mov esp, ebp
0x45178a: pop ebp
0x45178b: ret
