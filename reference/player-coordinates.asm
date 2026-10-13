
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43ec00: 83 ec 0c                     	subl	$0xc, %esp
  43ec03: 56                           	pushl	%esi
  43ec04: 57                           	pushl	%edi
  43ec05: 8b 7c 24 1c                  	movl	0x1c(%esp), %edi
  43ec09: 8b 07                        	movl	(%edi), %eax
  43ec0b: 8b 4f 04                     	movl	0x4(%edi), %ecx
  43ec0e: 8b 57 08                     	movl	0x8(%edi), %edx
  43ec11: 89 44 24 08                  	movl	%eax, 0x8(%esp)
  43ec15: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  43ec19: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  43ec1d: e8 5e 30 34 00               	calll	0x781c80 <.text+0x380c80>
  43ec22: 33 c9                        	xorl	%ecx, %ecx
  43ec24: e8 c7 89 34 00               	calll	0x7875f0 <.text+0x3865f0>
  43ec29: 8b 74 24 18                  	movl	0x18(%esp), %esi
  43ec2d: 8b 46 18                     	movl	0x18(%esi), %eax
  43ec30: 85 c0                        	testl	%eax, %eax
  43ec32: 74 0a                        	je	0x43ec3e <.text+0x3dc3e>
  43ec34: 0f b7 d0                     	movzwl	%ax, %edx
  43ec37: 33 c9                        	xorl	%ecx, %ecx
  43ec39: e8 e2 61 34 00               	calll	0x784e20 <.text+0x383e20>
  43ec3e: 8b 46 14                     	movl	0x14(%esi), %eax
  43ec41: f7 d8                        	negl	%eax
  43ec43: 74 11                        	je	0x43ec56 <.text+0x3dc56>
  43ec45: 66 8b 4e 14                  	movw	0x14(%esi), %cx
  43ec49: 66 f7 d9                     	negw	%cx
  43ec4c: 0f b7 d1                     	movzwl	%cx, %edx
  43ec4f: 33 c9                        	xorl	%ecx, %ecx
  43ec51: e8 8a 62 34 00               	calll	0x784ee0 <.text+0x383ee0>
  43ec56: 8b 56 1c                     	movl	0x1c(%esi), %edx
  43ec59: f7 da                        	negl	%edx
  43ec5b: 74 11                        	je	0x43ec6e <.text+0x3dc6e>
  43ec5d: 66 8b 46 1c                  	movw	0x1c(%esi), %ax
  43ec61: 66 f7 d8                     	negw	%ax
  43ec64: 0f b7 d0                     	movzwl	%ax, %edx
  43ec67: 33 c9                        	xorl	%ecx, %ecx
  43ec69: e8 42 63 34 00               	calll	0x784fb0 <.text+0x383fb0>
  43ec6e: 57                           	pushl	%edi
  43ec6f: 8d 54 24 0c                  	leal	0xc(%esp), %edx
  43ec73: 33 c9                        	xorl	%ecx, %ecx
  43ec75: e8 e6 60 34 00               	calll	0x784d60 <.text+0x383d60>
  43ec7a: b9 01 00 00 00               	movl	$0x1, %ecx
  43ec7f: e8 bc 64 34 00               	calll	0x785140 <.text+0x384140>
  43ec84: 5f                           	popl	%edi
  43ec85: 5e                           	popl	%esi
  43ec86: 83 c4 0c                     	addl	$0xc, %esp
  43ec89: c3                           	retl
  43ec8a: 90                           	nop
  43ec8b: 90                           	nop
  43ec8c: 90                           	nop
  43ec8d: 90                           	nop
  43ec8e: 90                           	nop
  43ec8f: 90                           	nop
  43ec90: 83 ec 0c                     	subl	$0xc, %esp
  43ec93: 56                           	pushl	%esi
  43ec94: 8b 74 24 14                  	movl	0x14(%esp), %esi
  43ec98: 85 f6                        	testl	%esi, %esi
  43ec9a: 74 76                        	je	0x43ed12 <.text+0x3dd12>
  43ec9c: 57                           	pushl	%edi
  43ec9d: 8b 7c 24 1c                  	movl	0x1c(%esp), %edi
  43eca1: 8b 07                        	movl	(%edi), %eax
  43eca3: 8b 4f 04                     	movl	0x4(%edi), %ecx
  43eca6: 8b 57 08                     	movl	0x8(%edi), %edx
  43eca9: 89 44 24 08                  	movl	%eax, 0x8(%esp)
  43ecad: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  43ecb1: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  43ecb5: e8 c6 2f 34 00               	calll	0x781c80 <.text+0x380c80>
  43ecba: 33 c9                        	xorl	%ecx, %ecx
  43ecbc: e8 2f 89 34 00               	calll	0x7875f0 <.text+0x3865f0>
  43ecc1: 8b 46 1c                     	movl	0x1c(%esi), %eax
  43ecc4: 85 c0                        	testl	%eax, %eax
  43ecc6: 74 0a                        	je	0x43ecd2 <.text+0x3dcd2>
  43ecc8: 0f b7 d0                     	movzwl	%ax, %edx
  43eccb: 33 c9                        	xorl	%ecx, %ecx
  43eccd: e8 de 62 34 00               	calll	0x784fb0 <.text+0x383fb0>
  43ecd2: 8b 46 14                     	movl	0x14(%esi), %eax
  43ecd5: 85 c0                        	testl	%eax, %eax
  43ecd7: 74 0a                        	je	0x43ece3 <.text+0x3dce3>
  43ecd9: 0f b7 d0                     	movzwl	%ax, %edx
  43ecdc: 33 c9                        	xorl	%ecx, %ecx
  43ecde: e8 fd 61 34 00               	calll	0x784ee0 <.text+0x383ee0>
  43ece3: 8b 46 18                     	movl	0x18(%esi), %eax
  43ece6: f7 d8                        	negl	%eax
  43ece8: 74 11                        	je	0x43ecfb <.text+0x3dcfb>
  43ecea: 66 8b 4e 18                  	movw	0x18(%esi), %cx
  43ecee: 66 f7 d9                     	negw	%cx
  43ecf1: 0f b7 d1                     	movzwl	%cx, %edx
  43ecf4: 33 c9                        	xorl	%ecx, %ecx
  43ecf6: e8 25 61 34 00               	calll	0x784e20 <.text+0x383e20>
  43ecfb: 57                           	pushl	%edi
  43ecfc: 8d 54 24 0c                  	leal	0xc(%esp), %edx
  43ed00: 33 c9                        	xorl	%ecx, %ecx
  43ed02: e8 59 60 34 00               	calll	0x784d60 <.text+0x383d60>
  43ed07: b9 01 00 00 00               	movl	$0x1, %ecx
  43ed0c: e8 2f 64 34 00               	calll	0x785140 <.text+0x384140>
  43ed11: 5f                           	popl	%edi
  43ed12: 5e                           	popl	%esi
  43ed13: 83 c4 0c                     	addl	$0xc, %esp
  43ed16: c3                           	retl
  43ed17: 90                           	nop
  43ed18: 90                           	nop
  43ed19: 90                           	nop
  43ed1a: 90                           	nop
  43ed1b: 90                           	nop
  43ed1c: 90                           	nop
  43ed1d: 90                           	nop
  43ed1e: 90                           	nop
  43ed1f: 90                           	nop
  43ed20: 83 ec 0c                     	subl	$0xc, %esp
  43ed23: 56                           	pushl	%esi
  43ed24: 8b 74 24 18                  	movl	0x18(%esp), %esi
  43ed28: 8b 06                        	movl	(%esi), %eax
  43ed2a: 8b 4e 04                     	movl	0x4(%esi), %ecx
  43ed2d: 8b 56 08                     	movl	0x8(%esi), %edx
  43ed30: 89 44 24 04                  	movl	%eax, 0x4(%esp)
  43ed34: 8d 44 24 14                  	leal	0x14(%esp), %eax
  43ed38: 50                           	pushl	%eax
  43ed39: 8b 44 24 18                  	movl	0x18(%esp), %eax
  43ed3d: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  43ed41: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  43ed45: 8b 50 08                     	movl	0x8(%eax), %edx
  43ed48: 8d 4c 24 1c                  	leal	0x1c(%esp), %ecx
  43ed4c: 51                           	pushl	%ecx
  43ed4d: 8b 48 04                     	movl	0x4(%eax), %ecx
  43ed50: 52                           	pushl	%edx
  43ed51: 8b 10                        	movl	(%eax), %edx
  43ed53: 51                           	pushl	%ecx
  43ed54: 52                           	pushl	%edx
  43ed55: e8 26 a1 07 00               	calll	0x4b8e80 <.text+0xb7e80>
  43ed5a: 83 c4 14                     	addl	$0x14, %esp
  43ed5d: e8 1e 2f 34 00               	calll	0x781c80 <.text+0x380c80>
  43ed62: 33 c9                        	xorl	%ecx, %ecx
  43ed64: e8 87 88 34 00               	calll	0x7875f0 <.text+0x3865f0>
  43ed69: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43ed6d: 85 c0                        	testl	%eax, %eax
  43ed6f: 74 0a                        	je	0x43ed7b <.text+0x3dd7b>
  43ed71: 0f b7 d0                     	movzwl	%ax, %edx
  43ed74: 33 c9                        	xorl	%ecx, %ecx
  43ed76: e8 a5 60 34 00               	calll	0x784e20 <.text+0x383e20>
  43ed7b: 8b 44 24 18                  	movl	0x18(%esp), %eax
  43ed7f: 8b c8                        	movl	%eax, %ecx
  43ed81: f7 d9                        	negl	%ecx
  43ed83: 74 0c                        	je	0x43ed91 <.text+0x3dd91>
  43ed85: f7 d8                        	negl	%eax
  43ed87: 0f b7 d0                     	movzwl	%ax, %edx
  43ed8a: 33 c9                        	xorl	%ecx, %ecx
  43ed8c: e8 4f 61 34 00               	calll	0x784ee0 <.text+0x383ee0>
  43ed91: 8b 44 24 14                  	movl	0x14(%esp), %eax
  43ed95: 8b d0                        	movl	%eax, %edx
  43ed97: f7 da                        	negl	%edx
  43ed99: 74 0c                        	je	0x43eda7 <.text+0x3dda7>
  43ed9b: f7 d8                        	negl	%eax
  43ed9d: 0f b7 d0                     	movzwl	%ax, %edx
  43eda0: 33 c9                        	xorl	%ecx, %ecx
  43eda2: e8 09 62 34 00               	calll	0x784fb0 <.text+0x383fb0>
  43eda7: 56                           	pushl	%esi
  43eda8: 8d 54 24 08                  	leal	0x8(%esp), %edx
  43edac: 33 c9                        	xorl	%ecx, %ecx
  43edae: e8 ad 5f 34 00               	calll	0x784d60 <.text+0x383d60>
  43edb3: b9 01 00 00 00               	movl	$0x1, %ecx
  43edb8: e8 83 63 34 00               	calll	0x785140 <.text+0x384140>
  43edbd: 5e                           	popl	%esi
  43edbe: 83 c4 0c                     	addl	$0xc, %esp
  43edc1: c3                           	retl
  43edc2: 90                           	nop
  43edc3: 90                           	nop
  43edc4: 90                           	nop
  43edc5: 90                           	nop
  43edc6: 90                           	nop
  43edc7: 90                           	nop
  43edc8: 90                           	nop
  43edc9: 90                           	nop
  43edca: 90                           	nop
  43edcb: 90                           	nop
  43edcc: 90                           	nop
  43edcd: 90                           	nop
  43edce: 90                           	nop
  43edcf: 90                           	nop
  43edd0: 83 ec 0c                     	subl	$0xc, %esp
  43edd3: 56                           	pushl	%esi
  43edd4: 8b 74 24 18                  	movl	0x18(%esp), %esi
  43edd8: 8b 06                        	movl	(%esi), %eax
  43edda: 8b 4e 04                     	movl	0x4(%esi), %ecx
  43eddd: 8b 56 08                     	movl	0x8(%esi), %edx
  43ede0: 89 44 24 04                  	movl	%eax, 0x4(%esp)
  43ede4: 8d 44 24 18                  	leal	0x18(%esp), %eax
  43ede8: 50                           	pushl	%eax
  43ede9: 8b 44 24 18                  	movl	0x18(%esp), %eax
  43eded: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  43edf1: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  43edf5: 8b 50 08                     	movl	0x8(%eax), %edx
  43edf8: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  43edfc: 51                           	pushl	%ecx
  43edfd: 8b 48 04                     	movl	0x4(%eax), %ecx
  43ee00: 52                           	pushl	%edx
  43ee01: 8b 10                        	movl	(%eax), %edx
  43ee03: 51                           	pushl	%ecx
  43ee04: 52                           	pushl	%edx
  43ee05: e8 76 a0 07 00               	calll	0x4b8e80 <.text+0xb7e80>
  43ee0a: 83 c4 14                     	addl	$0x14, %esp
  43ee0d: e8 6e 2e 34 00               	calll	0x781c80 <.text+0x380c80>
  43ee12: 33 c9                        	xorl	%ecx, %ecx
  43ee14: e8 d7 87 34 00               	calll	0x7875f0 <.text+0x3865f0>
  43ee19: 8b 44 24 18                  	movl	0x18(%esp), %eax
  43ee1d: 85 c0                        	testl	%eax, %eax
  43ee1f: 74 0a                        	je	0x43ee2b <.text+0x3de2b>
  43ee21: 0f b7 d0                     	movzwl	%ax, %edx
  43ee24: 33 c9                        	xorl	%ecx, %ecx
  43ee26: e8 85 61 34 00               	calll	0x784fb0 <.text+0x383fb0>
  43ee2b: 8b 44 24 14                  	movl	0x14(%esp), %eax
  43ee2f: 85 c0                        	testl	%eax, %eax
  43ee31: 74 0a                        	je	0x43ee3d <.text+0x3de3d>
  43ee33: 0f b7 d0                     	movzwl	%ax, %edx
  43ee36: 33 c9                        	xorl	%ecx, %ecx
  43ee38: e8 a3 60 34 00               	calll	0x784ee0 <.text+0x383ee0>
  43ee3d: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43ee41: 8b c8                        	movl	%eax, %ecx
  43ee43: f7 d9                        	negl	%ecx
  43ee45: 74 0c                        	je	0x43ee53 <.text+0x3de53>
  43ee47: f7 d8                        	negl	%eax
  43ee49: 0f b7 d0                     	movzwl	%ax, %edx
  43ee4c: 33 c9                        	xorl	%ecx, %ecx
  43ee4e: e8 cd 5f 34 00               	calll	0x784e20 <.text+0x383e20>
  43ee53: 56                           	pushl	%esi
  43ee54: 8d 54 24 08                  	leal	0x8(%esp), %edx
  43ee58: 33 c9                        	xorl	%ecx, %ecx
  43ee5a: e8 01 5f 34 00               	calll	0x784d60 <.text+0x383d60>
  43ee5f: b9 01 00 00 00               	movl	$0x1, %ecx
  43ee64: e8 d7 62 34 00               	calll	0x785140 <.text+0x384140>
  43ee69: 5e                           	popl	%esi
  43ee6a: 83 c4 0c                     	addl	$0xc, %esp
  43ee6d: c3                           	retl
  43ee6e: 90                           	nop
  43ee6f: 90                           	nop
  43ee70: 83 ec 0c                     	subl	$0xc, %esp
  43ee73: 55                           	pushl	%ebp
  43ee74: 8b 6c 24 1c                  	movl	0x1c(%esp), %ebp
  43ee78: 56                           	pushl	%esi
  43ee79: 8b 74 24 18                  	movl	0x18(%esp), %esi
  43ee7d: 57                           	pushl	%edi
  43ee7e: 8b 7c 24 20                  	movl	0x20(%esp), %edi
  43ee82: 8d 47 04                     	leal	0x4(%edi), %eax
  43ee85: 8b 08                        	movl	(%eax), %ecx
  43ee87: 8b 50 04                     	movl	0x4(%eax), %edx
  43ee8a: 8b 40 08                     	movl	0x8(%eax), %eax
  43ee8d: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  43ee91: f6 46 04 03                  	testb	$0x3, 0x4(%esi)
  43ee95: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  43ee99: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  43ee9d: 74 1f                        	je	0x43eebe <.text+0x3debe>
  43ee9f: 8b 45 5c                     	movl	0x5c(%ebp), %eax
  43eea2: f6 c4 40                     	testb	$0x40, %ah
  43eea5: 74 17                        	je	0x43eebe <.text+0x3debe>
  43eea7: 8b 4e 18                     	movl	0x18(%esi), %ecx
  43eeaa: 51                           	pushl	%ecx
  43eeab: 8d 54 24 10                  	leal	0x10(%esp), %edx
  43eeaf: 52                           	pushl	%edx
  43eeb0: 8d 45 50                     	leal	0x50(%ebp), %eax
  43eeb3: 50                           	pushl	%eax
  43eeb4: e8 67 fe ff ff               	calll	0x43ed20 <.text+0x3dd20>
  43eeb9: 83 c4 0c                     	addl	$0xc, %esp
  43eebc: eb 0e                        	jmp	0x43eecc <.text+0x3decc>
  43eebe: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43eec2: 51                           	pushl	%ecx
  43eec3: 56                           	pushl	%esi
  43eec4: e8 37 fd ff ff               	calll	0x43ec00 <.text+0x3dc00>
  43eec9: 83 c4 08                     	addl	$0x8, %esp
  43eecc: 66 8b 46 04                  	movw	0x4(%esi), %ax
  43eed0: 66 25 03 00                  	andw	$0x3, %ax
  43eed4: 74 08                        	je	0x43eede <.text+0x3dede>
  43eed6: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  43eede: 33 c9                        	xorl	%ecx, %ecx
  43eee0: 66 3b c1                     	cmpw	%cx, %ax
  43eee3: 74 60                        	je	0x43ef45 <.text+0x3df45>
  43eee5: f6 45 60 0c                  	testb	$0xc, 0x60(%ebp)
  43eee9: 0f 85 ae 00 00 00            	jne	0x43ef9d <.text+0x3df9d>
  43eeef: d9 85 d8 00 00 00            	flds	0xd8(%ebp)
  43eef5: d8 0d f0 ff 7d 00            	fmuls	0x7dfff0
  43eefb: d9 44 24 0c                  	flds	0xc(%esp)
  43eeff: d8 1d 54 d5 7d 00            	fcomps	0x7dd554
  43ef05: df e0                        	fnstsw	%ax
  43ef07: f6 c4 05                     	testb	$0x5, %ah
  43ef0a: 7a 15                        	jp	0x43ef21 <.text+0x3df21>
  43ef0c: d9 44 24 0c                  	flds	0xc(%esp)
  43ef10: d8 1d 90 00 7e 00            	fcomps	0x7e0090
  43ef16: df e0                        	fnstsw	%ax
  43ef18: f6 c4 41                     	testb	$0x41, %ah
  43ef1b: 75 04                        	jne	0x43ef21 <.text+0x3df21>
  43ef1d: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  43ef21: d9 44 24 14                  	flds	0x14(%esp)
  43ef25: d8 d9                        	fcomp	%st(1)
  43ef27: df e0                        	fnstsw	%ax
  43ef29: f6 c4 41                     	testb	$0x41, %ah
  43ef2c: 7a 6d                        	jp	0x43ef9b <.text+0x3df9b>
  43ef2e: d9 e0                        	fchs
  43ef30: d8 5c 24 14                  	fcomps	0x14(%esp)
  43ef34: df e0                        	fnstsw	%ax
  43ef36: f6 c4 41                     	testb	$0x41, %ah
  43ef39: 7a 62                        	jp	0x43ef9d <.text+0x3df9d>
  43ef3b: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  43ef43: eb 58                        	jmp	0x43ef9d <.text+0x3df9d>
  43ef45: d9 44 24 0c                  	flds	0xc(%esp)
  43ef49: d8 1d 7c d5 7d 00            	fcomps	0x7dd57c
  43ef4f: df e0                        	fnstsw	%ax
  43ef51: f6 c4 41                     	testb	$0x41, %ah
  43ef54: 7a 19                        	jp	0x43ef6f <.text+0x3df6f>
  43ef56: d9 44 24 0c                  	flds	0xc(%esp)
  43ef5a: d8 1d bc f1 7d 00            	fcomps	0x7df1bc
  43ef60: df e0                        	fnstsw	%ax
  43ef62: f6 c4 01                     	testb	$0x1, %ah
  43ef65: 75 08                        	jne	0x43ef6f <.text+0x3df6f>
  43ef67: c7 44 24 0c 00 00 00 00      	movl	$0x0, 0xc(%esp)
  43ef6f: d9 44 24 14                  	flds	0x14(%esp)
  43ef73: d8 1d 7c d5 7d 00            	fcomps	0x7dd57c
  43ef79: df e0                        	fnstsw	%ax
  43ef7b: f6 c4 41                     	testb	$0x41, %ah
  43ef7e: 7a 1d                        	jp	0x43ef9d <.text+0x3df9d>
