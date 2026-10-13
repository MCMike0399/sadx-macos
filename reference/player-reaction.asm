
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43c580: 83 ec 6c                     	subl	$0x6c, %esp
  43c583: 53                           	pushl	%ebx
  43c584: 8b d8                        	movl	%eax, %ebx
  43c586: f6 47 04 03                  	testb	$0x3, 0x4(%edi)
  43c58a: 0f 84 2e 02 00 00            	je	0x43c7be <.text+0x3b7be>
  43c590: 56                           	pushl	%esi
  43c591: 8b 74 24 78                  	movl	0x78(%esp), %esi
  43c595: 83 c6 04                     	addl	$0x4, %esi
  43c598: 8b ce                        	movl	%esi, %ecx
  43c59a: e8 21 b7 34 00               	calll	0x787cc0 <.text+0x386cc0>
  43c59f: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  43c5a5: df e0                        	fnstsw	%ax
  43c5a7: f6 c4 05                     	testb	$0x5, %ah
  43c5aa: 0f 8b 0d 02 00 00            	jnp	0x43c7bd <.text+0x3b7bd>
  43c5b0: 8b 47 38                     	movl	0x38(%edi), %eax
  43c5b3: 66 8b 40 04                  	movw	0x4(%eax), %ax
  43c5b7: a8 10                        	testb	$0x10, %al
  43c5b9: 0f 84 fe 01 00 00            	je	0x43c7bd <.text+0x3b7bd>
  43c5bf: f6 47 04 04                  	testb	$0x4, 0x4(%edi)
  43c5c3: 0f 85 f4 01 00 00            	jne	0x43c7bd <.text+0x3b7bd>
  43c5c9: a8 04                        	testb	$0x4, %al
  43c5cb: 0f 84 ec 01 00 00            	je	0x43c7bd <.text+0x3b7bd>
  43c5d1: 55                           	pushl	%ebp
  43c5d2: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43c5d6: 51                           	pushl	%ecx
  43c5d7: 57                           	pushl	%edi
  43c5d8: e8 f3 f8 fd ff               	calll	0x41bed0 <.text+0x1aed0>
  43c5dd: 8b 44 24 14                  	movl	0x14(%esp), %eax
  43c5e1: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  43c5e5: 8d 6b 44                     	leal	0x44(%ebx), %ebp
  43c5e8: 8b d5                        	movl	%ebp, %edx
  43c5ea: 89 02                        	movl	%eax, (%edx)
  43c5ec: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43c5f0: 89 4a 04                     	movl	%ecx, 0x4(%edx)
  43c5f3: 83 c4 08                     	addl	$0x8, %esp
  43c5f6: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43c5fa: 89 42 08                     	movl	%eax, 0x8(%edx)
  43c5fd: e8 be b6 34 00               	calll	0x787cc0 <.text+0x386cc0>
  43c602: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  43c608: df e0                        	fnstsw	%ax
  43c60a: f6 c4 44                     	testb	$0x44, %ah
  43c60d: 7a 1a                        	jp	0x43c629 <.text+0x3b629>
  43c60f: c7 44 24 0c 00 00 00 00      	movl	$0x0, 0xc(%esp)
  43c617: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  43c61f: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  43c627: eb 0f                        	jmp	0x43c638 <.text+0x3b638>
  43c629: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43c62d: 51                           	pushl	%ecx
  43c62e: e8 8d c4 fd ff               	calll	0x418ac0 <.text+0x17ac0>
  43c633: dd d8                        	fstp	%st(0)
  43c635: 83 c4 04                     	addl	$0x4, %esp
  43c638: 8b 55 00                     	movl	(%ebp), %edx
  43c63b: 8b 4b 4c                     	movl	0x4c(%ebx), %ecx
  43c63e: 8b 43 48                     	movl	0x48(%ebx), %eax
  43c641: 89 54 24 6c                  	movl	%edx, 0x6c(%esp)
  43c645: 8b 53 50                     	movl	0x50(%ebx), %edx
  43c648: 8d 6b 50                     	leal	0x50(%ebx), %ebp
  43c64b: 89 4c 24 74                  	movl	%ecx, 0x74(%esp)
  43c64f: 8b 4b 58                     	movl	0x58(%ebx), %ecx
  43c652: 89 54 24 54                  	movl	%edx, 0x54(%esp)
  43c656: 8d 54 24 30                  	leal	0x30(%esp), %edx
  43c65a: 89 44 24 70                  	movl	%eax, 0x70(%esp)
  43c65e: 8b 43 54                     	movl	0x54(%ebx), %eax
  43c661: 89 4c 24 5c                  	movl	%ecx, 0x5c(%esp)
  43c665: 52                           	pushl	%edx
  43c666: 8d 54 24 4c                  	leal	0x4c(%esp), %edx
  43c66a: 8d 4c 24 64                  	leal	0x64(%esp), %ecx
  43c66e: c7 44 24 64 00 00 00 00      	movl	$0x0, 0x64(%esp)
  43c676: c7 44 24 68 00 00 00 00      	movl	$0x0, 0x68(%esp)
  43c67e: c7 44 24 6c 00 00 00 00      	movl	$0x0, 0x6c(%esp)
  43c686: c7 44 24 4c 00 00 00 00      	movl	$0x0, 0x4c(%esp)
  43c68e: c7 44 24 50 00 00 00 00      	movl	$0x0, 0x50(%esp)
  43c696: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%esp)
  43c69e: 89 44 24 5c                  	movl	%eax, 0x5c(%esp)
  43c6a2: e8 29 c1 34 00               	calll	0x7887d0 <.text+0x3877d0>
  43c6a7: dd d8                        	fstp	%st(0)
  43c6a9: 8b c6                        	movl	%esi, %eax
  43c6ab: 8b 08                        	movl	(%eax), %ecx
  43c6ad: 8b 50 04                     	movl	0x4(%eax), %edx
  43c6b0: 8b 40 08                     	movl	0x8(%eax), %eax
  43c6b3: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  43c6b7: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  43c6bb: 89 54 24 1c                  	movl	%edx, 0x1c(%esp)
  43c6bf: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  43c6c3: e8 f8 b5 34 00               	calll	0x787cc0 <.text+0x386cc0>
  43c6c8: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  43c6ce: df e0                        	fnstsw	%ax
  43c6d0: f6 c4 44                     	testb	$0x44, %ah
  43c6d3: 7b 0f                        	jnp	0x43c6e4 <.text+0x3b6e4>
  43c6d5: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  43c6d9: 51                           	pushl	%ecx
  43c6da: e8 e1 c3 fd ff               	calll	0x418ac0 <.text+0x17ac0>
  43c6df: dd d8                        	fstp	%st(0)
  43c6e1: 83 c4 04                     	addl	$0x4, %esp
  43c6e4: 8d 54 24 18                  	leal	0x18(%esp), %edx
  43c6e8: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43c6ec: e8 ff b5 34 00               	calll	0x787cf0 <.text+0x386cf0>
  43c6f1: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  43c6f7: df e0                        	fnstsw	%ax
  43c6f9: f6 c4 05                     	testb	$0x5, %ah
  43c6fc: 7a 1e                        	jp	0x43c71c <.text+0x3b71c>
  43c6fe: d9 44 24 3c                  	flds	0x3c(%esp)
  43c702: d9 e0                        	fchs
  43c704: d9 5c 24 3c                  	fstps	0x3c(%esp)
  43c708: d9 44 24 40                  	flds	0x40(%esp)
  43c70c: d9 e0                        	fchs
  43c70e: d9 5c 24 40                  	fstps	0x40(%esp)
  43c712: d9 44 24 44                  	flds	0x44(%esp)
  43c716: d9 e0                        	fchs
  43c718: d9 5c 24 44                  	fstps	0x44(%esp)
  43c71c: 8b ce                        	movl	%esi, %ecx
  43c71e: e8 9d b5 34 00               	calll	0x787cc0 <.text+0x386cc0>
  43c723: d9 44 24 3c                  	flds	0x3c(%esp)
  43c727: 8b 5c 24 7c                  	movl	0x7c(%esp), %ebx
  43c72b: d8 c9                        	fmul	%st(1), %st
  43c72d: 8b d6                        	movl	%esi, %edx
  43c72f: 8b 02                        	movl	(%edx), %eax
  43c731: 8b 4a 04                     	movl	0x4(%edx), %ecx
  43c734: 8b 52 08                     	movl	0x8(%edx), %edx
  43c737: d9 1e                        	fstps	(%esi)
  43c739: d9 44 24 40                  	flds	0x40(%esp)
  43c73d: 89 44 24 24                  	movl	%eax, 0x24(%esp)
  43c741: d8 c9                        	fmul	%st(1), %st
  43c743: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  43c747: 89 54 24 2c                  	movl	%edx, 0x2c(%esp)
  43c74b: d9 5b 08                     	fstps	0x8(%ebx)
  43c74e: d8 4c 24 44                  	fmuls	0x44(%esp)
  43c752: d9 5b 0c                     	fstps	0xc(%ebx)
  43c755: 8b 06                        	movl	(%esi), %eax
  43c757: 8b 4e 04                     	movl	0x4(%esi), %ecx
  43c75a: 8b 56 08                     	movl	0x8(%esi), %edx
  43c75d: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  43c761: 8d 44 24 24                  	leal	0x24(%esp), %eax
  43c765: 50                           	pushl	%eax
  43c766: 89 4c 24 20                  	movl	%ecx, 0x20(%esp)
  43c76a: 89 54 24 24                  	movl	%edx, 0x24(%esp)
  43c76e: e8 4d c3 fd ff               	calll	0x418ac0 <.text+0x17ac0>
  43c773: dd d8                        	fstp	%st(0)
  43c775: 8d 4c 24 1c                  	leal	0x1c(%esp), %ecx
  43c779: 51                           	pushl	%ecx
  43c77a: e8 41 c3 fd ff               	calll	0x418ac0 <.text+0x17ac0>
  43c77f: dd d8                        	fstp	%st(0)
  43c781: 55                           	pushl	%ebp
  43c782: 8d 54 24 24                  	leal	0x24(%esp), %edx
  43c786: 52                           	pushl	%edx
  43c787: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43c78b: 50                           	pushl	%eax
  43c78c: e8 7f bc ff ff               	calll	0x438410 <.text+0x37410>
  43c791: 8b 4f 18                     	movl	0x18(%edi), %ecx
  43c794: 2b c8                        	subl	%eax, %ecx
  43c796: 8b c1                        	movl	%ecx, %eax
  43c798: 89 4f 18                     	movl	%ecx, 0x18(%edi)
  43c79b: 8b 4b 20                     	movl	0x20(%ebx), %ecx
  43c79e: 50                           	pushl	%eax
  43c79f: 51                           	pushl	%ecx
  43c7a0: e8 0b bc ff ff               	calll	0x4383b0 <.text+0x373b0>
  43c7a5: 83 c4 1c                     	addl	$0x1c, %esp
  43c7a8: 3d 00 60 00 00               	cmpl	$0x6000, %eax           # imm = 0x6000
  43c7ad: 5d                           	popl	%ebp
  43c7ae: 7e 07                        	jle	0x43c7b7 <.text+0x3b7b7>
  43c7b0: 81 47 18 00 80 00 00         	addl	$0x8000, 0x18(%edi)     # imm = 0x8000
  43c7b7: 8b 57 18                     	movl	0x18(%edi), %edx
  43c7ba: 89 53 20                     	movl	%edx, 0x20(%ebx)
  43c7bd: 5e                           	popl	%esi
  43c7be: 5b                           	popl	%ebx
  43c7bf: 83 c4 6c                     	addl	$0x6c, %esp
  43c7c2: c3                           	retl
