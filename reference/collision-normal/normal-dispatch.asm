
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  41bc00: 83 ec 2c                     	subl	$0x2c, %esp
  41bc03: 55                           	pushl	%ebp
  41bc04: 8b 6c 24 34                  	movl	0x34(%esp), %ebp
  41bc08: 8b 45 38                     	movl	0x38(%ebp), %eax
  41bc0b: 8b 88 a4 00 00 00            	movl	0xa4(%eax), %ecx
  41bc11: 85 c9                        	testl	%ecx, %ecx
  41bc13: 66 8b 90 a2 00 00 00         	movw	0xa2(%eax), %dx
  41bc1a: 8b 40 0c                     	movl	0xc(%eax), %eax
  41bc1d: 75 07                        	jne	0x41bc26 <.text+0x1ac26>
  41bc1f: 32 c0                        	xorb	%al, %al
  41bc21: 5d                           	popl	%ebp
  41bc22: 83 c4 2c                     	addl	$0x2c, %esp
  41bc25: c3                           	retl
  41bc26: 56                           	pushl	%esi
  41bc27: 57                           	pushl	%edi
  41bc28: 0f bf fa                     	movswl	%dx, %edi
  41bc2b: 0f b7 51 06                  	movzwl	0x6(%ecx), %edx
  41bc2f: 3b fa                        	cmpl	%edx, %edi
  41bc31: 0f 8f 6a 02 00 00            	jg	0x41bea1 <.text+0x1aea1>
  41bc37: 8b 71 0c                     	movl	0xc(%ecx), %esi
  41bc3a: 8d 3c 7f                     	leal	(%edi,%edi,2), %edi
  41bc3d: c1 e7 04                     	shll	$0x4, %edi
  41bc40: 03 fe                        	addl	%esi, %edi
  41bc42: 85 c0                        	testl	%eax, %eax
  41bc44: 0f 84 57 02 00 00            	je	0x41bea1 <.text+0x1aea1>
  41bc4a: 85 ff                        	testl	%edi, %edi
  41bc4c: 0f 84 4f 02 00 00            	je	0x41bea1 <.text+0x1aea1>
  41bc52: 8b 50 14                     	movl	0x14(%eax), %edx
  41bc55: 8b 89 9c 00 00 00            	movl	0x9c(%ecx), %ecx
  41bc5b: 8b 71 20                     	movl	0x20(%ecx), %esi
  41bc5e: 89 54 24 3c                  	movl	%edx, 0x3c(%esp)
  41bc62: 8b 57 14                     	movl	0x14(%edi), %edx
  41bc65: 89 54 24 0c                  	movl	%edx, 0xc(%esp)
  41bc69: 8b 57 1c                     	movl	0x1c(%edi), %edx
  41bc6c: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  41bc70: 8d 54 24 20                  	leal	0x20(%esp), %edx
  41bc74: 52                           	pushl	%edx
  41bc75: 50                           	pushl	%eax
  41bc76: 55                           	pushl	%ebp
  41bc77: e8 f4 d5 ff ff               	calll	0x419270 <.text+0x18270>
  41bc7c: 0f b6 47 01                  	movzbl	0x1(%edi), %eax
  41bc80: 83 c4 0c                     	addl	$0xc, %esp
  41bc83: 83 f8 07                     	cmpl	$0x7, %eax
  41bc86: 0f 87 15 02 00 00            	ja	0x41bea1 <.text+0x1aea1>
  41bc8c: ff 24 85 ac be 41 00         	jmpl	*0x41beac(,%eax,4)
  41bc93: 8d 44 24 14                  	leal	0x14(%esp), %eax
  41bc97: 50                           	pushl	%eax
  41bc98: 57                           	pushl	%edi
  41bc99: 56                           	pushl	%esi
  41bc9a: e8 d1 d5 ff ff               	calll	0x419270 <.text+0x18270>
  41bc9f: d9 44 24 2c                  	flds	0x2c(%esp)
  41bca3: d8 64 24 20                  	fsubs	0x20(%esp)
  41bca7: 83 c4 0c                     	addl	$0xc, %esp
  41bcaa: d9 13                        	fsts	(%ebx)
  41bcac: d9 44 24 24                  	flds	0x24(%esp)
  41bcb0: d8 64 24 18                  	fsubs	0x18(%esp)
  41bcb4: d9 53 04                     	fsts	0x4(%ebx)
  41bcb7: d9 44 24 28                  	flds	0x28(%esp)
  41bcbb: d8 64 24 1c                  	fsubs	0x1c(%esp)
  41bcbf: d9 53 08                     	fsts	0x8(%ebx)
  41bcc2: d9 c9                        	fxch	%st(1)
  41bcc4: d9 c0                        	fld	%st(0)
  41bcc6: d8 c9                        	fmul	%st(1), %st
  41bcc8: d9 c2                        	fld	%st(2)
  41bcca: d8 cb                        	fmul	%st(3), %st
  41bccc: de c1                        	faddp	%st, %st(1)
  41bcce: d9 c3                        	fld	%st(3)
  41bcd0: d8 cc                        	fmul	%st(4), %st
  41bcd2: de c1                        	faddp	%st, %st(1)
  41bcd4: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  41bcda: dd d8                        	fstp	%st(0)
  41bcdc: df e0                        	fnstsw	%ax
  41bcde: dd d8                        	fstp	%st(0)
  41bce0: f6 c4 44                     	testb	$0x44, %ah
  41bce3: dd d8                        	fstp	%st(0)
  41bce5: 7b 14                        	jnp	0x41bcfb <.text+0x1acfb>
  41bce7: 53                           	pushl	%ebx
  41bce8: e8 d3 cd ff ff               	calll	0x418ac0 <.text+0x17ac0>
  41bced: dd d8                        	fstp	%st(0)
  41bcef: 83 c4 04                     	addl	$0x4, %esp
  41bcf2: 5f                           	popl	%edi
  41bcf3: 5e                           	popl	%esi
  41bcf4: b0 01                        	movb	$0x1, %al
  41bcf6: 5d                           	popl	%ebp
  41bcf7: 83 c4 2c                     	addl	$0x2c, %esp
  41bcfa: c3                           	retl
  41bcfb: 5f                           	popl	%edi
  41bcfc: 5e                           	popl	%esi
  41bcfd: c7 03 00 00 00 00            	movl	$0x0, (%ebx)
  41bd03: c7 43 04 00 00 00 00         	movl	$0x0, 0x4(%ebx)
  41bd0a: c7 43 08 00 00 80 3f         	movl	$0x3f800000, 0x8(%ebx)  # imm = 0x3F800000
  41bd11: b0 01                        	movb	$0x1, %al
  41bd13: 5d                           	popl	%ebp
  41bd14: 83 c4 2c                     	addl	$0x2c, %esp
  41bd17: c3                           	retl
  41bd18: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  41bd1c: 51                           	pushl	%ecx
  41bd1d: e9 27 01 00 00               	jmp	0x41be49 <.text+0x1ae49>
  41bd22: 8d 54 24 14                  	leal	0x14(%esp), %edx
  41bd26: 52                           	pushl	%edx
  41bd27: 57                           	pushl	%edi
  41bd28: 56                           	pushl	%esi
  41bd29: e8 02 d3 ff ff               	calll	0x419030 <.text+0x18030>
  41bd2e: e9 1d 01 00 00               	jmp	0x41be50 <.text+0x1ae50>
  41bd33: 8d 44 24 14                  	leal	0x14(%esp), %eax
  41bd37: 50                           	pushl	%eax
  41bd38: 57                           	pushl	%edi
  41bd39: 56                           	pushl	%esi
  41bd3a: e8 f1 d2 ff ff               	calll	0x419030 <.text+0x18030>
  41bd3f: e9 0c 01 00 00               	jmp	0x41be50 <.text+0x1ae50>
  41bd44: 8b 4d 20                     	movl	0x20(%ebp), %ecx
  41bd47: 8b 55 24                     	movl	0x24(%ebp), %edx
  41bd4a: 8b 45 28                     	movl	0x28(%ebp), %eax
  41bd4d: 89 4c 24 20                  	movl	%ecx, 0x20(%esp)
  41bd51: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  41bd55: 51                           	pushl	%ecx
  41bd56: 57                           	pushl	%edi
  41bd57: 56                           	pushl	%esi
  41bd58: 89 54 24 30                  	movl	%edx, 0x30(%esp)
  41bd5c: 89 44 24 34                  	movl	%eax, 0x34(%esp)
  41bd60: e8 3b d6 ff ff               	calll	0x4193a0 <.text+0x183a0>
  41bd65: 8b 47 04                     	movl	0x4(%edi), %eax
  41bd68: 83 c4 0c                     	addl	$0xc, %esp
  41bd6b: a8 20                        	testb	$0x20, %al
  41bd6d: 75 0c                        	jne	0x41bd7b <.text+0x1ad7b>
  41bd6f: 84 e4                        	testb	%ah, %ah
  41bd71: 78 08                        	js	0x41bd7b <.text+0x1ad7b>
  41bd73: 8b 76 18                     	movl	0x18(%esi), %esi
  41bd76: 03 77 28                     	addl	0x28(%edi), %esi
  41bd79: eb 03                        	jmp	0x41bd7e <.text+0x1ad7e>
  41bd7b: 8b 77 28                     	movl	0x28(%edi), %esi
  41bd7e: f7 de                        	negl	%esi
  41bd80: 8d 7c 24 2c                  	leal	0x2c(%esp), %edi
  41bd84: 8d 44 24 14                  	leal	0x14(%esp), %eax
  41bd88: 8d 4c 24 20                  	leal	0x20(%esp), %ecx
  41bd8c: e8 6f db ff ff               	calll	0x419900 <.text+0x18900>
  41bd91: d9 44 24 0c                  	flds	0xc(%esp)
  41bd95: d8 44 24 3c                  	fadds	0x3c(%esp)
  41bd99: d8 4c 24 34                  	fmuls	0x34(%esp)
  41bd9d: d9 5c 24 0c                  	fstps	0xc(%esp)
  41bda1: d9 44 24 10                  	flds	0x10(%esp)
  41bda5: d8 44 24 3c                  	fadds	0x3c(%esp)
  41bda9: d8 4c 24 2c                  	fmuls	0x2c(%esp)
  41bdad: d9 54 24 3c                  	fsts	0x3c(%esp)
  41bdb1: d8 44 24 0c                  	fadds	0xc(%esp)
  41bdb5: d9 44 24 0c                  	flds	0xc(%esp)
  41bdb9: d8 64 24 3c                  	fsubs	0x3c(%esp)
  41bdbd: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  41bdc3: df e0                        	fnstsw	%ax
  41bdc5: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  41bdcb: f6 c4 41                     	testb	$0x41, %ah
  41bdce: df e0                        	fnstsw	%ax
  41bdd0: 75 10                        	jne	0x41bde2 <.text+0x1ade2>
  41bdd2: f6 c4 41                     	testb	$0x41, %ah
  41bdd5: 75 04                        	jne	0x41bddb <.text+0x1addb>
  41bdd7: 33 ff                        	xorl	%edi, %edi
  41bdd9: eb 16                        	jmp	0x41bdf1 <.text+0x1adf1>
  41bddb: bf 00 c0 00 00               	movl	$0xc000, %edi           # imm = 0xC000
  41bde0: eb 0f                        	jmp	0x41bdf1 <.text+0x1adf1>
  41bde2: f6 c4 41                     	testb	$0x41, %ah
  41bde5: bf 00 40 00 00               	movl	$0x4000, %edi           # imm = 0x4000
  41bdea: 74 05                        	je	0x41bdf1 <.text+0x1adf1>
  41bdec: bf 00 80 00 00               	movl	$0x8000, %edi           # imm = 0x8000
  41bdf1: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  41bdf6: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  41bdfe: c7 44 24 30 00 00 00 00      	movl	$0x0, 0x30(%esp)
  41be06: c7 44 24 34 00 00 80 3f      	movl	$0x3f800000, 0x34(%esp) # imm = 0x3F800000
  41be0e: e8 dd 92 36 00               	calll	0x7850f0 <.text+0x3840f0>
  41be13: 8b d7                        	movl	%edi, %edx
  41be15: 2b d6                        	subl	%esi, %edx
  41be17: 74 0c                        	je	0x41be25 <.text+0x1ae25>
  41be19: 2b fe                        	subl	%esi, %edi
  41be1b: 0f b7 d7                     	movzwl	%di, %edx
  41be1e: 33 c9                        	xorl	%ecx, %ecx
  41be20: e8 fb 8f 36 00               	calll	0x784e20 <.text+0x383e20>
  41be25: 53                           	pushl	%ebx
  41be26: 8d 54 24 30                  	leal	0x30(%esp), %edx
  41be2a: 33 c9                        	xorl	%ecx, %ecx
  41be2c: e8 2f 8f 36 00               	calll	0x784d60 <.text+0x383d60>
  41be31: b9 01 00 00 00               	movl	$0x1, %ecx
  41be36: e8 05 93 36 00               	calll	0x785140 <.text+0x384140>
  41be3b: 5f                           	popl	%edi
  41be3c: 5e                           	popl	%esi
  41be3d: b0 01                        	movb	$0x1, %al
  41be3f: 5d                           	popl	%ebp
  41be40: 83 c4 2c                     	addl	$0x2c, %esp
  41be43: c3                           	retl
  41be44: 8d 44 24 14                  	leal	0x14(%esp), %eax
  41be48: 50                           	pushl	%eax
  41be49: 57                           	pushl	%edi
  41be4a: 56                           	pushl	%esi
  41be4b: e8 20 d4 ff ff               	calll	0x419270 <.text+0x18270>
  41be50: d9 44 24 2c                  	flds	0x2c(%esp)
  41be54: 33 c9                        	xorl	%ecx, %ecx
  41be56: d8 64 24 20                  	fsubs	0x20(%esp)
  41be5a: 83 c4 0c                     	addl	$0xc, %esp
  41be5d: 89 4b 04                     	movl	%ecx, 0x4(%ebx)
  41be60: d9 13                        	fsts	(%ebx)
  41be62: d9 44 24 28                  	flds	0x28(%esp)
  41be66: d8 64 24 1c                  	fsubs	0x1c(%esp)
  41be6a: d9 53 08                     	fsts	0x8(%ebx)
  41be6d: d9 c0                        	fld	%st(0)
  41be6f: d8 c9                        	fmul	%st(1), %st
  41be71: d9 c2                        	fld	%st(2)
  41be73: d8 cb                        	fmul	%st(3), %st
  41be75: de c1                        	faddp	%st, %st(1)
  41be77: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  41be7d: df e0                        	fnstsw	%ax
  41be7f: dd d8                        	fstp	%st(0)
  41be81: f6 c4 44                     	testb	$0x44, %ah
  41be84: dd d8                        	fstp	%st(0)
  41be86: 0f 8a 5b fe ff ff            	jp	0x41bce7 <.text+0x1ace7>
  41be8c: 5f                           	popl	%edi
  41be8d: 5e                           	popl	%esi
  41be8e: 89 0b                        	movl	%ecx, (%ebx)
  41be90: 89 4b 04                     	movl	%ecx, 0x4(%ebx)
  41be93: c7 43 08 00 00 80 3f         	movl	$0x3f800000, 0x8(%ebx)  # imm = 0x3F800000
  41be9a: b0 01                        	movb	$0x1, %al
  41be9c: 5d                           	popl	%ebp
  41be9d: 83 c4 2c                     	addl	$0x2c, %esp
  41bea0: c3                           	retl
  41bea1: 5f                           	popl	%edi
  41bea2: 5e                           	popl	%esi
  41bea3: 32 c0                        	xorb	%al, %al
  41bea5: 5d                           	popl	%ebp
  41bea6: 83 c4 2c                     	addl	$0x2c, %esp
  41bea9: c3                           	retl
  41beaa: 8b ff                        	movl	%edi, %edi
  41beac: 93                           	xchgl	%ebx, %eax
  41bead: bc 41 00 18 bd               	movl	$0xbd180041, %esp       # imm = 0xBD180041
  41beb2: 41                           	incl	%ecx
  41beb3: 00 22                        	addb	%ah, (%edx)
  41beb5: bd 41 00 44 bd               	movl	$0xbd440041, %ebp       # imm = 0xBD440041
  41beba: 41                           	incl	%ecx
  41bebb: 00 44 be 41                  	addb	%al, 0x41(%esi,%edi,4)
  41bebf: 00 a1 be 41 00 33            	addb	%ah, 0x330041be(%ecx)
  41bec5: bd 41 00 93 bc               	movl	$0xbc930041, %ebp       # imm = 0xBC930041
  41beca: 41                           	incl	%ecx
  41becb: 00 90 90 90 90 8b            	addb	%dl, -0x746f6f70(%eax)
