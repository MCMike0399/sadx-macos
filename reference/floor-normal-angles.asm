
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4b8e80: d9 44 24 0c                  	flds	0xc(%esp)
  4b8e84: d8 1d d8 66 7e 00            	fcomps	0x7e66d8
  4b8e8a: df e0                        	fnstsw	%ax
  4b8e8c: f6 c4 41                     	testb	$0x41, %ah
  4b8e8f: 0f 84 d8 00 00 00            	je	0x4b8f6d <.text+0xb7f6d>
  4b8e95: d9 44 24 08                  	flds	0x8(%esp)
  4b8e99: d9 e1                        	fabs
  4b8e9b: d8 15 e0 00 7e 00            	fcoms	0x7e00e0
  4b8ea1: df e0                        	fnstsw	%ax
  4b8ea3: f6 c4 05                     	testb	$0x5, %ah
  4b8ea6: 7a 28                        	jp	0x4b8ed0 <.text+0xb7ed0>
  4b8ea8: d9 44 24 04                  	flds	0x4(%esp)
  4b8eac: d9 e1                        	fabs
  4b8eae: d8 1d e0 00 7e 00            	fcomps	0x7e00e0
  4b8eb4: df e0                        	fnstsw	%ax
  4b8eb6: f6 c4 05                     	testb	$0x5, %ah
  4b8eb9: 7a 15                        	jp	0x4b8ed0 <.text+0xb7ed0>
  4b8ebb: d9 44 24 0c                  	flds	0xc(%esp)
  4b8ebf: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4b8ec5: df e0                        	fnstsw	%ax
  4b8ec7: f6 c4 41                     	testb	$0x41, %ah
  4b8eca: 0f 84 9b 00 00 00            	je	0x4b8f6b <.text+0xb7f6b>
  4b8ed0: d9 44 24 0c                  	flds	0xc(%esp)
  4b8ed4: d8 1d d4 66 7e 00            	fcomps	0x7e66d4
  4b8eda: df e0                        	fnstsw	%ax
  4b8edc: f6 c4 05                     	testb	$0x5, %ah
  4b8edf: 7b 73                        	jnp	0x4b8f54 <.text+0xb7f54>
  4b8ee1: d8 1d e0 00 7e 00            	fcomps	0x7e00e0
  4b8ee7: df e0                        	fnstsw	%ax
  4b8ee9: f6 c4 05                     	testb	$0x5, %ah
  4b8eec: 7a 24                        	jp	0x4b8f12 <.text+0xb7f12>
  4b8eee: d9 44 24 04                  	flds	0x4(%esp)
  4b8ef2: d9 e1                        	fabs
  4b8ef4: d8 1d e0 00 7e 00            	fcomps	0x7e00e0
  4b8efa: df e0                        	fnstsw	%ax
  4b8efc: f6 c4 05                     	testb	$0x5, %ah
  4b8eff: 7a 11                        	jp	0x4b8f12 <.text+0xb7f12>
  4b8f01: d9 44 24 0c                  	flds	0xc(%esp)
  4b8f05: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4b8f0b: df e0                        	fnstsw	%ax
  4b8f0d: f6 c4 05                     	testb	$0x5, %ah
  4b8f10: 7b 44                        	jnp	0x4b8f56 <.text+0xb7f56>
  4b8f12: d9 44 24 0c                  	flds	0xc(%esp)
  4b8f16: e8 05 c1 18 00               	calll	0x645020 <.text+0x244020>
  4b8f1b: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  4b8f21: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  4b8f27: e8 14 b4 18 00               	calll	0x644340 <.text+0x243340>
  4b8f2c: d9 44 24 04                  	flds	0x4(%esp)
  4b8f30: d9 44 24 08                  	flds	0x8(%esp)
  4b8f34: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
  4b8f38: d9 f3                        	fpatan
  4b8f3a: 89 01                        	movl	%eax, (%ecx)
  4b8f3c: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  4b8f42: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  4b8f48: e8 f3 b3 18 00               	calll	0x644340 <.text+0x243340>
  4b8f4d: 8b 54 24 14                  	movl	0x14(%esp), %edx
  4b8f51: 89 02                        	movl	%eax, (%edx)
  4b8f53: c3                           	retl
  4b8f54: dd d8                        	fstp	%st(0)
  4b8f56: 8b 44 24 10                  	movl	0x10(%esp), %eax
  4b8f5a: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  4b8f5e: c7 00 00 c0 00 00            	movl	$0xc000, (%eax)         # imm = 0xC000
  4b8f64: c7 01 00 00 00 00            	movl	$0x0, (%ecx)
  4b8f6a: c3                           	retl
  4b8f6b: dd d8                        	fstp	%st(0)
  4b8f6d: 8b 54 24 10                  	movl	0x10(%esp), %edx
  4b8f71: 8b 44 24 14                  	movl	0x14(%esp), %eax
  4b8f75: c7 02 00 40 00 00            	movl	$0x4000, (%edx)         # imm = 0x4000
  4b8f7b: c7 00 00 00 00 00            	movl	$0x0, (%eax)
  4b8f81: c3                           	retl
  4b8f82: 90                           	nop
  4b8f83: 90                           	nop
  4b8f84: 90                           	nop
  4b8f85: 90                           	nop
  4b8f86: 90                           	nop
  4b8f87: 90                           	nop
  4b8f88: 90                           	nop
  4b8f89: 90                           	nop
  4b8f8a: 90                           	nop
  4b8f8b: 90                           	nop
  4b8f8c: 90                           	nop
  4b8f8d: 90                           	nop
  4b8f8e: 90                           	nop
  4b8f8f: 90                           	nop
  4b8f90: 0f b6 47 08                  	movzbl	0x8(%edi), %eax
  4b8f94: 83 ec 18                     	subl	$0x18, %esp
  4b8f97: 56                           	pushl	%esi
  4b8f98: c6 07 01                     	movb	$0x1, (%edi)
  4b8f9b: 8b 34 85 10 2e b4 03         	movl	0x3b42e10(,%eax,4), %esi
  4b8fa2: 85 f6                        	testl	%esi, %esi
  4b8fa4: 0f 84 94 00 00 00            	je	0x4b903e <.text+0xb803e>
  4b8faa: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  4b8faf: c7 44 24 0c 00 00 00 00      	movl	$0x0, 0xc(%esp)
  4b8fb7: c7 44 24 04 00 00 00 00      	movl	$0x0, 0x4(%esp)
  4b8fbf: c7 44 24 08 00 00 a0 40      	movl	$0x40a00000, 0x8(%esp)  # imm = 0x40A00000
  4b8fc7: e8 24 c1 2c 00               	calll	0x7850f0 <.text+0x3840f0>
  4b8fcc: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4b8fcf: 85 c0                        	testl	%eax, %eax
  4b8fd1: 74 0a                        	je	0x4b8fdd <.text+0xb7fdd>
  4b8fd3: 0f b7 d0                     	movzwl	%ax, %edx
  4b8fd6: 33 c9                        	xorl	%ecx, %ecx
  4b8fd8: e8 d3 bf 2c 00               	calll	0x784fb0 <.text+0x383fb0>
  4b8fdd: 8b 46 14                     	movl	0x14(%esi), %eax
  4b8fe0: 85 c0                        	testl	%eax, %eax
  4b8fe2: 74 0a                        	je	0x4b8fee <.text+0xb7fee>
  4b8fe4: 0f b7 d0                     	movzwl	%ax, %edx
  4b8fe7: 33 c9                        	xorl	%ecx, %ecx
  4b8fe9: e8 f2 be 2c 00               	calll	0x784ee0 <.text+0x383ee0>
  4b8fee: 8b 4e 18                     	movl	0x18(%esi), %ecx
  4b8ff1: f7 d9                        	negl	%ecx
  4b8ff3: 74 11                        	je	0x4b9006 <.text+0xb8006>
  4b8ff5: 66 8b 56 18                  	movw	0x18(%esi), %dx
  4b8ff9: 66 f7 da                     	negw	%dx
  4b8ffc: 0f b7 d2                     	movzwl	%dx, %edx
  4b8fff: 33 c9                        	xorl	%ecx, %ecx
  4b9001: e8 1a be 2c 00               	calll	0x784e20 <.text+0x383e20>
  4b9006: 8d 44 24 10                  	leal	0x10(%esp), %eax
  4b900a: 50                           	pushl	%eax
  4b900b: 8d 54 24 08                  	leal	0x8(%esp), %edx
  4b900f: 33 c9                        	xorl	%ecx, %ecx
  4b9011: e8 4a bd 2c 00               	calll	0x784d60 <.text+0x383d60>
  4b9016: b9 01 00 00 00               	movl	$0x1, %ecx
  4b901b: e8 20 c1 2c 00               	calll	0x785140 <.text+0x384140>
  4b9020: d9 44 24 10                  	flds	0x10(%esp)
  4b9024: d8 46 20                     	fadds	0x20(%esi)
  4b9027: d9 5f 20                     	fstps	0x20(%edi)
  4b902a: d9 44 24 14                  	flds	0x14(%esp)
  4b902e: d8 46 24                     	fadds	0x24(%esi)
  4b9031: d9 5f 24                     	fstps	0x24(%edi)
  4b9034: d9 44 24 18                  	flds	0x18(%esp)
  4b9038: d8 46 28                     	fadds	0x28(%esi)
  4b903b: d9 5f 28                     	fstps	0x28(%edi)
  4b903e: 5e                           	popl	%esi
  4b903f: 83 c4 18                     	addl	$0x18, %esp
  4b9042: c3                           	retl
  4b9043: 90                           	nop
  4b9044: 90                           	nop
  4b9045: 90                           	nop
  4b9046: 90                           	nop
  4b9047: 90                           	nop
  4b9048: 90                           	nop
  4b9049: 90                           	nop
  4b904a: 90                           	nop
  4b904b: 90                           	nop
  4b904c: 90                           	nop
  4b904d: 90                           	nop
  4b904e: 90                           	nop
  4b904f: 90                           	nop
  4b9050: 83 ec 18                     	subl	$0x18, %esp
  4b9053: e8 18 bd f5 ff               	calll	0x414d70 <.text+0x13d70>
  4b9058: 85 c0                        	testl	%eax, %eax
  4b905a: 75 1a                        	jne	0x4b9076 <.text+0xb8076>
  4b905c: 33 c0                        	xorl	%eax, %eax
  4b905e: 66 8b 46 06                  	movw	0x6(%esi), %ax
  4b9062: 66 3d 0a 00                  	cmpw	$0xa, %ax
  4b9066: 8d 48 01                     	leal	0x1(%eax), %ecx
  4b9069: 66 89 4e 06                  	movw	%cx, 0x6(%esi)
  4b906d: 72 07                        	jb	0x4b9076 <.text+0xb8076>
  4b906f: c6 06 02                     	movb	$0x2, (%esi)
  4b9072: 83 c4 18                     	addl	$0x18, %esp
  4b9075: c3                           	retl
  4b9076: a1 7c 11 b1 03               	movl	0x3b1117c, %eax
  4b907b: 85 c0                        	testl	%eax, %eax
  4b907d: 75 5c                        	jne	0x4b90db <.text+0xb80db>
  4b907f: 33 c9                        	xorl	%ecx, %ecx
  4b9081: e8 6a c0 2c 00               	calll	0x7850f0 <.text+0x3840f0>
  4b9086: 8d 56 20                     	leal	0x20(%esi), %edx
  4b9089: 33 c9                        	xorl	%ecx, %ecx
  4b908b: e8 e0 bf 2c 00               	calll	0x785070 <.text+0x384070>
