
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  44c270: 83 ec 30                     	subl	$0x30, %esp
  44c273: 57                           	pushl	%edi
  44c274: 8b 7c 24 38                  	movl	0x38(%esp), %edi
  44c278: 33 c0                        	xorl	%eax, %eax
  44c27a: 66 8b 47 04                  	movw	0x4(%edi), %ax
  44c27e: 25 00 40 00 00               	andl	$0x4000, %eax           # imm = 0x4000
  44c283: 66 85 c0                     	testw	%ax, %ax
  44c286: 74 18                        	je	0x44c2a0 <.text+0x4b2a0>
  44c288: 8b 4c 24 40                  	movl	0x40(%esp), %ecx
  44c28c: 8b 54 24 3c                  	movl	0x3c(%esp), %edx
  44c290: 51                           	pushl	%ecx
  44c291: 52                           	pushl	%edx
  44c292: 57                           	pushl	%edi
  44c293: e8 78 b2 ff ff               	calll	0x447510 <.text+0x46510>
  44c298: 83 c4 0c                     	addl	$0xc, %esp
  44c29b: 5f                           	popl	%edi
  44c29c: 83 c4 30                     	addl	$0x30, %esp
  44c29f: c3                           	retl
  44c2a0: 55                           	pushl	%ebp
  44c2a1: 8b 6c 24 44                  	movl	0x44(%esp), %ebp
  44c2a5: 8b 45 5c                     	movl	0x5c(%ebp), %eax
  44c2a8: a9 00 00 02 00               	testl	$0x20000, %eax          # imm = 0x20000
  44c2ad: 74 17                        	je	0x44c2c6 <.text+0x4b2c6>
  44c2af: 8b 44 24 40                  	movl	0x40(%esp), %eax
  44c2b3: 50                           	pushl	%eax
  44c2b4: 8b c5                        	movl	%ebp, %eax
  44c2b6: 8b cf                        	movl	%edi, %ecx
  44c2b8: e8 23 ee ff ff               	calll	0x44b0e0 <.text+0x4a0e0>
  44c2bd: 83 c4 04                     	addl	$0x4, %esp
  44c2c0: 5d                           	popl	%ebp
  44c2c1: 5f                           	popl	%edi
  44c2c2: 83 c4 30                     	addl	$0x30, %esp
  44c2c5: c3                           	retl
  44c2c6: f6 c4 40                     	testb	$0x40, %ah
  44c2c9: 74 15                        	je	0x44c2e0 <.text+0x4b2e0>
  44c2cb: 8b 4c 24 40                  	movl	0x40(%esp), %ecx
  44c2cf: 55                           	pushl	%ebp
  44c2d0: 51                           	pushl	%ecx
  44c2d1: 57                           	pushl	%edi
  44c2d2: e8 39 ad ff ff               	calll	0x447010 <.text+0x46010>
  44c2d7: 83 c4 0c                     	addl	$0xc, %esp
  44c2da: 5d                           	popl	%ebp
  44c2db: 5f                           	popl	%edi
  44c2dc: 83 c4 30                     	addl	$0x30, %esp
  44c2df: c3                           	retl
  44c2e0: d9 05 f8 f0 b0 03            	flds	0x3b0f0f8
  44c2e6: 53                           	pushl	%ebx
  44c2e7: d8 8d 14 01 00 00            	fmuls	0x114(%ebp)
  44c2ed: 8b 5c 24 44                  	movl	0x44(%esp), %ebx
  44c2f1: 8d 53 10                     	leal	0x10(%ebx), %edx
  44c2f4: 8b 02                        	movl	(%edx), %eax
  44c2f6: 8b 4a 04                     	movl	0x4(%edx), %ecx
  44c2f9: 8b 52 08                     	movl	0x8(%edx), %edx
  44c2fc: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  44c300: d8 44 24 18                  	fadds	0x18(%esp)
  44c304: 89 4c 24 1c                  	movl	%ecx, 0x1c(%esp)
  44c308: 89 54 24 20                  	movl	%edx, 0x20(%esp)
  44c30c: d9 5c 24 18                  	fstps	0x18(%esp)
  44c310: 56                           	pushl	%esi
  44c311: d9 05 fc f0 b0 03            	flds	0x3b0f0fc
  44c317: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  44c31b: d8 8d 14 01 00 00            	fmuls	0x114(%ebp)
  44c321: 50                           	pushl	%eax
  44c322: 57                           	pushl	%edi
  44c323: c7 44 24 38 00 00 00 00      	movl	$0x0, 0x38(%esp)
  44c32b: d8 44 24 28                  	fadds	0x28(%esp)
  44c32f: c7 44 24 34 00 00 00 00      	movl	$0x0, 0x34(%esp)
  44c337: c7 44 24 30 00 00 00 00      	movl	$0x0, 0x30(%esp)
  44c33f: d9 5c 24 28                  	fstps	0x28(%esp)
  44c343: d9 05 00 f1 b0 03            	flds	0x3b0f100
  44c349: d8 8d 14 01 00 00            	fmuls	0x114(%ebp)
  44c34f: d8 44 24 2c                  	fadds	0x2c(%esp)
  44c353: d9 5c 24 2c                  	fstps	0x2c(%esp)
  44c357: e8 a4 28 ff ff               	calll	0x43ec00 <.text+0x3dc00>
  44c35c: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  44c360: 51                           	pushl	%ecx
  44c361: 8d 54 24 20                  	leal	0x20(%esp), %edx
  44c365: 52                           	pushl	%edx
  44c366: 57                           	pushl	%edi
  44c367: e8 54 fa fe ff               	calll	0x43bdc0 <.text+0x3adc0>
  44c36c: 8b 4d 64                     	movl	0x64(%ebp), %ecx
  44c36f: 8b f0                        	movl	%eax, %esi
  44c371: 83 c4 14                     	addl	$0x14, %esp
  44c374: 8d 44 24 34                  	leal	0x34(%esp), %eax
  44c378: 50                           	pushl	%eax
  44c379: 8d 53 04                     	leal	0x4(%ebx), %edx
  44c37c: 83 c1 1c                     	addl	$0x1c, %ecx
  44c37f: 89 74 24 1c                  	movl	%esi, 0x1c(%esp)
  44c383: e8 68 c6 33 00               	calll	0x7889f0 <.text+0x3879f0>
  44c388: dd d8                        	fstp	%st(0)
  44c38a: d9 45 1c                     	flds	0x1c(%ebp)
  44c38d: d8 1d 68 d5 7d 00            	fcomps	0x7dd568
  44c393: df e0                        	fnstsw	%ax
  44c395: f6 c4 05                     	testb	$0x5, %ah
  44c398: 7a 3f                        	jp	0x44c3d9 <.text+0x4b3d9>
  44c39a: d9 44 24 38                  	flds	0x38(%esp)
  44c39e: d9 e1                        	fabs
  44c3a0: d8 1d 84 d5 7d 00            	fcomps	0x7dd584
  44c3a6: df e0                        	fnstsw	%ax
  44c3a8: f6 c4 41                     	testb	$0x41, %ah
  44c3ab: 75 2c                        	jne	0x44c3d9 <.text+0x4b3d9>
  44c3ad: d9 45 38                     	flds	0x38(%ebp)
  44c3b0: d8 1d 78 01 7e 00            	fcomps	0x7e0178
  44c3b6: df e0                        	fnstsw	%ax
  44c3b8: f6 c4 41                     	testb	$0x41, %ah
  44c3bb: 75 1c                        	jne	0x44c3d9 <.text+0x4b3d9>
  44c3bd: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44c3c3: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44c3cb: d9 85 14 01 00 00            	flds	0x114(%ebp)
  44c3d1: d9 e0                        	fchs
  44c3d3: d9 5c 24 20                  	fstps	0x20(%esp)
  44c3d7: eb 38                        	jmp	0x44c411 <.text+0x4b411>
  44c3d9: d9 45 1c                     	flds	0x1c(%ebp)
  44c3dc: d8 1d e4 00 7e 00            	fcomps	0x7e00e4
  44c3e2: df e0                        	fnstsw	%ax
  44c3e4: f6 c4 05                     	testb	$0x5, %ah
  44c3e7: 7a 6c                        	jp	0x44c455 <.text+0x4b455>
  44c3e9: d9 45 38                     	flds	0x38(%ebp)
  44c3ec: d8 1d 78 01 7e 00            	fcomps	0x7e0178
  44c3f2: df e0                        	fnstsw	%ax
  44c3f4: f6 c4 41                     	testb	$0x41, %ah
  44c3f7: 75 5c                        	jne	0x44c455 <.text+0x4b455>
  44c3f9: d9 85 14 01 00 00            	flds	0x114(%ebp)
  44c3ff: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  44c405: d8 6c 24 20                  	fsubrs	0x20(%esp)
  44c409: d9 5c 24 20                  	fstps	0x20(%esp)
  44c40d: d9 44 24 24                  	flds	0x24(%esp)
  44c411: 85 f6                        	testl	%esi, %esi
  44c413: d9 45 38                     	flds	0x38(%ebp)
  44c416: 0f 85 71 01 00 00            	jne	0x44c58d <.text+0x4b58d>
  44c41c: d8 9d c4 00 00 00            	fcomps	0xc4(%ebp)
  44c422: df e0                        	fnstsw	%ax
  44c424: f6 c4 41                     	testb	$0x41, %ah
  44c427: 0f 84 a9 01 00 00            	je	0x44c5d6 <.text+0x4b5d6>
  44c42d: d9 45 38                     	flds	0x38(%ebp)
  44c430: d8 9d ac 00 00 00            	fcomps	0xac(%ebp)
  44c436: d9 45 38                     	flds	0x38(%ebp)
  44c439: df e0                        	fnstsw	%ax
  44c43b: f6 c4 41                     	testb	$0x41, %ah
  44c43e: 0f 85 85 01 00 00            	jne	0x44c5c9 <.text+0x4b5c9>
  44c444: d8 a5 ac 00 00 00            	fsubs	0xac(%ebp)
  44c44a: d8 8d f0 00 00 00            	fmuls	0xf0(%ebp)
  44c450: e9 8a 01 00 00               	jmp	0x44c5df <.text+0x4b5df>
  44c455: d9 45 1c                     	flds	0x1c(%ebp)
  44c458: d8 1d a8 01 7e 00            	fcomps	0x7e01a8
  44c45e: df e0                        	fnstsw	%ax
  44c460: f6 c4 05                     	testb	$0x5, %ah
  44c463: 7a 1e                        	jp	0x44c483 <.text+0x4b483>
  44c465: d9 45 38                     	flds	0x38(%ebp)
  44c468: d8 1d 78 01 7e 00            	fcomps	0x7e0178
  44c46e: df e0                        	fnstsw	%ax
  44c470: f6 c4 41                     	testb	$0x41, %ah
  44c473: 75 0e                        	jne	0x44c483 <.text+0x4b483>
  44c475: d9 85 14 01 00 00            	flds	0x114(%ebp)
  44c47b: d8 0d 5c ce 7d 00            	fmuls	0x7dce5c
  44c481: eb 82                        	jmp	0x44c405 <.text+0x4b405>
  44c483: d9 45 1c                     	flds	0x1c(%ebp)
  44c486: d8 1d 64 00 7e 00            	fcomps	0x7e0064
  44c48c: df e0                        	fnstsw	%ax
  44c48e: f6 c4 05                     	testb	$0x5, %ah
  44c491: 7a 21                        	jp	0x44c4b4 <.text+0x4b4b4>
  44c493: d9 45 38                     	flds	0x38(%ebp)
  44c496: d8 1d 78 01 7e 00            	fcomps	0x7e0178
  44c49c: df e0                        	fnstsw	%ax
  44c49e: f6 c4 41                     	testb	$0x41, %ah
  44c4a1: 75 11                        	jne	0x44c4b4 <.text+0x4b4b4>
  44c4a3: d9 85 14 01 00 00            	flds	0x114(%ebp)
  44c4a9: d8 0d 50 d5 7d 00            	fmuls	0x7dd550
  44c4af: e9 51 ff ff ff               	jmp	0x44c405 <.text+0x4b405>
  44c4b4: d9 45 1c                     	flds	0x1c(%ebp)
  44c4b7: d8 1d 98 cc 7d 00            	fcomps	0x7dcc98
  44c4bd: df e0                        	fnstsw	%ax
  44c4bf: f6 c4 05                     	testb	$0x5, %ah
  44c4c2: 7a 3f                        	jp	0x44c503 <.text+0x4b503>
  44c4c4: d9 45 38                     	flds	0x38(%ebp)
  44c4c7: d8 9d c4 00 00 00            	fcomps	0xc4(%ebp)
  44c4cd: df e0                        	fnstsw	%ax
  44c4cf: f6 c4 05                     	testb	$0x5, %ah
  44c4d2: 7a 2f                        	jp	0x44c503 <.text+0x4b503>
  44c4d4: d9 85 c4 00 00 00            	flds	0xc4(%ebp)
  44c4da: d9 e0                        	fchs
  44c4dc: d8 5d 38                     	fcomps	0x38(%ebp)
  44c4df: df e0                        	fnstsw	%ax
  44c4e1: f6 c4 05                     	testb	$0x5, %ah
  44c4e4: 7a 1d                        	jp	0x44c503 <.text+0x4b503>
  44c4e6: d9 44 24 1c                  	flds	0x1c(%esp)
  44c4ea: d8 0d a4 01 7e 00            	fmuls	0x7e01a4
  44c4f0: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44c4f4: d9 44 24 24                  	flds	0x24(%esp)
  44c4f8: d8 0d a4 01 7e 00            	fmuls	0x7e01a4
  44c4fe: e9 0e ff ff ff               	jmp	0x44c411 <.text+0x4b411>
  44c503: d9 45 1c                     	flds	0x1c(%ebp)
  44c506: d8 1d 78 d5 7d 00            	fcomps	0x7dd578
  44c50c: df e0                        	fnstsw	%ax
  44c50e: f6 c4 05                     	testb	$0x5, %ah
  44c511: 7a 2d                        	jp	0x44c540 <.text+0x4b540>
  44c513: d9 45 38                     	flds	0x38(%ebp)
  44c516: d8 9d c4 00 00 00            	fcomps	0xc4(%ebp)
  44c51c: df e0                        	fnstsw	%ax
  44c51e: f6 c4 05                     	testb	$0x5, %ah
  44c521: 7a 1d                        	jp	0x44c540 <.text+0x4b540>
  44c523: d9 85 c4 00 00 00            	flds	0xc4(%ebp)
  44c529: d9 e0                        	fchs
  44c52b: d8 5d 38                     	fcomps	0x38(%ebp)
  44c52e: df e0                        	fnstsw	%ax
  44c530: f6 c4 05                     	testb	$0x5, %ah
  44c533: 7a 0b                        	jp	0x44c540 <.text+0x4b540>
  44c535: d9 44 24 24                  	flds	0x24(%esp)
  44c539: dc c0                        	fadd	%st, %st(0)
  44c53b: e9 d1 fe ff ff               	jmp	0x44c411 <.text+0x4b411>
  44c540: d9 45 1c                     	flds	0x1c(%ebp)
  44c543: d8 1d 24 01 7e 00            	fcomps	0x7e0124
  44c549: df e0                        	fnstsw	%ax
  44c54b: f6 c4 05                     	testb	$0x5, %ah
  44c54e: 0f 8a b9 fe ff ff            	jp	0x44c40d <.text+0x4b40d>
  44c554: d9 45 38                     	flds	0x38(%ebp)
  44c557: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  44c55d: df e0                        	fnstsw	%ax
  44c55f: f6 c4 05                     	testb	$0x5, %ah
  44c562: 0f 8a a5 fe ff ff            	jp	0x44c40d <.text+0x4b40d>
  44c568: d9 85 c4 00 00 00            	flds	0xc4(%ebp)
  44c56e: d9 e0                        	fchs
  44c570: d8 5d 38                     	fcomps	0x38(%ebp)
  44c573: df e0                        	fnstsw	%ax
  44c575: f6 c4 05                     	testb	$0x5, %ah
  44c578: 0f 8a 8f fe ff ff            	jp	0x44c40d <.text+0x4b40d>
  44c57e: d9 44 24 24                  	flds	0x24(%esp)
  44c582: d8 0d 68 00 7e 00            	fmuls	0x7e0068
  44c588: e9 84 fe ff ff               	jmp	0x44c411 <.text+0x4b411>
  44c58d: d8 9d ac 00 00 00            	fcomps	0xac(%ebp)
  44c593: df e0                        	fnstsw	%ax
  44c595: f6 c4 41                     	testb	$0x41, %ah
  44c598: 0f 85 8f fe ff ff            	jne	0x44c42d <.text+0x4b42d>
  44c59e: d9 45 1c                     	flds	0x1c(%ebp)
  44c5a1: d8 1d 88 00 7e 00            	fcomps	0x7e0088
  44c5a7: df e0                        	fnstsw	%ax
  44c5a9: f6 c4 41                     	testb	$0x41, %ah
  44c5ac: 0f 85 7b fe ff ff            	jne	0x44c42d <.text+0x4b42d>
  44c5b2: d9 45 38                     	flds	0x38(%ebp)
  44c5b5: d8 a5 ac 00 00 00            	fsubs	0xac(%ebp)
  44c5bb: d8 8d f0 00 00 00            	fmuls	0xf0(%ebp)
  44c5c1: d8 0d 9c 01 7e 00            	fmuls	0x7e019c
  44c5c7: eb 16                        	jmp	0x44c5df <.text+0x4b5df>
  44c5c9: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44c5cf: df e0                        	fnstsw	%ax
  44c5d1: f6 c4 05                     	testb	$0x5, %ah
  44c5d4: 7a 11                        	jp	0x44c5e7 <.text+0x4b5e7>
  44c5d6: d9 85 f0 00 00 00            	flds	0xf0(%ebp)
  44c5dc: d8 4d 38                     	fmuls	0x38(%ebp)
  44c5df: d8 44 24 1c                  	fadds	0x1c(%esp)
  44c5e3: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44c5e7: 85 f6                        	testl	%esi, %esi
  44c5e9: d9 85 f4 00 00 00            	flds	0xf4(%ebp)
  44c5ef: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44c5f2: d8 44 24 20                  	fadds	0x20(%esp)
  44c5f6: d9 5c 24 20                  	fstps	0x20(%esp)
  44c5fa: d9 85 f8 00 00 00            	flds	0xf8(%ebp)
  44c600: d8 4d 40                     	fmuls	0x40(%ebp)
  44c603: d8 c1                        	fadd	%st(1), %st
  44c605: d9 5c 24 24                  	fstps	0x24(%esp)
  44c609: dd d8                        	fstp	%st(0)
  44c60b: 75 71                        	jne	0x44c67e <.text+0x4b67e>
  44c60d: d9 45 1c                     	flds	0x1c(%ebp)
  44c610: d8 1d 38 01 7e 00            	fcomps	0x7e0138
  44c616: df e0                        	fnstsw	%ax
  44c618: f6 c4 05                     	testb	$0x5, %ah
  44c61b: 7a 13                        	jp	0x44c630 <.text+0x4b630>
  44c61d: 55                           	pushl	%ebp
  44c61e: 53                           	pushl	%ebx
  44c61f: 57                           	pushl	%edi
  44c620: e8 bb cb ff ff               	calll	0x4491e0 <.text+0x481e0>
  44c625: 83 c4 0c                     	addl	$0xc, %esp
  44c628: 89 73 34                     	movl	%esi, 0x34(%ebx)
  44c62b: e9 f3 02 00 00               	jmp	0x44c923 <.text+0x4b923>
  44c630: d9 45 38                     	flds	0x38(%ebp)
  44c633: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44c639: df e0                        	fnstsw	%ax
  44c63b: f6 c4 41                     	testb	$0x41, %ah
  44c63e: 75 16                        	jne	0x44c656 <.text+0x4b656>
  44c640: 8b 8d e0 00 00 00            	movl	0xe0(%ebp), %ecx
  44c646: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  44c64a: c7 43 34 00 00 00 00         	movl	$0x0, 0x34(%ebx)
  44c651: e9 cd 02 00 00               	jmp	0x44c923 <.text+0x4b923>
  44c656: d9 45 38                     	flds	0x38(%ebp)
  44c659: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44c65f: df e0                        	fnstsw	%ax
  44c661: f6 c4 05                     	testb	$0x5, %ah
  44c664: 7a 0c                        	jp	0x44c672 <.text+0x4b672>
  44c666: d9 85 e0 00 00 00            	flds	0xe0(%ebp)
  44c66c: d9 e0                        	fchs
  44c66e: d9 5c 24 28                  	fstps	0x28(%esp)
  44c672: c7 43 34 00 00 00 00         	movl	$0x0, 0x34(%ebx)
  44c679: e9 a5 02 00 00               	jmp	0x44c923 <.text+0x4b923>
  44c67e: 33 d2                        	xorl	%edx, %edx
  44c680: 66 8b 57 04                  	movw	0x4(%edi), %dx
  44c684: 81 e2 00 20 00 00            	andl	$0x2000, %edx           # imm = 0x2000
  44c68a: 66 85 d2                     	testw	%dx, %dx
  44c68d: 74 14                        	je	0x44c6a3 <.text+0x4b6a3>
  44c68f: 8b c5                        	movl	%ebp, %eax
  44c691: e8 9a fa fe ff               	calll	0x43c130 <.text+0x3b130>
  44c696: 8b 73 20                     	movl	0x20(%ebx), %esi
  44c699: c7 44 24 2c cd cc 4c bf      	movl	$0xbf4ccccd, 0x2c(%esp) # imm = 0xBF4CCCCD
  44c6a1: eb 34                        	jmp	0x44c6d7 <.text+0x4b6d7>
  44c6a3: d9 45 1c                     	flds	0x1c(%ebp)
  44c6a6: d8 1d a0 01 7e 00            	fcomps	0x7e01a0
  44c6ac: df e0                        	fnstsw	%ax
  44c6ae: f6 c4 05                     	testb	$0x5, %ah
  44c6b1: 7a 20                        	jp	0x44c6d3 <.text+0x4b6d3>
  44c6b3: f7 45 5c 00 00 80 00         	testl	$0x800000, 0x5c(%ebp)   # imm = 0x800000
  44c6ba: 74 17                        	je	0x44c6d3 <.text+0x4b6d3>
  44c6bc: 8b 74 24 48                  	movl	0x48(%esp), %esi
  44c6c0: 8b 44 24 44                  	movl	0x44(%esp), %eax
  44c6c4: 55                           	pushl	%ebp
  44c6c5: e8 b6 cc ff ff               	calll	0x449380 <.text+0x48380>
  44c6ca: 8b 7c 24 48                  	movl	0x48(%esp), %edi
  44c6ce: 83 c4 04                     	addl	$0x4, %esp
  44c6d1: 8b de                        	movl	%esi, %ebx
  44c6d3: 8b 74 24 14                  	movl	0x14(%esp), %esi
  44c6d7: d9 45 38                     	flds	0x38(%ebp)
  44c6da: d8 9d ac 00 00 00            	fcomps	0xac(%ebp)
  44c6e0: d9 45 38                     	flds	0x38(%ebp)
  44c6e3: df e0                        	fnstsw	%ax
  44c6e5: f6 c4 05                     	testb	$0x5, %ah
  44c6e8: 0f 8a bd 00 00 00            	jp	0x44c7ab <.text+0x4b7ab>
  44c6ee: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  44c6f4: df e0                        	fnstsw	%ax
  44c6f6: f6 c4 05                     	testb	$0x5, %ah
  44c6f9: 7a 1f                        	jp	0x44c71a <.text+0x4b71a>
  44c6fb: d9 44 24 10                  	flds	0x10(%esp)
  44c6ff: d8 1d 98 cc 7d 00            	fcomps	0x7dcc98
  44c705: df e0                        	fnstsw	%ax
  44c707: f6 c4 41                     	testb	$0x41, %ah
  44c70a: 74 56                        	je	0x44c762 <.text+0x4b762>
  44c70c: d9 85 c0 00 00 00            	flds	0xc0(%ebp)
  44c712: d8 0d 50 d5 7d 00            	fmuls	0x7dd550
  44c718: eb 3e                        	jmp	0x44c758 <.text+0x4b758>
  44c71a: d9 45 38                     	flds	0x38(%ebp)
  44c71d: d8 9d c4 00 00 00            	fcomps	0xc4(%ebp)
  44c723: df e0                        	fnstsw	%ax
  44c725: f6 c4 05                     	testb	$0x5, %ah
  44c728: 7a 4e                        	jp	0x44c778 <.text+0x4b778>
  44c72a: d9 44 24 10                  	flds	0x10(%esp)
  44c72e: d8 1d 78 d5 7d 00            	fcomps	0x7dd578
  44c734: df e0                        	fnstsw	%ax
  44c736: f6 c4 41                     	testb	$0x41, %ah
  44c739: 75 0f                        	jne	0x44c74a <.text+0x4b74a>
  44c73b: d9 44 24 10                  	flds	0x10(%esp)
  44c73f: d8 8d d8 00 00 00            	fmuls	0xd8(%ebp)
  44c745: e9 9a 00 00 00               	jmp	0x44c7e4 <.text+0x4b7e4>
  44c74a: d9 85 c4 00 00 00            	flds	0xc4(%ebp)
  44c750: dc c0                        	fadd	%st, %st(0)
  44c752: d8 0d 98 cc 7d 00            	fmuls	0x7dcc98
  44c758: d8 5d 38                     	fcomps	0x38(%ebp)
  44c75b: df e0                        	fnstsw	%ax
  44c75d: f6 c4 41                     	testb	$0x41, %ah
  44c760: 75 0c                        	jne	0x44c76e <.text+0x4b76e>
  44c762: d9 44 24 10                  	flds	0x10(%esp)
  44c766: d8 8d d8 00 00 00            	fmuls	0xd8(%ebp)
  44c76c: eb 76                        	jmp	0x44c7e4 <.text+0x4b7e4>
  44c76e: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  44c776: eb 70                        	jmp	0x44c7e8 <.text+0x4b7e8>
  44c778: d9 45 38                     	flds	0x38(%ebp)
  44c77b: d8 9d c8 00 00 00            	fcomps	0xc8(%ebp)
  44c781: d9 44 24 10                  	flds	0x10(%esp)
  44c785: df e0                        	fnstsw	%ax
  44c787: f6 c4 05                     	testb	$0x5, %ah
  44c78a: 7a 40                        	jp	0x44c7cc <.text+0x4b7cc>
  44c78c: d8 1d f0 ff 7d 00            	fcomps	0x7dfff0
  44c792: d9 44 24 10                  	flds	0x10(%esp)
  44c796: d8 8d d8 00 00 00            	fmuls	0xd8(%ebp)
  44c79c: df e0                        	fnstsw	%ax
  44c79e: f6 c4 41                     	testb	$0x41, %ah
  44c7a1: 74 41                        	je	0x44c7e4 <.text+0x4b7e4>
  44c7a3: d8 0d 40 d5 7d 00            	fmuls	0x7dd540
  44c7a9: eb 39                        	jmp	0x44c7e4 <.text+0x4b7e4>
  44c7ab: d8 9d ac 00 00 00            	fcomps	0xac(%ebp)
  44c7b1: df e0                        	fnstsw	%ax
  44c7b3: f6 c4 01                     	testb	$0x1, %ah
  44c7b6: 75 1c                        	jne	0x44c7d4 <.text+0x4b7d4>
  44c7b8: d9 45 1c                     	flds	0x1c(%ebp)
  44c7bb: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44c7c1: df e0                        	fnstsw	%ax
  44c7c3: f6 c4 05                     	testb	$0x5, %ah
  44c7c6: 7a 0c                        	jp	0x44c7d4 <.text+0x4b7d4>
  44c7c8: d9 44 24 10                  	flds	0x10(%esp)
  44c7cc: d8 8d d8 00 00 00            	fmuls	0xd8(%ebp)
  44c7d2: eb 10                        	jmp	0x44c7e4 <.text+0x4b7e4>
  44c7d4: d9 44 24 10                  	flds	0x10(%esp)
  44c7d8: d8 8d d8 00 00 00            	fmuls	0xd8(%ebp)
  44c7de: d8 0d 50 d5 7d 00            	fmuls	0x7dd550
  44c7e4: d9 5c 24 10                  	fstps	0x10(%esp)
  44c7e8: 8b 47 18                     	movl	0x18(%edi), %eax
  44c7eb: 56                           	pushl	%esi
  44c7ec: 50                           	pushl	%eax
  44c7ed: e8 be bb fe ff               	calll	0x4383b0 <.text+0x373b0>
  44c7f2: d9 45 38                     	flds	0x38(%ebp)
  44c7f5: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44c7fb: 8b c8                        	movl	%eax, %ecx
  44c7fd: 83 c4 08                     	addl	$0x8, %esp
  44c800: df e0                        	fnstsw	%ax
  44c802: f6 c4 44                     	testb	$0x44, %ah
  44c805: 7a 21                        	jp	0x44c828 <.text+0x4b828>
  44c807: 81 f9 00 10 00 00            	cmpl	$0x1000, %ecx           # imm = 0x1000
  44c80d: 7e 19                        	jle	0x44c828 <.text+0x4b828>
  44c80f: 56                           	pushl	%esi
  44c810: 53                           	pushl	%ebx
  44c811: 8b c5                        	movl	%ebp, %eax
  44c813: 89 73 20                     	movl	%esi, 0x20(%ebx)
  44c816: c7 44 24 18 00 00 00 00      	movl	$0x0, 0x18(%esp)
  44c81e: e8 cd 75 ff ff               	calll	0x443df0 <.text+0x42df0>
  44c823: e9 e1 00 00 00               	jmp	0x44c909 <.text+0x4b909>
  44c828: d9 85 c4 00 00 00            	flds	0xc4(%ebp)
  44c82e: d8 85 c0 00 00 00            	fadds	0xc0(%ebp)
  44c834: d8 0d 98 cc 7d 00            	fmuls	0x7dcc98
  44c83a: d8 5d 38                     	fcomps	0x38(%ebp)
  44c83d: df e0                        	fnstsw	%ax
  44c83f: f6 c4 41                     	testb	$0x41, %ah
  44c842: 7a 1e                        	jp	0x44c862 <.text+0x4b862>
  44c844: 81 f9 00 10 00 00            	cmpl	$0x1000, %ecx           # imm = 0x1000
  44c84a: 7e 16                        	jle	0x44c862 <.text+0x4b862>
  44c84c: 8b 44 24 48                  	movl	0x48(%esp), %eax
  44c850: 89 70 20                     	movl	%esi, 0x20(%eax)
  44c853: 8b 8d e0 00 00 00            	movl	0xe0(%ebp), %ecx
  44c859: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  44c85d: e9 9a 00 00 00               	jmp	0x44c8fc <.text+0x4b8fc>
  44c862: d9 45 38                     	flds	0x38(%ebp)
  44c865: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  44c86b: df e0                        	fnstsw	%ax
  44c86d: f6 c4 01                     	testb	$0x1, %ah
  44c870: 75 1e                        	jne	0x44c890 <.text+0x4b890>
  44c872: 81 f9 00 10 00 00            	cmpl	$0x1000, %ecx           # imm = 0x1000
  44c878: 7d 16                        	jge	0x44c890 <.text+0x4b890>
  44c87a: 8b 44 24 48                  	movl	0x48(%esp), %eax
  44c87e: 8b 5c 24 44                  	movl	0x44(%esp), %ebx
  44c882: 56                           	pushl	%esi
  44c883: 89 70 20                     	movl	%esi, 0x20(%eax)
  44c886: 50                           	pushl	%eax
  44c887: 8b f5                        	movl	%ebp, %esi
  44c889: e8 d2 75 ff ff               	calll	0x443e60 <.text+0x42e60>
  44c88e: eb 79                        	jmp	0x44c909 <.text+0x4b909>
  44c890: d9 45 38                     	flds	0x38(%ebp)
  44c893: d8 9d d0 00 00 00            	fcomps	0xd0(%ebp)
  44c899: df e0                        	fnstsw	%ax
  44c89b: f6 c4 01                     	testb	$0x1, %ah
  44c89e: 75 1f                        	jne	0x44c8bf <.text+0x4b8bf>
  44c8a0: f7 45 5c 00 00 80 00         	testl	$0x800000, 0x5c(%ebp)   # imm = 0x800000
  44c8a7: 74 16                        	je	0x44c8bf <.text+0x4b8bf>
  44c8a9: 8b 44 24 48                  	movl	0x48(%esp), %eax
  44c8ad: 8b 5c 24 44                  	movl	0x44(%esp), %ebx
  44c8b1: 56                           	pushl	%esi
  44c8b2: 89 70 20                     	movl	%esi, 0x20(%eax)
  44c8b5: 50                           	pushl	%eax
  44c8b6: 8b f5                        	movl	%ebp, %esi
  44c8b8: e8 a3 75 ff ff               	calll	0x443e60 <.text+0x42e60>
  44c8bd: eb 4a                        	jmp	0x44c909 <.text+0x4b909>
  44c8bf: d9 45 38                     	flds	0x38(%ebp)
  44c8c2: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  44c8c8: df e0                        	fnstsw	%ax
  44c8ca: f6 c4 01                     	testb	$0x1, %ah
  44c8cd: 75 26                        	jne	0x44c8f5 <.text+0x4b8f5>
  44c8cf: d9 45 38                     	flds	0x38(%ebp)
  44c8d2: d8 9d c8 00 00 00            	fcomps	0xc8(%ebp)
  44c8d8: df e0                        	fnstsw	%ax
  44c8da: f6 c4 41                     	testb	$0x41, %ah
  44c8dd: 7a 16                        	jp	0x44c8f5 <.text+0x4b8f5>
  44c8df: 81 f9 00 20 00 00            	cmpl	$0x2000, %ecx           # imm = 0x2000
  44c8e5: 7e 0e                        	jle	0x44c8f5 <.text+0x4b8f5>
  44c8e7: d9 44 24 10                  	flds	0x10(%esp)
  44c8eb: d8 0d 5c ce 7d 00            	fmuls	0x7dce5c
  44c8f1: d9 5c 24 10                  	fstps	0x10(%esp)
  44c8f5: 8b 44 24 48                  	movl	0x48(%esp), %eax
  44c8f9: 89 70 20                     	movl	%esi, 0x20(%eax)
  44c8fc: 56                           	pushl	%esi
  44c8fd: 8b 74 24 48                  	movl	0x48(%esp), %esi
  44c901: 8b fd                        	movl	%ebp, %edi
  44c903: 50                           	pushl	%eax
  44c904: e8 47 73 ff ff               	calll	0x443c50 <.text+0x42c50>
  44c909: 8b 54 24 18                  	movl	0x18(%esp), %edx
  44c90d: 8b 44 24 50                  	movl	0x50(%esp), %eax
  44c911: 8b 74 24 20                  	movl	0x20(%esp), %esi
  44c915: 8b ca                        	movl	%edx, %ecx
  44c917: 83 c4 08                     	addl	$0x8, %esp
  44c91a: 89 50 34                     	movl	%edx, 0x34(%eax)
  44c91d: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  44c921: 8b d8                        	movl	%eax, %ebx
  44c923: d9 45 1c                     	flds	0x1c(%ebp)
  44c926: b1 04                        	movb	$0x4, %cl
  44c928: d8 1d 38 01 7e 00            	fcomps	0x7e0138
  44c92e: df e0                        	fnstsw	%ax
  44c930: f6 c4 05                     	testb	$0x5, %ah
  44c933: 7a 47                        	jp	0x44c97c <.text+0x4b97c>
  44c935: d9 45 38                     	flds	0x38(%ebp)
  44c938: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  44c93e: df e0                        	fnstsw	%ax
  44c940: f6 c4 05                     	testb	$0x5, %ah
  44c943: 7a 37                        	jp	0x44c97c <.text+0x4b97c>
  44c945: d9 85 c0 00 00 00            	flds	0xc0(%ebp)
  44c94b: d9 e0                        	fchs
  44c94d: d8 5d 38                     	fcomps	0x38(%ebp)
  44c950: df e0                        	fnstsw	%ax
  44c952: f6 c4 05                     	testb	$0x5, %ah
  44c955: 7a 25                        	jp	0x44c97c <.text+0x4b97c>
  44c957: 85 f6                        	testl	%esi, %esi
  44c959: 75 21                        	jne	0x44c97c <.text+0x4b97c>
  44c95b: d9 44 24 1c                  	flds	0x1c(%esp)
  44c95f: d8 0d 34 cb 7d 00            	fmuls	0x7dcb34
  44c965: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44c969: d9 44 24 24                  	flds	0x24(%esp)
  44c96d: d8 0d 34 cb 7d 00            	fmuls	0x7dcb34
  44c973: d9 5c 24 24                  	fstps	0x24(%esp)
  44c977: e9 e0 02 00 00               	jmp	0x44cc5c <.text+0x4bc5c>
  44c97c: d9 45 38                     	flds	0x38(%ebp)
  44c97f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44c985: df e0                        	fnstsw	%ax
  44c987: f6 c4 44                     	testb	$0x44, %ah
  44c98a: 7a 7c                        	jp	0x44ca08 <.text+0x4ba08>
  44c98c: 85 f6                        	testl	%esi, %esi
  44c98e: d9 85 04 01 00 00            	flds	0x104(%ebp)
  44c994: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44c997: d8 4c 24 20                  	fmuls	0x20(%esp)
  44c99b: d9 5c 24 44                  	fstps	0x44(%esp)
  44c99f: d9 44 24 1c                  	flds	0x1c(%esp)
  44c9a3: d8 44 24 28                  	fadds	0x28(%esp)
  44c9a7: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44c9ab: 0f 85 ab 02 00 00            	jne	0x44cc5c <.text+0x4bc5c>
  44c9b1: d9 44 24 1c                  	flds	0x1c(%esp)
  44c9b5: d8 5c 24 44                  	fcomps	0x44(%esp)
  44c9b9: df e0                        	fnstsw	%ax
  44c9bb: f6 c4 05                     	testb	$0x5, %ah
  44c9be: 7a 11                        	jp	0x44c9d1 <.text+0x4b9d1>
  44c9c0: d9 44 24 44                  	flds	0x44(%esp)
  44c9c4: d9 e0                        	fchs
  44c9c6: d8 5c 24 1c                  	fcomps	0x1c(%esp)
  44c9ca: df e0                        	fnstsw	%ax
  44c9cc: f6 c4 05                     	testb	$0x5, %ah
  44c9cf: 7b 2a                        	jnp	0x44c9fb <.text+0x4b9fb>
  44c9d1: d9 44 24 1c                  	flds	0x1c(%esp)
  44c9d5: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44c9db: df e0                        	fnstsw	%ax
  44c9dd: f6 c4 05                     	testb	$0x5, %ah
  44c9e0: 0f 8a 76 02 00 00            	jp	0x44cc5c <.text+0x4bc5c>
  44c9e6: d9 44 24 1c                  	flds	0x1c(%esp)
  44c9ea: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44c9f0: df e0                        	fnstsw	%ax
  44c9f2: f6 c4 41                     	testb	$0x41, %ah
  44c9f5: 0f 85 61 02 00 00            	jne	0x44cc5c <.text+0x4bc5c>
  44c9fb: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44ca03: e9 54 02 00 00               	jmp	0x44cc5c <.text+0x4bc5c>
  44ca08: d9 44 24 28                  	flds	0x28(%esp)
  44ca0c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44ca12: df e0                        	fnstsw	%ax
  44ca14: f6 c4 05                     	testb	$0x5, %ah
  44ca17: 0f 8a 46 01 00 00            	jp	0x44cb63 <.text+0x4bb63>
  44ca1d: 84 4d 5c                     	testb	%cl, 0x5c(%ebp)
  44ca20: 0f 84 9c 00 00 00            	je	0x44cac2 <.text+0x4bac2>
  44ca26: d9 44 24 20                  	flds	0x20(%esp)
  44ca2a: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44ca30: df e0                        	fnstsw	%ax
  44ca32: f6 c4 05                     	testb	$0x5, %ah
  44ca35: 0f 8a d9 01 00 00            	jp	0x44cc14 <.text+0x4bc14>
  44ca3b: d9 85 fc 00 00 00            	flds	0xfc(%ebp)
  44ca41: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44ca44: d8 4c 24 20                  	fmuls	0x20(%esp)
  44ca48: d9 54 24 44                  	fsts	0x44(%esp)
  44ca4c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44ca52: df e0                        	fnstsw	%ax
  44ca54: f6 c4 41                     	testb	$0x41, %ah
  44ca57: 0f 85 b7 01 00 00            	jne	0x44cc14 <.text+0x4bc14>
  44ca5d: d9 44 24 28                  	flds	0x28(%esp)
  44ca61: d9 e0                        	fchs
  44ca63: d8 5c 24 44                  	fcomps	0x44(%esp)
  44ca67: df e0                        	fnstsw	%ax
  44ca69: f6 c4 41                     	testb	$0x41, %ah
  44ca6c: 0f 85 a2 01 00 00            	jne	0x44cc14 <.text+0x4bc14>
  44ca72: d9 44 24 1c                  	flds	0x1c(%esp)
  44ca76: d8 44 24 28                  	fadds	0x28(%esp)
  44ca7a: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44ca80: df e0                        	fnstsw	%ax
  44ca82: f6 c4 05                     	testb	$0x5, %ah
  44ca85: 7a 25                        	jp	0x44caac <.text+0x4baac>
  44ca87: d9 44 24 44                  	flds	0x44(%esp)
  44ca8b: d9 e0                        	fchs
  44ca8d: d9 5c 24 14                  	fstps	0x14(%esp)
  44ca91: d8 54 24 14                  	fcoms	0x14(%esp)
  44ca95: df e0                        	fnstsw	%ax
  44ca97: f6 c4 05                     	testb	$0x5, %ah
  44ca9a: 7a 10                        	jp	0x44caac <.text+0x4baac>
  44ca9c: d8 64 24 44                  	fsubs	0x44(%esp)
  44caa0: d9 5b 38                     	fstps	0x38(%ebx)
  44caa3: d9 44 24 14                  	flds	0x14(%esp)
  44caa7: e9 ac 01 00 00               	jmp	0x44cc58 <.text+0x4bc58>
  44caac: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44cab2: df e0                        	fnstsw	%ax
  44cab4: f6 c4 41                     	testb	$0x41, %ah
  44cab7: 0f 85 9b 01 00 00            	jne	0x44cc58 <.text+0x4bc58>
  44cabd: e9 3a 01 00 00               	jmp	0x44cbfc <.text+0x4bbfc>
  44cac2: d9 45 38                     	flds	0x38(%ebp)
  44cac5: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cacb: df e0                        	fnstsw	%ax
  44cacd: f6 c4 41                     	testb	$0x41, %ah
  44cad0: 75 2c                        	jne	0x44cafe <.text+0x4bafe>
  44cad2: d9 44 24 1c                  	flds	0x1c(%esp)
  44cad6: d8 44 24 28                  	fadds	0x28(%esp)
  44cada: d9 44 24 28                  	flds	0x28(%esp)
  44cade: d8 c9                        	fmul	%st(1), %st
  44cae0: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cae6: df e0                        	fnstsw	%ax
  44cae8: f6 c4 05                     	testb	$0x5, %ah
  44caeb: 0f 8a 67 01 00 00            	jp	0x44cc58 <.text+0x4bc58>
  44caf1: dd d8                        	fstp	%st(0)
  44caf3: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44caf9: e9 5a 01 00 00               	jmp	0x44cc58 <.text+0x4bc58>
  44cafe: 85 f6                        	testl	%esi, %esi
  44cb00: 75 32                        	jne	0x44cb34 <.text+0x4bb34>
  44cb02: d9 45 38                     	flds	0x38(%ebp)
  44cb05: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  44cb0b: df e0                        	fnstsw	%ax
  44cb0d: f6 c4 41                     	testb	$0x41, %ah
  44cb10: 7a 22                        	jp	0x44cb34 <.text+0x4bb34>
  44cb12: d9 44 24 1c                  	flds	0x1c(%esp)
  44cb16: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44cb1c: df e0                        	fnstsw	%ax
  44cb1e: f6 c4 05                     	testb	$0x5, %ah
  44cb21: 7a 11                        	jp	0x44cb34 <.text+0x4bb34>
  44cb23: d9 44 24 1c                  	flds	0x1c(%esp)
  44cb27: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44cb2d: df e0                        	fnstsw	%ax
  44cb2f: f6 c4 41                     	testb	$0x41, %ah
  44cb32: 74 24                        	je	0x44cb58 <.text+0x4bb58>
  44cb34: d9 85 04 01 00 00            	flds	0x104(%ebp)
  44cb3a: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44cb3d: d8 4c 24 20                  	fmuls	0x20(%esp)
  44cb41: d8 44 24 28                  	fadds	0x28(%esp)
  44cb45: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44cb4b: df e0                        	fnstsw	%ax
  44cb4d: f6 c4 01                     	testb	$0x1, %ah
  44cb50: 0f 85 02 01 00 00            	jne	0x44cc58 <.text+0x4bc58>
  44cb56: dd d8                        	fstp	%st(0)
  44cb58: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44cb5e: e9 f5 00 00 00               	jmp	0x44cc58 <.text+0x4bc58>
  44cb63: d9 85 04 01 00 00            	flds	0x104(%ebp)
  44cb69: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44cb6c: d8 4c 24 20                  	fmuls	0x20(%esp)
  44cb70: d9 5c 24 44                  	fstps	0x44(%esp)
  44cb74: d9 45 38                     	flds	0x38(%ebp)
  44cb77: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cb7d: df e0                        	fnstsw	%ax
  44cb7f: f6 c4 05                     	testb	$0x5, %ah
  44cb82: 7a 50                        	jp	0x44cbd4 <.text+0x4bbd4>
  44cb84: d9 44 24 1c                  	flds	0x1c(%esp)
  44cb88: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44cb8e: df e0                        	fnstsw	%ax
  44cb90: f6 c4 05                     	testb	$0x5, %ah
  44cb93: 7a 3f                        	jp	0x44cbd4 <.text+0x4bbd4>
  44cb95: d9 44 24 1c                  	flds	0x1c(%esp)
  44cb99: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44cb9f: df e0                        	fnstsw	%ax
  44cba1: f6 c4 41                     	testb	$0x41, %ah
  44cba4: 75 2e                        	jne	0x44cbd4 <.text+0x4bbd4>
  44cba6: d9 44 24 1c                  	flds	0x1c(%esp)
  44cbaa: d8 44 24 28                  	fadds	0x28(%esp)
  44cbae: d9 54 24 1c                  	fsts	0x1c(%esp)
  44cbb2: d8 4c 24 28                  	fmuls	0x28(%esp)
  44cbb6: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cbbc: df e0                        	fnstsw	%ax
  44cbbe: f6 c4 05                     	testb	$0x5, %ah
  44cbc1: 0f 8a 95 00 00 00            	jp	0x44cc5c <.text+0x4bc5c>
  44cbc7: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44cbcf: e9 88 00 00 00               	jmp	0x44cc5c <.text+0x4bc5c>
  44cbd4: d9 44 24 44                  	flds	0x44(%esp)
  44cbd8: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cbde: df e0                        	fnstsw	%ax
  44cbe0: f6 c4 41                     	testb	$0x41, %ah
  44cbe3: 75 2f                        	jne	0x44cc14 <.text+0x4bc14>
  44cbe5: d9 44 24 44                  	flds	0x44(%esp)
  44cbe9: d8 5c 24 28                  	fcomps	0x28(%esp)
  44cbed: df e0                        	fnstsw	%ax
  44cbef: f6 c4 05                     	testb	$0x5, %ah
  44cbf2: 7a 20                        	jp	0x44cc14 <.text+0x4bc14>
  44cbf4: d9 44 24 1c                  	flds	0x1c(%esp)
  44cbf8: d8 44 24 28                  	fadds	0x28(%esp)
  44cbfc: d8 54 24 44                  	fcoms	0x44(%esp)
  44cc00: df e0                        	fnstsw	%ax
  44cc02: f6 c4 41                     	testb	$0x41, %ah
  44cc05: 75 51                        	jne	0x44cc58 <.text+0x4bc58>
  44cc07: d8 64 24 44                  	fsubs	0x44(%esp)
  44cc0b: d9 5b 38                     	fstps	0x38(%ebx)
  44cc0e: d9 44 24 44                  	flds	0x44(%esp)
  44cc12: eb 44                        	jmp	0x44cc58 <.text+0x4bc58>
  44cc14: 85 f6                        	testl	%esi, %esi
  44cc16: 75 38                        	jne	0x44cc50 <.text+0x4bc50>
  44cc18: d9 45 38                     	flds	0x38(%ebp)
  44cc1b: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  44cc21: df e0                        	fnstsw	%ax
  44cc23: f6 c4 41                     	testb	$0x41, %ah
  44cc26: 7a 28                        	jp	0x44cc50 <.text+0x4bc50>
  44cc28: d9 44 24 1c                  	flds	0x1c(%esp)
  44cc2c: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44cc32: df e0                        	fnstsw	%ax
  44cc34: f6 c4 05                     	testb	$0x5, %ah
  44cc37: 7a 17                        	jp	0x44cc50 <.text+0x4bc50>
  44cc39: d9 44 24 1c                  	flds	0x1c(%esp)
  44cc3d: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  44cc43: df e0                        	fnstsw	%ax
  44cc45: f6 c4 41                     	testb	$0x41, %ah
  44cc48: 75 06                        	jne	0x44cc50 <.text+0x4bc50>
  44cc4a: 89 74 24 1c                  	movl	%esi, 0x1c(%esp)
  44cc4e: eb 0c                        	jmp	0x44cc5c <.text+0x4bc5c>
  44cc50: d9 44 24 1c                  	flds	0x1c(%esp)
  44cc54: d8 44 24 28                  	fadds	0x28(%esp)
  44cc58: d9 5c 24 1c                  	fstps	0x1c(%esp)
  44cc5c: d9 45 40                     	flds	0x40(%ebp)
  44cc5f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cc65: df e0                        	fnstsw	%ax
  44cc67: f6 c4 44                     	testb	$0x44, %ah
  44cc6a: 7a 3c                        	jp	0x44cca8 <.text+0x4bca8>
  44cc6c: d9 85 04 01 00 00            	flds	0x104(%ebp)
  44cc72: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44cc75: d8 4c 24 20                  	fmuls	0x20(%esp)
  44cc79: d9 44 24 24                  	flds	0x24(%esp)
  44cc7d: d8 d9                        	fcomp	%st(1)
  44cc7f: df e0                        	fnstsw	%ax
  44cc81: f6 c4 05                     	testb	$0x5, %ah
  44cc84: 0f 8a 2d 01 00 00            	jp	0x44cdb7 <.text+0x4bdb7>
  44cc8a: d9 e0                        	fchs
  44cc8c: d8 5c 24 24                  	fcomps	0x24(%esp)
  44cc90: df e0                        	fnstsw	%ax
  44cc92: f6 c4 05                     	testb	$0x5, %ah
  44cc95: 0f 8a 1e 01 00 00            	jp	0x44cdb9 <.text+0x4bdb9>
  44cc9b: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  44cca3: e9 11 01 00 00               	jmp	0x44cdb9 <.text+0x4bdb9>
  44cca8: d9 44 24 20                  	flds	0x20(%esp)
  44ccac: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44ccb2: df e0                        	fnstsw	%ax
  44ccb4: f6 c4 05                     	testb	$0x5, %ah
  44ccb7: 7a 13                        	jp	0x44cccc <.text+0x4bccc>
  44ccb9: d9 85 00 01 00 00            	flds	0x100(%ebp)
  44ccbf: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44ccc2: d8 4c 24 20                  	fmuls	0x20(%esp)
  44ccc6: d9 5c 24 44                  	fstps	0x44(%esp)
  44ccca: eb 08                        	jmp	0x44ccd4 <.text+0x4bcd4>
  44cccc: c7 44 24 44 00 00 00 00      	movl	$0x0, 0x44(%esp)
  44ccd4: 8a 45 5c                     	movb	0x5c(%ebp), %al
  44ccd7: d9 44 24 24                  	flds	0x24(%esp)
  44ccdb: 84 c1                        	testb	%al, %cl
  44ccdd: 74 5b                        	je	0x44cd3a <.text+0x4bd3a>
  44ccdf: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44cce5: df e0                        	fnstsw	%ax
  44cce7: f6 c4 41                     	testb	$0x41, %ah
  44ccea: 75 22                        	jne	0x44cd0e <.text+0x4bd0e>
  44ccec: d8 64 24 44                  	fsubs	0x44(%esp)
  44ccf0: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44ccf6: df e0                        	fnstsw	%ax
  44ccf8: f6 c4 05                     	testb	$0x5, %ah
  44ccfb: 7a 37                        	jp	0x44cd34 <.text+0x4bd34>
  44ccfd: dd d8                        	fstp	%st(0)
  44ccff: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44cd05: d9 5c 24 24                  	fstps	0x24(%esp)
  44cd09: e9 ab 00 00 00               	jmp	0x44cdb9 <.text+0x4bdb9>
  44cd0e: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44cd14: df e0                        	fnstsw	%ax
  44cd16: f6 c4 05                     	testb	$0x5, %ah
  44cd19: 7a 19                        	jp	0x44cd34 <.text+0x4bd34>
  44cd1b: d8 44 24 44                  	fadds	0x44(%esp)
  44cd1f: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44cd25: df e0                        	fnstsw	%ax
  44cd27: f6 c4 41                     	testb	$0x41, %ah
  44cd2a: 75 08                        	jne	0x44cd34 <.text+0x4bd34>
  44cd2c: dd d8                        	fstp	%st(0)
  44cd2e: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44cd34: d9 5c 24 24                  	fstps	0x24(%esp)
  44cd38: eb 7f                        	jmp	0x44cdb9 <.text+0x4bdb9>
  44cd3a: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cd40: df e0                        	fnstsw	%ax
  44cd42: f6 c4 41                     	testb	$0x41, %ah
  44cd45: 75 0c                        	jne	0x44cd53 <.text+0x4bd53>
  44cd47: d9 44 24 44                  	flds	0x44(%esp)
  44cd4b: d9 e0                        	fchs
  44cd4d: d9 5c 24 30                  	fstps	0x30(%esp)
  44cd51: eb 19                        	jmp	0x44cd6c <.text+0x4bd6c>
  44cd53: d9 44 24 24                  	flds	0x24(%esp)
  44cd57: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cd5d: df e0                        	fnstsw	%ax
  44cd5f: f6 c4 05                     	testb	$0x5, %ah
  44cd62: 7a 08                        	jp	0x44cd6c <.text+0x4bd6c>
  44cd64: 8b 54 24 44                  	movl	0x44(%esp), %edx
  44cd68: 89 54 24 30                  	movl	%edx, 0x30(%esp)
  44cd6c: d9 44 24 24                  	flds	0x24(%esp)
  44cd70: d8 44 24 30                  	fadds	0x30(%esp)
  44cd74: d9 44 24 24                  	flds	0x24(%esp)
  44cd78: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cd7e: df e0                        	fnstsw	%ax
  44cd80: f6 c4 44                     	testb	$0x44, %ah
  44cd83: 7b 2c                        	jnp	0x44cdb1 <.text+0x4bdb1>
  44cd85: d9 44 24 30                  	flds	0x30(%esp)
  44cd89: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cd8f: df e0                        	fnstsw	%ax
  44cd91: f6 c4 44                     	testb	$0x44, %ah
  44cd94: 7b 1b                        	jnp	0x44cdb1 <.text+0x4bdb1>
  44cd96: d9 44 24 24                  	flds	0x24(%esp)
  44cd9a: d8 c9                        	fmul	%st(1), %st
  44cd9c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cda2: df e0                        	fnstsw	%ax
  44cda4: f6 c4 05                     	testb	$0x5, %ah
  44cda7: 7a 08                        	jp	0x44cdb1 <.text+0x4bdb1>
  44cda9: dd d8                        	fstp	%st(0)
  44cdab: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44cdb1: d9 5c 24 24                  	fstps	0x24(%esp)
  44cdb5: eb 02                        	jmp	0x44cdb9 <.text+0x4bdb9>
  44cdb7: dd d8                        	fstp	%st(0)
  44cdb9: d9 44 24 20                  	flds	0x20(%esp)
  44cdbd: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  44cdc1: d8 44 24 2c                  	fadds	0x2c(%esp)
  44cdc5: 8b 54 24 24                  	movl	0x24(%esp), %edx
  44cdc9: 83 c5 2c                     	addl	$0x2c, %ebp
  44cdcc: 89 45 00                     	movl	%eax, (%ebp)
  44cdcf: d9 5c 24 20                  	fstps	0x20(%esp)
  44cdd3: 8b 4c 24 20                  	movl	0x20(%esp), %ecx
  44cdd7: 5e                           	popl	%esi
  44cdd8: 89 4d 04                     	movl	%ecx, 0x4(%ebp)
  44cddb: 5b                           	popl	%ebx
  44cddc: 89 55 08                     	movl	%edx, 0x8(%ebp)
  44cddf: 5d                           	popl	%ebp
  44cde0: 5f                           	popl	%edi
  44cde1: 83 c4 30                     	addl	$0x30, %esp
  44cde4: c3                           	retl
