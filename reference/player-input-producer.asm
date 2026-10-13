
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  40f170: 83 ec 08                     	subl	$0x8, %esp
  40f173: 53                           	pushl	%ebx
  40f174: 55                           	pushl	%ebp
  40f175: 56                           	pushl	%esi
  40f176: 57                           	pushl	%edi
  40f177: 33 ed                        	xorl	%ebp, %ebp
  40f179: 33 ff                        	xorl	%edi, %edi
  40f17b: b3 01                        	movb	$0x1, %bl
  40f17d: 8d 49 00                     	leal	(%ecx), %ecx
  40f180: 83 ff 02                     	cmpl	$0x2, %edi
  40f183: 7d 71                        	jge	0x40f1f6 <.text+0xe1f6>
  40f185: 8b 04 bd 7c e7 b0 03         	movl	0x3b0e77c(,%edi,4), %eax
  40f18c: 0f bf 48 1c                  	movswl	0x1c(%eax), %ecx
  40f190: 0f bf 50 1e                  	movswl	0x1e(%eax), %edx
  40f194: c1 e1 08                     	shll	$0x8, %ecx
  40f197: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  40f19b: c1 e2 08                     	shll	$0x8, %edx
  40f19e: db 44 24 14                  	fildl	0x14(%esp)
  40f1a2: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  40f1a6: d9 5c 24 10                  	fstps	0x10(%esp)
  40f1aa: db 44 24 14                  	fildl	0x14(%esp)
  40f1ae: d9 5c 24 14                  	fstps	0x14(%esp)
  40f1b2: d9 44 24 10                  	flds	0x10(%esp)
  40f1b6: d8 1d f4 cc 7d 00            	fcomps	0x7dccf4
  40f1bc: df e0                        	fnstsw	%ax
  40f1be: f6 c4 41                     	testb	$0x41, %ah
  40f1c1: 74 40                        	je	0x40f203 <.text+0xe203>
  40f1c3: d9 44 24 10                  	flds	0x10(%esp)
  40f1c7: d8 1d f0 cc 7d 00            	fcomps	0x7dccf0
  40f1cd: df e0                        	fnstsw	%ax
  40f1cf: f6 c4 05                     	testb	$0x5, %ah
  40f1d2: 7b 2f                        	jnp	0x40f203 <.text+0xe203>
  40f1d4: d9 44 24 14                  	flds	0x14(%esp)
  40f1d8: d8 1d f4 cc 7d 00            	fcomps	0x7dccf4
  40f1de: df e0                        	fnstsw	%ax
  40f1e0: f6 c4 41                     	testb	$0x41, %ah
  40f1e3: 74 1e                        	je	0x40f203 <.text+0xe203>
  40f1e5: d9 44 24 14                  	flds	0x14(%esp)
  40f1e9: d8 1d f0 cc 7d 00            	fcomps	0x7dccf0
  40f1ef: df e0                        	fnstsw	%ax
  40f1f1: f6 c4 05                     	testb	$0x5, %ah
  40f1f4: 7b 0d                        	jnp	0x40f203 <.text+0xe203>
  40f1f6: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  40f1fc: 33 f6                        	xorl	%esi, %esi
  40f1fe: e9 ea 00 00 00               	jmp	0x40f2ed <.text+0xe2ed>
  40f203: d9 44 24 10                  	flds	0x10(%esp)
  40f207: d8 1d f4 cc 7d 00            	fcomps	0x7dccf4
  40f20d: d9 44 24 10                  	flds	0x10(%esp)
  40f211: df e0                        	fnstsw	%ax
  40f213: f6 c4 41                     	testb	$0x41, %ah
  40f216: 75 08                        	jne	0x40f220 <.text+0xe220>
  40f218: d8 25 f4 cc 7d 00            	fsubs	0x7dccf4
  40f21e: eb 1f                        	jmp	0x40f23f <.text+0xe23f>
  40f220: d8 1d f0 cc 7d 00            	fcomps	0x7dccf0
  40f226: df e0                        	fnstsw	%ax
  40f228: f6 c4 05                     	testb	$0x5, %ah
  40f22b: 7a 0c                        	jp	0x40f239 <.text+0xe239>
  40f22d: d9 44 24 10                  	flds	0x10(%esp)
  40f231: d8 05 f4 cc 7d 00            	fadds	0x7dccf4
  40f237: eb 06                        	jmp	0x40f23f <.text+0xe23f>
  40f239: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  40f23f: d9 44 24 14                  	flds	0x14(%esp)
  40f243: d8 1d f4 cc 7d 00            	fcomps	0x7dccf4
  40f249: d9 44 24 14                  	flds	0x14(%esp)
  40f24d: df e0                        	fnstsw	%ax
  40f24f: f6 c4 41                     	testb	$0x41, %ah
  40f252: 75 08                        	jne	0x40f25c <.text+0xe25c>
  40f254: d8 25 f4 cc 7d 00            	fsubs	0x7dccf4
  40f25a: eb 1f                        	jmp	0x40f27b <.text+0xe27b>
  40f25c: d8 1d f0 cc 7d 00            	fcomps	0x7dccf0
  40f262: df e0                        	fnstsw	%ax
  40f264: f6 c4 05                     	testb	$0x5, %ah
  40f267: 7a 0c                        	jp	0x40f275 <.text+0xe275>
  40f269: d9 44 24 14                  	flds	0x14(%esp)
  40f26d: d8 05 f4 cc 7d 00            	fadds	0x7dccf4
  40f273: eb 06                        	jmp	0x40f27b <.text+0xe27b>
  40f275: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  40f27b: d9 c0                        	fld	%st(0)
  40f27d: 8b 35 b0 cb b2 03            	movl	0x3b2cbb0, %esi
  40f283: 3b f5                        	cmpl	%ebp, %esi
  40f285: d9 c2                        	fld	%st(2)
  40f287: d9 f3                        	fpatan
  40f289: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  40f28f: 74 14                        	je	0x40f2a5 <.text+0xe2a5>
  40f291: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  40f297: e8 a4 50 23 00               	calll	0x644340 <.text+0x243340>
  40f29c: 8b 76 18                     	movl	0x18(%esi), %esi
  40f29f: f7 de                        	negl	%esi
  40f2a1: 2b f0                        	subl	%eax, %esi
  40f2a3: eb 0d                        	jmp	0x40f2b2 <.text+0xe2b2>
  40f2a5: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  40f2ab: e8 90 50 23 00               	calll	0x644340 <.text+0x243340>
  40f2b0: 8b f0                        	movl	%eax, %esi
  40f2b2: d9 c0                        	fld	%st(0)
  40f2b4: d8 c9                        	fmul	%st(1), %st
  40f2b6: d9 c2                        	fld	%st(2)
  40f2b8: d8 cb                        	fmul	%st(3), %st
  40f2ba: de c1                        	faddp	%st, %st(1)
  40f2bc: d9 5c 24 14                  	fstps	0x14(%esp)
  40f2c0: 8b 44 24 14                  	movl	0x14(%esp), %eax
  40f2c4: dd d8                        	fstp	%st(0)
  40f2c6: 50                           	pushl	%eax
  40f2c7: dd d8                        	fstp	%st(0)
  40f2c9: e8 22 7f 37 00               	calll	0x7871f0 <.text+0x3861f0>
  40f2ce: d8 4c 24 14                  	fmuls	0x14(%esp)
  40f2d2: d8 0d e0 cc 7d 00            	fmuls	0x7dcce0
  40f2d8: d8 15 10 cb 7d 00            	fcoms	0x7dcb10
  40f2de: df e0                        	fnstsw	%ax
  40f2e0: f6 c4 41                     	testb	$0x41, %ah
  40f2e3: 75 08                        	jne	0x40f2ed <.text+0xe2ed>
  40f2e5: dd d8                        	fstp	%st(0)
  40f2e7: d9 05 10 cb 7d 00            	flds	0x7dcb10
  40f2ed: a0 b0 9f 90 00               	movb	0x909fb0, %al
  40f2f2: d9 14 fd 6c e3 b0 03         	fsts	0x3b0e36c(,%edi,8)
  40f2f9: 81 e6 ff ff 00 00            	andl	$0xffff, %esi           # imm = 0xFFFF
  40f2ff: 3a c3                        	cmpb	%bl, %al
  40f301: 89 34 fd 68 e3 b0 03         	movl	%esi, 0x3b0e368(,%edi,8)
  40f308: 75 18                        	jne	0x40f322 <.text+0xe322>
  40f30a: 38 9f b4 9f 90 00            	cmpb	%bl, 0x909fb4(%edi)
  40f310: 75 10                        	jne	0x40f322 <.text+0xe322>
  40f312: d9 1c fd a4 e7 b0 03         	fstps	0x3b0e7a4(,%edi,8)
  40f319: 89 34 fd a0 e7 b0 03         	movl	%esi, 0x3b0e7a0(,%edi,8)
  40f320: eb 10                        	jmp	0x40f332 <.text+0xe332>
  40f322: dd d8                        	fstp	%st(0)
  40f324: 89 2c fd a0 e7 b0 03         	movl	%ebp, 0x3b0e7a0(,%edi,8)
  40f32b: 89 2c fd a4 e7 b0 03         	movl	%ebp, 0x3b0e7a4(,%edi,8)
  40f332: 47                           	incl	%edi
  40f333: 83 ff 08                     	cmpl	$0x8, %edi
  40f336: 0f 8c 44 fe ff ff            	jl	0x40f180 <.text+0xe180>
  40f33c: 5f                           	popl	%edi
  40f33d: 5e                           	popl	%esi
  40f33e: 5d                           	popl	%ebp
  40f33f: 5b                           	popl	%ebx
  40f340: 83 c4 08                     	addl	$0x8, %esp
  40f343: c3                           	retl
