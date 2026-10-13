
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43bdc0: 56                           	pushl	%esi
  43bdc1: 8b 74 24 08                  	movl	0x8(%esp), %esi
  43bdc5: 33 c0                        	xorl	%eax, %eax
  43bdc7: 66 8b 46 04                  	movw	0x4(%esi), %ax
  43bdcb: 8b c8                        	movl	%eax, %ecx
  43bdcd: 81 e1 00 40 00 00            	andl	$0x4000, %ecx           # imm = 0x4000
  43bdd3: 66 85 c9                     	testw	%cx, %cx
  43bdd6: 74 0f                        	je	0x43bde7 <.text+0x3ade7>
  43bdd8: 8b 56 3c                     	movl	0x3c(%esi), %edx
  43bddb: 8a 4a 24                     	movb	0x24(%edx), %cl
  43bdde: 33 c0                        	xorl	%eax, %eax
  43bde0: 84 c9                        	testb	%cl, %cl
  43bde2: 0f 95 c0                     	setne	%al
  43bde5: 5e                           	popl	%esi
  43bde6: c3                           	retl
  43bde7: 25 00 20 00 00               	andl	$0x2000, %eax           # imm = 0x2000
  43bdec: 66 85 c0                     	testw	%ax, %ax
  43bdef: 74 7c                        	je	0x43be6d <.text+0x3ae6d>
  43bdf1: 66 8b 15 cc 2d b2 03         	movw	0x3b22dcc, %dx
  43bdf8: 66 83 fa 02                  	cmpw	$0x2, %dx
  43bdfc: 75 0b                        	jne	0x43be09 <.text+0x3ae09>
  43bdfe: 66 39 15 ec 2d b2 03         	cmpw	%dx, 0x3b22dec
  43be05: 74 12                        	je	0x43be19 <.text+0x3ae19>
  43be07: eb 6b                        	jmp	0x43be74 <.text+0x3ae74>
  43be09: 66 83 fa 04                  	cmpw	$0x4, %dx
  43be0d: 75 65                        	jne	0x43be74 <.text+0x3ae74>
  43be0f: 66 83 3d ec 2d b2 03 00      	cmpw	$0x0, 0x3b22dec
  43be17: 75 5b                        	jne	0x43be74 <.text+0x3ae74>
  43be19: e8 c2 86 fd ff               	calll	0x4144e0 <.text+0x134e0>
  43be1e: 85 c0                        	testl	%eax, %eax
  43be20: 75 4b                        	jne	0x43be6d <.text+0x3ae6d>
  43be22: 8b 74 24 0c                  	movl	0xc(%esp), %esi
  43be26: 85 f6                        	testl	%esi, %esi
  43be28: 74 2e                        	je	0x43be58 <.text+0x3ae58>
  43be2a: dd 05 50 00 7e 00            	fldl	0x7e0050
  43be30: dd 05 48 00 7e 00            	fldl	0x7e0048
  43be36: d9 f3                        	fpatan
  43be38: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  43be3e: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  43be44: e8 f7 84 20 00               	calll	0x644340 <.text+0x243340>
  43be49: 8b 0d b0 cb b2 03            	movl	0x3b2cbb0, %ecx
  43be4f: 8b 51 18                     	movl	0x18(%ecx), %edx
  43be52: f7 da                        	negl	%edx
  43be54: 2b d0                        	subl	%eax, %edx
  43be56: 89 16                        	movl	%edx, (%esi)
  43be58: 8b 44 24 10                  	movl	0x10(%esp), %eax
  43be5c: 85 c0                        	testl	%eax, %eax
  43be5e: 74 06                        	je	0x43be66 <.text+0x3ae66>
  43be60: c7 00 00 00 80 3f            	movl	$0x3f800000, (%eax)     # imm = 0x3F800000
  43be66: b8 01 00 00 00               	movl	$0x1, %eax
  43be6b: 5e                           	popl	%esi
  43be6c: c3                           	retl
  43be6d: 66 8b 15 cc 2d b2 03         	movw	0x3b22dcc, %dx
  43be74: 0f b6 4e 08                  	movzbl	0x8(%esi), %ecx
  43be78: d9 04 cd a4 e7 b0 03         	flds	0x3b0e7a4(,%ecx,8)
  43be7f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  43be85: df e0                        	fnstsw	%ax
  43be87: f6 c4 44                     	testb	$0x44, %ah
  43be8a: 7b 4e                        	jnp	0x43beda <.text+0x3aeda>
  43be8c: 33 c0                        	xorl	%eax, %eax
  43be8e: 66 8b 46 04                  	movw	0x4(%esi), %ax
  43be92: 25 00 20 00 00               	andl	$0x2000, %eax           # imm = 0x2000
  43be97: 66 85 c0                     	testw	%ax, %ax
  43be9a: 75 11                        	jne	0x43bead <.text+0x3aead>
  43be9c: 8b 14 8d f0 cd b3 03         	movl	0x3b3cdf0(,%ecx,4), %edx
  43bea3: 66 39 42 0a                  	cmpw	%ax, 0xa(%edx)
  43bea7: 0f 85 83 00 00 00            	jne	0x43bf30 <.text+0x3af30>
  43bead: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  43beb1: 85 c0                        	testl	%eax, %eax
  43beb3: 74 09                        	je	0x43bebe <.text+0x3aebe>
  43beb5: 8b 0c cd a0 e7 b0 03         	movl	0x3b0e7a0(,%ecx,8), %ecx
  43bebc: 89 08                        	movl	%ecx, (%eax)
  43bebe: 8b 44 24 10                  	movl	0x10(%esp), %eax
  43bec2: 85 c0                        	testl	%eax, %eax
  43bec4: 74 0d                        	je	0x43bed3 <.text+0x3aed3>
  43bec6: 0f b6 56 08                  	movzbl	0x8(%esi), %edx
  43beca: 8b 0c d5 a4 e7 b0 03         	movl	0x3b0e7a4(,%edx,8), %ecx
  43bed1: 89 08                        	movl	%ecx, (%eax)
  43bed3: b8 01 00 00 00               	movl	$0x1, %eax
  43bed8: 5e                           	popl	%esi
  43bed9: c3                           	retl
  43beda: 8a 46 09                     	movb	0x9(%esi), %al
  43bedd: 84 c0                        	testb	%al, %al
  43bedf: 75 4f                        	jne	0x43bf30 <.text+0x3af30>
  43bee1: 33 c0                        	xorl	%eax, %eax
  43bee3: 66 8b 46 04                  	movw	0x4(%esi), %ax
  43bee7: 25 00 01 00 00               	andl	$0x100, %eax            # imm = 0x100
  43beec: 66 85 c0                     	testw	%ax, %ax
  43beef: 74 3f                        	je	0x43bf30 <.text+0x3af30>
  43bef1: 0f bf 0d ec 2d b2 03         	movswl	0x3b22dec, %ecx
  43bef8: 0f bf c2                     	movswl	%dx, %eax
  43befb: c1 e0 08                     	shll	$0x8, %eax
  43befe: 0b c1                        	orl	%ecx, %eax
  43bf00: 3d 02 09 00 00               	cmpl	$0x902, %eax            # imm = 0x902
  43bf05: 74 07                        	je	0x43bf0e <.text+0x3af0e>
  43bf07: 3d 03 09 00 00               	cmpl	$0x903, %eax            # imm = 0x903
  43bf0c: 75 22                        	jne	0x43bf30 <.text+0x3af30>
  43bf0e: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  43bf12: 85 c0                        	testl	%eax, %eax
  43bf14: 74 05                        	je	0x43bf1b <.text+0x3af1b>
  43bf16: 8b 56 18                     	movl	0x18(%esi), %edx
  43bf19: 89 10                        	movl	%edx, (%eax)
  43bf1b: 8b 44 24 10                  	movl	0x10(%esp), %eax
  43bf1f: 85 c0                        	testl	%eax, %eax
  43bf21: 74 06                        	je	0x43bf29 <.text+0x3af29>
  43bf23: c7 00 00 00 00 00            	movl	$0x0, (%eax)
  43bf29: b8 01 00 00 00               	movl	$0x1, %eax
  43bf2e: 5e                           	popl	%esi
  43bf2f: c3                           	retl
  43bf30: 33 c0                        	xorl	%eax, %eax
  43bf32: 5e                           	popl	%esi
  43bf33: c3                           	retl
  43bf34: 90                           	nop
  43bf35: 90                           	nop
  43bf36: 90                           	nop
  43bf37: 90                           	nop
  43bf38: 90                           	nop
  43bf39: 90                           	nop
  43bf3a: 90                           	nop
  43bf3b: 90                           	nop
  43bf3c: 90                           	nop
  43bf3d: 90                           	nop
  43bf3e: 90                           	nop
  43bf3f: 90                           	nop
  43bf40: 8b 44 24 04                  	movl	0x4(%esp), %eax
  43bf44: 33 c9                        	xorl	%ecx, %ecx
  43bf46: 66 8b 48 04                  	movw	0x4(%eax), %cx
  43bf4a: 81 e1 00 40 00 00            	andl	$0x4000, %ecx           # imm = 0x4000
  43bf50: 66 85 c9                     	testw	%cx, %cx
  43bf53: 75 29                        	jne	0x43bf7e <.text+0x3af7e>
  43bf55: 0f b6 50 08                  	movzbl	0x8(%eax), %edx
  43bf59: 8b 0d 98 9f 90 00            	movl	0x909f98, %ecx
  43bf5f: 6b d2 34                     	imull	$0x34, %edx, %edx
  43bf62: 85 8a 00 e8 b0 03            	testl	%ecx, 0x3b0e800(%edx)
  43bf68: 74 14                        	je	0x43bf7e <.text+0x3af7e>
  43bf6a: 50                           	pushl	%eax
  43bf6b: e8 80 c4 07 00               	calll	0x4b83f0 <.text+0xb73f0>
  43bf70: 33 d2                        	xorl	%edx, %edx
  43bf72: 83 c4 04                     	addl	$0x4, %esp
  43bf75: 85 c0                        	testl	%eax, %eax
  43bf77: 0f 9f c2                     	setg	%dl
  43bf7a: 42                           	incl	%edx
  43bf7b: 8b c2                        	movl	%edx, %eax
  43bf7d: c3                           	retl
  43bf7e: 33 c0                        	xorl	%eax, %eax
  43bf80: c3                           	retl
  43bf81: 90                           	nop
  43bf82: 90                           	nop
  43bf83: 90                           	nop
  43bf84: 90                           	nop
  43bf85: 90                           	nop
  43bf86: 90                           	nop
  43bf87: 90                           	nop
  43bf88: 90                           	nop
  43bf89: 90                           	nop
  43bf8a: 90                           	nop
  43bf8b: 90                           	nop
  43bf8c: 90                           	nop
  43bf8d: 90                           	nop
  43bf8e: 90                           	nop
  43bf8f: 90                           	nop
  43bf90: 8b 44 24 04                  	movl	0x4(%esp), %eax
  43bf94: 33 c9                        	xorl	%ecx, %ecx
  43bf96: 66 8b 48 04                  	movw	0x4(%eax), %cx
  43bf9a: 81 e1 00 40 00 00            	andl	$0x4000, %ecx           # imm = 0x4000
