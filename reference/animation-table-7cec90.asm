
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  7cec90: a1 7c 6e 8f 03               	movl	0x38f6e7c, %eax
  7cec95: 8b 48 04                     	movl	0x4(%eax), %ecx
  7cec98: 89 0d 10 62 c5 03            	movl	%ecx, 0x3c56210
  7cec9e: b9 49 00 00 00               	movl	$0x49, %ecx
  7ceca3: 53                           	pushl	%ebx
  7ceca4: 88 0d 14 62 c5 03            	movb	%cl, 0x3c56214
  7cecaa: c7 05 18 62 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56218  # imm = 0x3D800000
  7cecb4: c7 05 1c 62 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5621c  # imm = 0x3F000000
  7cecbe: 55                           	pushl	%ebp
  7cecbf: b3 03                        	movb	$0x3, %bl
  7cecc1: 88 1d 15 62 c5 03            	movb	%bl, 0x3c56215
  7cecc7: 56                           	pushl	%esi
  7cecc8: 57                           	pushl	%edi
  7cecc9: 33 f6                        	xorl	%esi, %esi
  7ceccb: 66 89 35 16 62 c5 03         	movw	%si, 0x3c56216
  7cecd2: 8b 90 e8 01 00 00            	movl	0x1e8(%eax), %edx
  7cecd8: 89 15 20 62 c5 03            	movl	%edx, 0x3c56220
  7cecde: 88 0d 24 62 c5 03            	movb	%cl, 0x3c56224
  7cece4: c7 05 28 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56228  # imm = 0x3E800000
  7cecee: c7 05 2c 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5622c  # imm = 0x3E800000
  7cecf8: b2 04                        	movb	$0x4, %dl
  7cecfa: 88 15 25 62 c5 03            	movb	%dl, 0x3c56225
  7ced00: bf 02 00 00 00               	movl	$0x2, %edi
  7ced05: 66 89 3d 26 62 c5 03         	movw	%di, 0x3c56226
  7ced0c: 8b a8 ec 01 00 00            	movl	0x1ec(%eax), %ebp
  7ced12: 89 2d 30 62 c5 03            	movl	%ebp, 0x3c56230
  7ced18: 88 0d 34 62 c5 03            	movb	%cl, 0x3c56234
  7ced1e: 88 1d 35 62 c5 03            	movb	%bl, 0x3c56235
  7ced24: 66 89 3d 36 62 c5 03         	movw	%di, 0x3c56236
  7ced2b: c7 05 38 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56238  # imm = 0x3E800000
  7ced35: c7 05 3c 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5623c  # imm = 0x3E800000
  7ced3f: 8b 78 08                     	movl	0x8(%eax), %edi
  7ced42: 89 3d 40 62 c5 03            	movl	%edi, 0x3c56240
  7ced48: 88 0d 44 62 c5 03            	movb	%cl, 0x3c56244
  7ced4e: 88 15 45 62 c5 03            	movb	%dl, 0x3c56245
  7ced54: 66 89 35 46 62 c5 03         	movw	%si, 0x3c56246
  7ced5b: c7 05 48 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56248  # imm = 0x3E800000
  7ced65: c7 05 4c 62 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5624c  # imm = 0x3F800000
  7ced6f: 8b b8 e0 01 00 00            	movl	0x1e0(%eax), %edi
  7ced75: 89 3d 50 62 c5 03            	movl	%edi, 0x3c56250
  7ced7b: 88 0d 54 62 c5 03            	movb	%cl, 0x3c56254
  7ced81: 88 15 55 62 c5 03            	movb	%dl, 0x3c56255
  7ced87: 66 c7 05 56 62 c5 03 05 00   	movw	$0x5, 0x3c56256
  7ced90: c7 05 58 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56258  # imm = 0x3E800000
  7ced9a: c7 05 5c 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5625c  # imm = 0x3E800000
  7ceda4: 8b b8 e4 01 00 00            	movl	0x1e4(%eax), %edi
  7cedaa: 89 3d 60 62 c5 03            	movl	%edi, 0x3c56260
  7cedb0: 88 0d 64 62 c5 03            	movb	%cl, 0x3c56264
  7cedb6: 88 15 65 62 c5 03            	movb	%dl, 0x3c56265
  7cedbc: 66 c7 05 66 62 c5 03 06 00   	movw	$0x6, 0x3c56266
  7cedc5: c7 05 68 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56268  # imm = 0x3E800000
  7cedcf: c7 05 6c 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5626c  # imm = 0x3E800000
  7cedd9: 8b b8 e0 01 00 00            	movl	0x1e0(%eax), %edi
  7ceddf: 89 3d 70 62 c5 03            	movl	%edi, 0x3c56270
  7cede5: 88 0d 74 62 c5 03            	movb	%cl, 0x3c56274
  7cedeb: c6 05 75 62 c5 03 06         	movb	$0x6, 0x3c56275
  7cedf2: 66 89 35 76 62 c5 03         	movw	%si, 0x3c56276
  7cedf9: c7 05 78 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56278  # imm = 0x3E800000
  7cee03: c7 05 7c 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5627c  # imm = 0x3E800000
  7cee0d: 8b b8 80 01 00 00            	movl	0x180(%eax), %edi
  7cee13: 89 3d 80 62 c5 03            	movl	%edi, 0x3c56280
  7cee19: 88 0d 84 62 c5 03            	movb	%cl, 0x3c56284
  7cee1f: 88 1d 85 62 c5 03            	movb	%bl, 0x3c56285
  7cee25: 66 c7 05 86 62 c5 03 07 00   	movw	$0x7, 0x3c56286
  7cee2e: c7 05 88 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56288  # imm = 0x3E800000
  7cee38: c7 05 8c 62 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5628c  # imm = 0x3F800000
  7cee42: 8b b8 84 01 00 00            	movl	0x184(%eax), %edi
  7cee48: 89 3d 90 62 c5 03            	movl	%edi, 0x3c56290
  7cee4e: 88 0d 94 62 c5 03            	movb	%cl, 0x3c56294
  7cee54: 88 1d 95 62 c5 03            	movb	%bl, 0x3c56295
  7cee5a: 66 c7 05 96 62 c5 03 08 00   	movw	$0x8, 0x3c56296
  7cee63: c7 05 98 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56298  # imm = 0x3E800000
  7cee6d: c7 05 9c 62 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5629c  # imm = 0x3F800000
  7cee77: 8b 78 0c                     	movl	0xc(%eax), %edi
  7cee7a: 89 3d a0 62 c5 03            	movl	%edi, 0x3c562a0
  7cee80: 88 0d a4 62 c5 03            	movb	%cl, 0x3c562a4
  7cee86: c6 05 a5 62 c5 03 0a         	movb	$0xa, 0x3c562a5
  7cee8d: 66 c7 05 a6 62 c5 03 09 00   	movw	$0x9, 0x3c562a6
  7cee96: c7 05 a8 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c562a8  # imm = 0x3E800000
  7ceea0: c7 05 ac 62 c5 03 9a 99 59 40	movl	$0x4059999a, 0x3c562ac  # imm = 0x4059999A
  7ceeaa: 8b 78 10                     	movl	0x10(%eax), %edi
  7ceead: 89 3d b0 62 c5 03            	movl	%edi, 0x3c562b0
  7ceeb3: 88 0d b4 62 c5 03            	movb	%cl, 0x3c562b4
  7ceeb9: c6 05 b5 62 c5 03 0a         	movb	$0xa, 0x3c562b5
  7ceec0: 66 c7 05 b6 62 c5 03 0a 00   	movw	$0xa, 0x3c562b6
  7ceec9: c7 05 b8 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c562b8  # imm = 0x3E800000
  7ceed3: c7 05 bc 62 c5 03 33 33 b3 3f	movl	$0x3fb33333, 0x3c562bc  # imm = 0x3FB33333
  7ceedd: 8b 78 14                     	movl	0x14(%eax), %edi
  7ceee0: 89 3d c0 62 c5 03            	movl	%edi, 0x3c562c0
  7ceee6: 88 0d c4 62 c5 03            	movb	%cl, 0x3c562c4
  7ceeec: c6 05 c5 62 c5 03 0a         	movb	$0xa, 0x3c562c5
  7ceef3: 66 c7 05 c6 62 c5 03 0b 00   	movw	$0xb, 0x3c562c6
  7ceefc: c7 05 c8 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c562c8  # imm = 0x3E800000
  7cef06: c7 05 cc 62 c5 03 cd cc 4c 3f	movl	$0x3f4ccccd, 0x3c562cc  # imm = 0x3F4CCCCD
  7cef10: 8b 78 18                     	movl	0x18(%eax), %edi
  7cef13: 89 3d d0 62 c5 03            	movl	%edi, 0x3c562d0
  7cef19: 88 0d d4 62 c5 03            	movb	%cl, 0x3c562d4
  7cef1f: c6 05 d5 62 c5 03 0a         	movb	$0xa, 0x3c562d5
  7cef26: 66 c7 05 d6 62 c5 03 0c 00   	movw	$0xc, 0x3c562d6
  7cef2f: c7 05 d8 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c562d8  # imm = 0x3E800000
  7cef39: c7 05 dc 62 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c562dc  # imm = 0x3F000000
  7cef43: 8b 78 48                     	movl	0x48(%eax), %edi
  7cef46: 89 3d e0 62 c5 03            	movl	%edi, 0x3c562e0
  7cef4c: 88 0d e4 62 c5 03            	movb	%cl, 0x3c562e4
  7cef52: c6 05 e5 62 c5 03 09         	movb	$0x9, 0x3c562e5
  7cef59: 66 c7 05 e6 62 c5 03 0d 00   	movw	$0xd, 0x3c562e6
  7cef62: c7 05 e8 62 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c562e8  # imm = 0x3F000000
  7cef6c: c7 05 ec 62 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c562ec  # imm = 0x3F000000
  7cef76: 8b 78 38                     	movl	0x38(%eax), %edi
  7cef79: 89 3d f0 62 c5 03            	movl	%edi, 0x3c562f0
  7cef7f: c6 05 f4 62 c5 03 4a         	movb	$0x4a, 0x3c562f4
  7cef86: c6 05 f5 62 c5 03 0a         	movb	$0xa, 0x3c562f5
  7cef8d: 66 c7 05 f6 62 c5 03 0e 00   	movw	$0xe, 0x3c562f6
  7cef96: c7 05 f8 62 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c562f8  # imm = 0x3E800000
  7cefa0: c7 05 fc 62 c5 03 cd cc 4c 3e	movl	$0x3e4ccccd, 0x3c562fc  # imm = 0x3E4CCCCD
  7cefaa: 8b 78 38                     	movl	0x38(%eax), %edi
  7cefad: 89 3d 00 63 c5 03            	movl	%edi, 0x3c56300
  7cefb3: c6 05 04 63 c5 03 4a         	movb	$0x4a, 0x3c56304
  7cefba: c6 05 05 63 c5 03 09         	movb	$0x9, 0x3c56305
  7cefc1: 66 c7 05 06 63 c5 03 0f 00   	movw	$0xf, 0x3c56306
  7cefca: c7 05 08 63 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56308  # imm = 0x3E800000
  7cefd4: c7 05 0c 63 c5 03 33 33 33 3f	movl	$0x3f333333, 0x3c5630c  # imm = 0x3F333333
  7cefde: 8b 78 20                     	movl	0x20(%eax), %edi
  7cefe1: 89 3d 10 63 c5 03            	movl	%edi, 0x3c56310
  7cefe7: 88 0d 14 63 c5 03            	movb	%cl, 0x3c56314
  7cefed: 88 1d 15 63 c5 03            	movb	%bl, 0x3c56315
  7ceff3: 66 c7 05 16 63 c5 03 10 00   	movw	$0x10, 0x3c56316
  7ceffc: c7 05 18 63 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56318  # imm = 0x3E800000
  7cf006: c7 05 1c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5631c  # imm = 0x3F800000
  7cf010: 8b 78 24                     	movl	0x24(%eax), %edi
  7cf013: 89 3d 20 63 c5 03            	movl	%edi, 0x3c56320
  7cf019: 88 0d 24 63 c5 03            	movb	%cl, 0x3c56324
  7cf01f: 88 15 25 63 c5 03            	movb	%dl, 0x3c56325
  7cf025: bf 12 00 00 00               	movl	$0x12, %edi
  7cf02a: 66 89 3d 26 63 c5 03         	movw	%di, 0x3c56326
  7cf031: c7 05 28 63 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56328  # imm = 0x3E000000
  7cf03b: c7 05 2c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5632c  # imm = 0x3F800000
  7cf045: 8b 68 28                     	movl	0x28(%eax), %ebp
  7cf048: 89 2d 30 63 c5 03            	movl	%ebp, 0x3c56330
  7cf04e: 88 0d 34 63 c5 03            	movb	%cl, 0x3c56334
  7cf054: 88 1d 35 63 c5 03            	movb	%bl, 0x3c56335
  7cf05a: 66 89 3d 36 63 c5 03         	movw	%di, 0x3c56336
  7cf061: c7 05 38 63 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56338  # imm = 0x3E800000
  7cf06b: c7 05 3c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5633c  # imm = 0x3F800000
  7cf075: 8b 68 2c                     	movl	0x2c(%eax), %ebp
  7cf078: 89 2d 40 63 c5 03            	movl	%ebp, 0x3c56340
  7cf07e: 88 0d 44 63 c5 03            	movb	%cl, 0x3c56344
  7cf084: 88 15 45 63 c5 03            	movb	%dl, 0x3c56345
  7cf08a: 66 89 35 46 63 c5 03         	movw	%si, 0x3c56346
  7cf091: c7 05 48 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56348  # imm = 0x3F800000
  7cf09b: c7 05 4c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5634c  # imm = 0x3F800000
  7cf0a5: 8b 68 1c                     	movl	0x1c(%eax), %ebp
  7cf0a8: 89 2d 50 63 c5 03            	movl	%ebp, 0x3c56350
  7cf0ae: 88 0d 54 63 c5 03            	movb	%cl, 0x3c56354
  7cf0b4: 88 15 55 63 c5 03            	movb	%dl, 0x3c56355
  7cf0ba: 66 89 35 56 63 c5 03         	movw	%si, 0x3c56356
  7cf0c1: c7 05 58 63 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56358  # imm = 0x3E800000
  7cf0cb: c7 05 5c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5635c  # imm = 0x3F800000
  7cf0d5: 8b 68 30                     	movl	0x30(%eax), %ebp
  7cf0d8: 89 2d 60 63 c5 03            	movl	%ebp, 0x3c56360
  7cf0de: 88 0d 64 63 c5 03            	movb	%cl, 0x3c56364
  7cf0e4: 88 15 65 63 c5 03            	movb	%dl, 0x3c56365
  7cf0ea: 66 c7 05 66 63 c5 03 16 00   	movw	$0x16, 0x3c56366
  7cf0f3: c7 05 68 63 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56368  # imm = 0x3E000000
  7cf0fd: c7 05 6c 63 c5 03 00 00 40 40	movl	$0x40400000, 0x3c5636c  # imm = 0x40400000
  7cf107: 8b 68 34                     	movl	0x34(%eax), %ebp
  7cf10a: 89 2d 70 63 c5 03            	movl	%ebp, 0x3c56370
  7cf110: 88 0d 74 63 c5 03            	movb	%cl, 0x3c56374
  7cf116: 88 1d 75 63 c5 03            	movb	%bl, 0x3c56375
  7cf11c: 66 c7 05 76 63 c5 03 16 00   	movw	$0x16, 0x3c56376
  7cf125: c7 05 78 63 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56378  # imm = 0x3E800000
  7cf12f: c7 05 7c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5637c  # imm = 0x3F800000
  7cf139: 8b a8 d0 01 00 00            	movl	0x1d0(%eax), %ebp
  7cf13f: 89 2d 80 63 c5 03            	movl	%ebp, 0x3c56380
  7cf145: 88 0d 84 63 c5 03            	movb	%cl, 0x3c56384
  7cf14b: c6 05 85 63 c5 03 05         	movb	$0x5, 0x3c56385
  7cf152: 66 c7 05 86 63 c5 03 17 00   	movw	$0x17, 0x3c56386
  7cf15b: c7 05 88 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56388  # imm = 0x3F800000
  7cf165: c7 05 8c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5638c  # imm = 0x3F800000
  7cf16f: 8b a8 d4 01 00 00            	movl	0x1d4(%eax), %ebp
  7cf175: 89 2d 90 63 c5 03            	movl	%ebp, 0x3c56390
  7cf17b: 88 0d 94 63 c5 03            	movb	%cl, 0x3c56394
  7cf181: 88 15 95 63 c5 03            	movb	%dl, 0x3c56395
  7cf187: 66 89 35 96 63 c5 03         	movw	%si, 0x3c56396
  7cf18e: c7 05 98 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56398  # imm = 0x3F800000
  7cf198: c7 05 9c 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5639c  # imm = 0x3F800000
  7cf1a2: 8b 68 1c                     	movl	0x1c(%eax), %ebp
  7cf1a5: 89 2d a0 63 c5 03            	movl	%ebp, 0x3c563a0
  7cf1ab: 88 0d a4 63 c5 03            	movb	%cl, 0x3c563a4
  7cf1b1: 88 1d a5 63 c5 03            	movb	%bl, 0x3c563a5
  7cf1b7: 66 c7 05 a6 63 c5 03 19 00   	movw	$0x19, 0x3c563a6
  7cf1c0: c7 05 a8 63 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c563a8  # imm = 0x3E800000
  7cf1ca: c7 05 ac 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c563ac  # imm = 0x3F800000
  7cf1d4: 8b 68 3c                     	movl	0x3c(%eax), %ebp
  7cf1d7: 89 2d b0 63 c5 03            	movl	%ebp, 0x3c563b0
  7cf1dd: 88 0d b4 63 c5 03            	movb	%cl, 0x3c563b4
  7cf1e3: 88 1d b5 63 c5 03            	movb	%bl, 0x3c563b5
  7cf1e9: 66 c7 05 b6 63 c5 03 1a 00   	movw	$0x1a, 0x3c563b6
  7cf1f2: c7 05 b8 63 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c563b8  # imm = 0x3D800000
  7cf1fc: c7 05 bc 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c563bc  # imm = 0x3F800000
  7cf206: 8b 68 40                     	movl	0x40(%eax), %ebp
  7cf209: 89 2d c0 63 c5 03            	movl	%ebp, 0x3c563c0
  7cf20f: 88 0d c4 63 c5 03            	movb	%cl, 0x3c563c4
  7cf215: 88 15 c5 63 c5 03            	movb	%dl, 0x3c563c5
  7cf21b: 66 c7 05 c6 63 c5 03 1c 00   	movw	$0x1c, 0x3c563c6
  7cf224: c7 05 c8 63 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c563c8  # imm = 0x3F000000
  7cf22e: c7 05 cc 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c563cc  # imm = 0x3F800000
  7cf238: 8b 68 44                     	movl	0x44(%eax), %ebp
  7cf23b: 89 2d d0 63 c5 03            	movl	%ebp, 0x3c563d0
  7cf241: 88 0d d4 63 c5 03            	movb	%cl, 0x3c563d4
  7cf247: 88 15 d5 63 c5 03            	movb	%dl, 0x3c563d5
  7cf24d: 66 89 35 d6 63 c5 03         	movw	%si, 0x3c563d6
  7cf254: c7 05 d8 63 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c563d8  # imm = 0x3D800000
  7cf25e: c7 05 dc 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c563dc  # imm = 0x3F800000
  7cf268: 8b 68 58                     	movl	0x58(%eax), %ebp
  7cf26b: 89 2d e0 63 c5 03            	movl	%ebp, 0x3c563e0
  7cf271: 88 0d e4 63 c5 03            	movb	%cl, 0x3c563e4
  7cf277: 88 15 e5 63 c5 03            	movb	%dl, 0x3c563e5
  7cf27d: 66 c7 05 e6 63 c5 03 01 00   	movw	$0x1, 0x3c563e6
  7cf286: c7 05 e8 63 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c563e8  # imm = 0x3D800000
  7cf290: c7 05 ec 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c563ec  # imm = 0x3F800000
  7cf29a: 8b 68 50                     	movl	0x50(%eax), %ebp
  7cf29d: 89 2d f0 63 c5 03            	movl	%ebp, 0x3c563f0
  7cf2a3: 88 0d f4 63 c5 03            	movb	%cl, 0x3c563f4
  7cf2a9: 88 15 f5 63 c5 03            	movb	%dl, 0x3c563f5
  7cf2af: 66 89 35 f6 63 c5 03         	movw	%si, 0x3c563f6
  7cf2b6: c7 05 f8 63 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c563f8  # imm = 0x3F800000
  7cf2c0: c7 05 fc 63 c5 03 66 66 66 3f	movl	$0x3f666666, 0x3c563fc  # imm = 0x3F666666
  7cf2ca: 8b 68 4c                     	movl	0x4c(%eax), %ebp
  7cf2cd: 89 2d 00 64 c5 03            	movl	%ebp, 0x3c56400
  7cf2d3: 88 0d 04 64 c5 03            	movb	%cl, 0x3c56404
  7cf2d9: 88 1d 05 64 c5 03            	movb	%bl, 0x3c56405
  7cf2df: 66 c7 05 06 64 c5 03 1f 00   	movw	$0x1f, 0x3c56406
  7cf2e8: c7 05 08 64 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56408  # imm = 0x3D800000
  7cf2f2: c7 05 0c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5640c  # imm = 0x3F800000
  7cf2fc: 8b 68 54                     	movl	0x54(%eax), %ebp
  7cf2ff: 89 2d 10 64 c5 03            	movl	%ebp, 0x3c56410
  7cf305: c6 05 14 64 c5 03 01         	movb	$0x1, 0x3c56414
  7cf30c: 88 1d 15 64 c5 03            	movb	%bl, 0x3c56415
  7cf312: 66 c7 05 16 64 c5 03 20 00   	movw	$0x20, 0x3c56416
  7cf31b: c7 05 18 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56418  # imm = 0x3F000000
  7cf325: c7 05 1c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5641c  # imm = 0x3F800000
  7cf32f: 8b 68 5c                     	movl	0x5c(%eax), %ebp
  7cf332: 89 2d 20 64 c5 03            	movl	%ebp, 0x3c56420
  7cf338: 88 0d 24 64 c5 03            	movb	%cl, 0x3c56424
  7cf33e: 88 15 25 64 c5 03            	movb	%dl, 0x3c56425
  7cf344: 66 c7 05 26 64 c5 03 25 00   	movw	$0x25, 0x3c56426
  7cf34d: c7 05 28 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56428  # imm = 0x3F000000
  7cf357: c7 05 2c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5642c  # imm = 0x3F800000
  7cf361: 8b 68 7c                     	movl	0x7c(%eax), %ebp
  7cf364: 89 2d 30 64 c5 03            	movl	%ebp, 0x3c56430
  7cf36a: 88 0d 34 64 c5 03            	movb	%cl, 0x3c56434
  7cf370: 88 1d 35 64 c5 03            	movb	%bl, 0x3c56435
  7cf376: 66 c7 05 36 64 c5 03 22 00   	movw	$0x22, 0x3c56436
  7cf37f: c7 05 38 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56438  # imm = 0x3F000000
  7cf389: c7 05 3c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5643c  # imm = 0x3F800000
  7cf393: 8b a8 80 00 00 00            	movl	0x80(%eax), %ebp
  7cf399: 89 2d 40 64 c5 03            	movl	%ebp, 0x3c56440
  7cf39f: 88 0d 44 64 c5 03            	movb	%cl, 0x3c56444
  7cf3a5: 88 15 45 64 c5 03            	movb	%dl, 0x3c56445
  7cf3ab: 66 c7 05 46 64 c5 03 25 00   	movw	$0x25, 0x3c56446
  7cf3b4: c7 05 48 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56448  # imm = 0x3F000000
  7cf3be: c7 05 4c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5644c  # imm = 0x3F800000
  7cf3c8: 8b a8 84 00 00 00            	movl	0x84(%eax), %ebp
  7cf3ce: 89 2d 50 64 c5 03            	movl	%ebp, 0x3c56450
  7cf3d4: 88 0d 54 64 c5 03            	movb	%cl, 0x3c56454
  7cf3da: 88 15 55 64 c5 03            	movb	%dl, 0x3c56455
  7cf3e0: 66 c7 05 56 64 c5 03 25 00   	movw	$0x25, 0x3c56456
  7cf3e9: c7 05 58 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56458  # imm = 0x3F000000
  7cf3f3: c7 05 5c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5645c  # imm = 0x3F800000
  7cf3fd: 8b a8 88 00 00 00            	movl	0x88(%eax), %ebp
  7cf403: 89 2d 60 64 c5 03            	movl	%ebp, 0x3c56460
  7cf409: 88 0d 64 64 c5 03            	movb	%cl, 0x3c56464
  7cf40f: 88 1d 65 64 c5 03            	movb	%bl, 0x3c56465
  7cf415: 66 c7 05 66 64 c5 03 25 00   	movw	$0x25, 0x3c56466
  7cf41e: c7 05 68 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56468  # imm = 0x3F000000
  7cf428: 89 35 6c 64 c5 03            	movl	%esi, 0x3c5646c
  7cf42e: 8b a8 8c 00 00 00            	movl	0x8c(%eax), %ebp
  7cf434: 89 2d 70 64 c5 03            	movl	%ebp, 0x3c56470
  7cf43a: 88 0d 74 64 c5 03            	movb	%cl, 0x3c56474
  7cf440: 88 1d 75 64 c5 03            	movb	%bl, 0x3c56475
  7cf446: 66 c7 05 76 64 c5 03 26 00   	movw	$0x26, 0x3c56476
  7cf44f: c7 05 78 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56478  # imm = 0x3F000000
  7cf459: c7 05 7c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5647c  # imm = 0x3F800000
  7cf463: 8b 68 20                     	movl	0x20(%eax), %ebp
  7cf466: 89 2d 80 64 c5 03            	movl	%ebp, 0x3c56480
  7cf46c: 88 0d 84 64 c5 03            	movb	%cl, 0x3c56484
  7cf472: 88 1d 85 64 c5 03            	movb	%bl, 0x3c56485
  7cf478: 66 c7 05 86 64 c5 03 10 00   	movw	$0x10, 0x3c56486
  7cf481: c7 05 88 64 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56488  # imm = 0x3E800000
  7cf48b: c7 05 8c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5648c  # imm = 0x3F800000
  7cf495: 8b a8 b4 00 00 00            	movl	0xb4(%eax), %ebp
  7cf49b: 89 2d 90 64 c5 03            	movl	%ebp, 0x3c56490
  7cf4a1: 88 0d 94 64 c5 03            	movb	%cl, 0x3c56494
  7cf4a7: 88 15 95 64 c5 03            	movb	%dl, 0x3c56495
  7cf4ad: 66 89 3d 96 64 c5 03         	movw	%di, 0x3c56496
  7cf4b4: c7 05 98 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56498  # imm = 0x3F000000
  7cf4be: c7 05 9c 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5649c  # imm = 0x3F800000
  7cf4c8: 8b 68 20                     	movl	0x20(%eax), %ebp
  7cf4cb: 89 2d a0 64 c5 03            	movl	%ebp, 0x3c564a0
  7cf4d1: 88 0d a4 64 c5 03            	movb	%cl, 0x3c564a4
  7cf4d7: 88 1d a5 64 c5 03            	movb	%bl, 0x3c564a5
  7cf4dd: 66 c7 05 a6 64 c5 03 10 00   	movw	$0x10, 0x3c564a6
  7cf4e6: c7 05 a8 64 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c564a8  # imm = 0x3E800000
  7cf4f0: c7 05 ac 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c564ac  # imm = 0x3F800000
  7cf4fa: 8b a8 b8 00 00 00            	movl	0xb8(%eax), %ebp
  7cf500: 89 2d b0 64 c5 03            	movl	%ebp, 0x3c564b0
  7cf506: 88 0d b4 64 c5 03            	movb	%cl, 0x3c564b4
  7cf50c: 88 15 b5 64 c5 03            	movb	%dl, 0x3c564b5
  7cf512: 66 89 3d b6 64 c5 03         	movw	%di, 0x3c564b6
  7cf519: c7 05 b8 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c564b8  # imm = 0x3F000000
  7cf523: c7 05 bc 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c564bc  # imm = 0x3F800000
  7cf52d: 8b 68 20                     	movl	0x20(%eax), %ebp
  7cf530: 89 2d c0 64 c5 03            	movl	%ebp, 0x3c564c0
  7cf536: 88 0d c4 64 c5 03            	movb	%cl, 0x3c564c4
  7cf53c: 88 1d c5 64 c5 03            	movb	%bl, 0x3c564c5
  7cf542: 66 c7 05 c6 64 c5 03 10 00   	movw	$0x10, 0x3c564c6
  7cf54b: c7 05 c8 64 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c564c8  # imm = 0x3E800000
  7cf555: c7 05 cc 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c564cc  # imm = 0x3F800000
  7cf55f: 8b a8 b0 00 00 00            	movl	0xb0(%eax), %ebp
  7cf565: 89 2d d0 64 c5 03            	movl	%ebp, 0x3c564d0
  7cf56b: 88 0d d4 64 c5 03            	movb	%cl, 0x3c564d4
  7cf571: 88 15 d5 64 c5 03            	movb	%dl, 0x3c564d5
  7cf577: 66 c7 05 d6 64 c5 03 10 00   	movw	$0x10, 0x3c564d6
  7cf580: c7 05 d8 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c564d8  # imm = 0x3F000000
  7cf58a: c7 05 dc 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c564dc  # imm = 0x3F800000
  7cf594: 8b a8 bc 00 00 00            	movl	0xbc(%eax), %ebp
  7cf59a: 89 2d e0 64 c5 03            	movl	%ebp, 0x3c564e0
  7cf5a0: 88 0d e4 64 c5 03            	movb	%cl, 0x3c564e4
  7cf5a6: 88 15 e5 64 c5 03            	movb	%dl, 0x3c564e5
  7cf5ac: 66 c7 05 e6 64 c5 03 10 00   	movw	$0x10, 0x3c564e6
  7cf5b5: c7 05 e8 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c564e8  # imm = 0x3F000000
  7cf5bf: c7 05 ec 64 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c564ec  # imm = 0x3F800000
  7cf5c9: 8b a8 c4 00 00 00            	movl	0xc4(%eax), %ebp
  7cf5cf: 89 2d f0 64 c5 03            	movl	%ebp, 0x3c564f0
  7cf5d5: 88 0d f4 64 c5 03            	movb	%cl, 0x3c564f4
  7cf5db: 88 15 f5 64 c5 03            	movb	%dl, 0x3c564f5
  7cf5e1: 66 c7 05 f6 64 c5 03 09 00   	movw	$0x9, 0x3c564f6
  7cf5ea: c7 05 f8 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c564f8  # imm = 0x3F000000
  7cf5f4: c7 05 fc 64 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c564fc  # imm = 0x3F000000
  7cf5fe: 8b a8 c8 00 00 00            	movl	0xc8(%eax), %ebp
  7cf604: 89 2d 00 65 c5 03            	movl	%ebp, 0x3c56500
  7cf60a: 88 0d 04 65 c5 03            	movb	%cl, 0x3c56504
  7cf610: 88 1d 05 65 c5 03            	movb	%bl, 0x3c56505
  7cf616: 66 c7 05 06 65 c5 03 2f 00   	movw	$0x2f, 0x3c56506
  7cf61f: c7 05 08 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56508  # imm = 0x3F000000
  7cf629: c7 05 0c 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5650c  # imm = 0x3F000000
  7cf633: 8b a8 94 00 00 00            	movl	0x94(%eax), %ebp
  7cf639: 89 2d 10 65 c5 03            	movl	%ebp, 0x3c56510
  7cf63f: 88 0d 14 65 c5 03            	movb	%cl, 0x3c56514
  7cf645: 88 1d 15 65 c5 03            	movb	%bl, 0x3c56515
  7cf64b: 66 c7 05 16 65 c5 03 30 00   	movw	$0x30, 0x3c56516
  7cf654: c7 05 18 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56518  # imm = 0x3F000000
  7cf65e: c7 05 1c 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5651c  # imm = 0x3F800000
  7cf668: 8b a8 90 00 00 00            	movl	0x90(%eax), %ebp
  7cf66e: 89 2d 20 65 c5 03            	movl	%ebp, 0x3c56520
  7cf674: 88 0d 24 65 c5 03            	movb	%cl, 0x3c56524
  7cf67a: 88 1d 25 65 c5 03            	movb	%bl, 0x3c56525
  7cf680: 66 c7 05 26 65 c5 03 31 00   	movw	$0x31, 0x3c56526
  7cf689: c7 05 28 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56528  # imm = 0x3F000000
  7cf693: c7 05 2c 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5652c  # imm = 0x3F000000
  7cf69d: 8b a8 f0 01 00 00            	movl	0x1f0(%eax), %ebp
  7cf6a3: 89 2d 30 65 c5 03            	movl	%ebp, 0x3c56530
  7cf6a9: 88 0d 34 65 c5 03            	movb	%cl, 0x3c56534
  7cf6af: 88 15 35 65 c5 03            	movb	%dl, 0x3c56535
  7cf6b5: 66 c7 05 36 65 c5 03 33 00   	movw	$0x33, 0x3c56536
  7cf6be: c7 05 38 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56538  # imm = 0x3F000000
  7cf6c8: c7 05 3c 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5653c  # imm = 0x3F000000
  7cf6d2: 8b a8 f4 01 00 00            	movl	0x1f4(%eax), %ebp
  7cf6d8: 89 2d 40 65 c5 03            	movl	%ebp, 0x3c56540
  7cf6de: 88 0d 44 65 c5 03            	movb	%cl, 0x3c56544
  7cf6e4: 88 1d 45 65 c5 03            	movb	%bl, 0x3c56545
  7cf6ea: 66 c7 05 46 65 c5 03 33 00   	movw	$0x33, 0x3c56546
  7cf6f3: c7 05 48 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56548  # imm = 0x3F000000
  7cf6fd: c7 05 4c 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5654c  # imm = 0x3F000000
  7cf707: 8b a8 f8 01 00 00            	movl	0x1f8(%eax), %ebp
  7cf70d: 89 2d 50 65 c5 03            	movl	%ebp, 0x3c56550
  7cf713: 88 0d 54 65 c5 03            	movb	%cl, 0x3c56554
  7cf719: 88 15 55 65 c5 03            	movb	%dl, 0x3c56555
  7cf71f: 66 c7 05 56 65 c5 03 25 00   	movw	$0x25, 0x3c56556
  7cf728: c7 05 58 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56558  # imm = 0x3F000000
  7cf732: c7 05 5c 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5655c  # imm = 0x3F000000
  7cf73c: 8b a8 a8 00 00 00            	movl	0xa8(%eax), %ebp
  7cf742: 89 2d 60 65 c5 03            	movl	%ebp, 0x3c56560
  7cf748: 88 0d 64 65 c5 03            	movb	%cl, 0x3c56564
  7cf74e: 88 15 65 65 c5 03            	movb	%dl, 0x3c56565
  7cf754: 66 c7 05 66 65 c5 03 36 00   	movw	$0x36, 0x3c56566
  7cf75d: c7 05 68 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56568  # imm = 0x3F000000
  7cf767: c7 05 6c 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5656c  # imm = 0x3F000000
  7cf771: 8b a8 ac 00 00 00            	movl	0xac(%eax), %ebp
  7cf777: 89 2d 70 65 c5 03            	movl	%ebp, 0x3c56570
  7cf77d: 88 0d 74 65 c5 03            	movb	%cl, 0x3c56574
  7cf783: 88 1d 75 65 c5 03            	movb	%bl, 0x3c56575
  7cf789: 66 c7 05 76 65 c5 03 36 00   	movw	$0x36, 0x3c56576
  7cf792: c7 05 78 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56578  # imm = 0x3F000000
  7cf79c: c7 05 7c 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5657c  # imm = 0x3F000000
  7cf7a6: 8b 68 6c                     	movl	0x6c(%eax), %ebp
  7cf7a9: 89 2d 80 65 c5 03            	movl	%ebp, 0x3c56580
  7cf7af: 88 0d 84 65 c5 03            	movb	%cl, 0x3c56584
  7cf7b5: 88 1d 85 65 c5 03            	movb	%bl, 0x3c56585
  7cf7bb: 66 c7 05 86 65 c5 03 37 00   	movw	$0x37, 0x3c56586
  7cf7c4: c7 05 88 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56588  # imm = 0x3F000000
  7cf7ce: c7 05 8c 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5658c  # imm = 0x3F800000
  7cf7d8: 8b 68 70                     	movl	0x70(%eax), %ebp
  7cf7db: 89 2d 90 65 c5 03            	movl	%ebp, 0x3c56590
  7cf7e1: 88 0d 94 65 c5 03            	movb	%cl, 0x3c56594
  7cf7e7: 88 15 95 65 c5 03            	movb	%dl, 0x3c56595
  7cf7ed: 66 c7 05 96 65 c5 03 39 00   	movw	$0x39, 0x3c56596
  7cf7f6: c7 05 98 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56598  # imm = 0x3F000000
  7cf800: c7 05 9c 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5659c  # imm = 0x3F800000
  7cf80a: 8b 68 74                     	movl	0x74(%eax), %ebp
  7cf80d: 89 2d a0 65 c5 03            	movl	%ebp, 0x3c565a0
  7cf813: 88 0d a4 65 c5 03            	movb	%cl, 0x3c565a4
  7cf819: 88 1d a5 65 c5 03            	movb	%bl, 0x3c565a5
  7cf81f: 66 c7 05 a6 65 c5 03 39 00   	movw	$0x39, 0x3c565a6
  7cf828: c7 05 a8 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c565a8  # imm = 0x3F000000
  7cf832: c7 05 ac 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c565ac  # imm = 0x3F800000
  7cf83c: 8b 68 78                     	movl	0x78(%eax), %ebp
  7cf83f: 89 2d b0 65 c5 03            	movl	%ebp, 0x3c565b0
  7cf845: 88 0d b4 65 c5 03            	movb	%cl, 0x3c565b4
  7cf84b: 88 15 b5 65 c5 03            	movb	%dl, 0x3c565b5
  7cf851: 66 c7 05 b6 65 c5 03 25 00   	movw	$0x25, 0x3c565b6
  7cf85a: c7 05 b8 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c565b8  # imm = 0x3F000000
  7cf864: c7 05 bc 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c565bc  # imm = 0x3F800000
  7cf86e: 8b a8 98 00 00 00            	movl	0x98(%eax), %ebp
  7cf874: 89 2d c0 65 c5 03            	movl	%ebp, 0x3c565c0
  7cf87a: 88 0d c4 65 c5 03            	movb	%cl, 0x3c565c4
  7cf880: 88 15 c5 65 c5 03            	movb	%dl, 0x3c565c5
  7cf886: 66 89 35 c6 65 c5 03         	movw	%si, 0x3c565c6
  7cf88d: c7 05 c8 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c565c8  # imm = 0x3F000000
  7cf897: c7 05 cc 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c565cc  # imm = 0x3F800000
  7cf8a1: 8b a8 9c 00 00 00            	movl	0x9c(%eax), %ebp
  7cf8a7: 89 2d d0 65 c5 03            	movl	%ebp, 0x3c565d0
  7cf8ad: 88 0d d4 65 c5 03            	movb	%cl, 0x3c565d4
  7cf8b3: 88 15 d5 65 c5 03            	movb	%dl, 0x3c565d5
  7cf8b9: 66 89 35 d6 65 c5 03         	movw	%si, 0x3c565d6
  7cf8c0: c7 05 d8 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c565d8  # imm = 0x3F000000
  7cf8ca: c7 05 dc 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c565dc  # imm = 0x3F800000
  7cf8d4: 8b a8 a0 00 00 00            	movl	0xa0(%eax), %ebp
  7cf8da: 89 2d e0 65 c5 03            	movl	%ebp, 0x3c565e0
  7cf8e0: 88 0d e4 65 c5 03            	movb	%cl, 0x3c565e4
  7cf8e6: 88 15 e5 65 c5 03            	movb	%dl, 0x3c565e5
  7cf8ec: 66 89 3d e6 65 c5 03         	movw	%di, 0x3c565e6
  7cf8f3: c7 05 e8 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c565e8  # imm = 0x3F000000
  7cf8fd: c7 05 ec 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c565ec  # imm = 0x3F800000
  7cf907: 8b a8 a4 00 00 00            	movl	0xa4(%eax), %ebp
  7cf90d: 89 2d f0 65 c5 03            	movl	%ebp, 0x3c565f0
  7cf913: 88 0d f4 65 c5 03            	movb	%cl, 0x3c565f4
  7cf919: 88 15 f5 65 c5 03            	movb	%dl, 0x3c565f5
  7cf91f: 66 89 3d f6 65 c5 03         	movw	%di, 0x3c565f6
  7cf926: c7 05 f8 65 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c565f8  # imm = 0x3F000000
  7cf930: c7 05 fc 65 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c565fc  # imm = 0x3F800000
  7cf93a: 8b 68 5c                     	movl	0x5c(%eax), %ebp
  7cf93d: 89 2d 00 66 c5 03            	movl	%ebp, 0x3c56600
  7cf943: 88 0d 04 66 c5 03            	movb	%cl, 0x3c56604
  7cf949: c6 05 05 66 c5 03 06         	movb	$0x6, 0x3c56605
  7cf950: 66 89 35 06 66 c5 03         	movw	%si, 0x3c56606
  7cf957: c7 05 08 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56608  # imm = 0x3F800000
  7cf961: c7 05 0c 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5660c  # imm = 0x3F800000
  7cf96b: 8b a8 dc 01 00 00            	movl	0x1dc(%eax), %ebp
  7cf971: 89 2d 10 66 c5 03            	movl	%ebp, 0x3c56610
  7cf977: 88 0d 14 66 c5 03            	movb	%cl, 0x3c56614
  7cf97d: 88 1d 15 66 c5 03            	movb	%bl, 0x3c56615
  7cf983: 66 c7 05 16 66 c5 03 1f 00   	movw	$0x1f, 0x3c56616
  7cf98c: c7 05 18 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56618  # imm = 0x3F800000
  7cf996: c7 05 1c 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5661c  # imm = 0x3F800000
  7cf9a0: 8b a8 c0 00 00 00            	movl	0xc0(%eax), %ebp
  7cf9a6: 89 2d 20 66 c5 03            	movl	%ebp, 0x3c56620
  7cf9ac: 88 0d 24 66 c5 03            	movb	%cl, 0x3c56624
  7cf9b2: c6 05 25 66 c5 03 05         	movb	$0x5, 0x3c56625
  7cf9b9: 66 c7 05 26 66 c5 03 41 00   	movw	$0x41, 0x3c56626
  7cf9c2: c7 05 28 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56628  # imm = 0x3F800000
  7cf9cc: c7 05 2c 66 c5 03 9a 99 99 3e	movl	$0x3e99999a, 0x3c5662c  # imm = 0x3E99999A
  7cf9d6: 8b a8 cc 00 00 00            	movl	0xcc(%eax), %ebp
  7cf9dc: 89 2d 30 66 c5 03            	movl	%ebp, 0x3c56630
  7cf9e2: 88 0d 34 66 c5 03            	movb	%cl, 0x3c56634
  7cf9e8: c6 05 35 66 c5 03 0c         	movb	$0xc, 0x3c56635
  7cf9ef: 66 c7 05 36 66 c5 03 42 00   	movw	$0x42, 0x3c56636
  7cf9f8: c7 05 38 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56638  # imm = 0x3F800000
  7cfa02: c7 05 3c 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5663c  # imm = 0x3F800000
  7cfa0c: 8b a8 d4 00 00 00            	movl	0xd4(%eax), %ebp
  7cfa12: 89 2d 40 66 c5 03            	movl	%ebp, 0x3c56640
  7cfa18: 88 0d 44 66 c5 03            	movb	%cl, 0x3c56644
  7cfa1e: 88 1d 45 66 c5 03            	movb	%bl, 0x3c56645
  7cfa24: 66 c7 05 46 66 c5 03 43 00   	movw	$0x43, 0x3c56646
  7cfa2d: c7 05 48 66 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56648  # imm = 0x3E800000
  7cfa37: c7 05 4c 66 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5664c  # imm = 0x3E800000
  7cfa41: 8b a8 d0 00 00 00            	movl	0xd0(%eax), %ebp
  7cfa47: 89 2d 50 66 c5 03            	movl	%ebp, 0x3c56650
  7cfa4d: 88 0d 54 66 c5 03            	movb	%cl, 0x3c56654
  7cfa53: 88 1d 55 66 c5 03            	movb	%bl, 0x3c56655
  7cfa59: 66 c7 05 56 66 c5 03 44 00   	movw	$0x44, 0x3c56656
  7cfa62: c7 05 58 66 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56658  # imm = 0x3E800000
  7cfa6c: c7 05 5c 66 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5665c  # imm = 0x3E800000
  7cfa76: 8b a8 74 01 00 00            	movl	0x174(%eax), %ebp
  7cfa7c: 89 2d 60 66 c5 03            	movl	%ebp, 0x3c56660
  7cfa82: 88 0d 64 66 c5 03            	movb	%cl, 0x3c56664
  7cfa88: c6 05 65 66 c5 03 0c         	movb	$0xc, 0x3c56665
  7cfa8f: 66 c7 05 66 66 c5 03 45 00   	movw	$0x45, 0x3c56666
  7cfa98: c7 05 68 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56668  # imm = 0x3F800000
  7cfaa2: c7 05 6c 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5666c  # imm = 0x3F800000
  7cfaac: 8b a8 78 01 00 00            	movl	0x178(%eax), %ebp
  7cfab2: 89 2d 70 66 c5 03            	movl	%ebp, 0x3c56670
  7cfab8: 88 0d 74 66 c5 03            	movb	%cl, 0x3c56674
  7cfabe: 88 1d 75 66 c5 03            	movb	%bl, 0x3c56675
  7cfac4: 66 c7 05 76 66 c5 03 46 00   	movw	$0x46, 0x3c56676
  7cfacd: c7 05 78 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56678  # imm = 0x3F800000
  7cfad7: c7 05 7c 66 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5667c  # imm = 0x3F000000
  7cfae1: 8b a8 7c 01 00 00            	movl	0x17c(%eax), %ebp
  7cfae7: 89 2d 80 66 c5 03            	movl	%ebp, 0x3c56680
  7cfaed: 88 0d 84 66 c5 03            	movb	%cl, 0x3c56684
  7cfaf3: c6 05 85 66 c5 03 09         	movb	$0x9, 0x3c56685
  7cfafa: 66 c7 05 86 66 c5 03 47 00   	movw	$0x47, 0x3c56686
  7cfb03: c7 05 88 66 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56688  # imm = 0x3F000000
  7cfb0d: c7 05 8c 66 c5 03 00 00 00 40	movl	$0x40000000, 0x3c5668c  # imm = 0x40000000
  7cfb17: 8b a8 c4 01 00 00            	movl	0x1c4(%eax), %ebp
  7cfb1d: 89 2d 90 66 c5 03            	movl	%ebp, 0x3c56690
  7cfb23: 88 0d 94 66 c5 03            	movb	%cl, 0x3c56694
  7cfb29: 88 15 95 66 c5 03            	movb	%dl, 0x3c56695
  7cfb2f: 66 c7 05 96 66 c5 03 4a 00   	movw	$0x4a, 0x3c56696
  7cfb38: c7 05 98 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56698  # imm = 0x3F800000
  7cfb42: c7 05 9c 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5669c  # imm = 0x3F800000
  7cfb4c: 8b a8 c8 01 00 00            	movl	0x1c8(%eax), %ebp
  7cfb52: 89 2d a0 66 c5 03            	movl	%ebp, 0x3c566a0
  7cfb58: 88 0d a4 66 c5 03            	movb	%cl, 0x3c566a4
  7cfb5e: c6 05 a5 66 c5 03 05         	movb	$0x5, 0x3c566a5
  7cfb65: 66 89 0d a6 66 c5 03         	movw	%cx, 0x3c566a6
  7cfb6c: c7 05 a8 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c566a8  # imm = 0x3F800000
  7cfb76: c7 05 ac 66 c5 03 00 00 00 40	movl	$0x40000000, 0x3c566ac  # imm = 0x40000000
  7cfb80: 8b a8 ec 01 00 00            	movl	0x1ec(%eax), %ebp
  7cfb86: 89 2d b0 66 c5 03            	movl	%ebp, 0x3c566b0
  7cfb8c: 88 0d b4 66 c5 03            	movb	%cl, 0x3c566b4
  7cfb92: 88 1d b5 66 c5 03            	movb	%bl, 0x3c566b5
  7cfb98: 66 c7 05 b6 66 c5 03 4a 00   	movw	$0x4a, 0x3c566b6
  7cfba1: c7 05 b8 66 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c566b8  # imm = 0x3E800000
  7cfbab: c7 05 bc 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c566bc  # imm = 0x3F800000
  7cfbb5: 89 35 c0 66 c5 03            	movl	%esi, 0x3c566c0
  7cfbbb: 88 0d c4 66 c5 03            	movb	%cl, 0x3c566c4
  7cfbc1: c6 05 c5 66 c5 03 05         	movb	$0x5, 0x3c566c5
  7cfbc8: 66 c7 05 c6 66 c5 03 4b 00   	movw	$0x4b, 0x3c566c6
  7cfbd1: c7 05 c8 66 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c566c8  # imm = 0x3F000000
  7cfbdb: c7 05 cc 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c566cc  # imm = 0x3F800000
  7cfbe5: 89 35 d0 66 c5 03            	movl	%esi, 0x3c566d0
  7cfbeb: 88 0d d4 66 c5 03            	movb	%cl, 0x3c566d4
  7cfbf1: 88 15 d5 66 c5 03            	movb	%dl, 0x3c566d5
  7cfbf7: 66 89 35 d6 66 c5 03         	movw	%si, 0x3c566d6
  7cfbfe: c7 05 d8 66 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c566d8  # imm = 0x3F000000
  7cfc08: c7 05 dc 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c566dc  # imm = 0x3F800000
  7cfc12: 8b a8 a8 00 00 00            	movl	0xa8(%eax), %ebp
  7cfc18: 89 2d e0 66 c5 03            	movl	%ebp, 0x3c566e0
  7cfc1e: 88 0d e4 66 c5 03            	movb	%cl, 0x3c566e4
  7cfc24: 88 15 e5 66 c5 03            	movb	%dl, 0x3c566e5
  7cfc2a: 66 c7 05 e6 66 c5 03 4e 00   	movw	$0x4e, 0x3c566e6
  7cfc33: c7 05 e8 66 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c566e8  # imm = 0x3E000000
  7cfc3d: c7 05 ec 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c566ec  # imm = 0x3F800000
  7cfc47: 8b a8 ac 00 00 00            	movl	0xac(%eax), %ebp
  7cfc4d: 89 2d f0 66 c5 03            	movl	%ebp, 0x3c566f0
  7cfc53: 88 0d f4 66 c5 03            	movb	%cl, 0x3c566f4
  7cfc59: 88 1d f5 66 c5 03            	movb	%bl, 0x3c566f5
  7cfc5f: 66 c7 05 f6 66 c5 03 4e 00   	movw	$0x4e, 0x3c566f6
  7cfc68: c7 05 f8 66 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c566f8  # imm = 0x3E000000
  7cfc72: c7 05 fc 66 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c566fc  # imm = 0x3F800000
  7cfc7c: 8b a8 8c 01 00 00            	movl	0x18c(%eax), %ebp
  7cfc82: 89 2d 00 67 c5 03            	movl	%ebp, 0x3c56700
  7cfc88: 88 0d 04 67 c5 03            	movb	%cl, 0x3c56704
  7cfc8e: 88 1d 05 67 c5 03            	movb	%bl, 0x3c56705
  7cfc94: 66 c7 05 06 67 c5 03 4f 00   	movw	$0x4f, 0x3c56706
  7cfc9d: c7 05 08 67 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56708  # imm = 0x3E000000
  7cfca7: c7 05 0c 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5670c  # imm = 0x3F800000
  7cfcb1: 8b a8 88 01 00 00            	movl	0x188(%eax), %ebp
  7cfcb7: 89 2d 10 67 c5 03            	movl	%ebp, 0x3c56710
  7cfcbd: 88 0d 14 67 c5 03            	movb	%cl, 0x3c56714
  7cfcc3: 88 1d 15 67 c5 03            	movb	%bl, 0x3c56715
  7cfcc9: 66 c7 05 16 67 c5 03 50 00   	movw	$0x50, 0x3c56716
  7cfcd2: c7 05 18 67 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56718  # imm = 0x3E000000
  7cfcdc: c7 05 1c 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5671c  # imm = 0x3F800000
  7cfce6: 8b a8 9c 01 00 00            	movl	0x19c(%eax), %ebp
  7cfcec: 89 2d 20 67 c5 03            	movl	%ebp, 0x3c56720
  7cfcf2: 88 0d 24 67 c5 03            	movb	%cl, 0x3c56724
  7cfcf8: 88 1d 25 67 c5 03            	movb	%bl, 0x3c56725
  7cfcfe: 66 c7 05 26 67 c5 03 51 00   	movw	$0x51, 0x3c56726
  7cfd07: c7 05 28 67 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56728  # imm = 0x3E000000
  7cfd11: c7 05 2c 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5672c  # imm = 0x3F800000
  7cfd1b: 8b a8 d0 01 00 00            	movl	0x1d0(%eax), %ebp
  7cfd21: 89 2d 30 67 c5 03            	movl	%ebp, 0x3c56730
  7cfd27: 88 0d 34 67 c5 03            	movb	%cl, 0x3c56734
  7cfd2d: c6 05 35 67 c5 03 05         	movb	$0x5, 0x3c56735
  7cfd34: 66 c7 05 36 67 c5 03 52 00   	movw	$0x52, 0x3c56736
  7cfd3d: c7 05 38 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56738  # imm = 0x3F800000
  7cfd47: c7 05 3c 67 c5 03 00 00 70 41	movl	$0x41700000, 0x3c5673c  # imm = 0x41700000
  7cfd51: 8b 68 38                     	movl	0x38(%eax), %ebp
  7cfd54: 89 2d 40 67 c5 03            	movl	%ebp, 0x3c56740
  7cfd5a: c6 05 44 67 c5 03 4a         	movb	$0x4a, 0x3c56744
  7cfd61: c6 05 45 67 c5 03 0a         	movb	$0xa, 0x3c56745
  7cfd68: 66 c7 05 46 67 c5 03 53 00   	movw	$0x53, 0x3c56746
  7cfd71: c7 05 48 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56748  # imm = 0x3F800000
  7cfd7b: c7 05 4c 67 c5 03 cd cc 4c 3e	movl	$0x3e4ccccd, 0x3c5674c  # imm = 0x3E4CCCCD
  7cfd85: 8b a8 cc 01 00 00            	movl	0x1cc(%eax), %ebp
  7cfd8b: 66 89 3d 56 67 c5 03         	movw	%di, 0x3c56756
  7cfd92: 89 2d 50 67 c5 03            	movl	%ebp, 0x3c56750
  7cfd98: 88 0d 54 67 c5 03            	movb	%cl, 0x3c56754
  7cfd9e: 88 15 55 67 c5 03            	movb	%dl, 0x3c56755
  7cfda4: c7 05 58 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56758  # imm = 0x3F800000
  7cfdae: c7 05 5c 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c5675c  # imm = 0x3E800000
  7cfdb8: 8b b8 fc 01 00 00            	movl	0x1fc(%eax), %edi
  7cfdbe: 89 3d 60 67 c5 03            	movl	%edi, 0x3c56760
  7cfdc4: 88 0d 64 67 c5 03            	movb	%cl, 0x3c56764
  7cfdca: c6 05 65 67 c5 03 05         	movb	$0x5, 0x3c56765
  7cfdd1: 66 c7 05 66 67 c5 03 55 00   	movw	$0x55, 0x3c56766
  7cfdda: c7 05 68 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56768  # imm = 0x3F800000
  7cfde4: c7 05 6c 67 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c5676c  # imm = 0x3F000000
  7cfdee: 8b b8 d0 01 00 00            	movl	0x1d0(%eax), %edi
  7cfdf4: 89 3d 70 67 c5 03            	movl	%edi, 0x3c56770
  7cfdfa: 88 0d 74 67 c5 03            	movb	%cl, 0x3c56774
  7cfe00: c6 05 75 67 c5 03 05         	movb	$0x5, 0x3c56775
  7cfe07: 66 c7 05 76 67 c5 03 56 00   	movw	$0x56, 0x3c56776
  7cfe10: c7 05 78 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56778  # imm = 0x3F800000
  7cfe1a: c7 05 7c 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5677c  # imm = 0x3F800000
  7cfe24: 8b b8 d8 01 00 00            	movl	0x1d8(%eax), %edi
  7cfe2a: 89 3d 80 67 c5 03            	movl	%edi, 0x3c56780
  7cfe30: 88 0d 84 67 c5 03            	movb	%cl, 0x3c56784
  7cfe36: c6 05 85 67 c5 03 05         	movb	$0x5, 0x3c56785
  7cfe3d: 66 c7 05 86 67 c5 03 57 00   	movw	$0x57, 0x3c56786
  7cfe46: c7 05 88 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56788  # imm = 0x3E800000
  7cfe50: c7 05 8c 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5678c  # imm = 0x3F800000
  7cfe5a: 8b b8 98 01 00 00            	movl	0x198(%eax), %edi
  7cfe60: 89 3d 90 67 c5 03            	movl	%edi, 0x3c56790
  7cfe66: 88 0d 94 67 c5 03            	movb	%cl, 0x3c56794
  7cfe6c: 88 1d 95 67 c5 03            	movb	%bl, 0x3c56795
  7cfe72: 66 c7 05 96 67 c5 03 58 00   	movw	$0x58, 0x3c56796
  7cfe7b: c7 05 98 67 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56798  # imm = 0x3E000000
  7cfe85: c7 05 9c 67 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5679c  # imm = 0x3F800000
  7cfe8f: 8b b8 a8 01 00 00            	movl	0x1a8(%eax), %edi
  7cfe95: 89 3d a0 67 c5 03            	movl	%edi, 0x3c567a0
  7cfe9b: bf 59 00 00 00               	movl	$0x59, %edi
  7cfea0: 88 0d a4 67 c5 03            	movb	%cl, 0x3c567a4
  7cfea6: 88 1d a5 67 c5 03            	movb	%bl, 0x3c567a5
  7cfeac: 66 89 3d a6 67 c5 03         	movw	%di, 0x3c567a6
  7cfeb3: c7 05 a8 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c567a8  # imm = 0x3E800000
  7cfebd: c7 05 ac 67 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c567ac  # imm = 0x3F000000
  7cfec7: 8b a8 a4 01 00 00            	movl	0x1a4(%eax), %ebp
  7cfecd: 89 2d b0 67 c5 03            	movl	%ebp, 0x3c567b0
  7cfed3: 88 0d b4 67 c5 03            	movb	%cl, 0x3c567b4
  7cfed9: 88 1d b5 67 c5 03            	movb	%bl, 0x3c567b5
  7cfedf: 66 c7 05 b6 67 c5 03 5a 00   	movw	$0x5a, 0x3c567b6
  7cfee8: c7 05 b8 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c567b8  # imm = 0x3E800000
  7cfef2: c7 05 bc 67 c5 03 00 00 00 40	movl	$0x40000000, 0x3c567bc  # imm = 0x40000000
  7cfefc: 8b a8 a0 01 00 00            	movl	0x1a0(%eax), %ebp
  7cff02: 89 2d c0 67 c5 03            	movl	%ebp, 0x3c567c0
  7cff08: 88 0d c4 67 c5 03            	movb	%cl, 0x3c567c4
  7cff0e: 88 1d c5 67 c5 03            	movb	%bl, 0x3c567c5
  7cff14: 66 c7 05 c6 67 c5 03 5b 00   	movw	$0x5b, 0x3c567c6
  7cff1d: c7 05 c8 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c567c8  # imm = 0x3E800000
  7cff27: c7 05 cc 67 c5 03 00 00 00 40	movl	$0x40000000, 0x3c567cc  # imm = 0x40000000
  7cff31: 8b a8 00 02 00 00            	movl	0x200(%eax), %ebp
  7cff37: 89 2d d0 67 c5 03            	movl	%ebp, 0x3c567d0
  7cff3d: 88 0d d4 67 c5 03            	movb	%cl, 0x3c567d4
  7cff43: c6 05 d5 67 c5 03 05         	movb	$0x5, 0x3c567d5
  7cff4a: 66 c7 05 d6 67 c5 03 5c 00   	movw	$0x5c, 0x3c567d6
  7cff53: c7 05 d8 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c567d8  # imm = 0x3E800000
  7cff5d: c7 05 dc 67 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c567dc  # imm = 0x3F000000
  7cff67: 8b a8 04 02 00 00            	movl	0x204(%eax), %ebp
  7cff6d: 66 89 3d e6 67 c5 03         	movw	%di, 0x3c567e6
  7cff74: 89 2d e0 67 c5 03            	movl	%ebp, 0x3c567e0
  7cff7a: 88 0d e4 67 c5 03            	movb	%cl, 0x3c567e4
  7cff80: 88 15 e5 67 c5 03            	movb	%dl, 0x3c567e5
  7cff86: c7 05 e8 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c567e8  # imm = 0x3E800000
  7cff90: c7 05 ec 67 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c567ec  # imm = 0x3F000000
  7cff9a: 8b b8 6c 01 00 00            	movl	0x16c(%eax), %edi
  7cffa0: 89 3d f0 67 c5 03            	movl	%edi, 0x3c567f0
  7cffa6: 88 0d f4 67 c5 03            	movb	%cl, 0x3c567f4
  7cffac: 88 1d f5 67 c5 03            	movb	%bl, 0x3c567f5
  7cffb2: c7 05 f8 67 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c567f8  # imm = 0x3E800000
  7cffbc: c7 05 fc 67 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c567fc  # imm = 0x3F000000
  7cffc6: bf 5e 00 00 00               	movl	$0x5e, %edi
  7cffcb: 66 89 3d f6 67 c5 03         	movw	%di, 0x3c567f6
  7cffd2: 8b a8 70 01 00 00            	movl	0x170(%eax), %ebp
  7cffd8: 89 2d 00 68 c5 03            	movl	%ebp, 0x3c56800
  7cffde: 88 0d 04 68 c5 03            	movb	%cl, 0x3c56804
  7cffe4: 88 15 05 68 c5 03            	movb	%dl, 0x3c56805
  7cffea: 66 89 3d 06 68 c5 03         	movw	%di, 0x3c56806
  7cfff1: c7 05 08 68 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56808  # imm = 0x3E800000
  7cfffb: c7 05 0c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5680c  # imm = 0x3F800000
  7d0005: 8b a8 5c 01 00 00            	movl	0x15c(%eax), %ebp
  7d000b: 89 2d 10 68 c5 03            	movl	%ebp, 0x3c56810
  7d0011: 88 0d 14 68 c5 03            	movb	%cl, 0x3c56814
  7d0017: 88 15 15 68 c5 03            	movb	%dl, 0x3c56815
  7d001d: 66 c7 05 16 68 c5 03 61 00   	movw	$0x61, 0x3c56816
  7d0026: c7 05 18 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56818  # imm = 0x3F800000
  7d0030: c7 05 1c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5681c  # imm = 0x3F800000
  7d003a: 8b a8 60 01 00 00            	movl	0x160(%eax), %ebp
  7d0040: 89 2d 20 68 c5 03            	movl	%ebp, 0x3c56820
  7d0046: 88 0d 24 68 c5 03            	movb	%cl, 0x3c56824
  7d004c: c6 05 25 68 c5 03 0c         	movb	$0xc, 0x3c56825
  7d0053: 66 c7 05 26 68 c5 03 61 00   	movw	$0x61, 0x3c56826
  7d005c: c7 05 28 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56828  # imm = 0x3F800000
  7d0066: c7 05 2c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5682c  # imm = 0x3F800000
  7d0070: 8b a8 60 01 00 00            	movl	0x160(%eax), %ebp
  7d0076: 89 2d 30 68 c5 03            	movl	%ebp, 0x3c56830
  7d007c: 88 0d 34 68 c5 03            	movb	%cl, 0x3c56834
  7d0082: 88 15 35 68 c5 03            	movb	%dl, 0x3c56835
  7d0088: 66 89 3d 36 68 c5 03         	movw	%di, 0x3c56836
  7d008f: c7 05 38 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56838  # imm = 0x3F800000
  7d0099: c7 05 3c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5683c  # imm = 0x3F800000
  7d00a3: 8b b8 64 01 00 00            	movl	0x164(%eax), %edi
  7d00a9: 89 3d 40 68 c5 03            	movl	%edi, 0x3c56840
  7d00af: 88 0d 44 68 c5 03            	movb	%cl, 0x3c56844
  7d00b5: 88 1d 45 68 c5 03            	movb	%bl, 0x3c56845
  7d00bb: 66 c7 05 46 68 c5 03 63 00   	movw	$0x63, 0x3c56846
  7d00c4: c7 05 48 68 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56848  # imm = 0x3E800000
  7d00ce: c7 05 4c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5684c  # imm = 0x3F800000
  7d00d8: 8b b8 68 01 00 00            	movl	0x168(%eax), %edi
  7d00de: 89 3d 50 68 c5 03            	movl	%edi, 0x3c56850
  7d00e4: 88 0d 54 68 c5 03            	movb	%cl, 0x3c56854
  7d00ea: 88 1d 55 68 c5 03            	movb	%bl, 0x3c56855
  7d00f0: 66 c7 05 56 68 c5 03 64 00   	movw	$0x64, 0x3c56856
  7d00f9: c7 05 58 68 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56858  # imm = 0x3E800000
  7d0103: c7 05 5c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5685c  # imm = 0x3F800000
  7d010d: 8b b8 68 01 00 00            	movl	0x168(%eax), %edi
  7d0113: 89 3d 60 68 c5 03            	movl	%edi, 0x3c56860
  7d0119: 88 0d 64 68 c5 03            	movb	%cl, 0x3c56864
  7d011f: 88 1d 65 68 c5 03            	movb	%bl, 0x3c56865
  7d0125: 66 c7 05 66 68 c5 03 65 00   	movw	$0x65, 0x3c56866
  7d012e: c7 05 68 68 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56868  # imm = 0x3E800000
  7d0138: c7 05 6c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5686c  # imm = 0x3F800000
  7d0142: 8b b8 f8 00 00 00            	movl	0xf8(%eax), %edi
  7d0148: 89 3d 70 68 c5 03            	movl	%edi, 0x3c56870
  7d014e: 88 0d 74 68 c5 03            	movb	%cl, 0x3c56874
  7d0154: 88 1d 75 68 c5 03            	movb	%bl, 0x3c56875
  7d015a: 66 c7 05 76 68 c5 03 66 00   	movw	$0x66, 0x3c56876
  7d0163: c7 05 78 68 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56878  # imm = 0x3D800000
  7d016d: c7 05 7c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5687c  # imm = 0x3F800000
  7d0177: 8b b8 e8 00 00 00            	movl	0xe8(%eax), %edi
  7d017d: 89 3d 80 68 c5 03            	movl	%edi, 0x3c56880
  7d0183: 88 0d 84 68 c5 03            	movb	%cl, 0x3c56884
  7d0189: c6 05 85 68 c5 03 05         	movb	$0x5, 0x3c56885
  7d0190: 66 c7 05 86 68 c5 03 67 00   	movw	$0x67, 0x3c56886
  7d0199: c7 05 88 68 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56888  # imm = 0x3E000000
  7d01a3: c7 05 8c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5688c  # imm = 0x3F800000
  7d01ad: 8b b8 ec 00 00 00            	movl	0xec(%eax), %edi
  7d01b3: 89 3d 90 68 c5 03            	movl	%edi, 0x3c56890
  7d01b9: 88 0d 94 68 c5 03            	movb	%cl, 0x3c56894
  7d01bf: c6 05 95 68 c5 03 05         	movb	$0x5, 0x3c56895
  7d01c6: 66 c7 05 96 68 c5 03 68 00   	movw	$0x68, 0x3c56896
  7d01cf: c7 05 98 68 c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56898  # imm = 0x3E000000
  7d01d9: c7 05 9c 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5689c  # imm = 0x3F800000
  7d01e3: 8b b8 f4 00 00 00            	movl	0xf4(%eax), %edi
  7d01e9: 89 3d a0 68 c5 03            	movl	%edi, 0x3c568a0
  7d01ef: 88 0d a4 68 c5 03            	movb	%cl, 0x3c568a4
  7d01f5: 88 15 a5 68 c5 03            	movb	%dl, 0x3c568a5
  7d01fb: 66 c7 05 a6 68 c5 03 6f 00   	movw	$0x6f, 0x3c568a6
  7d0204: c7 05 a8 68 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c568a8  # imm = 0x3D800000
  7d020e: c7 05 ac 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568ac  # imm = 0x3F800000
  7d0218: 8b b8 f0 00 00 00            	movl	0xf0(%eax), %edi
  7d021e: 89 3d b0 68 c5 03            	movl	%edi, 0x3c568b0
  7d0224: 88 0d b4 68 c5 03            	movb	%cl, 0x3c568b4
  7d022a: 88 15 b5 68 c5 03            	movb	%dl, 0x3c568b5
  7d0230: 66 c7 05 b6 68 c5 03 69 00   	movw	$0x69, 0x3c568b6
  7d0239: c7 05 b8 68 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c568b8  # imm = 0x3E800000
  7d0243: c7 05 bc 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568bc  # imm = 0x3F800000
  7d024d: 8b b8 d8 00 00 00            	movl	0xd8(%eax), %edi
  7d0253: 89 3d c0 68 c5 03            	movl	%edi, 0x3c568c0
  7d0259: 88 0d c4 68 c5 03            	movb	%cl, 0x3c568c4
  7d025f: c6 05 c5 68 c5 03 0c         	movb	$0xc, 0x3c568c5
  7d0266: 66 c7 05 c6 68 c5 03 6b 00   	movw	$0x6b, 0x3c568c6
  7d026f: c7 05 c8 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568c8  # imm = 0x3F800000
  7d0279: c7 05 cc 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568cc  # imm = 0x3F800000
  7d0283: 8b b8 dc 00 00 00            	movl	0xdc(%eax), %edi
  7d0289: 89 3d d0 68 c5 03            	movl	%edi, 0x3c568d0
  7d028f: 88 0d d4 68 c5 03            	movb	%cl, 0x3c568d4
  7d0295: c6 05 d5 68 c5 03 0c         	movb	$0xc, 0x3c568d5
  7d029c: 66 c7 05 d6 68 c5 03 6c 00   	movw	$0x6c, 0x3c568d6
  7d02a5: c7 05 d8 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568d8  # imm = 0x3F800000
  7d02af: c7 05 dc 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568dc  # imm = 0x3F800000
  7d02b9: 8b b8 e0 00 00 00            	movl	0xe0(%eax), %edi
  7d02bf: 89 3d e0 68 c5 03            	movl	%edi, 0x3c568e0
  7d02c5: bf 72 00 00 00               	movl	$0x72, %edi
  7d02ca: 66 89 3d e6 68 c5 03         	movw	%di, 0x3c568e6
  7d02d1: 88 0d e4 68 c5 03            	movb	%cl, 0x3c568e4
  7d02d7: 88 15 e5 68 c5 03            	movb	%dl, 0x3c568e5
  7d02dd: c7 05 e8 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568e8  # imm = 0x3F800000
  7d02e7: c7 05 ec 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568ec  # imm = 0x3F800000
  7d02f1: 8b a8 e4 00 00 00            	movl	0xe4(%eax), %ebp
  7d02f7: 66 89 3d f6 68 c5 03         	movw	%di, 0x3c568f6
  7d02fe: 89 2d f0 68 c5 03            	movl	%ebp, 0x3c568f0
  7d0304: 88 0d f4 68 c5 03            	movb	%cl, 0x3c568f4
  7d030a: 88 15 f5 68 c5 03            	movb	%dl, 0x3c568f5
  7d0310: c7 05 f8 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568f8  # imm = 0x3F800000
  7d031a: c7 05 fc 68 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c568fc  # imm = 0x3F800000
  7d0324: 8b b8 00 01 00 00            	movl	0x100(%eax), %edi
  7d032a: 89 3d 00 69 c5 03            	movl	%edi, 0x3c56900
  7d0330: bf 70 00 00 00               	movl	$0x70, %edi
  7d0335: 88 0d 04 69 c5 03            	movb	%cl, 0x3c56904
  7d033b: 88 15 05 69 c5 03            	movb	%dl, 0x3c56905
  7d0341: 66 89 3d 06 69 c5 03         	movw	%di, 0x3c56906
  7d0348: c7 05 08 69 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56908  # imm = 0x3F000000
  7d0352: c7 05 0c 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5690c  # imm = 0x3F800000
  7d035c: 8b a8 04 01 00 00            	movl	0x104(%eax), %ebp
  7d0362: 89 2d 10 69 c5 03            	movl	%ebp, 0x3c56910
  7d0368: 88 0d 14 69 c5 03            	movb	%cl, 0x3c56914
  7d036e: 88 1d 15 69 c5 03            	movb	%bl, 0x3c56915
  7d0374: 66 89 3d 16 69 c5 03         	movw	%di, 0x3c56916
  7d037b: c7 05 18 69 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56918  # imm = 0x3F000000
  7d0385: c7 05 1c 69 c5 03 9a 99 99 3e	movl	$0x3e99999a, 0x3c5691c  # imm = 0x3E99999A
  7d038f: 8b a8 08 01 00 00            	movl	0x108(%eax), %ebp
  7d0395: 89 2d 20 69 c5 03            	movl	%ebp, 0x3c56920
  7d039b: 88 0d 24 69 c5 03            	movb	%cl, 0x3c56924
  7d03a1: 88 15 25 69 c5 03            	movb	%dl, 0x3c56925
  7d03a7: 66 c7 05 26 69 c5 03 66 00   	movw	$0x66, 0x3c56926
  7d03b0: c7 05 28 69 c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56928  # imm = 0x3F000000
  7d03ba: c7 05 2c 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5692c  # imm = 0x3F800000
  7d03c4: 8b a8 0c 01 00 00            	movl	0x10c(%eax), %ebp
  7d03ca: 89 2d 30 69 c5 03            	movl	%ebp, 0x3c56930
  7d03d0: 88 0d 34 69 c5 03            	movb	%cl, 0x3c56934
  7d03d6: 88 1d 35 69 c5 03            	movb	%bl, 0x3c56935
  7d03dc: 66 c7 05 36 69 c5 03 72 00   	movw	$0x72, 0x3c56936
  7d03e5: c7 05 38 69 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56938  # imm = 0x3D800000
  7d03ef: c7 05 3c 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5693c  # imm = 0x3F800000
  7d03f9: 8b a8 10 01 00 00            	movl	0x110(%eax), %ebp
  7d03ff: 89 2d 40 69 c5 03            	movl	%ebp, 0x3c56940
  7d0405: 88 0d 44 69 c5 03            	movb	%cl, 0x3c56944
  7d040b: 88 15 45 69 c5 03            	movb	%dl, 0x3c56945
  7d0411: 66 c7 05 46 69 c5 03 66 00   	movw	$0x66, 0x3c56946
  7d041a: c7 05 48 69 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56948  # imm = 0x3D800000
  7d0424: c7 05 4c 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5694c  # imm = 0x3F800000
  7d042e: 8b a8 14 01 00 00            	movl	0x114(%eax), %ebp
  7d0434: 89 2d 50 69 c5 03            	movl	%ebp, 0x3c56950
  7d043a: 88 0d 54 69 c5 03            	movb	%cl, 0x3c56954
  7d0440: 88 15 55 69 c5 03            	movb	%dl, 0x3c56955
  7d0446: 66 89 3d 56 69 c5 03         	movw	%di, 0x3c56956
  7d044d: c7 05 58 69 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56958  # imm = 0x3D800000
  7d0457: c7 05 5c 69 c5 03 9a 99 59 3f	movl	$0x3f59999a, 0x3c5695c  # imm = 0x3F59999A
  7d0461: 8b a8 18 01 00 00            	movl	0x118(%eax), %ebp
  7d0467: 89 2d 60 69 c5 03            	movl	%ebp, 0x3c56960
  7d046d: 88 0d 64 69 c5 03            	movb	%cl, 0x3c56964
  7d0473: 88 15 65 69 c5 03            	movb	%dl, 0x3c56965
  7d0479: 66 89 3d 66 69 c5 03         	movw	%di, 0x3c56966
  7d0480: c7 05 68 69 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56968  # imm = 0x3D800000
  7d048a: c7 05 6c 69 c5 03 9a 99 59 3f	movl	$0x3f59999a, 0x3c5696c  # imm = 0x3F59999A
  7d0494: 8b a8 b4 01 00 00            	movl	0x1b4(%eax), %ebp
  7d049a: 89 2d 70 69 c5 03            	movl	%ebp, 0x3c56970
  7d04a0: 88 0d 74 69 c5 03            	movb	%cl, 0x3c56974
  7d04a6: 88 15 75 69 c5 03            	movb	%dl, 0x3c56975
  7d04ac: 66 89 3d 76 69 c5 03         	movw	%di, 0x3c56976
  7d04b3: c7 05 78 69 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56978  # imm = 0x3D800000
  7d04bd: c7 05 7c 69 c5 03 9a 99 59 3f	movl	$0x3f59999a, 0x3c5697c  # imm = 0x3F59999A
  7d04c7: 8b a8 b4 01 00 00            	movl	0x1b4(%eax), %ebp
  7d04cd: 89 2d 80 69 c5 03            	movl	%ebp, 0x3c56980
  7d04d3: 88 0d 84 69 c5 03            	movb	%cl, 0x3c56984
  7d04d9: c6 05 85 69 c5 03 06         	movb	$0x6, 0x3c56985
  7d04e0: 66 89 3d 86 69 c5 03         	movw	%di, 0x3c56986
  7d04e7: c7 05 88 69 c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56988  # imm = 0x3D800000
  7d04f1: c7 05 8c 69 c5 03 9a 99 59 3f	movl	$0x3f59999a, 0x3c5698c  # imm = 0x3F59999A
  7d04fb: 8b a8 1c 01 00 00            	movl	0x11c(%eax), %ebp
  7d0501: 89 2d 90 69 c5 03            	movl	%ebp, 0x3c56990
  7d0507: 88 0d 94 69 c5 03            	movb	%cl, 0x3c56994
  7d050d: 88 15 95 69 c5 03            	movb	%dl, 0x3c56995
  7d0513: 66 c7 05 96 69 c5 03 79 00   	movw	$0x79, 0x3c56996
  7d051c: c7 05 98 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56998  # imm = 0x3F800000
  7d0526: c7 05 9c 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c5699c  # imm = 0x3F800000
  7d0530: 8b a8 ac 01 00 00            	movl	0x1ac(%eax), %ebp
  7d0536: 89 2d a0 69 c5 03            	movl	%ebp, 0x3c569a0
  7d053c: 88 0d a4 69 c5 03            	movb	%cl, 0x3c569a4
  7d0542: 88 15 a5 69 c5 03            	movb	%dl, 0x3c569a5
  7d0548: 66 c7 05 a6 69 c5 03 7a 00   	movw	$0x7a, 0x3c569a6
  7d0551: c7 05 a8 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c569a8  # imm = 0x3F800000
  7d055b: c7 05 ac 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c569ac  # imm = 0x3F800000
  7d0565: 8b a8 b0 01 00 00            	movl	0x1b0(%eax), %ebp
  7d056b: 89 2d b0 69 c5 03            	movl	%ebp, 0x3c569b0
  7d0571: 88 0d b4 69 c5 03            	movb	%cl, 0x3c569b4
  7d0577: 88 15 b5 69 c5 03            	movb	%dl, 0x3c569b5
  7d057d: 66 89 3d b6 69 c5 03         	movw	%di, 0x3c569b6
  7d0584: c7 05 b8 69 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c569b8  # imm = 0x3E800000
  7d058e: c7 05 bc 69 c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c569bc  # imm = 0x3F800000
  7d0598: 8b a8 20 01 00 00            	movl	0x120(%eax), %ebp
  7d059e: 89 2d c0 69 c5 03            	movl	%ebp, 0x3c569c0
  7d05a4: 88 0d c4 69 c5 03            	movb	%cl, 0x3c569c4
  7d05aa: 88 15 c5 69 c5 03            	movb	%dl, 0x3c569c5
  7d05b0: 66 89 3d c6 69 c5 03         	movw	%di, 0x3c569c6
  7d05b7: c7 05 c8 69 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c569c8  # imm = 0x3E800000
  7d05c1: c7 05 cc 69 c5 03 00 00 c0 3f	movl	$0x3fc00000, 0x3c569cc  # imm = 0x3FC00000
  7d05cb: 8b b8 fc 00 00 00            	movl	0xfc(%eax), %edi
  7d05d1: 89 3d d0 69 c5 03            	movl	%edi, 0x3c569d0
  7d05d7: 88 0d d4 69 c5 03            	movb	%cl, 0x3c569d4
  7d05dd: 88 1d d5 69 c5 03            	movb	%bl, 0x3c569d5
  7d05e3: 66 c7 05 d6 69 c5 03 7c 00   	movw	$0x7c, 0x3c569d6
  7d05ec: c7 05 d8 69 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c569d8  # imm = 0x3E800000
  7d05f6: c7 05 dc 69 c5 03 00 00 c0 3f	movl	$0x3fc00000, 0x3c569dc  # imm = 0x3FC00000
  7d0600: 8b 78 38                     	movl	0x38(%eax), %edi
  7d0603: 89 3d e0 69 c5 03            	movl	%edi, 0x3c569e0
  7d0609: c6 05 e4 69 c5 03 4a         	movb	$0x4a, 0x3c569e4
  7d0610: c6 05 e5 69 c5 03 0a         	movb	$0xa, 0x3c569e5
  7d0617: 66 c7 05 e6 69 c5 03 7d 00   	movw	$0x7d, 0x3c569e6
  7d0620: c7 05 e8 69 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c569e8  # imm = 0x3E800000
  7d062a: c7 05 ec 69 c5 03 cd cc 4c 3e	movl	$0x3e4ccccd, 0x3c569ec  # imm = 0x3E4CCCCD
  7d0634: 8b b8 1c 02 00 00            	movl	0x21c(%eax), %edi
  7d063a: 89 3d f0 69 c5 03            	movl	%edi, 0x3c569f0
  7d0640: 88 0d f4 69 c5 03            	movb	%cl, 0x3c569f4
  7d0646: 88 15 f5 69 c5 03            	movb	%dl, 0x3c569f5
  7d064c: c7 05 f8 69 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c569f8  # imm = 0x3E800000
  7d0656: c7 05 fc 69 c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c569fc  # imm = 0x3E800000
  7d0660: bf 7f 00 00 00               	movl	$0x7f, %edi
  7d0665: 66 89 3d f6 69 c5 03         	movw	%di, 0x3c569f6
  7d066c: 8b a8 20 02 00 00            	movl	0x220(%eax), %ebp
  7d0672: 66 89 3d 06 6a c5 03         	movw	%di, 0x3c56a06
  7d0679: 89 2d 00 6a c5 03            	movl	%ebp, 0x3c56a00
  7d067f: 88 0d 04 6a c5 03            	movb	%cl, 0x3c56a04
  7d0685: 88 1d 05 6a c5 03            	movb	%bl, 0x3c56a05
  7d068b: c7 05 08 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a08  # imm = 0x3E800000
  7d0695: c7 05 0c 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a0c  # imm = 0x3E800000
  7d069f: 8b b8 24 02 00 00            	movl	0x224(%eax), %edi
  7d06a5: 89 3d 10 6a c5 03            	movl	%edi, 0x3c56a10
  7d06ab: 66 89 35 16 6a c5 03         	movw	%si, 0x3c56a16
  7d06b2: 88 0d 14 6a c5 03            	movb	%cl, 0x3c56a14
  7d06b8: 88 15 15 6a c5 03            	movb	%dl, 0x3c56a15
  7d06be: c7 05 18 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a18  # imm = 0x3E800000
  7d06c8: c7 05 1c 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a1c  # imm = 0x3E800000
  7d06d2: 8b b8 18 02 00 00            	movl	0x218(%eax), %edi
  7d06d8: 89 3d 20 6a c5 03            	movl	%edi, 0x3c56a20
  7d06de: 66 89 35 26 6a c5 03         	movw	%si, 0x3c56a26
  7d06e5: 88 0d 24 6a c5 03            	movb	%cl, 0x3c56a24
  7d06eb: 88 15 25 6a c5 03            	movb	%dl, 0x3c56a25
  7d06f1: c7 05 28 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a28  # imm = 0x3E800000
  7d06fb: c7 05 2c 6a c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56a2c  # imm = 0x3F000000
  7d0705: 8b b8 44 02 00 00            	movl	0x244(%eax), %edi
  7d070b: 89 3d 30 6a c5 03            	movl	%edi, 0x3c56a30
  7d0711: 88 0d 34 6a c5 03            	movb	%cl, 0x3c56a34
  7d0717: 88 15 35 6a c5 03            	movb	%dl, 0x3c56a35
  7d071d: c7 05 38 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56a38  # imm = 0x3F800000
  7d0727: c7 05 3c 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56a3c  # imm = 0x3F800000
  7d0731: bf 83 00 00 00               	movl	$0x83, %edi
  7d0736: 66 89 3d 36 6a c5 03         	movw	%di, 0x3c56a36
  7d073d: 8b a8 48 02 00 00            	movl	0x248(%eax), %ebp
  7d0743: 89 2d 40 6a c5 03            	movl	%ebp, 0x3c56a40
  7d0749: 88 0d 44 6a c5 03            	movb	%cl, 0x3c56a44
  7d074f: 88 1d 45 6a c5 03            	movb	%bl, 0x3c56a45
  7d0755: 66 89 3d 46 6a c5 03         	movw	%di, 0x3c56a46
  7d075c: c7 05 48 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56a48  # imm = 0x3F800000
  7d0766: c7 05 4c 6a c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56a4c  # imm = 0x3F000000
  7d0770: 8b b8 4c 02 00 00            	movl	0x24c(%eax), %edi
  7d0776: 66 89 35 56 6a c5 03         	movw	%si, 0x3c56a56
  7d077d: 89 3d 50 6a c5 03            	movl	%edi, 0x3c56a50
  7d0783: 88 0d 54 6a c5 03            	movb	%cl, 0x3c56a54
  7d0789: 88 15 55 6a c5 03            	movb	%dl, 0x3c56a55
  7d078f: c7 05 58 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56a58  # imm = 0x3F800000
  7d0799: c7 05 5c 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56a5c  # imm = 0x3F800000
  7d07a3: 8b b8 50 02 00 00            	movl	0x250(%eax), %edi
  7d07a9: 66 89 35 66 6a c5 03         	movw	%si, 0x3c56a66
  7d07b0: 89 3d 60 6a c5 03            	movl	%edi, 0x3c56a60
  7d07b6: 88 0d 64 6a c5 03            	movb	%cl, 0x3c56a64
  7d07bc: 88 15 65 6a c5 03            	movb	%dl, 0x3c56a65
  7d07c2: c7 05 68 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56a68  # imm = 0x3F800000
  7d07cc: c7 05 6c 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56a6c  # imm = 0x3F800000
  7d07d6: 8b b0 08 02 00 00            	movl	0x208(%eax), %esi
  7d07dc: 89 35 70 6a c5 03            	movl	%esi, 0x3c56a70
  7d07e2: be 86 00 00 00               	movl	$0x86, %esi
  7d07e7: 88 0d 74 6a c5 03            	movb	%cl, 0x3c56a74
  7d07ed: 88 1d 75 6a c5 03            	movb	%bl, 0x3c56a75
  7d07f3: 66 89 35 76 6a c5 03         	movw	%si, 0x3c56a76
  7d07fa: c7 05 78 6a c5 03 00 00 80 3d	movl	$0x3d800000, 0x3c56a78  # imm = 0x3D800000
  7d0804: c7 05 7c 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a7c  # imm = 0x3E800000
  7d080e: 8b b8 0c 02 00 00            	movl	0x20c(%eax), %edi
  7d0814: 89 3d 80 6a c5 03            	movl	%edi, 0x3c56a80
  7d081a: 88 0d 84 6a c5 03            	movb	%cl, 0x3c56a84
  7d0820: 88 1d 85 6a c5 03            	movb	%bl, 0x3c56a85
  7d0826: 66 c7 05 86 6a c5 03 87 00   	movw	$0x87, 0x3c56a86
  7d082f: c7 05 88 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a88  # imm = 0x3E800000
  7d0839: c7 05 8c 6a c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56a8c  # imm = 0x3F000000
  7d0843: 8b b8 10 02 00 00            	movl	0x210(%eax), %edi
  7d0849: 89 3d 90 6a c5 03            	movl	%edi, 0x3c56a90
  7d084f: 88 0d 94 6a c5 03            	movb	%cl, 0x3c56a94
  7d0855: 88 1d 95 6a c5 03            	movb	%bl, 0x3c56a95
  7d085b: 66 c7 05 96 6a c5 03 88 00   	movw	$0x88, 0x3c56a96
  7d0864: c7 05 98 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56a98  # imm = 0x3E800000
  7d086e: c7 05 9c 6a c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56a9c  # imm = 0x3F000000
  7d0878: 8b b8 14 02 00 00            	movl	0x214(%eax), %edi
  7d087e: 89 3d a0 6a c5 03            	movl	%edi, 0x3c56aa0
  7d0884: 88 1d a5 6a c5 03            	movb	%bl, 0x3c56aa5
  7d088a: 88 0d a4 6a c5 03            	movb	%cl, 0x3c56aa4
  7d0890: 66 c7 05 a6 6a c5 03 89 00   	movw	$0x89, 0x3c56aa6
  7d0899: c7 05 a8 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56aa8  # imm = 0x3E800000
  7d08a3: c7 05 ac 6a c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56aac  # imm = 0x3F000000
  7d08ad: 8b b8 14 02 00 00            	movl	0x214(%eax), %edi
  7d08b3: 89 3d b0 6a c5 03            	movl	%edi, 0x3c56ab0
  7d08b9: 88 1d b5 6a c5 03            	movb	%bl, 0x3c56ab5
  7d08bf: 88 0d b4 6a c5 03            	movb	%cl, 0x3c56ab4
  7d08c5: 66 c7 05 b6 6a c5 03 8a 00   	movw	$0x8a, 0x3c56ab6
  7d08ce: c7 05 b8 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56ab8  # imm = 0x3E800000
  7d08d8: c7 05 bc 6a c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56abc  # imm = 0x3F000000
  7d08e2: 8b b8 28 02 00 00            	movl	0x228(%eax), %edi
  7d08e8: 89 3d c0 6a c5 03            	movl	%edi, 0x3c56ac0
  7d08ee: 88 1d c5 6a c5 03            	movb	%bl, 0x3c56ac5
  7d08f4: 88 0d c4 6a c5 03            	movb	%cl, 0x3c56ac4
  7d08fa: 66 c7 05 c6 6a c5 03 8b 00   	movw	$0x8b, 0x3c56ac6
  7d0903: c7 05 c8 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56ac8  # imm = 0x3E800000
  7d090d: c7 05 cc 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56acc  # imm = 0x3F800000
  7d0917: 8b b8 2c 02 00 00            	movl	0x22c(%eax), %edi
  7d091d: 89 3d d0 6a c5 03            	movl	%edi, 0x3c56ad0
  7d0923: 88 0d d4 6a c5 03            	movb	%cl, 0x3c56ad4
  7d0929: 88 15 d5 6a c5 03            	movb	%dl, 0x3c56ad5
  7d092f: c7 05 d8 6a c5 03 00 00 00 3e	movl	$0x3e000000, 0x3c56ad8  # imm = 0x3E000000
  7d0939: c7 05 dc 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56adc  # imm = 0x3F800000
  7d0943: bf 8d 00 00 00               	movl	$0x8d, %edi
  7d0948: 66 89 3d d6 6a c5 03         	movw	%di, 0x3c56ad6
  7d094f: 8b a8 30 02 00 00            	movl	0x230(%eax), %ebp
  7d0955: 89 2d e0 6a c5 03            	movl	%ebp, 0x3c56ae0
  7d095b: 88 1d e5 6a c5 03            	movb	%bl, 0x3c56ae5
  7d0961: 66 89 3d e6 6a c5 03         	movw	%di, 0x3c56ae6
  7d0968: 88 0d e4 6a c5 03            	movb	%cl, 0x3c56ae4
  7d096e: c7 05 e8 6a c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56ae8  # imm = 0x3E800000
  7d0978: c7 05 ec 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56aec  # imm = 0x3F800000
  7d0982: 8b b8 34 02 00 00            	movl	0x234(%eax), %edi
  7d0988: 89 3d f0 6a c5 03            	movl	%edi, 0x3c56af0
  7d098e: 66 89 35 f6 6a c5 03         	movw	%si, 0x3c56af6
  7d0995: 88 0d f4 6a c5 03            	movb	%cl, 0x3c56af4
  7d099b: 88 15 f5 6a c5 03            	movb	%dl, 0x3c56af5
  7d09a1: c7 05 f8 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56af8  # imm = 0x3F800000
  7d09ab: c7 05 fc 6a c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56afc  # imm = 0x3F800000
  7d09b5: 8b b8 3c 02 00 00            	movl	0x23c(%eax), %edi
  7d09bb: 89 3d 00 6b c5 03            	movl	%edi, 0x3c56b00
  7d09c1: 88 0d 04 6b c5 03            	movb	%cl, 0x3c56b04
  7d09c7: c6 05 05 6b c5 03 05         	movb	$0x5, 0x3c56b05
  7d09ce: 66 c7 05 06 6b c5 03 8f 00   	movw	$0x8f, 0x3c56b06
  7d09d7: c7 05 08 6b c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56b08  # imm = 0x3F000000
  7d09e1: c7 05 0c 6b c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56b0c  # imm = 0x3F800000
  7d09eb: 8b b8 40 02 00 00            	movl	0x240(%eax), %edi
  7d09f1: 89 3d 10 6b c5 03            	movl	%edi, 0x3c56b10
  7d09f7: 66 89 35 16 6b c5 03         	movw	%si, 0x3c56b16
  7d09fe: 5f                           	popl	%edi
  7d09ff: 88 0d 14 6b c5 03            	movb	%cl, 0x3c56b14
  7d0a05: 88 15 15 6b c5 03            	movb	%dl, 0x3c56b15
  7d0a0b: c7 05 18 6b c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56b18  # imm = 0x3F000000
  7d0a15: c7 05 1c 6b c5 03 00 00 80 3f	movl	$0x3f800000, 0x3c56b1c  # imm = 0x3F800000
  7d0a1f: 8b 90 38 02 00 00            	movl	0x238(%eax), %edx
  7d0a25: 5e                           	popl	%esi
  7d0a26: 89 15 20 6b c5 03            	movl	%edx, 0x3c56b20
  7d0a2c: c6 05 24 6b c5 03 01         	movb	$0x1, 0x3c56b24
  7d0a33: c6 05 25 6b c5 03 0a         	movb	$0xa, 0x3c56b25
  7d0a3a: 66 c7 05 26 6b c5 03 91 00   	movw	$0x91, 0x3c56b26
  7d0a43: c7 05 28 6b c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56b28  # imm = 0x3E800000
  7d0a4d: c7 05 2c 6b c5 03 cd cc 4c 3e	movl	$0x3e4ccccd, 0x3c56b2c  # imm = 0x3E4CCCCD
  7d0a57: 8b 00                        	movl	(%eax), %eax
  7d0a59: 5d                           	popl	%ebp
  7d0a5a: 88 1d 35 6b c5 03            	movb	%bl, 0x3c56b35
  7d0a60: a3 30 6b c5 03               	movl	%eax, 0x3c56b30
  7d0a65: 88 0d 34 6b c5 03            	movb	%cl, 0x3c56b34
  7d0a6b: 66 c7 05 36 6b c5 03 92 00   	movw	$0x92, 0x3c56b36
  7d0a74: c7 05 38 6b c5 03 00 00 80 3e	movl	$0x3e800000, 0x3c56b38  # imm = 0x3E800000
  7d0a7e: c7 05 3c 6b c5 03 00 00 00 3f	movl	$0x3f000000, 0x3c56b3c  # imm = 0x3F000000
  7d0a88: 5b                           	popl	%ebx
  7d0a89: c3                           	retl
