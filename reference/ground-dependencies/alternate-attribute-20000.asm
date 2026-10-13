
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  44b0e0: 83 ec 44                     	subl	$0x44, %esp
  44b0e3: 55                           	pushl	%ebp
  44b0e4: 8b 6c 24 4c                  	movl	0x4c(%esp), %ebp
  44b0e8: 56                           	pushl	%esi
  44b0e9: 8b f0                        	movl	%eax, %esi
  44b0eb: d9 46 50                     	flds	0x50(%esi)
  44b0ee: 57                           	pushl	%edi
  44b0ef: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  44b0f5: 8b f9                        	movl	%ecx, %edi
  44b0f7: 8d 45 10                     	leal	0x10(%ebp), %eax
  44b0fa: 8b 08                        	movl	(%eax), %ecx
  44b0fc: 8b 50 04                     	movl	0x4(%eax), %edx
  44b0ff: 8b 40 08                     	movl	0x8(%eax), %eax
  44b102: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  44b106: d8 6c 24 14                  	fsubrs	0x14(%esp)
  44b10a: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  44b10e: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  44b112: d9 5c 24 14                  	fstps	0x14(%esp)
  44b116: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  44b11a: d9 46 54                     	flds	0x54(%esi)
  44b11d: 51                           	pushl	%ecx
  44b11e: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  44b124: 57                           	pushl	%edi
  44b125: c7 44 24 30 00 00 00 00      	movl	$0x0, 0x30(%esp)
  44b12d: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  44b135: d8 6c 24 20                  	fsubrs	0x20(%esp)
  44b139: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%esp)
  44b141: d9 5c 24 20                  	fstps	0x20(%esp)
  44b145: d9 46 58                     	flds	0x58(%esi)
  44b148: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  44b14e: d8 6c 24 24                  	fsubrs	0x24(%esp)
  44b152: d9 5c 24 24                  	fstps	0x24(%esp)
  44b156: e8 a5 3a ff ff               	calll	0x43ec00 <.text+0x3dc00>
  44b15b: 8d 54 24 5c                  	leal	0x5c(%esp), %edx
  44b15f: 52                           	pushl	%edx
  44b160: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  44b164: 50                           	pushl	%eax
  44b165: 57                           	pushl	%edi
  44b166: e8 55 0c ff ff               	calll	0x43bdc0 <.text+0x3adc0>
  44b16b: d9 46 38                     	flds	0x38(%esi)
  44b16e: 8b c8                        	movl	%eax, %ecx
  44b170: 83 c4 14                     	addl	$0x14, %esp
  44b173: 85 c9                        	testl	%ecx, %ecx
  44b175: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  44b179: 75 2e                        	jne	0x44b1a9 <.text+0x4a1a9>
  44b17b: d8 9e c4 00 00 00            	fcomps	0xc4(%esi)
  44b181: df e0                        	fnstsw	%ax
  44b183: f6 c4 41                     	testb	$0x41, %ah
  44b186: 74 62                        	je	0x44b1ea <.text+0x4a1ea>
  44b188: d9 46 38                     	flds	0x38(%esi)
  44b18b: d8 9e ac 00 00 00            	fcomps	0xac(%esi)
  44b191: d9 46 38                     	flds	0x38(%esi)
  44b194: df e0                        	fnstsw	%ax
  44b196: f6 c4 41                     	testb	$0x41, %ah
  44b199: 75 42                        	jne	0x44b1dd <.text+0x4a1dd>
  44b19b: d8 a6 ac 00 00 00            	fsubs	0xac(%esi)
  44b1a1: d8 8e f0 00 00 00            	fmuls	0xf0(%esi)
  44b1a7: eb 4a                        	jmp	0x44b1f3 <.text+0x4a1f3>
  44b1a9: d8 9e ac 00 00 00            	fcomps	0xac(%esi)
  44b1af: df e0                        	fnstsw	%ax
  44b1b1: f6 c4 41                     	testb	$0x41, %ah
  44b1b4: 75 d2                        	jne	0x44b188 <.text+0x4a188>
  44b1b6: d9 46 1c                     	flds	0x1c(%esi)
  44b1b9: d8 1d 88 00 7e 00            	fcomps	0x7e0088
  44b1bf: df e0                        	fnstsw	%ax
  44b1c1: f6 c4 41                     	testb	$0x41, %ah
  44b1c4: 75 c2                        	jne	0x44b188 <.text+0x4a188>
  44b1c6: d9 46 38                     	flds	0x38(%esi)
  44b1c9: d8 a6 ac 00 00 00            	fsubs	0xac(%esi)
  44b1cf: d8 8e f0 00 00 00            	fmuls	0xf0(%esi)
  44b1d5: d8 0d 9c 01 7e 00            	fmuls	0x7e019c
  44b1db: eb 16                        	jmp	0x44b1f3 <.text+0x4a1f3>
  44b1dd: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b1e3: df e0                        	fnstsw	%ax
  44b1e5: f6 c4 05                     	testb	$0x5, %ah
  44b1e8: 7a 11                        	jp	0x44b1fb <.text+0x4a1fb>
  44b1ea: d9 86 f0 00 00 00            	flds	0xf0(%esi)
  44b1f0: d8 4e 38                     	fmuls	0x38(%esi)
  44b1f3: d8 44 24 14                  	fadds	0x14(%esp)
  44b1f7: d9 5c 24 14                  	fstps	0x14(%esp)
  44b1fb: 85 c9                        	testl	%ecx, %ecx
  44b1fd: d9 86 f4 00 00 00            	flds	0xf4(%esi)
  44b203: d8 4e 3c                     	fmuls	0x3c(%esi)
  44b206: d8 44 24 18                  	fadds	0x18(%esp)
  44b20a: d9 5c 24 18                  	fstps	0x18(%esp)
  44b20e: d9 86 f8 00 00 00            	flds	0xf8(%esi)
  44b214: d8 4e 40                     	fmuls	0x40(%esi)
  44b217: d8 44 24 1c                  	fadds	0x1c(%esp)
  44b21b: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44b21f: 75 79                        	jne	0x44b29a <.text+0x4a29a>
  44b221: d9 46 1c                     	flds	0x1c(%esi)
  44b224: d8 1d 38 01 7e 00            	fcomps	0x7e0138
  44b22a: df e0                        	fnstsw	%ax
  44b22c: f6 c4 05                     	testb	$0x5, %ah
  44b22f: 7a 1b                        	jp	0x44b24c <.text+0x4a24c>
  44b231: 56                           	pushl	%esi
  44b232: 55                           	pushl	%ebp
  44b233: 57                           	pushl	%edi
  44b234: e8 a7 df ff ff               	calll	0x4491e0 <.text+0x481e0>
  44b239: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  44b23d: 83 c4 0c                     	addl	$0xc, %esp
  44b240: c7 45 34 00 00 00 00         	movl	$0x0, 0x34(%ebp)
  44b247: e9 fd 02 00 00               	jmp	0x44b549 <.text+0x4a549>
  44b24c: d9 46 38                     	flds	0x38(%esi)
  44b24f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b255: df e0                        	fnstsw	%ax
  44b257: f6 c4 41                     	testb	$0x41, %ah
  44b25a: 75 16                        	jne	0x44b272 <.text+0x4a272>
  44b25c: 8b 96 e0 00 00 00            	movl	0xe0(%esi), %edx
  44b262: 89 54 24 20                  	movl	%edx, 0x20(%esp)
  44b266: c7 45 34 00 00 00 00         	movl	$0x0, 0x34(%ebp)
  44b26d: e9 d7 02 00 00               	jmp	0x44b549 <.text+0x4a549>
  44b272: d9 46 38                     	flds	0x38(%esi)
  44b275: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b27b: df e0                        	fnstsw	%ax
  44b27d: f6 c4 05                     	testb	$0x5, %ah
  44b280: 7a 0c                        	jp	0x44b28e <.text+0x4a28e>
  44b282: d9 86 e0 00 00 00            	flds	0xe0(%esi)
  44b288: d9 e0                        	fchs
  44b28a: d9 5c 24 20                  	fstps	0x20(%esp)
  44b28e: c7 45 34 00 00 00 00         	movl	$0x0, 0x34(%ebp)
  44b295: e9 af 02 00 00               	jmp	0x44b549 <.text+0x4a549>
  44b29a: 33 c0                        	xorl	%eax, %eax
  44b29c: 66 8b 47 04                  	movw	0x4(%edi), %ax
  44b2a0: 53                           	pushl	%ebx
  44b2a1: 25 00 20 00 00               	andl	$0x2000, %eax           # imm = 0x2000
  44b2a6: 66 85 c0                     	testw	%ax, %ax
  44b2a9: 74 16                        	je	0x44b2c1 <.text+0x4a2c1>
  44b2ab: 8b c6                        	movl	%esi, %eax
  44b2ad: 8b dd                        	movl	%ebp, %ebx
  44b2af: e8 7c 0e ff ff               	calll	0x43c130 <.text+0x3b130>
  44b2b4: 8b 5d 20                     	movl	0x20(%ebp), %ebx
  44b2b7: c7 44 24 28 cd cc 4c bf      	movl	$0xbf4ccccd, 0x28(%esp) # imm = 0xBF4CCCCD
  44b2bf: eb 04                        	jmp	0x44b2c5 <.text+0x4a2c5>
  44b2c1: 8b 5c 24 14                  	movl	0x14(%esp), %ebx
  44b2c5: d9 46 38                     	flds	0x38(%esi)
  44b2c8: d8 9e ac 00 00 00            	fcomps	0xac(%esi)
  44b2ce: d9 46 38                     	flds	0x38(%esi)
  44b2d1: df e0                        	fnstsw	%ax
  44b2d3: f6 c4 05                     	testb	$0x5, %ah
  44b2d6: 0f 8a bd 00 00 00            	jp	0x44b399 <.text+0x4a399>
  44b2dc: d8 9e c0 00 00 00            	fcomps	0xc0(%esi)
  44b2e2: df e0                        	fnstsw	%ax
  44b2e4: f6 c4 05                     	testb	$0x5, %ah
  44b2e7: 7a 1f                        	jp	0x44b308 <.text+0x4a308>
  44b2e9: d9 44 24 58                  	flds	0x58(%esp)
  44b2ed: d8 1d 98 cc 7d 00            	fcomps	0x7dcc98
  44b2f3: df e0                        	fnstsw	%ax
  44b2f5: f6 c4 41                     	testb	$0x41, %ah
  44b2f8: 74 56                        	je	0x44b350 <.text+0x4a350>
  44b2fa: d9 86 c0 00 00 00            	flds	0xc0(%esi)
  44b300: d8 0d 50 d5 7d 00            	fmuls	0x7dd550
  44b306: eb 3e                        	jmp	0x44b346 <.text+0x4a346>
  44b308: d9 46 38                     	flds	0x38(%esi)
  44b30b: d8 9e c4 00 00 00            	fcomps	0xc4(%esi)
  44b311: df e0                        	fnstsw	%ax
  44b313: f6 c4 05                     	testb	$0x5, %ah
  44b316: 7a 4e                        	jp	0x44b366 <.text+0x4a366>
  44b318: d9 44 24 58                  	flds	0x58(%esp)
  44b31c: d8 1d 78 d5 7d 00            	fcomps	0x7dd578
  44b322: df e0                        	fnstsw	%ax
  44b324: f6 c4 41                     	testb	$0x41, %ah
  44b327: 75 0f                        	jne	0x44b338 <.text+0x4a338>
  44b329: d9 44 24 58                  	flds	0x58(%esp)
  44b32d: d8 8e d8 00 00 00            	fmuls	0xd8(%esi)
  44b333: e9 9a 00 00 00               	jmp	0x44b3d2 <.text+0x4a3d2>
  44b338: d9 86 c4 00 00 00            	flds	0xc4(%esi)
  44b33e: dc c0                        	fadd	%st, %st(0)
  44b340: d8 0d 98 cc 7d 00            	fmuls	0x7dcc98
  44b346: d8 5e 38                     	fcomps	0x38(%esi)
  44b349: df e0                        	fnstsw	%ax
  44b34b: f6 c4 41                     	testb	$0x41, %ah
  44b34e: 75 0c                        	jne	0x44b35c <.text+0x4a35c>
  44b350: d9 44 24 58                  	flds	0x58(%esp)
  44b354: d8 8e d8 00 00 00            	fmuls	0xd8(%esi)
  44b35a: eb 76                        	jmp	0x44b3d2 <.text+0x4a3d2>
  44b35c: c7 44 24 58 00 00 00 00      	movl	$0x0, 0x58(%esp)
  44b364: eb 70                        	jmp	0x44b3d6 <.text+0x4a3d6>
  44b366: d9 46 38                     	flds	0x38(%esi)
  44b369: d8 9e c8 00 00 00            	fcomps	0xc8(%esi)
  44b36f: d9 44 24 58                  	flds	0x58(%esp)
  44b373: df e0                        	fnstsw	%ax
  44b375: f6 c4 05                     	testb	$0x5, %ah
  44b378: 7a 40                        	jp	0x44b3ba <.text+0x4a3ba>
  44b37a: d8 1d f0 ff 7d 00            	fcomps	0x7dfff0
  44b380: d9 44 24 58                  	flds	0x58(%esp)
  44b384: d8 8e d8 00 00 00            	fmuls	0xd8(%esi)
  44b38a: df e0                        	fnstsw	%ax
  44b38c: f6 c4 41                     	testb	$0x41, %ah
  44b38f: 74 41                        	je	0x44b3d2 <.text+0x4a3d2>
  44b391: d8 0d 40 d5 7d 00            	fmuls	0x7dd540
  44b397: eb 39                        	jmp	0x44b3d2 <.text+0x4a3d2>
  44b399: d8 9e ac 00 00 00            	fcomps	0xac(%esi)
  44b39f: df e0                        	fnstsw	%ax
  44b3a1: f6 c4 01                     	testb	$0x1, %ah
  44b3a4: 75 1c                        	jne	0x44b3c2 <.text+0x4a3c2>
  44b3a6: d9 46 1c                     	flds	0x1c(%esi)
  44b3a9: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b3af: df e0                        	fnstsw	%ax
  44b3b1: f6 c4 05                     	testb	$0x5, %ah
  44b3b4: 7a 0c                        	jp	0x44b3c2 <.text+0x4a3c2>
  44b3b6: d9 44 24 58                  	flds	0x58(%esp)
  44b3ba: d8 8e d8 00 00 00            	fmuls	0xd8(%esi)
  44b3c0: eb 10                        	jmp	0x44b3d2 <.text+0x4a3d2>
  44b3c2: d9 44 24 58                  	flds	0x58(%esp)
  44b3c6: d8 8e d8 00 00 00            	fmuls	0xd8(%esi)
  44b3cc: d8 0d 50 d5 7d 00            	fmuls	0x7dd550
  44b3d2: d9 5c 24 58                  	fstps	0x58(%esp)
  44b3d6: 8b 4f 18                     	movl	0x18(%edi), %ecx
  44b3d9: 53                           	pushl	%ebx
  44b3da: 51                           	pushl	%ecx
  44b3db: e8 d0 cf fe ff               	calll	0x4383b0 <.text+0x373b0>
  44b3e0: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44b3e6: d8 56 38                     	fcoms	0x38(%esi)
  44b3e9: 8b c8                        	movl	%eax, %ecx
  44b3eb: 83 c4 08                     	addl	$0x8, %esp
  44b3ee: df e0                        	fnstsw	%ax
  44b3f0: f6 c4 44                     	testb	$0x44, %ah
  44b3f3: 7a 20                        	jp	0x44b415 <.text+0x4a415>
  44b3f5: 81 f9 00 10 00 00            	cmpl	$0x1000, %ecx           # imm = 0x1000
  44b3fb: 7e 18                        	jle	0x44b415 <.text+0x4a415>
  44b3fd: 53                           	pushl	%ebx
  44b3fe: d9 5c 24 5c                  	fstps	0x5c(%esp)
  44b402: 55                           	pushl	%ebp
  44b403: 8b c6                        	movl	%esi, %eax
  44b405: 89 5d 20                     	movl	%ebx, 0x20(%ebp)
  44b408: e8 e3 89 ff ff               	calll	0x443df0 <.text+0x42df0>
  44b40d: 83 c4 08                     	addl	$0x8, %esp
  44b410: e9 22 01 00 00               	jmp	0x44b537 <.text+0x4a537>
  44b415: 0f b6 47 08                  	movzbl	0x8(%edi), %eax
  44b419: 6b c0 34                     	imull	$0x34, %eax, %eax
  44b41c: 0f bf 90 0c e8 b0 03         	movswl	0x3b0e80c(%eax), %edx
  44b423: 0f bf 80 0e e8 b0 03         	movswl	0x3b0e80e(%eax), %eax
  44b42a: c1 e2 08                     	shll	$0x8, %edx
  44b42d: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  44b431: c1 e0 08                     	shll	$0x8, %eax
  44b434: db 44 24 14                  	fildl	0x14(%esp)
  44b438: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  44b43c: db 44 24 14                  	fildl	0x14(%esp)
  44b440: d9 f3                        	fpatan
  44b442: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  44b448: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  44b44e: e8 ed 8e 1f 00               	calll	0x644340 <.text+0x243340>
  44b453: d9 54 24 30                  	fsts	0x30(%esp)
  44b457: d9 5c 24 34                  	fstps	0x34(%esp)
  44b45b: 8b d8                        	movl	%eax, %ebx
  44b45d: c7 44 24 38 00 00 80 bf      	movl	$0xbf800000, 0x38(%esp) # imm = 0xBF800000
  44b465: e8 16 68 33 00               	calll	0x781c80 <.text+0x380c80>
  44b46a: 33 c9                        	xorl	%ecx, %ecx
  44b46c: e8 7f c1 33 00               	calll	0x7875f0 <.text+0x3865f0>
  44b471: a1 b0 cb b2 03               	movl	0x3b2cbb0, %eax
  44b476: 8b 48 18                     	movl	0x18(%eax), %ecx
  44b479: 85 c9                        	testl	%ecx, %ecx
  44b47b: 74 0f                        	je	0x44b48c <.text+0x4a48c>
  44b47d: 0f b7 d1                     	movzwl	%cx, %edx
  44b480: 33 c9                        	xorl	%ecx, %ecx
  44b482: e8 99 99 33 00               	calll	0x784e20 <.text+0x383e20>
  44b487: a1 b0 cb b2 03               	movl	0x3b2cbb0, %eax
  44b48c: 8b 48 14                     	movl	0x14(%eax), %ecx
  44b48f: 85 c9                        	testl	%ecx, %ecx
  44b491: 74 0a                        	je	0x44b49d <.text+0x4a49d>
  44b493: 0f b7 d1                     	movzwl	%cx, %edx
  44b496: 33 c9                        	xorl	%ecx, %ecx
  44b498: e8 43 9a 33 00               	calll	0x784ee0 <.text+0x383ee0>
  44b49d: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  44b4a1: 51                           	pushl	%ecx
  44b4a2: 8d 54 24 34                  	leal	0x34(%esp), %edx
  44b4a6: 33 c9                        	xorl	%ecx, %ecx
  44b4a8: e8 b3 98 33 00               	calll	0x784d60 <.text+0x383d60>
  44b4ad: b9 01 00 00 00               	movl	$0x1, %ecx
  44b4b2: e8 89 9c 33 00               	calll	0x785140 <.text+0x384140>
  44b4b7: e8 c4 67 33 00               	calll	0x781c80 <.text+0x380c80>
  44b4bc: 33 c9                        	xorl	%ecx, %ecx
  44b4be: e8 2d c1 33 00               	calll	0x7875f0 <.text+0x3865f0>
  44b4c3: 53                           	pushl	%ebx
  44b4c4: 8d 5e 50                     	leal	0x50(%esi), %ebx
  44b4c7: 8b d3                        	movl	%ebx, %edx
  44b4c9: 33 c9                        	xorl	%ecx, %ecx
  44b4cb: e8 20 d9 33 00               	calll	0x788df0 <.text+0x387df0>
  44b4d0: 8d 54 24 48                  	leal	0x48(%esp), %edx
  44b4d4: 52                           	pushl	%edx
  44b4d5: 8d 54 24 40                  	leal	0x40(%esp), %edx
  44b4d9: 33 c9                        	xorl	%ecx, %ecx
  44b4db: e8 80 98 33 00               	calll	0x784d60 <.text+0x383d60>
  44b4e0: b9 01 00 00 00               	movl	$0x1, %ecx
  44b4e5: e8 56 9c 33 00               	calll	0x785140 <.text+0x384140>
  44b4ea: 8d 44 24 30                  	leal	0x30(%esp), %eax
  44b4ee: 50                           	pushl	%eax
  44b4ef: 57                           	pushl	%edi
  44b4f0: c7 44 24 38 00 00 80 3f      	movl	$0x3f800000, 0x38(%esp) # imm = 0x3F800000
  44b4f8: c7 44 24 3c 00 00 00 00      	movl	$0x0, 0x3c(%esp)
  44b500: c7 44 24 40 00 00 00 00      	movl	$0x0, 0x40(%esp)
  44b508: e8 83 37 ff ff               	calll	0x43ec90 <.text+0x3dc90>
  44b50d: 53                           	pushl	%ebx
  44b50e: 8d 4c 24 54                  	leal	0x54(%esp), %ecx
  44b512: 51                           	pushl	%ecx
  44b513: 8d 54 24 40                  	leal	0x40(%esp), %edx
  44b517: 52                           	pushl	%edx
  44b518: e8 f3 ce fe ff               	calll	0x438410 <.text+0x37410>
  44b51d: 8b 4f 18                     	movl	0x18(%edi), %ecx
  44b520: 03 c8                        	addl	%eax, %ecx
  44b522: 81 e1 ff ff 00 00            	andl	$0xffff, %ecx           # imm = 0xFFFF
  44b528: 51                           	pushl	%ecx
  44b529: 55                           	pushl	%ebp
  44b52a: 8b df                        	movl	%edi, %ebx
  44b52c: 89 4d 20                     	movl	%ecx, 0x20(%ebp)
  44b52f: e8 2c 89 ff ff               	calll	0x443e60 <.text+0x42e60>
  44b534: 83 c4 1c                     	addl	$0x1c, %esp
  44b537: 8b 44 24 58                  	movl	0x58(%esp), %eax
  44b53b: 8b c8                        	movl	%eax, %ecx
  44b53d: 89 4c 24 24                  	movl	%ecx, 0x24(%esp)
  44b541: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
  44b545: 89 45 34                     	movl	%eax, 0x34(%ebp)
  44b548: 5b                           	popl	%ebx
  44b549: d9 46 38                     	flds	0x38(%esi)
  44b54c: b2 04                        	movb	$0x4, %dl
  44b54e: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b554: df e0                        	fnstsw	%ax
  44b556: f6 c4 44                     	testb	$0x44, %ah
  44b559: 7a 7c                        	jp	0x44b5d7 <.text+0x4a5d7>
  44b55b: 85 c9                        	testl	%ecx, %ecx
  44b55d: d9 86 04 01 00 00            	flds	0x104(%esi)
  44b563: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44b566: d8 4c 24 18                  	fmuls	0x18(%esp)
  44b56a: d9 5c 24 54                  	fstps	0x54(%esp)
  44b56e: d9 44 24 14                  	flds	0x14(%esp)
  44b572: d8 44 24 20                  	fadds	0x20(%esp)
  44b576: d9 5c 24 14                  	fstps	0x14(%esp)
  44b57a: 0f 85 ab 02 00 00            	jne	0x44b82b <.text+0x4a82b>
  44b580: d9 44 24 14                  	flds	0x14(%esp)
  44b584: d8 5c 24 54                  	fcomps	0x54(%esp)
  44b588: df e0                        	fnstsw	%ax
  44b58a: f6 c4 05                     	testb	$0x5, %ah
  44b58d: 7a 11                        	jp	0x44b5a0 <.text+0x4a5a0>
  44b58f: d9 44 24 54                  	flds	0x54(%esp)
  44b593: d9 e0                        	fchs
  44b595: d8 5c 24 14                  	fcomps	0x14(%esp)
  44b599: df e0                        	fnstsw	%ax
  44b59b: f6 c4 05                     	testb	$0x5, %ah
  44b59e: 7b 2a                        	jnp	0x44b5ca <.text+0x4a5ca>
  44b5a0: d9 44 24 14                  	flds	0x14(%esp)
  44b5a4: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44b5aa: df e0                        	fnstsw	%ax
  44b5ac: f6 c4 05                     	testb	$0x5, %ah
  44b5af: 0f 8a 76 02 00 00            	jp	0x44b82b <.text+0x4a82b>
  44b5b5: d9 44 24 14                  	flds	0x14(%esp)
  44b5b9: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44b5bf: df e0                        	fnstsw	%ax
  44b5c1: f6 c4 41                     	testb	$0x41, %ah
  44b5c4: 0f 85 61 02 00 00            	jne	0x44b82b <.text+0x4a82b>
  44b5ca: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  44b5d2: e9 54 02 00 00               	jmp	0x44b82b <.text+0x4a82b>
  44b5d7: d9 44 24 20                  	flds	0x20(%esp)
  44b5db: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b5e1: df e0                        	fnstsw	%ax
  44b5e3: f6 c4 05                     	testb	$0x5, %ah
  44b5e6: 0f 8a 46 01 00 00            	jp	0x44b732 <.text+0x4a732>
  44b5ec: 84 56 5c                     	testb	%dl, 0x5c(%esi)
  44b5ef: 0f 84 9c 00 00 00            	je	0x44b691 <.text+0x4a691>
  44b5f5: d9 44 24 18                  	flds	0x18(%esp)
  44b5f9: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b5ff: df e0                        	fnstsw	%ax
  44b601: f6 c4 05                     	testb	$0x5, %ah
  44b604: 0f 8a d9 01 00 00            	jp	0x44b7e3 <.text+0x4a7e3>
  44b60a: d9 86 fc 00 00 00            	flds	0xfc(%esi)
  44b610: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44b613: d8 4c 24 18                  	fmuls	0x18(%esp)
  44b617: d9 54 24 54                  	fsts	0x54(%esp)
  44b61b: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b621: df e0                        	fnstsw	%ax
  44b623: f6 c4 41                     	testb	$0x41, %ah
  44b626: 0f 85 b7 01 00 00            	jne	0x44b7e3 <.text+0x4a7e3>
  44b62c: d9 44 24 20                  	flds	0x20(%esp)
  44b630: d9 e0                        	fchs
  44b632: d8 5c 24 54                  	fcomps	0x54(%esp)
  44b636: df e0                        	fnstsw	%ax
  44b638: f6 c4 41                     	testb	$0x41, %ah
  44b63b: 0f 85 a2 01 00 00            	jne	0x44b7e3 <.text+0x4a7e3>
  44b641: d9 44 24 14                  	flds	0x14(%esp)
  44b645: d8 44 24 20                  	fadds	0x20(%esp)
  44b649: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44b64f: df e0                        	fnstsw	%ax
  44b651: f6 c4 05                     	testb	$0x5, %ah
  44b654: 7a 25                        	jp	0x44b67b <.text+0x4a67b>
  44b656: d9 44 24 54                  	flds	0x54(%esp)
  44b65a: d9 e0                        	fchs
  44b65c: d9 5c 24 0c                  	fstps	0xc(%esp)
  44b660: d8 54 24 0c                  	fcoms	0xc(%esp)
  44b664: df e0                        	fnstsw	%ax
  44b666: f6 c4 05                     	testb	$0x5, %ah
  44b669: 7a 10                        	jp	0x44b67b <.text+0x4a67b>
  44b66b: d8 64 24 54                  	fsubs	0x54(%esp)
  44b66f: d9 5d 38                     	fstps	0x38(%ebp)
  44b672: d9 44 24 0c                  	flds	0xc(%esp)
  44b676: e9 ac 01 00 00               	jmp	0x44b827 <.text+0x4a827>
  44b67b: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44b681: df e0                        	fnstsw	%ax
  44b683: f6 c4 41                     	testb	$0x41, %ah
  44b686: 0f 85 9b 01 00 00            	jne	0x44b827 <.text+0x4a827>
  44b68c: e9 3a 01 00 00               	jmp	0x44b7cb <.text+0x4a7cb>
  44b691: d9 46 38                     	flds	0x38(%esi)
  44b694: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b69a: df e0                        	fnstsw	%ax
  44b69c: f6 c4 41                     	testb	$0x41, %ah
  44b69f: 75 2c                        	jne	0x44b6cd <.text+0x4a6cd>
  44b6a1: d9 44 24 14                  	flds	0x14(%esp)
  44b6a5: d8 44 24 20                  	fadds	0x20(%esp)
  44b6a9: d9 44 24 20                  	flds	0x20(%esp)
  44b6ad: d8 c9                        	fmul	%st(1), %st
  44b6af: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b6b5: df e0                        	fnstsw	%ax
  44b6b7: f6 c4 05                     	testb	$0x5, %ah
  44b6ba: 0f 8a 67 01 00 00            	jp	0x44b827 <.text+0x4a827>
  44b6c0: dd d8                        	fstp	%st(0)
  44b6c2: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44b6c8: e9 5a 01 00 00               	jmp	0x44b827 <.text+0x4a827>
  44b6cd: 85 c9                        	testl	%ecx, %ecx
  44b6cf: 75 32                        	jne	0x44b703 <.text+0x4a703>
  44b6d1: d9 46 38                     	flds	0x38(%esi)
  44b6d4: d8 9e c0 00 00 00            	fcomps	0xc0(%esi)
  44b6da: df e0                        	fnstsw	%ax
  44b6dc: f6 c4 41                     	testb	$0x41, %ah
  44b6df: 7a 22                        	jp	0x44b703 <.text+0x4a703>
  44b6e1: d9 44 24 14                  	flds	0x14(%esp)
  44b6e5: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44b6eb: df e0                        	fnstsw	%ax
  44b6ed: f6 c4 05                     	testb	$0x5, %ah
  44b6f0: 7a 11                        	jp	0x44b703 <.text+0x4a703>
  44b6f2: d9 44 24 14                  	flds	0x14(%esp)
  44b6f6: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44b6fc: df e0                        	fnstsw	%ax
  44b6fe: f6 c4 41                     	testb	$0x41, %ah
  44b701: 74 24                        	je	0x44b727 <.text+0x4a727>
  44b703: d9 86 04 01 00 00            	flds	0x104(%esi)
  44b709: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44b70c: d8 4c 24 18                  	fmuls	0x18(%esp)
  44b710: d8 44 24 20                  	fadds	0x20(%esp)
  44b714: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44b71a: df e0                        	fnstsw	%ax
  44b71c: f6 c4 01                     	testb	$0x1, %ah
  44b71f: 0f 85 02 01 00 00            	jne	0x44b827 <.text+0x4a827>
  44b725: dd d8                        	fstp	%st(0)
  44b727: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44b72d: e9 f5 00 00 00               	jmp	0x44b827 <.text+0x4a827>
  44b732: d9 86 04 01 00 00            	flds	0x104(%esi)
  44b738: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44b73b: d8 4c 24 18                  	fmuls	0x18(%esp)
  44b73f: d9 5c 24 54                  	fstps	0x54(%esp)
  44b743: d9 46 38                     	flds	0x38(%esi)
  44b746: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b74c: df e0                        	fnstsw	%ax
  44b74e: f6 c4 05                     	testb	$0x5, %ah
  44b751: 7a 50                        	jp	0x44b7a3 <.text+0x4a7a3>
  44b753: d9 44 24 14                  	flds	0x14(%esp)
  44b757: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44b75d: df e0                        	fnstsw	%ax
  44b75f: f6 c4 05                     	testb	$0x5, %ah
  44b762: 7a 3f                        	jp	0x44b7a3 <.text+0x4a7a3>
  44b764: d9 44 24 14                  	flds	0x14(%esp)
  44b768: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44b76e: df e0                        	fnstsw	%ax
  44b770: f6 c4 41                     	testb	$0x41, %ah
  44b773: 75 2e                        	jne	0x44b7a3 <.text+0x4a7a3>
  44b775: d9 44 24 14                  	flds	0x14(%esp)
  44b779: d8 44 24 20                  	fadds	0x20(%esp)
  44b77d: d9 54 24 14                  	fsts	0x14(%esp)
  44b781: d8 4c 24 20                  	fmuls	0x20(%esp)
  44b785: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b78b: df e0                        	fnstsw	%ax
  44b78d: f6 c4 05                     	testb	$0x5, %ah
  44b790: 0f 8a 95 00 00 00            	jp	0x44b82b <.text+0x4a82b>
  44b796: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  44b79e: e9 88 00 00 00               	jmp	0x44b82b <.text+0x4a82b>
  44b7a3: d9 44 24 54                  	flds	0x54(%esp)
  44b7a7: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b7ad: df e0                        	fnstsw	%ax
  44b7af: f6 c4 41                     	testb	$0x41, %ah
  44b7b2: 75 2f                        	jne	0x44b7e3 <.text+0x4a7e3>
  44b7b4: d9 44 24 54                  	flds	0x54(%esp)
  44b7b8: d8 5c 24 20                  	fcomps	0x20(%esp)
  44b7bc: df e0                        	fnstsw	%ax
  44b7be: f6 c4 05                     	testb	$0x5, %ah
  44b7c1: 7a 20                        	jp	0x44b7e3 <.text+0x4a7e3>
  44b7c3: d9 44 24 14                  	flds	0x14(%esp)
  44b7c7: d8 44 24 20                  	fadds	0x20(%esp)
  44b7cb: d8 54 24 54                  	fcoms	0x54(%esp)
  44b7cf: df e0                        	fnstsw	%ax
  44b7d1: f6 c4 41                     	testb	$0x41, %ah
  44b7d4: 75 51                        	jne	0x44b827 <.text+0x4a827>
  44b7d6: d8 64 24 54                  	fsubs	0x54(%esp)
  44b7da: d9 5d 38                     	fstps	0x38(%ebp)
  44b7dd: d9 44 24 54                  	flds	0x54(%esp)
  44b7e1: eb 44                        	jmp	0x44b827 <.text+0x4a827>
  44b7e3: 85 c9                        	testl	%ecx, %ecx
  44b7e5: 75 38                        	jne	0x44b81f <.text+0x4a81f>
  44b7e7: d9 46 38                     	flds	0x38(%esi)
  44b7ea: d8 9e c0 00 00 00            	fcomps	0xc0(%esi)
  44b7f0: df e0                        	fnstsw	%ax
  44b7f2: f6 c4 41                     	testb	$0x41, %ah
  44b7f5: 7a 28                        	jp	0x44b81f <.text+0x4a81f>
  44b7f7: d9 44 24 14                  	flds	0x14(%esp)
  44b7fb: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44b801: df e0                        	fnstsw	%ax
  44b803: f6 c4 05                     	testb	$0x5, %ah
  44b806: 7a 17                        	jp	0x44b81f <.text+0x4a81f>
  44b808: d9 44 24 14                  	flds	0x14(%esp)
  44b80c: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44b812: df e0                        	fnstsw	%ax
  44b814: f6 c4 41                     	testb	$0x41, %ah
  44b817: 75 06                        	jne	0x44b81f <.text+0x4a81f>
  44b819: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  44b81d: eb 0c                        	jmp	0x44b82b <.text+0x4a82b>
  44b81f: d9 44 24 14                  	flds	0x14(%esp)
  44b823: d8 44 24 20                  	fadds	0x20(%esp)
  44b827: d9 5c 24 14                  	fstps	0x14(%esp)
  44b82b: d9 46 40                     	flds	0x40(%esi)
  44b82e: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b834: df e0                        	fnstsw	%ax
  44b836: f6 c4 44                     	testb	$0x44, %ah
  44b839: 7a 3c                        	jp	0x44b877 <.text+0x4a877>
  44b83b: d9 86 04 01 00 00            	flds	0x104(%esi)
  44b841: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44b844: d8 4c 24 18                  	fmuls	0x18(%esp)
  44b848: d9 44 24 1c                  	flds	0x1c(%esp)
  44b84c: d8 d9                        	fcomp	%st(1)
  44b84e: df e0                        	fnstsw	%ax
  44b850: f6 c4 05                     	testb	$0x5, %ah
  44b853: 0f 8a 2d 01 00 00            	jp	0x44b986 <.text+0x4a986>
  44b859: d9 e0                        	fchs
  44b85b: d8 5c 24 1c                  	fcomps	0x1c(%esp)
  44b85f: df e0                        	fnstsw	%ax
  44b861: f6 c4 05                     	testb	$0x5, %ah
  44b864: 0f 8a 1e 01 00 00            	jp	0x44b988 <.text+0x4a988>
  44b86a: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44b872: e9 11 01 00 00               	jmp	0x44b988 <.text+0x4a988>
  44b877: d9 44 24 18                  	flds	0x18(%esp)
  44b87b: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b881: df e0                        	fnstsw	%ax
  44b883: f6 c4 05                     	testb	$0x5, %ah
  44b886: 7a 13                        	jp	0x44b89b <.text+0x4a89b>
  44b888: d9 86 00 01 00 00            	flds	0x100(%esi)
  44b88e: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44b891: d8 4c 24 18                  	fmuls	0x18(%esp)
  44b895: d9 5c 24 54                  	fstps	0x54(%esp)
  44b899: eb 08                        	jmp	0x44b8a3 <.text+0x4a8a3>
  44b89b: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%esp)
  44b8a3: 8a 46 5c                     	movb	0x5c(%esi), %al
  44b8a6: d9 44 24 1c                  	flds	0x1c(%esp)
  44b8aa: 84 c2                        	testb	%al, %dl
  44b8ac: 74 5b                        	je	0x44b909 <.text+0x4a909>
  44b8ae: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44b8b4: df e0                        	fnstsw	%ax
  44b8b6: f6 c4 41                     	testb	$0x41, %ah
  44b8b9: 75 22                        	jne	0x44b8dd <.text+0x4a8dd>
  44b8bb: d8 64 24 54                  	fsubs	0x54(%esp)
  44b8bf: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44b8c5: df e0                        	fnstsw	%ax
  44b8c7: f6 c4 05                     	testb	$0x5, %ah
  44b8ca: 7a 37                        	jp	0x44b903 <.text+0x4a903>
  44b8cc: dd d8                        	fstp	%st(0)
  44b8ce: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44b8d4: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44b8d8: e9 ab 00 00 00               	jmp	0x44b988 <.text+0x4a988>
  44b8dd: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44b8e3: df e0                        	fnstsw	%ax
  44b8e5: f6 c4 05                     	testb	$0x5, %ah
  44b8e8: 7a 19                        	jp	0x44b903 <.text+0x4a903>
  44b8ea: d8 44 24 54                  	fadds	0x54(%esp)
  44b8ee: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44b8f4: df e0                        	fnstsw	%ax
  44b8f6: f6 c4 41                     	testb	$0x41, %ah
  44b8f9: 75 08                        	jne	0x44b903 <.text+0x4a903>
  44b8fb: dd d8                        	fstp	%st(0)
  44b8fd: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44b903: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44b907: eb 7f                        	jmp	0x44b988 <.text+0x4a988>
  44b909: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b90f: df e0                        	fnstsw	%ax
  44b911: f6 c4 41                     	testb	$0x41, %ah
  44b914: 75 0c                        	jne	0x44b922 <.text+0x4a922>
  44b916: d9 44 24 54                  	flds	0x54(%esp)
  44b91a: d9 e0                        	fchs
  44b91c: d9 5c 24 28                  	fstps	0x28(%esp)
  44b920: eb 19                        	jmp	0x44b93b <.text+0x4a93b>
  44b922: d9 44 24 1c                  	flds	0x1c(%esp)
  44b926: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b92c: df e0                        	fnstsw	%ax
  44b92e: f6 c4 05                     	testb	$0x5, %ah
  44b931: 7a 08                        	jp	0x44b93b <.text+0x4a93b>
  44b933: 8b 54 24 54                  	movl	0x54(%esp), %edx
  44b937: 89 54 24 28                  	movl	%edx, 0x28(%esp)
  44b93b: d9 44 24 1c                  	flds	0x1c(%esp)
  44b93f: d8 44 24 28                  	fadds	0x28(%esp)
  44b943: d9 44 24 1c                  	flds	0x1c(%esp)
  44b947: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b94d: df e0                        	fnstsw	%ax
  44b94f: f6 c4 44                     	testb	$0x44, %ah
  44b952: 7b 2c                        	jnp	0x44b980 <.text+0x4a980>
  44b954: d9 44 24 28                  	flds	0x28(%esp)
  44b958: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b95e: df e0                        	fnstsw	%ax
  44b960: f6 c4 44                     	testb	$0x44, %ah
  44b963: 7b 1b                        	jnp	0x44b980 <.text+0x4a980>
  44b965: d9 44 24 1c                  	flds	0x1c(%esp)
  44b969: d8 c9                        	fmul	%st(1), %st
  44b96b: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44b971: df e0                        	fnstsw	%ax
  44b973: f6 c4 05                     	testb	$0x5, %ah
  44b976: 7a 08                        	jp	0x44b980 <.text+0x4a980>
  44b978: dd d8                        	fstp	%st(0)
  44b97a: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44b980: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44b984: eb 02                        	jmp	0x44b988 <.text+0x4a988>
  44b986: dd d8                        	fstp	%st(0)
  44b988: d9 44 24 18                  	flds	0x18(%esp)
  44b98c: 8b 44 24 14                  	movl	0x14(%esp), %eax
  44b990: d8 44 24 24                  	fadds	0x24(%esp)
  44b994: 8b 54 24 1c                  	movl	0x1c(%esp), %edx
  44b998: 83 c6 2c                     	addl	$0x2c, %esi
  44b99b: 89 06                        	movl	%eax, (%esi)
  44b99d: d9 5c 24 18                  	fstps	0x18(%esp)
  44b9a1: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  44b9a5: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  44b9a8: 5f                           	popl	%edi
  44b9a9: 89 56 08                     	movl	%edx, 0x8(%esi)
  44b9ac: 5e                           	popl	%esi
  44b9ad: 5d                           	popl	%ebp
  44b9ae: 83 c4 44                     	addl	$0x44, %esp
  44b9b1: c3                           	retl
