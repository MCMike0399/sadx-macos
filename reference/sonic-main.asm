
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  49a9b0: 83 ec 18                     	subl	$0x18, %esp
  49a9b3: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  49a9b7: 53                           	pushl	%ebx
  49a9b8: 8b 58 20                     	movl	0x20(%eax), %ebx
  49a9bb: 55                           	pushl	%ebp
  49a9bc: 56                           	pushl	%esi
  49a9bd: 8b 70 24                     	movl	0x24(%eax), %esi
  49a9c0: 8b 2e                        	movl	(%esi), %ebp
  49a9c2: 33 c9                        	xorl	%ecx, %ecx
  49a9c4: 89 1d 60 df ab 03            	movl	%ebx, 0x3abdf60
  49a9ca: 89 2d 64 df ab 03            	movl	%ebp, 0x3abdf64
  49a9d0: 66 8b 4b 04                  	movw	0x4(%ebx), %cx
  49a9d4: f6 c1 20                     	testb	$0x20, %cl
  49a9d7: 89 74 24 0c                  	movl	%esi, 0xc(%esp)
  49a9db: 74 3a                        	je	0x49aa17 <.text+0x99a17>
  49a9dd: d9 46 08                     	flds	0x8(%esi)
  49a9e0: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  49a9e6: df e0                        	fnstsw	%ax
  49a9e8: f6 c4 41                     	testb	$0x41, %ah
  49a9eb: 7a 2a                        	jp	0x49aa17 <.text+0x99a17>
  49a9ed: 8b 43 38                     	movl	0x38(%ebx), %eax
  49a9f0: 83 c9 02                     	orl	$0x2, %ecx
  49a9f3: 66 89 4b 04                  	movw	%cx, 0x4(%ebx)
  49a9f7: 8b 88 9c 00 00 00            	movl	0x9c(%eax), %ecx
  49a9fd: 8b 51 20                     	movl	0x20(%ecx), %edx
  49aa00: 8b 4a 14                     	movl	0x14(%edx), %ecx
  49aa03: 89 4b 14                     	movl	%ecx, 0x14(%ebx)
  49aa06: 8b 90 9c 00 00 00            	movl	0x9c(%eax), %edx
  49aa0c: 8b 42 20                     	movl	0x20(%edx), %eax
  49aa0f: 8b 48 1c                     	movl	0x1c(%eax), %ecx
  49aa12: 89 4b 1c                     	movl	%ecx, 0x1c(%ebx)
  49aa15: eb 07                        	jmp	0x49aa1e <.text+0x99a1e>
  49aa17: 83 e1 fd                     	andl	$-0x3, %ecx
  49aa1a: 66 89 4b 04                  	movw	%cx, 0x4(%ebx)
  49aa1e: e8 cd ae f8 ff               	calll	0x4258f0 <.text+0x248f0>
  49aa23: 85 c0                        	testl	%eax, %eax
  49aa25: 74 05                        	je	0x49aa2c <.text+0x99a2c>
  49aa27: c6 03 57                     	movb	$0x57, (%ebx)
  49aa2a: eb 10                        	jmp	0x49aa3c <.text+0x99a3c>
  49aa2c: 80 3b 56                     	cmpb	$0x56, (%ebx)
  49aa2f: 74 0b                        	je	0x49aa3c <.text+0x99a3c>
  49aa31: 55                           	pushl	%ebp
  49aa32: 56                           	pushl	%esi
  49aa33: 53                           	pushl	%ebx
  49aa34: e8 17 c5 ff ff               	calll	0x496f50 <.text+0x95f50>
  49aa39: 83 c4 0c                     	addl	$0xc, %esp
  49aa3c: 8b 43 38                     	movl	0x38(%ebx), %eax
  49aa3f: 85 c0                        	testl	%eax, %eax
  49aa41: 57                           	pushl	%edi
  49aa42: 0f 84 a8 00 00 00            	je	0x49aaf0 <.text+0x99af0>
  49aa48: 8b 78 0c                     	movl	0xc(%eax), %edi
  49aa4b: b8 00 80 00 00               	movl	$0x8000, %eax           # imm = 0x8000
  49aa50: 66 85 45 06                  	testw	%ax, 0x6(%ebp)
  49aa54: 74 09                        	je	0x49aa5f <.text+0x99a5f>
  49aa56: b9 03 00 00 00               	movl	$0x3, %ecx
  49aa5b: 8b c1                        	movl	%ecx, %eax
  49aa5d: eb 5a                        	jmp	0x49aab9 <.text+0x99ab9>
  49aa5f: 66 85 45 04                  	testw	%ax, 0x4(%ebp)
  49aa63: 74 09                        	je	0x49aa6e <.text+0x99a6e>
  49aa65: b9 03 00 00 00               	movl	$0x3, %ecx
  49aa6a: 8b c1                        	movl	%ecx, %eax
  49aa6c: eb 4b                        	jmp	0x49aab9 <.text+0x99ab9>
  49aa6e: 8a 03                        	movb	(%ebx), %al
  49aa70: 3c 2c                        	cmpb	$0x2c, %al
  49aa72: 75 0c                        	jne	0x49aa80 <.text+0x99a80>
  49aa74: b9 02 00 00 00               	movl	$0x2, %ecx
  49aa79: b8 03 00 00 00               	movl	$0x3, %eax
  49aa7e: eb 39                        	jmp	0x49aab9 <.text+0x99ab9>
  49aa80: 3c 06                        	cmpb	$0x6, %al
  49aa82: 75 0c                        	jne	0x49aa90 <.text+0x99a90>
  49aa84: b9 01 00 00 00               	movl	$0x1, %ecx
  49aa89: b8 03 00 00 00               	movl	$0x3, %eax
  49aa8e: eb 29                        	jmp	0x49aab9 <.text+0x99ab9>
  49aa90: 33 d2                        	xorl	%edx, %edx
  49aa92: 66 8b 53 04                  	movw	0x4(%ebx), %dx
  49aa96: 81 e2 00 01 00 00            	andl	$0x100, %edx            # imm = 0x100
  49aa9c: 66 85 d2                     	testw	%dx, %dx
  49aa9f: 74 09                        	je	0x49aaaa <.text+0x99aaa>
  49aaa1: b9 01 00 00 00               	movl	$0x1, %ecx
  49aaa6: 8b c1                        	movl	%ecx, %eax
  49aaa8: eb 0f                        	jmp	0x49aab9 <.text+0x99ab9>
  49aaaa: 33 c9                        	xorl	%ecx, %ecx
  49aaac: 3c 2d                        	cmpb	$0x2d, %al
  49aaae: 75 07                        	jne	0x49aab7 <.text+0x99ab7>
  49aab0: b8 01 00 00 00               	movl	$0x1, %eax
  49aab5: eb 02                        	jmp	0x49aab9 <.text+0x99ab9>
  49aab7: 33 c0                        	xorl	%eax, %eax
  49aab9: 8a 57 03                     	movb	0x3(%edi), %dl
  49aabc: 24 03                        	andb	$0x3, %al
  49aabe: c0 e0 02                     	shlb	$0x2, %al
  49aac1: 80 e2 f0                     	andb	$-0x10, %dl
  49aac4: 0a c2                        	orb	%dl, %al
  49aac6: 80 e1 03                     	andb	$0x3, %cl
  49aac9: 0a c1                        	orb	%cl, %al
  49aacb: 88 47 03                     	movb	%al, 0x3(%edi)
  49aace: 80 3b 48                     	cmpb	$0x48, (%ebx)
  49aad1: 75 0e                        	jne	0x49aae1 <.text+0x99ae1>
  49aad3: 8b 43 38                     	movl	0x38(%ebx), %eax
  49aad6: 8b 40 0c                     	movl	0xc(%eax), %eax
  49aad9: 8b 48 34                     	movl	0x34(%eax), %ecx
  49aadc: 83 e1 ef                     	andl	$-0x11, %ecx
  49aadf: eb 0c                        	jmp	0x49aaed <.text+0x99aed>
  49aae1: 8b 4b 38                     	movl	0x38(%ebx), %ecx
  49aae4: 8b 41 0c                     	movl	0xc(%ecx), %eax
  49aae7: 8b 48 34                     	movl	0x34(%eax), %ecx
  49aaea: 83 c9 10                     	orl	$0x10, %ecx
  49aaed: 89 48 34                     	movl	%ecx, 0x34(%eax)
  49aaf0: 33 d2                        	xorl	%edx, %edx
  49aaf2: 66 8b 53 04                  	movw	0x4(%ebx), %dx
  49aaf6: 81 e2 00 01 00 00            	andl	$0x100, %edx            # imm = 0x100
  49aafc: 66 85 d2                     	testw	%dx, %dx
  49aaff: 74 04                        	je	0x49ab05 <.text+0x99b05>
  49ab01: 66 ff 45 7e                  	incw	0x7e(%ebp)
  49ab05: 8a 03                        	movb	(%ebx), %al
  49ab07: 3c 4b                        	cmpb	$0x4b, %al
  49ab09: 7c 0b                        	jl	0x49ab16 <.text+0x99b16>
  49ab0b: 3c 52                        	cmpb	$0x52, %al
  49ab0d: c6 05 45 5d c5 03 01         	movb	$0x1, 0x3c55d45
  49ab14: 7e 07                        	jle	0x49ab1d <.text+0x99b1d>
  49ab16: c6 05 45 5d c5 03 00         	movb	$0x0, 0x3c55d45
  49ab1d: 0f be 03                     	movsbl	(%ebx), %eax
  49ab20: 83 f8 57                     	cmpl	$0x57, %eax
  49ab23: 0f 87 db 12 00 00            	ja	0x49be04 <.text+0x9ae04>
  49ab29: ff 24 85 bc bf 49 00         	jmpl	*0x49bfbc(,%eax,4)
  49ab30: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  49ab34: 0f b6 4b 08                  	movzbl	0x8(%ebx), %ecx
  49ab38: 50                           	pushl	%eax
  49ab39: 51                           	pushl	%ecx
  49ab3a: c6 43 09 00                  	movb	$0x0, 0x9(%ebx)
  49ab3e: e8 0d 7c fa ff               	calll	0x442750 <.text+0x41750>
  49ab43: 8b 2e                        	movl	(%esi), %ebp
  49ab45: 33 ff                        	xorl	%edi, %edi
  49ab47: c7 85 40 01 00 00 10 62 c5 03	movl	$0x3c56210, 0x140(%ebp) # imm = 0x3C56210
  49ab51: 66 89 bd 24 01 00 00         	movw	%di, 0x124(%ebp)
  49ab58: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49ab5d: 83 c4 08                     	addl	$0x8, %esp
  49ab60: 84 c0                        	testb	%al, %al
  49ab62: 75 4c                        	jne	0x49abb0 <.text+0x99bb0>
  49ab64: 8b 0d 80 6e 8f 03            	movl	0x38f6e80, %ecx
  49ab6a: 8b 41 18                     	movl	0x18(%ecx), %eax
  49ab6d: 8b 49 14                     	movl	0x14(%ecx), %ecx
  49ab70: 39 48 30                     	cmpl	%ecx, 0x30(%eax)
  49ab73: 74 3b                        	je	0x49abb0 <.text+0x99bb0>
  49ab75: 89 48 30                     	movl	%ecx, 0x30(%eax)
  49ab78: c7 85 44 01 00 00 28 5e c5 03	movl	$0x3c55e28, 0x144(%ebp) # imm = 0x3C55E28
  49ab82: 8b 15 80 6e 8f 03            	movl	0x38f6e80, %edx
  49ab88: 8b 42 18                     	movl	0x18(%edx), %eax
  49ab8b: 8b 48 30                     	movl	0x30(%eax), %ecx
  49ab8e: 55                           	pushl	%ebp
  49ab8f: 56                           	pushl	%esi
  49ab90: 89 0d 38 5f c5 03            	movl	%ecx, 0x3c55f38
  49ab96: 53                           	pushl	%ebx
  49ab97: 66 89 bd 22 01 00 00         	movw	%di, 0x122(%ebp)
  49ab9e: e8 ed 4e fa ff               	calll	0x43fa90 <.text+0x3ea90>
  49aba3: 83 c4 0c                     	addl	$0xc, %esp
  49aba6: c7 85 44 01 00 00 28 5e c5 03	movl	$0x3c55e28, 0x144(%ebp) # imm = 0x3C55E28
  49abb0: 8b 15 80 6e 8f 03            	movl	0x38f6e80, %edx
  49abb6: 8b 42 18                     	movl	0x18(%edx), %eax
  49abb9: 8b 54 24 2c                  	movl	0x2c(%esp), %edx
  49abbd: 8b 48 30                     	movl	0x30(%eax), %ecx
  49abc0: 57                           	pushl	%edi
  49abc1: 6a 02                        	pushl	$0x2
  49abc3: 68 f0 cb 91 00               	pushl	$0x91cbf0               # imm = 0x91CBF0
  49abc8: 52                           	pushl	%edx
  49abc9: 89 0d 40 5d c5 03            	movl	%ecx, 0x3c55d40
  49abcf: e8 1c 1f f8 ff               	calll	0x41caf0 <.text+0x1baf0>
  49abd4: 8b 43 38                     	movl	0x38(%ebx), %eax
  49abd7: 8b 48 0c                     	movl	0xc(%eax), %ecx
  49abda: 8b 95 0c 01 00 00            	movl	0x10c(%ebp), %edx
  49abe0: 89 51 14                     	movl	%edx, 0x14(%ecx)
  49abe3: c6 03 01                     	movb	$0x1, (%ebx)
  49abe6: 57                           	pushl	%edi
  49abe7: 66 89 7d 0e                  	movw	%di, 0xe(%ebp)
  49abeb: e8 90 66 f8 ff               	calll	0x421280 <.text+0x20280>
  49abf0: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49abf5: 83 c4 14                     	addl	$0x14, %esp
  49abf8: 84 c0                        	testb	%al, %al
  49abfa: 74 0a                        	je	0x49ac06 <.text+0x99c06>
  49abfc: 6a 08                        	pushl	$0x8
  49abfe: e8 7d 66 f8 ff               	calll	0x421280 <.text+0x20280>
  49ac03: 83 c4 04                     	addl	$0x4, %esp
  49ac06: 33 c0                        	xorl	%eax, %eax
  49ac08: 89 85 94 00 00 00            	movl	%eax, 0x94(%ebp)
  49ac0e: 89 85 98 00 00 00            	movl	%eax, 0x98(%ebp)
  49ac14: 33 c0                        	xorl	%eax, %eax
  49ac16: 8a 43 08                     	movb	0x8(%ebx), %al
  49ac19: bf 58 5d c5 03               	movl	$0x3c55d58, %edi        # imm = 0x3C55D58
  49ac1e: 50                           	pushl	%eax
  49ac1f: e8 3c ac ff ff               	calll	0x495860 <.text+0x94860>
  49ac24: 8d 8d 20 01 00 00            	leal	0x120(%ebp), %ecx
  49ac2a: 51                           	pushl	%ecx
  49ac2b: e8 d0 fb fa ff               	calll	0x44a800 <.text+0x49800>
  49ac30: 8a 15 b5 8d b1 03            	movb	0x3b18db5, %dl
  49ac36: 83 c4 08                     	addl	$0x8, %esp
  49ac39: 84 d2                        	testb	%dl, %dl
  49ac3b: b8 28 5e c5 03               	movl	$0x3c55e28, %eax        # imm = 0x3C55E28
  49ac40: 74 05                        	je	0x49ac47 <.text+0x99c47>
  49ac42: b8 68 5d c5 03               	movl	$0x3c55d68, %eax        # imm = 0x3C55D68
  49ac47: 0f bf 0d cc 2d b2 03         	movswl	0x3b22dcc, %ecx
  49ac4e: 33 ff                        	xorl	%edi, %edi
  49ac50: 66 8b 3d ec 2d b2 03         	movw	0x3b22dec, %di
  49ac57: c1 e1 08                     	shll	$0x8, %ecx
  49ac5a: 0b cf                        	orl	%edi, %ecx
  49ac5c: 81 e1 00 ff 00 00            	andl	$0xff00, %ecx           # imm = 0xFF00
  49ac62: 81 f9 00 13 00 00            	cmpl	$0x1300, %ecx           # imm = 0x1300
  49ac68: 8b 08                        	movl	(%eax), %ecx
  49ac6a: 75 21                        	jne	0x49ac8d <.text+0x99c8d>
  49ac6c: 85 c9                        	testl	%ecx, %ecx
  49ac6e: 74 3c                        	je	0x49acac <.text+0x99cac>
  49ac70: 8b 0d 80 6e 8f 03            	movl	0x38f6e80, %ecx
  49ac76: 8b 38                        	movl	(%eax), %edi
  49ac78: 3b 79 58                     	cmpl	0x58(%ecx), %edi
  49ac7b: 75 04                        	jne	0x49ac81 <.text+0x99c81>
  49ac7d: c6 40 0d 02                  	movb	$0x2, 0xd(%eax)
  49ac81: 8b 78 18                     	movl	0x18(%eax), %edi
  49ac84: 83 c0 18                     	addl	$0x18, %eax
  49ac87: 85 ff                        	testl	%edi, %edi
  49ac89: 75 eb                        	jne	0x49ac76 <.text+0x99c76>
  49ac8b: eb 1f                        	jmp	0x49acac <.text+0x99cac>
  49ac8d: 85 c9                        	testl	%ecx, %ecx
  49ac8f: 74 1b                        	je	0x49acac <.text+0x99cac>
  49ac91: 8b 0d 80 6e 8f 03            	movl	0x38f6e80, %ecx
  49ac97: 8b 38                        	movl	(%eax), %edi
  49ac99: 3b 79 58                     	cmpl	0x58(%ecx), %edi
  49ac9c: 75 04                        	jne	0x49aca2 <.text+0x99ca2>
  49ac9e: c6 40 0d 00                  	movb	$0x0, 0xd(%eax)
  49aca2: 8b 78 18                     	movl	0x18(%eax), %edi
  49aca5: 83 c0 18                     	addl	$0x18, %eax
  49aca8: 85 ff                        	testl	%edi, %edi
  49acaa: 75 eb                        	jne	0x49ac97 <.text+0x99c97>
  49acac: 84 d2                        	testb	%dl, %dl
  49acae: 75 0c                        	jne	0x49acbc <.text+0x99cbc>
  49acb0: c7 85 44 01 00 00 28 5e c5 03	movl	$0x3c55e28, 0x144(%ebp) # imm = 0x3C55E28
  49acba: eb 0a                        	jmp	0x49acc6 <.text+0x99cc6>
  49acbc: c7 85 44 01 00 00 68 5d c5 03	movl	$0x3c55d68, 0x144(%ebp) # imm = 0x3C55D68
  49acc6: 55                           	pushl	%ebp
  49acc7: 56                           	pushl	%esi
  49acc8: 53                           	pushl	%ebx
  49acc9: e8 c2 4d fa ff               	calll	0x43fa90 <.text+0x3ea90>
  49acce: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49acd3: 83 c4 0c                     	addl	$0xc, %esp
  49acd6: 84 c0                        	testb	%al, %al
  49acd8: 75 3c                        	jne	0x49ad16 <.text+0x99d16>
  49acda: 68 30 37 49 00               	pushl	$0x493730               # imm = 0x493730
  49acdf: 6a 03                        	pushl	$0x3
  49ace1: 6a 02                        	pushl	$0x2
  49ace3: e8 78 0b f7 ff               	calll	0x40b860 <.text+0xa860>
  49ace8: 83 c4 0c                     	addl	$0xc, %esp
  49aceb: 85 c0                        	testl	%eax, %eax
  49aced: 74 09                        	je	0x49acf8 <.text+0x99cf8>
  49acef: 8b 50 20                     	movl	0x20(%eax), %edx
  49acf2: 8a 43 08                     	movb	0x8(%ebx), %al
  49acf5: 88 42 08                     	movb	%al, 0x8(%edx)
  49acf8: 68 b0 37 49 00               	pushl	$0x4937b0               # imm = 0x4937B0
  49acfd: 6a 03                        	pushl	$0x3
  49acff: 6a 03                        	pushl	$0x3
  49ad01: e8 5a 0b f7 ff               	calll	0x40b860 <.text+0xa860>
  49ad06: 83 c4 0c                     	addl	$0xc, %esp
  49ad09: 85 c0                        	testl	%eax, %eax
  49ad0b: 74 09                        	je	0x49ad16 <.text+0x99d16>
  49ad0d: 8b 48 20                     	movl	0x20(%eax), %ecx
  49ad10: 8a 53 08                     	movb	0x8(%ebx), %dl
  49ad13: 88 51 08                     	movb	%dl, 0x8(%ecx)
  49ad16: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  49ad1a: c7 40 18 60 48 49 00         	movl	$0x494860, 0x18(%eax)   # imm = 0x494860
  49ad21: c7 40 14 c0 48 49 00         	movl	$0x4948c0, 0x14(%eax)   # imm = 0x4948C0
  49ad28: e9 d7 10 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49ad2d: 8b 74 24 10                  	movl	0x10(%esp), %esi
  49ad31: 55                           	pushl	%ebp
  49ad32: 56                           	pushl	%esi
  49ad33: 53                           	pushl	%ebx
  49ad34: e8 27 0e fb ff               	calll	0x44bb60 <.text+0x4ab60>
  49ad39: 55                           	pushl	%ebp
  49ad3a: 56                           	pushl	%esi
  49ad3b: 53                           	pushl	%ebx
  49ad3c: e8 2f 15 fb ff               	calll	0x44c270 <.text+0x4b270>
  49ad41: 55                           	pushl	%ebp
  49ad42: 56                           	pushl	%esi
  49ad43: 53                           	pushl	%ebx
  49ad44: e8 07 92 fa ff               	calll	0x443f50 <.text+0x42f50>
  49ad49: 55                           	pushl	%ebp
  49ad4a: 56                           	pushl	%esi
  49ad4b: 53                           	pushl	%ebx
  49ad4c: e8 9f 20 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49ad51: 55                           	pushl	%ebp
  49ad52: 56                           	pushl	%esi
  49ad53: 53                           	pushl	%ebx
  49ad54: e8 17 41 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49ad59: 56                           	pushl	%esi
  49ad5a: 8b f5                        	movl	%ebp, %esi
  49ad5c: e8 ff 68 ff ff               	calll	0x491660 <.text+0x90660>
  49ad61: 83 c4 40                     	addl	$0x40, %esp
  49ad64: e9 97 10 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49ad69: b8 1e 00 00 00               	movl	$0x1e, %eax
  49ad6e: 66 39 85 28 01 00 00         	cmpw	%ax, 0x128(%ebp)
  49ad75: 75 15                        	jne	0x49ad8c <.text+0x99d8c>
  49ad77: 66 39 85 26 01 00 00         	cmpw	%ax, 0x126(%ebp)
  49ad7e: 74 0c                        	je	0x49ad8c <.text+0x99d8c>
  49ad80: d9 85 c0 00 00 00            	flds	0xc0(%ebp)
  49ad86: d8 45 38                     	fadds	0x38(%ebp)
  49ad89: d9 5d 38                     	fstps	0x38(%ebp)
  49ad8c: 55                           	pushl	%ebp
  49ad8d: 56                           	pushl	%esi
  49ad8e: 53                           	pushl	%ebx
  49ad8f: e8 dc 14 fb ff               	calll	0x44c270 <.text+0x4b270>
  49ad94: 55                           	pushl	%ebp
  49ad95: 56                           	pushl	%esi
  49ad96: 53                           	pushl	%ebx
  49ad97: e8 b4 91 fa ff               	calll	0x443f50 <.text+0x42f50>
  49ad9c: 55                           	pushl	%ebp
  49ad9d: 56                           	pushl	%esi
  49ad9e: 53                           	pushl	%ebx
  49ad9f: e8 4c 20 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49ada4: 83 c4 24                     	addl	$0x24, %esp
  49ada7: 83 f8 02                     	cmpl	$0x2, %eax
  49adaa: 75 3d                        	jne	0x49ade9 <.text+0x99de9>
  49adac: 8a 43 08                     	movb	0x8(%ebx), %al
  49adaf: 84 c0                        	testb	%al, %al
  49adb1: 75 1c                        	jne	0x49adcf <.text+0x99dcf>
  49adb3: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49adb8: 84 c0                        	testb	%al, %al
  49adba: 75 13                        	jne	0x49adcf <.text+0x99dcf>
  49adbc: 6a 00                        	pushl	$0x0
  49adbe: 6a 00                        	pushl	$0x0
  49adc0: 6a 00                        	pushl	$0x0
  49adc2: 68 c6 04 00 00               	pushl	$0x4c6                  # imm = 0x4C6
  49adc7: e8 a4 8f f8 ff               	calll	0x423d70 <.text+0x22d70>
  49adcc: 83 c4 10                     	addl	$0x10, %esp
  49adcf: c6 03 12                     	movb	$0x12, (%ebx)
  49add2: 53                           	pushl	%ebx
  49add3: 66 c7 85 24 01 00 00 1b 00   	movw	$0x1b, 0x124(%ebp)
  49addc: e8 af 59 fa ff               	calll	0x440790 <.text+0x3f790>
  49ade1: 83 c4 04                     	addl	$0x4, %esp
  49ade4: e9 86 0e 00 00               	jmp	0x49bc6f <.text+0x9ac6f>
  49ade9: 55                           	pushl	%ebp
  49adea: 56                           	pushl	%esi
  49adeb: 53                           	pushl	%ebx
  49adec: e8 7f 40 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49adf1: 83 c4 0c                     	addl	$0xc, %esp
  49adf4: 8b c5                        	movl	%ebp, %eax
  49adf6: 8b fe                        	movl	%esi, %edi
  49adf8: 8b cb                        	movl	%ebx, %ecx
  49adfa: e8 d1 ae ff ff               	calll	0x495cd0 <.text+0x94cd0>
  49adff: e9 00 10 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49ae04: 55                           	pushl	%ebp
  49ae05: 56                           	pushl	%esi
  49ae06: 53                           	pushl	%ebx
  49ae07: e8 44 7c fa ff               	calll	0x442a50 <.text+0x41a50>
  49ae0c: 55                           	pushl	%ebp
  49ae0d: 56                           	pushl	%esi
  49ae0e: 53                           	pushl	%ebx
  49ae0f: e8 9c 93 fa ff               	calll	0x4441b0 <.text+0x431b0>
  49ae14: 55                           	pushl	%ebp
  49ae15: 56                           	pushl	%esi
  49ae16: 53                           	pushl	%ebx
  49ae17: e8 e4 32 fa ff               	calll	0x43e100 <.text+0x3d100>
  49ae1c: 83 c4 24                     	addl	$0x24, %esp
  49ae1f: 8b c5                        	movl	%ebp, %eax
  49ae21: 8b fe                        	movl	%esi, %edi
  49ae23: 8b cb                        	movl	%ebx, %ecx
  49ae25: e8 a6 ae ff ff               	calll	0x495cd0 <.text+0x94cd0>
  49ae2a: e9 d5 0f 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49ae2f: 55                           	pushl	%ebp
  49ae30: 56                           	pushl	%esi
  49ae31: 53                           	pushl	%ebx
  49ae32: e8 29 0d fb ff               	calll	0x44bb60 <.text+0x4ab60>
  49ae37: 55                           	pushl	%ebp
  49ae38: 56                           	pushl	%esi
  49ae39: 53                           	pushl	%ebx
  49ae3a: e8 11 e0 fa ff               	calll	0x448e50 <.text+0x47e50>
  49ae3f: 55                           	pushl	%ebp
  49ae40: 56                           	pushl	%esi
  49ae41: 53                           	pushl	%ebx
  49ae42: e8 09 91 fa ff               	calll	0x443f50 <.text+0x42f50>
  49ae47: 55                           	pushl	%ebp
  49ae48: 56                           	pushl	%esi
  49ae49: 53                           	pushl	%ebx
  49ae4a: e8 a1 1f fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49ae4f: 55                           	pushl	%ebp
  49ae50: 56                           	pushl	%esi
  49ae51: 53                           	pushl	%ebx
  49ae52: e8 19 40 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49ae57: 68 10 2a 4a 00               	pushl	$0x4a2a10               # imm = 0x4A2A10
  49ae5c: 6a 05                        	pushl	$0x5
  49ae5e: 6a 02                        	pushl	$0x2
  49ae60: e8 fb 09 f7 ff               	calll	0x40b860 <.text+0xa860>
  49ae65: 83 c4 48                     	addl	$0x48, %esp
  49ae68: 85 c0                        	testl	%eax, %eax
  49ae6a: 74 09                        	je	0x49ae75 <.text+0x99e75>
  49ae6c: 8b 40 20                     	movl	0x20(%eax), %eax
  49ae6f: 8a 4b 08                     	movb	0x8(%ebx), %cl
  49ae72: 88 48 08                     	movb	%cl, 0x8(%eax)
  49ae75: f6 45 04 02                  	testb	$0x2, 0x4(%ebp)
  49ae79: 0f 84 85 0f 00 00            	je	0x49be04 <.text+0x9ae04>
  49ae7f: e8 3b 95 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49ae84: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  49ae88: db 44 24 10                  	fildl	0x10(%esp)
  49ae8c: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49ae92: d8 1d 68 d5 7d 00            	fcomps	0x7dd568
  49ae98: df e0                        	fnstsw	%ax
  49ae9a: f6 c4 05                     	testb	$0x5, %ah
  49ae9d: 0f 8a 61 0f 00 00            	jp	0x49be04 <.text+0x9ae04>
  49aea3: bf 03 00 00 00               	movl	$0x3, %edi
  49aea8: 6a 1f                        	pushl	$0x1f
  49aeaa: 53                           	pushl	%ebx
  49aeab: e8 90 71 00 00               	calll	0x4a2040 <.text+0xa1040>
  49aeb0: 83 c4 08                     	addl	$0x8, %esp
  49aeb3: 4f                           	decl	%edi
  49aeb4: 75 f2                        	jne	0x49aea8 <.text+0x99ea8>
  49aeb6: e9 49 0f 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49aebb: 55                           	pushl	%ebp
  49aebc: 56                           	pushl	%esi
  49aebd: 53                           	pushl	%ebx
  49aebe: e8 9d 0c fb ff               	calll	0x44bb60 <.text+0x4ab60>
  49aec3: e9 1a 0d 00 00               	jmp	0x49bbe2 <.text+0x9abe2>
  49aec8: 55                           	pushl	%ebp
  49aec9: 56                           	pushl	%esi
  49aeca: 53                           	pushl	%ebx
  49aecb: e8 e0 7b ff ff               	calll	0x492ab0 <.text+0x91ab0>
  49aed0: 55                           	pushl	%ebp
  49aed1: 56                           	pushl	%esi
  49aed2: 53                           	pushl	%ebx
  49aed3: e8 18 1f fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49aed8: 83 c4 18                     	addl	$0x18, %esp
  49aedb: 83 f8 02                     	cmpl	$0x2, %eax
  49aede: 75 4d                        	jne	0x49af2d <.text+0x99f2d>
  49aee0: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49aee5: 84 c0                        	testb	%al, %al
  49aee7: 75 13                        	jne	0x49aefc <.text+0x99efc>
  49aee9: 6a 00                        	pushl	$0x0
  49aeeb: 6a 00                        	pushl	$0x0
  49aeed: 6a 00                        	pushl	$0x0
  49aeef: 68 c6 04 00 00               	pushl	$0x4c6                  # imm = 0x4C6
  49aef4: e8 77 8e f8 ff               	calll	0x423d70 <.text+0x22d70>
  49aef9: 83 c4 10                     	addl	$0x10, %esp
  49aefc: c6 03 12                     	movb	$0x12, (%ebx)
  49aeff: 66 c7 85 24 01 00 00 1b 00   	movw	$0x1b, 0x124(%ebp)
  49af08: 80 63 05 fb                  	andb	$-0x5, 0x5(%ebx)
  49af0c: 68 fc 02 00 00               	pushl	$0x2fc                  # imm = 0x2FC
  49af11: 66 c7 85 80 00 00 00 00 00   	movw	$0x0, 0x80(%ebp)
  49af1a: e8 f1 92 f8 ff               	calll	0x424210 <.text+0x23210>
  49af1f: 53                           	pushl	%ebx
  49af20: e8 6b 58 fa ff               	calll	0x440790 <.text+0x3f790>
  49af25: 83 c4 08                     	addl	$0x8, %esp
  49af28: e9 d7 0e 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49af2d: 55                           	pushl	%ebp
  49af2e: 56                           	pushl	%esi
  49af2f: 53                           	pushl	%ebx
  49af30: e8 3b 3f fa ff               	calll	0x43ee70 <.text+0x3de70>
  49af35: 53                           	pushl	%ebx
  49af36: e8 15 91 ff ff               	calll	0x494050 <.text+0x93050>
  49af3b: 8a 45 7c                     	movb	0x7c(%ebp), %al
  49af3e: 83 c4 10                     	addl	$0x10, %esp
  49af41: 0c 01                        	orb	$0x1, %al
  49af43: 88 45 7c                     	movb	%al, 0x7c(%ebp)
  49af46: e9 b9 0e 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49af4b: 55                           	pushl	%ebp
  49af4c: 56                           	pushl	%esi
  49af4d: 53                           	pushl	%ebx
  49af4e: e8 fd 86 fa ff               	calll	0x443650 <.text+0x42650>
  49af53: 55                           	pushl	%ebp
  49af54: 56                           	pushl	%esi
  49af55: 53                           	pushl	%ebx
  49af56: e8 55 92 fa ff               	calll	0x4441b0 <.text+0x431b0>
  49af5b: 55                           	pushl	%ebp
  49af5c: 56                           	pushl	%esi
  49af5d: 53                           	pushl	%ebx
  49af5e: e8 9d 31 fa ff               	calll	0x43e100 <.text+0x3d100>
  49af63: 53                           	pushl	%ebx
  49af64: e8 47 91 ff ff               	calll	0x4940b0 <.text+0x930b0>
  49af69: 83 c4 28                     	addl	$0x28, %esp
  49af6c: e9 93 0e 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49af71: 55                           	pushl	%ebp
  49af72: 56                           	pushl	%esi
  49af73: 53                           	pushl	%ebx
  49af74: e8 47 0a fb ff               	calll	0x44b9c0 <.text+0x4a9c0>
  49af79: e9 0d 01 00 00               	jmp	0x49b08b <.text+0x9a08b>
  49af7e: d9 45 3c                     	flds	0x3c(%ebp)
  49af81: 55                           	pushl	%ebp
  49af82: d8 9d b8 00 00 00            	fcomps	0xb8(%ebp)
  49af88: 56                           	pushl	%esi
  49af89: 53                           	pushl	%ebx
  49af8a: df e0                        	fnstsw	%ax
  49af8c: f6 c4 01                     	testb	$0x1, %ah
  49af8f: 0f 85 a9 03 00 00            	jne	0x49b33e <.text+0x9a33e>
  49af95: e8 46 83 fa ff               	calll	0x4432e0 <.text+0x422e0>
  49af9a: e9 a4 03 00 00               	jmp	0x49b343 <.text+0x9a343>
  49af9f: 55                           	pushl	%ebp
  49afa0: 56                           	pushl	%esi
  49afa1: 53                           	pushl	%ebx
  49afa2: e8 59 88 fa ff               	calll	0x443800 <.text+0x42800>
  49afa7: e9 df 00 00 00               	jmp	0x49b08b <.text+0x9a08b>
  49afac: 8b 74 24 10                  	movl	0x10(%esp), %esi
  49afb0: 55                           	pushl	%ebp
  49afb1: 56                           	pushl	%esi
  49afb2: 53                           	pushl	%ebx
  49afb3: e8 18 8b fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49afb8: 56                           	pushl	%esi
  49afb9: 8b fd                        	movl	%ebp, %edi
  49afbb: 8b f3                        	movl	%ebx, %esi
  49afbd: e8 3e 73 ff ff               	calll	0x492300 <.text+0x91300>
  49afc2: 8b 74 24 20                  	movl	0x20(%esp), %esi
  49afc6: 55                           	pushl	%ebp
  49afc7: 56                           	pushl	%esi
  49afc8: 53                           	pushl	%ebx
  49afc9: e8 22 1e fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49afce: 55                           	pushl	%ebp
  49afcf: 56                           	pushl	%esi
  49afd0: 53                           	pushl	%ebx
  49afd1: e8 9a 3e fa ff               	calll	0x43ee70 <.text+0x3de70>
  49afd6: 53                           	pushl	%ebx
  49afd7: e8 74 90 ff ff               	calll	0x494050 <.text+0x93050>
  49afdc: 8a 55 7c                     	movb	0x7c(%ebp), %dl
  49afdf: 80 ca 01                     	orb	$0x1, %dl
  49afe2: 53                           	pushl	%ebx
  49afe3: 88 55 7c                     	movb	%dl, 0x7c(%ebp)
  49afe6: e8 c5 90 ff ff               	calll	0x4940b0 <.text+0x930b0>
  49afeb: 83 c4 30                     	addl	$0x30, %esp
  49afee: e9 0d 0e 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49aff3: 55                           	pushl	%ebp
  49aff4: bf 15 00 00 00               	movl	$0x15, %edi
  49aff9: 66 39 bd 24 01 00 00         	cmpw	%di, 0x124(%ebp)
  49b000: 56                           	pushl	%esi
  49b001: 53                           	pushl	%ebx
  49b002: 75 15                        	jne	0x49b019 <.text+0x9a019>
  49b004: e8 47 de fa ff               	calll	0x448e50 <.text+0x47e50>
  49b009: 83 c4 0c                     	addl	$0xc, %esp
  49b00c: 55                           	pushl	%ebp
  49b00d: 56                           	pushl	%esi
  49b00e: 53                           	pushl	%ebx
  49b00f: e8 ac 92 fa ff               	calll	0x4442c0 <.text+0x432c0>
  49b014: e9 35 03 00 00               	jmp	0x49b34e <.text+0x9a34e>
  49b019: e8 32 7b fa ff               	calll	0x442b50 <.text+0x41b50>
  49b01e: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b023: 83 c4 0c                     	addl	$0xc, %esp
  49b026: 84 c0                        	testb	%al, %al
  49b028: 75 3b                        	jne	0x49b065 <.text+0x9a065>
  49b02a: 53                           	pushl	%ebx
  49b02b: e8 90 09 fa ff               	calll	0x43b9c0 <.text+0x3a9c0>
  49b030: 83 c4 04                     	addl	$0x4, %esp
  49b033: f6 c4 04                     	testb	$0x4, %ah
  49b036: 74 11                        	je	0x49b049 <.text+0x9a049>
  49b038: 6a 02                        	pushl	$0x2
  49b03a: 6a 00                        	pushl	$0x0
  49b03c: 6a 01                        	pushl	$0x1
  49b03e: 53                           	pushl	%ebx
  49b03f: 6a 0f                        	pushl	$0xf
  49b041: e8 0a 8f f8 ff               	calll	0x423f50 <.text+0x22f50>
  49b046: 83 c4 14                     	addl	$0x14, %esp
  49b049: 66 39 bd 28 01 00 00         	cmpw	%di, 0x128(%ebp)
  49b050: 75 13                        	jne	0x49b065 <.text+0x9a065>
  49b052: 6a 00                        	pushl	$0x0
  49b054: 6a 00                        	pushl	$0x0
  49b056: 6a 00                        	pushl	$0x0
  49b058: 68 ca 04 00 00               	pushl	$0x4ca                  # imm = 0x4CA
  49b05d: e8 0e 8d f8 ff               	calll	0x423d70 <.text+0x22d70>
  49b062: 83 c4 10                     	addl	$0x10, %esp
  49b065: 55                           	pushl	%ebp
  49b066: 56                           	pushl	%esi
  49b067: 53                           	pushl	%ebx
  49b068: e8 53 92 fa ff               	calll	0x4442c0 <.text+0x432c0>
  49b06d: e9 dc 02 00 00               	jmp	0x49b34e <.text+0x9a34e>
  49b072: f6 43 04 03                  	testb	$0x3, 0x4(%ebx)
  49b076: 74 0b                        	je	0x49b083 <.text+0x9a083>
  49b078: 55                           	pushl	%ebp
  49b079: 56                           	pushl	%esi
  49b07a: 53                           	pushl	%ebx
  49b07b: e8 50 8a fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b080: 83 c4 0c                     	addl	$0xc, %esp
  49b083: 55                           	pushl	%ebp
  49b084: 56                           	pushl	%esi
  49b085: 53                           	pushl	%ebx
  49b086: e8 c5 dd fa ff               	calll	0x448e50 <.text+0x47e50>
  49b08b: 55                           	pushl	%ebp
  49b08c: 56                           	pushl	%esi
  49b08d: 53                           	pushl	%ebx
  49b08e: e8 bd 8e fa ff               	calll	0x443f50 <.text+0x42f50>
  49b093: 55                           	pushl	%ebp
  49b094: 56                           	pushl	%esi
  49b095: 53                           	pushl	%ebx
  49b096: e8 55 1d fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b09b: 55                           	pushl	%ebp
  49b09c: 56                           	pushl	%esi
  49b09d: 53                           	pushl	%ebx
  49b09e: e8 cd 3d fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b0a3: 83 c4 30                     	addl	$0x30, %esp
  49b0a6: e9 59 0d 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b0ab: 55                           	pushl	%ebp
  49b0ac: 56                           	pushl	%esi
  49b0ad: 53                           	pushl	%ebx
  49b0ae: e8 ad 0a fb ff               	calll	0x44bb60 <.text+0x4ab60>
  49b0b3: 55                           	pushl	%ebp
  49b0b4: 56                           	pushl	%esi
  49b0b5: 53                           	pushl	%ebx
  49b0b6: e8 25 82 fa ff               	calll	0x4432e0 <.text+0x422e0>
  49b0bb: e9 13 0d 00 00               	jmp	0x49bdd3 <.text+0x9add3>
  49b0c0: 8b 74 24 10                  	movl	0x10(%esp), %esi
  49b0c4: 55                           	pushl	%ebp
  49b0c5: 56                           	pushl	%esi
  49b0c6: 53                           	pushl	%ebx
  49b0c7: e8 24 32 fa ff               	calll	0x43e2f0 <.text+0x3d2f0>
  49b0cc: 56                           	pushl	%esi
  49b0cd: 8b f5                        	movl	%ebp, %esi
  49b0cf: 8b fb                        	movl	%ebx, %edi
  49b0d1: e8 ea 67 ff ff               	calll	0x4918c0 <.text+0x908c0>
  49b0d6: 83 c4 10                     	addl	$0x10, %esp
  49b0d9: e9 22 0d 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49b0de: 8b 74 24 10                  	movl	0x10(%esp), %esi
  49b0e2: 55                           	pushl	%ebp
  49b0e3: 56                           	pushl	%esi
  49b0e4: 53                           	pushl	%ebx
  49b0e5: e8 26 34 fa ff               	calll	0x43e510 <.text+0x3d510>
  49b0ea: 56                           	pushl	%esi
  49b0eb: 8b f5                        	movl	%ebp, %esi
  49b0ed: 8b fb                        	movl	%ebx, %edi
  49b0ef: e8 4c 6a ff ff               	calll	0x491b40 <.text+0x90b40>
  49b0f4: 83 c4 10                     	addl	$0x10, %esp
  49b0f7: e9 04 0d 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49b0fc: 55                           	pushl	%ebp
  49b0fd: 56                           	pushl	%esi
  49b0fe: 53                           	pushl	%ebx
  49b0ff: e8 0c d8 01 00               	calll	0x4b8910 <.text+0xb7910>
  49b104: 83 c4 0c                     	addl	$0xc, %esp
  49b107: 83 f8 1f                     	cmpl	$0x1f, %eax
  49b10a: 0f 8f f4 0c 00 00            	jg	0x49be04 <.text+0x9ae04>
  49b110: 66 c7 85 24 01 00 00 48 00   	movw	$0x48, 0x124(%ebp)
  49b119: e9 e6 0c 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b11e: 55                           	pushl	%ebp
  49b11f: 56                           	pushl	%esi
  49b120: 53                           	pushl	%ebx
  49b121: e8 ba 81 fa ff               	calll	0x4432e0 <.text+0x422e0>
  49b126: e9 60 ff ff ff               	jmp	0x49b08b <.text+0x9a08b>
  49b12b: 6a 1e                        	pushl	$0x1e
  49b12d: 6a 00                        	pushl	$0x0
  49b12f: e8 0c 56 fa ff               	calll	0x440740 <.text+0x3f740>
  49b134: 55                           	pushl	%ebp
  49b135: 56                           	pushl	%esi
  49b136: 53                           	pushl	%ebx
  49b137: e8 a4 81 fa ff               	calll	0x4432e0 <.text+0x422e0>
  49b13c: 55                           	pushl	%ebp
  49b13d: 56                           	pushl	%esi
  49b13e: 53                           	pushl	%ebx
  49b13f: e8 0c 8e fa ff               	calll	0x443f50 <.text+0x42f50>
  49b144: 8d 54 24 30                  	leal	0x30(%esp), %edx
  49b148: 52                           	pushl	%edx
  49b149: 8d 44 24 3c                  	leal	0x3c(%esp), %eax
  49b14d: 50                           	pushl	%eax
  49b14e: 53                           	pushl	%ebx
  49b14f: e8 6c 0c fa ff               	calll	0x43bdc0 <.text+0x3adc0>
  49b154: 83 c4 2c                     	addl	$0x2c, %esp
  49b157: 85 c0                        	testl	%eax, %eax
  49b159: 0f 84 89 00 00 00            	je	0x49b1e8 <.text+0x9a1e8>
  49b15f: e8 5b 92 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49b164: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  49b168: db 44 24 14                  	fildl	0x14(%esp)
  49b16c: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49b172: d8 2d 98 cc 7d 00            	fsubrs	0x7dcc98
  49b178: d8 4c 24 10                  	fmuls	0x10(%esp)
  49b17c: d9 5e 04                     	fstps	0x4(%esi)
  49b17f: 8a 43 04                     	movb	0x4(%ebx), %al
  49b182: a8 03                        	testb	$0x3, %al
  49b184: 74 42                        	je	0x49b1c8 <.text+0x9a1c8>
  49b186: e8 34 92 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49b18b: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  49b18f: db 44 24 14                  	fildl	0x14(%esp)
  49b193: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49b199: d8 1d 5c ce 7d 00            	fcomps	0x7dce5c
  49b19f: df e0                        	fnstsw	%ax
  49b1a1: f6 c4 41                     	testb	$0x41, %ah
  49b1a4: 75 22                        	jne	0x49b1c8 <.text+0x9a1c8>
  49b1a6: e8 14 92 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49b1ab: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  49b1af: db 44 24 14                  	fildl	0x14(%esp)
  49b1b3: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49b1b9: d8 0d 8c d5 7d 00            	fmuls	0x7dd58c
  49b1bf: d8 05 40 d5 7d 00            	fadds	0x7dd540
  49b1c5: d9 5e 08                     	fstps	0x8(%esi)
  49b1c8: e8 f2 91 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49b1cd: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  49b1d1: db 44 24 14                  	fildl	0x14(%esp)
  49b1d5: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49b1db: d8 2d 98 cc 7d 00            	fsubrs	0x7dcc98
  49b1e1: d8 4c 24 10                  	fmuls	0x10(%esp)
  49b1e5: d9 5e 0c                     	fstps	0xc(%esi)
  49b1e8: 55                           	pushl	%ebp
  49b1e9: 56                           	pushl	%esi
  49b1ea: 53                           	pushl	%ebx
  49b1eb: e8 00 1c fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b1f0: 55                           	pushl	%ebp
  49b1f1: 56                           	pushl	%esi
  49b1f2: 53                           	pushl	%ebx
  49b1f3: e8 78 3c fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b1f8: 83 c4 18                     	addl	$0x18, %esp
  49b1fb: e9 04 0c 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b200: 55                           	pushl	%ebp
  49b201: 56                           	pushl	%esi
  49b202: 53                           	pushl	%ebx
  49b203: e8 c8 88 fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b208: 56                           	pushl	%esi
  49b209: 8b c5                        	movl	%ebp, %eax
  49b20b: e8 c0 75 ff ff               	calll	0x4927d0 <.text+0x917d0>
  49b210: 83 c4 10                     	addl	$0x10, %esp
  49b213: 85 c0                        	testl	%eax, %eax
  49b215: 75 10                        	jne	0x49b227 <.text+0x9a227>
  49b217: c6 03 0c                     	movb	$0xc, (%ebx)
  49b21a: 66 c7 85 24 01 00 00 12 00   	movw	$0x12, 0x124(%ebp)
  49b223: 80 63 05 fa                  	andb	$-0x6, 0x5(%ebx)
  49b227: 55                           	pushl	%ebp
  49b228: 56                           	pushl	%esi
  49b229: 53                           	pushl	%ebx
  49b22a: e8 c1 1b fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b22f: 55                           	pushl	%ebp
  49b230: 56                           	pushl	%esi
  49b231: 53                           	pushl	%ebx
  49b232: e8 39 3c fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b237: 53                           	pushl	%ebx
  49b238: e8 13 8e ff ff               	calll	0x494050 <.text+0x93050>
  49b23d: 8a 45 7c                     	movb	0x7c(%ebp), %al
  49b240: 83 c4 1c                     	addl	$0x1c, %esp
  49b243: 0c 01                        	orb	$0x1, %al
  49b245: 88 45 7c                     	movb	%al, 0x7c(%ebp)
  49b248: e9 b7 0b 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b24d: 55                           	pushl	%ebp
  49b24e: 56                           	pushl	%esi
  49b24f: 53                           	pushl	%ebx
  49b250: e8 0b 09 fb ff               	calll	0x44bb60 <.text+0x4ab60>
  49b255: 55                           	pushl	%ebp
  49b256: 56                           	pushl	%esi
  49b257: 53                           	pushl	%ebx
  49b258: e8 13 10 fb ff               	calll	0x44c270 <.text+0x4b270>
  49b25d: e9 71 0b 00 00               	jmp	0x49bdd3 <.text+0x9add3>
  49b262: 55                           	pushl	%ebp
  49b263: 56                           	pushl	%esi
  49b264: 53                           	pushl	%ebx
  49b265: e8 e6 78 fa ff               	calll	0x442b50 <.text+0x41b50>
  49b26a: 55                           	pushl	%ebp
  49b26b: 56                           	pushl	%esi
  49b26c: 53                           	pushl	%ebx
  49b26d: e8 4e 90 fa ff               	calll	0x4442c0 <.text+0x432c0>
  49b272: e9 1c fe ff ff               	jmp	0x49b093 <.text+0x9a093>
  49b277: 55                           	pushl	%ebp
  49b278: 56                           	pushl	%esi
  49b279: 53                           	pushl	%ebx
  49b27a: e8 41 0f fb ff               	calll	0x44c1c0 <.text+0x4b1c0>
  49b27f: 55                           	pushl	%ebp
  49b280: 56                           	pushl	%esi
  49b281: 53                           	pushl	%ebx
  49b282: e8 59 80 fa ff               	calll	0x4432e0 <.text+0x422e0>
  49b287: e9 47 0b 00 00               	jmp	0x49bdd3 <.text+0x9add3>
  49b28c: b8 24 00 00 00               	movl	$0x24, %eax
  49b291: 66 39 85 28 01 00 00         	cmpw	%ax, 0x128(%ebp)
  49b298: 75 15                        	jne	0x49b2af <.text+0x9a2af>
  49b29a: 66 39 85 26 01 00 00         	cmpw	%ax, 0x126(%ebp)
  49b2a1: 74 0c                        	je	0x49b2af <.text+0x9a2af>
  49b2a3: d9 85 c0 00 00 00            	flds	0xc0(%ebp)
  49b2a9: d8 45 38                     	fadds	0x38(%ebp)
  49b2ac: d9 5d 38                     	fstps	0x38(%ebp)
  49b2af: 55                           	pushl	%ebp
  49b2b0: 56                           	pushl	%esi
  49b2b1: 53                           	pushl	%ebx
  49b2b2: e8 b9 0f fb ff               	calll	0x44c270 <.text+0x4b270>
  49b2b7: 55                           	pushl	%ebp
  49b2b8: 56                           	pushl	%esi
  49b2b9: 53                           	pushl	%ebx
  49b2ba: e8 91 8c fa ff               	calll	0x443f50 <.text+0x42f50>
  49b2bf: 55                           	pushl	%ebp
  49b2c0: 56                           	pushl	%esi
  49b2c1: 53                           	pushl	%ebx
  49b2c2: e8 29 1b fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b2c7: 55                           	pushl	%ebp
  49b2c8: 56                           	pushl	%esi
  49b2c9: 53                           	pushl	%ebx
  49b2ca: e8 a1 3b fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b2cf: 83 c4 30                     	addl	$0x30, %esp
  49b2d2: 8b c5                        	movl	%ebp, %eax
  49b2d4: 8b fe                        	movl	%esi, %edi
  49b2d6: 8b cb                        	movl	%ebx, %ecx
  49b2d8: e8 f3 a9 ff ff               	calll	0x495cd0 <.text+0x94cd0>
  49b2dd: e9 22 0b 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b2e2: 55                           	pushl	%ebp
  49b2e3: 56                           	pushl	%esi
  49b2e4: 53                           	pushl	%ebx
  49b2e5: e8 e6 87 fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b2ea: 55                           	pushl	%ebp
  49b2eb: 56                           	pushl	%esi
  49b2ec: 53                           	pushl	%ebx
  49b2ed: e8 7e 0a fb ff               	calll	0x44bd70 <.text+0x4ad70>
  49b2f2: 83 c4 18                     	addl	$0x18, %esp
  49b2f5: 85 c0                        	testl	%eax, %eax
  49b2f7: 75 0b                        	jne	0x49b304 <.text+0x9a304>
  49b2f9: 55                           	pushl	%ebp
  49b2fa: 56                           	pushl	%esi
  49b2fb: 53                           	pushl	%ebx
  49b2fc: e8 bf 06 fb ff               	calll	0x44b9c0 <.text+0x4a9c0>
  49b301: 83 c4 0c                     	addl	$0xc, %esp
  49b304: 55                           	pushl	%ebp
  49b305: 56                           	pushl	%esi
  49b306: 53                           	pushl	%ebx
  49b307: e8 44 8c fa ff               	calll	0x443f50 <.text+0x42f50>
  49b30c: 55                           	pushl	%ebp
  49b30d: 56                           	pushl	%esi
  49b30e: 53                           	pushl	%ebx
  49b30f: e8 dc 1a fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b314: 55                           	pushl	%ebp
  49b315: 56                           	pushl	%esi
  49b316: 53                           	pushl	%ebx
  49b317: e8 54 3b fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b31c: 83 c4 24                     	addl	$0x24, %esp
  49b31f: e9 e0 0a 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b324: 55                           	pushl	%ebp
  49b325: 56                           	pushl	%esi
  49b326: 53                           	pushl	%ebx
  49b327: e8 a4 87 fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b32c: 55                           	pushl	%ebp
  49b32d: 56                           	pushl	%esi
  49b32e: 53                           	pushl	%ebx
  49b32f: e8 3c 0a fb ff               	calll	0x44bd70 <.text+0x4ad70>
  49b334: 83 c4 18                     	addl	$0x18, %esp
  49b337: 85 c0                        	testl	%eax, %eax
  49b339: 75 0b                        	jne	0x49b346 <.text+0x9a346>
  49b33b: 55                           	pushl	%ebp
  49b33c: 56                           	pushl	%esi
  49b33d: 53                           	pushl	%ebx
  49b33e: e8 7d 06 fb ff               	calll	0x44b9c0 <.text+0x4a9c0>
  49b343: 83 c4 0c                     	addl	$0xc, %esp
  49b346: 55                           	pushl	%ebp
  49b347: 56                           	pushl	%esi
  49b348: 53                           	pushl	%ebx
  49b349: e8 02 8c fa ff               	calll	0x443f50 <.text+0x42f50>
  49b34e: 55                           	pushl	%ebp
  49b34f: 56                           	pushl	%esi
  49b350: 53                           	pushl	%ebx
  49b351: e8 9a 1a fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b356: 55                           	pushl	%ebp
  49b357: 56                           	pushl	%esi
  49b358: 53                           	pushl	%ebx
  49b359: e8 12 3b fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b35e: 83 c4 24                     	addl	$0x24, %esp
  49b361: e9 9e 0a 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b366: 55                           	pushl	%ebp
  49b367: 56                           	pushl	%esi
  49b368: 53                           	pushl	%ebx
  49b369: e8 e2 77 fa ff               	calll	0x442b50 <.text+0x41b50>
  49b36e: 55                           	pushl	%ebp
  49b36f: 56                           	pushl	%esi
  49b370: 53                           	pushl	%ebx
  49b371: e8 7a e1 fa ff               	calll	0x4494f0 <.text+0x484f0>
  49b376: 55                           	pushl	%ebp
  49b377: 56                           	pushl	%esi
  49b378: 53                           	pushl	%ebx
  49b379: e8 72 1a fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b37e: 55                           	pushl	%ebp
  49b37f: 56                           	pushl	%esi
  49b380: 53                           	pushl	%ebx
  49b381: e8 ea 3a fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b386: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b38b: 83 c4 30                     	addl	$0x30, %esp
  49b38e: 84 c0                        	testb	%al, %al
  49b390: 0f 85 6e 0a 00 00            	jne	0x49be04 <.text+0x9ae04>
  49b396: 53                           	pushl	%ebx
  49b397: e8 24 06 fa ff               	calll	0x43b9c0 <.text+0x3a9c0>
  49b39c: 83 c4 04                     	addl	$0x4, %esp
  49b39f: f6 c4 04                     	testb	$0x4, %ah
  49b3a2: 0f 84 5c 0a 00 00            	je	0x49be04 <.text+0x9ae04>
  49b3a8: 6a 02                        	pushl	$0x2
  49b3aa: 6a 00                        	pushl	$0x0
  49b3ac: 6a 01                        	pushl	$0x1
  49b3ae: 53                           	pushl	%ebx
  49b3af: 6a 0f                        	pushl	$0xf
  49b3b1: e8 9a 8b f8 ff               	calll	0x423f50 <.text+0x22f50>
  49b3b6: 83 c4 14                     	addl	$0x14, %esp
  49b3b9: e9 46 0a 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b3be: 55                           	pushl	%ebp
  49b3bf: 56                           	pushl	%esi
  49b3c0: 53                           	pushl	%ebx
  49b3c1: e8 0a 87 fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b3c6: 55                           	pushl	%ebp
  49b3c7: 56                           	pushl	%esi
  49b3c8: 53                           	pushl	%ebx
  49b3c9: e8 f2 0d fb ff               	calll	0x44c1c0 <.text+0x4b1c0>
  49b3ce: 55                           	pushl	%ebp
  49b3cf: 56                           	pushl	%esi
  49b3d0: 53                           	pushl	%ebx
  49b3d1: e8 0a 7f fa ff               	calll	0x4432e0 <.text+0x422e0>
  49b3d6: 55                           	pushl	%ebp
  49b3d7: 56                           	pushl	%esi
  49b3d8: 53                           	pushl	%ebx
  49b3d9: e8 72 8b fa ff               	calll	0x443f50 <.text+0x42f50>
  49b3de: 55                           	pushl	%ebp
  49b3df: 56                           	pushl	%esi
  49b3e0: 53                           	pushl	%ebx
  49b3e1: e8 0a 1a fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b3e6: 55                           	pushl	%ebp
  49b3e7: 56                           	pushl	%esi
  49b3e8: 53                           	pushl	%ebx
  49b3e9: e8 82 3a fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b3ee: 83 c4 48                     	addl	$0x48, %esp
  49b3f1: e9 0e 0a 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b3f6: 55                           	pushl	%ebp
  49b3f7: 56                           	pushl	%esi
  49b3f8: 53                           	pushl	%ebx
  49b3f9: e8 62 07 fa ff               	calll	0x43bb60 <.text+0x3ab60>
  49b3fe: 83 c4 0c                     	addl	$0xc, %esp
  49b401: e9 fe 09 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b406: d9 85 30 01 00 00            	flds	0x130(%ebp)
  49b40c: 8b 4d 68                     	movl	0x68(%ebp), %ecx
  49b40f: 8b 51 24                     	movl	0x24(%ecx), %edx
  49b412: d8 0d 54 d5 7d 00            	fmuls	0x7dd554
  49b418: d9 5a 18                     	fstps	0x18(%edx)
  49b41b: 8b 45 68                     	movl	0x68(%ebp), %eax
  49b41e: 8b 40 24                     	movl	0x24(%eax), %eax
  49b421: 8b 48 18                     	movl	0x18(%eax), %ecx
  49b424: 89 48 14                     	movl	%ecx, 0x14(%eax)
  49b427: 8b 55 68                     	movl	0x68(%ebp), %edx
  49b42a: 8b 42 24                     	movl	0x24(%edx), %eax
  49b42d: 8b 48 14                     	movl	0x14(%eax), %ecx
  49b430: 89 48 10                     	movl	%ecx, 0x10(%eax)
  49b433: e9 cc 09 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b438: 55                           	pushl	%ebp
  49b439: 56                           	pushl	%esi
  49b43a: 53                           	pushl	%ebx
  49b43b: e8 10 cd fa ff               	calll	0x448150 <.text+0x47150>
  49b440: 55                           	pushl	%ebp
  49b441: 56                           	pushl	%esi
  49b442: 53                           	pushl	%ebx
  49b443: e8 08 8b fa ff               	calll	0x443f50 <.text+0x42f50>
  49b448: 55                           	pushl	%ebp
  49b449: 56                           	pushl	%esi
  49b44a: 53                           	pushl	%ebx
  49b44b: e8 c0 31 fa ff               	calll	0x43e610 <.text+0x3d610>
  49b450: 83 c4 24                     	addl	$0x24, %esp
  49b453: 83 f8 02                     	cmpl	$0x2, %eax
  49b456: 0f 85 8f 00 00 00            	jne	0x49b4eb <.text+0x9a4eb>
  49b45c: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b461: 84 c0                        	testb	%al, %al
  49b463: 75 13                        	jne	0x49b478 <.text+0x9a478>
  49b465: 6a 00                        	pushl	$0x0
  49b467: 6a 00                        	pushl	$0x0
  49b469: 6a 00                        	pushl	$0x0
  49b46b: 68 c6 04 00 00               	pushl	$0x4c6                  # imm = 0x4C6
  49b470: e8 fb 88 f8 ff               	calll	0x423d70 <.text+0x22d70>
  49b475: 83 c4 10                     	addl	$0x10, %esp
  49b478: c6 03 32                     	movb	$0x32, (%ebx)
  49b47b: 53                           	pushl	%ebx
  49b47c: 66 c7 85 24 01 00 00 2e 00   	movw	$0x2e, 0x124(%ebp)
  49b485: e8 06 53 fa ff               	calll	0x440790 <.text+0x3f790>
  49b48a: 8b 55 64                     	movl	0x64(%ebp), %edx
  49b48d: d9 42 1c                     	flds	0x1c(%edx)
  49b490: 83 c4 04                     	addl	$0x4, %esp
  49b493: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b499: d8 43 20                     	fadds	0x20(%ebx)
  49b49c: d9 5b 20                     	fstps	0x20(%ebx)
  49b49f: 8b 45 64                     	movl	0x64(%ebp), %eax
  49b4a2: d9 40 20                     	flds	0x20(%eax)
  49b4a5: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b4ab: d8 43 24                     	fadds	0x24(%ebx)
  49b4ae: d9 5b 24                     	fstps	0x24(%ebx)
  49b4b1: 8b 4d 64                     	movl	0x64(%ebp), %ecx
  49b4b4: d9 41 24                     	flds	0x24(%ecx)
  49b4b7: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b4bd: d8 43 28                     	fadds	0x28(%ebx)
  49b4c0: d9 5b 28                     	fstps	0x28(%ebx)
  49b4c3: 8b 95 b4 00 00 00            	movl	0xb4(%ebp), %edx
  49b4c9: d9 45 38                     	flds	0x38(%ebp)
  49b4cc: 89 55 3c                     	movl	%edx, 0x3c(%ebp)
  49b4cf: d8 0d 8c d5 7d 00            	fmuls	0x7dd58c
  49b4d5: d9 5d 38                     	fstps	0x38(%ebp)
  49b4d8: c7 43 14 00 00 00 00         	movl	$0x0, 0x14(%ebx)
  49b4df: c7 43 1c 00 c0 00 00         	movl	$0xc000, 0x1c(%ebx)     # imm = 0xC000
  49b4e6: e9 19 09 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b4eb: 55                           	pushl	%ebp
  49b4ec: 56                           	pushl	%esi
  49b4ed: 53                           	pushl	%ebx
  49b4ee: e8 1d 3b fa ff               	calll	0x43f010 <.text+0x3e010>
  49b4f3: 83 c4 0c                     	addl	$0xc, %esp
  49b4f6: 8b c5                        	movl	%ebp, %eax
  49b4f8: 8b fe                        	movl	%esi, %edi
  49b4fa: 8b cb                        	movl	%ebx, %ecx
  49b4fc: e8 cf a7 ff ff               	calll	0x495cd0 <.text+0x94cd0>
  49b501: e9 fe 08 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b506: 55                           	pushl	%ebp
  49b507: 56                           	pushl	%esi
  49b508: 53                           	pushl	%ebx
  49b509: e8 42 cc fa ff               	calll	0x448150 <.text+0x47150>
  49b50e: 55                           	pushl	%ebp
  49b50f: 56                           	pushl	%esi
  49b510: 53                           	pushl	%ebx
  49b511: e8 3a 8a fa ff               	calll	0x443f50 <.text+0x42f50>
  49b516: 55                           	pushl	%ebp
  49b517: 56                           	pushl	%esi
  49b518: 53                           	pushl	%ebx
  49b519: e8 f2 30 fa ff               	calll	0x43e610 <.text+0x3d610>
  49b51e: 83 c4 24                     	addl	$0x24, %esp
  49b521: 83 f8 02                     	cmpl	$0x2, %eax
  49b524: 0f 85 93 00 00 00            	jne	0x49b5bd <.text+0x9a5bd>
  49b52a: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b52f: 84 c0                        	testb	%al, %al
  49b531: 75 13                        	jne	0x49b546 <.text+0x9a546>
  49b533: 6a 00                        	pushl	$0x0
  49b535: 6a 00                        	pushl	$0x0
  49b537: 6a 00                        	pushl	$0x0
  49b539: 68 c6 04 00 00               	pushl	$0x4c6                  # imm = 0x4C6
  49b53e: e8 2d 88 f8 ff               	calll	0x423d70 <.text+0x22d70>
  49b543: 83 c4 10                     	addl	$0x10, %esp
  49b546: 80 63 05 fa                  	andb	$-0x6, 0x5(%ebx)
  49b54a: c6 03 32                     	movb	$0x32, (%ebx)
  49b54d: 53                           	pushl	%ebx
  49b54e: 66 c7 85 24 01 00 00 2e 00   	movw	$0x2e, 0x124(%ebp)
  49b557: e8 34 52 fa ff               	calll	0x440790 <.text+0x3f790>
  49b55c: 8b 45 64                     	movl	0x64(%ebp), %eax
  49b55f: d9 40 1c                     	flds	0x1c(%eax)
  49b562: 83 c4 04                     	addl	$0x4, %esp
  49b565: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b56b: d8 43 20                     	fadds	0x20(%ebx)
  49b56e: d9 5b 20                     	fstps	0x20(%ebx)
  49b571: 8b 4d 64                     	movl	0x64(%ebp), %ecx
  49b574: d9 41 20                     	flds	0x20(%ecx)
  49b577: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b57d: d8 43 24                     	fadds	0x24(%ebx)
  49b580: d9 5b 24                     	fstps	0x24(%ebx)
  49b583: 8b 55 64                     	movl	0x64(%ebp), %edx
  49b586: d9 42 24                     	flds	0x24(%edx)
  49b589: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b58f: d8 43 28                     	fadds	0x28(%ebx)
  49b592: d9 5b 28                     	fstps	0x28(%ebx)
  49b595: 8b 85 b4 00 00 00            	movl	0xb4(%ebp), %eax
  49b59b: d9 45 38                     	flds	0x38(%ebp)
  49b59e: 89 45 3c                     	movl	%eax, 0x3c(%ebp)
  49b5a1: d8 0d 8c d5 7d 00            	fmuls	0x7dd58c
  49b5a7: d9 5d 38                     	fstps	0x38(%ebp)
  49b5aa: c7 43 14 00 00 00 00         	movl	$0x0, 0x14(%ebx)
  49b5b1: c7 43 1c 00 c0 00 00         	movl	$0xc000, 0x1c(%ebx)     # imm = 0xC000
  49b5b8: e9 74 05 00 00               	jmp	0x49bb31 <.text+0x9ab31>
  49b5bd: 55                           	pushl	%ebp
  49b5be: 56                           	pushl	%esi
  49b5bf: 53                           	pushl	%ebx
  49b5c0: e8 4b 3a fa ff               	calll	0x43f010 <.text+0x3e010>
  49b5c5: e9 64 05 00 00               	jmp	0x49bb2e <.text+0x9ab2e>
  49b5ca: 55                           	pushl	%ebp
  49b5cb: 56                           	pushl	%esi
  49b5cc: 53                           	pushl	%ebx
  49b5cd: e8 be 85 fa ff               	calll	0x443b90 <.text+0x42b90>
  49b5d2: 55                           	pushl	%ebp
  49b5d3: 56                           	pushl	%esi
  49b5d4: 53                           	pushl	%ebx
  49b5d5: e8 76 cb fa ff               	calll	0x448150 <.text+0x47150>
  49b5da: 55                           	pushl	%ebp
  49b5db: 56                           	pushl	%esi
  49b5dc: 53                           	pushl	%ebx
  49b5dd: e8 6e 89 fa ff               	calll	0x443f50 <.text+0x42f50>
  49b5e2: 55                           	pushl	%ebp
  49b5e3: 56                           	pushl	%esi
  49b5e4: 53                           	pushl	%ebx
  49b5e5: e8 26 30 fa ff               	calll	0x43e610 <.text+0x3d610>
  49b5ea: 83 c4 30                     	addl	$0x30, %esp
  49b5ed: 83 f8 02                     	cmpl	$0x2, %eax
  49b5f0: 0f 85 9e 00 00 00            	jne	0x49b694 <.text+0x9a694>
  49b5f6: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b5fb: 84 c0                        	testb	%al, %al
  49b5fd: 75 13                        	jne	0x49b612 <.text+0x9a612>
  49b5ff: 6a 00                        	pushl	$0x0
  49b601: 6a 00                        	pushl	$0x0
  49b603: 6a 00                        	pushl	$0x0
  49b605: 68 c6 04 00 00               	pushl	$0x4c6                  # imm = 0x4C6
  49b60a: e8 61 87 f8 ff               	calll	0x423d70 <.text+0x22d70>
  49b60f: 83 c4 10                     	addl	$0x10, %esp
  49b612: c6 03 32                     	movb	$0x32, (%ebx)
  49b615: 53                           	pushl	%ebx
  49b616: 66 c7 85 24 01 00 00 2e 00   	movw	$0x2e, 0x124(%ebp)
  49b61f: e8 6c 51 fa ff               	calll	0x440790 <.text+0x3f790>
  49b624: 8b 4d 64                     	movl	0x64(%ebp), %ecx
  49b627: d9 41 1c                     	flds	0x1c(%ecx)
  49b62a: 83 c4 04                     	addl	$0x4, %esp
  49b62d: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b633: 55                           	pushl	%ebp
  49b634: 56                           	pushl	%esi
  49b635: 53                           	pushl	%ebx
  49b636: d8 43 20                     	fadds	0x20(%ebx)
  49b639: d9 5b 20                     	fstps	0x20(%ebx)
  49b63c: 8b 55 64                     	movl	0x64(%ebp), %edx
  49b63f: d9 42 20                     	flds	0x20(%edx)
  49b642: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b648: d8 43 24                     	fadds	0x24(%ebx)
  49b64b: d9 5b 24                     	fstps	0x24(%ebx)
  49b64e: 8b 45 64                     	movl	0x64(%ebp), %eax
  49b651: d9 40 24                     	flds	0x24(%eax)
  49b654: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b65a: d8 43 28                     	fadds	0x28(%ebx)
  49b65d: d9 5b 28                     	fstps	0x28(%ebx)
  49b660: 8b 8d b4 00 00 00            	movl	0xb4(%ebp), %ecx
  49b666: d9 45 38                     	flds	0x38(%ebp)
  49b669: 89 4d 3c                     	movl	%ecx, 0x3c(%ebp)
  49b66c: d8 0d 8c d5 7d 00            	fmuls	0x7dd58c
  49b672: d9 5d 38                     	fstps	0x38(%ebp)
  49b675: 80 63 05 fa                  	andb	$-0x6, 0x5(%ebx)
  49b679: c7 43 14 00 00 00 00         	movl	$0x0, 0x14(%ebx)
  49b680: c7 43 1c 00 c0 00 00         	movl	$0xc000, 0x1c(%ebx)     # imm = 0xC000
  49b687: e8 84 39 fa ff               	calll	0x43f010 <.text+0x3e010>
  49b68c: 83 c4 0c                     	addl	$0xc, %esp
  49b68f: e9 70 07 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b694: 55                           	pushl	%ebp
  49b695: 56                           	pushl	%esi
  49b696: 53                           	pushl	%ebx
  49b697: e8 74 39 fa ff               	calll	0x43f010 <.text+0x3e010>
  49b69c: 83 c4 0c                     	addl	$0xc, %esp
  49b69f: 55                           	pushl	%ebp
  49b6a0: 56                           	pushl	%esi
  49b6a1: 53                           	pushl	%ebx
  49b6a2: e8 69 39 fa ff               	calll	0x43f010 <.text+0x3e010>
  49b6a7: 83 c4 0c                     	addl	$0xc, %esp
  49b6aa: e9 55 07 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b6af: 55                           	pushl	%ebp
  49b6b0: 56                           	pushl	%esi
  49b6b1: 53                           	pushl	%ebx
  49b6b2: e8 d9 84 fa ff               	calll	0x443b90 <.text+0x42b90>
  49b6b7: 55                           	pushl	%ebp
  49b6b8: 56                           	pushl	%esi
  49b6b9: 53                           	pushl	%ebx
  49b6ba: e8 91 ca fa ff               	calll	0x448150 <.text+0x47150>
  49b6bf: 55                           	pushl	%ebp
  49b6c0: 56                           	pushl	%esi
  49b6c1: 53                           	pushl	%ebx
  49b6c2: e8 89 88 fa ff               	calll	0x443f50 <.text+0x42f50>
  49b6c7: 55                           	pushl	%ebp
  49b6c8: 56                           	pushl	%esi
  49b6c9: 53                           	pushl	%ebx
  49b6ca: e8 41 2f fa ff               	calll	0x43e610 <.text+0x3d610>
  49b6cf: 83 c4 30                     	addl	$0x30, %esp
  49b6d2: 83 f8 02                     	cmpl	$0x2, %eax
  49b6d5: 75 bd                        	jne	0x49b694 <.text+0x9a694>
  49b6d7: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b6dc: 84 c0                        	testb	%al, %al
  49b6de: 75 13                        	jne	0x49b6f3 <.text+0x9a6f3>
  49b6e0: 6a 00                        	pushl	$0x0
  49b6e2: 6a 00                        	pushl	$0x0
  49b6e4: 6a 00                        	pushl	$0x0
  49b6e6: 68 c6 04 00 00               	pushl	$0x4c6                  # imm = 0x4C6
  49b6eb: e8 80 86 f8 ff               	calll	0x423d70 <.text+0x22d70>
  49b6f0: 83 c4 10                     	addl	$0x10, %esp
  49b6f3: c6 03 32                     	movb	$0x32, (%ebx)
  49b6f6: 53                           	pushl	%ebx
  49b6f7: 66 c7 85 24 01 00 00 2e 00   	movw	$0x2e, 0x124(%ebp)
  49b700: e8 8b 50 fa ff               	calll	0x440790 <.text+0x3f790>
  49b705: 8b 55 64                     	movl	0x64(%ebp), %edx
  49b708: d9 42 1c                     	flds	0x1c(%edx)
  49b70b: 83 c4 04                     	addl	$0x4, %esp
  49b70e: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b714: 55                           	pushl	%ebp
  49b715: 56                           	pushl	%esi
  49b716: 53                           	pushl	%ebx
  49b717: d8 43 20                     	fadds	0x20(%ebx)
  49b71a: d9 5b 20                     	fstps	0x20(%ebx)
  49b71d: 8b 45 64                     	movl	0x64(%ebp), %eax
  49b720: d9 40 20                     	flds	0x20(%eax)
  49b723: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b729: d8 43 24                     	fadds	0x24(%ebx)
  49b72c: d9 5b 24                     	fstps	0x24(%ebx)
  49b72f: 8b 4d 64                     	movl	0x64(%ebp), %ecx
  49b732: d9 41 24                     	flds	0x24(%ecx)
  49b735: d8 0d fc cc 7d 00            	fmuls	0x7dccfc
  49b73b: d8 43 28                     	fadds	0x28(%ebx)
  49b73e: d9 5b 28                     	fstps	0x28(%ebx)
  49b741: 8b 95 b4 00 00 00            	movl	0xb4(%ebp), %edx
  49b747: d9 45 38                     	flds	0x38(%ebp)
  49b74a: 89 55 3c                     	movl	%edx, 0x3c(%ebp)
  49b74d: d8 0d 8c d5 7d 00            	fmuls	0x7dd58c
  49b753: d9 5d 38                     	fstps	0x38(%ebp)
  49b756: c7 43 14 00 00 00 00         	movl	$0x0, 0x14(%ebx)
  49b75d: c7 43 1c 00 c0 00 00         	movl	$0xc000, 0x1c(%ebx)     # imm = 0xC000
  49b764: e8 a7 38 fa ff               	calll	0x43f010 <.text+0x3e010>
  49b769: 83 c4 0c                     	addl	$0xc, %esp
  49b76c: e9 93 06 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b771: 55                           	pushl	%ebp
  49b772: 56                           	pushl	%esi
  49b773: 53                           	pushl	%ebx
  49b774: e8 d7 c9 fa ff               	calll	0x448150 <.text+0x47150>
  49b779: 55                           	pushl	%ebp
  49b77a: 56                           	pushl	%esi
  49b77b: 53                           	pushl	%ebx
  49b77c: e8 cf 87 fa ff               	calll	0x443f50 <.text+0x42f50>
  49b781: d9 46 08                     	flds	0x8(%esi)
  49b784: d8 43 24                     	fadds	0x24(%ebx)
  49b787: 55                           	pushl	%ebp
  49b788: 56                           	pushl	%esi
  49b789: 53                           	pushl	%ebx
  49b78a: d9 5b 24                     	fstps	0x24(%ebx)
  49b78d: e8 7e 38 fa ff               	calll	0x43f010 <.text+0x3e010>
  49b792: 83 c4 24                     	addl	$0x24, %esp
  49b795: e9 6a 06 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b79a: d9 43 24                     	flds	0x24(%ebx)
  49b79d: 8b 45 68                     	movl	0x68(%ebp), %eax
  49b7a0: d8 25 98 cc 7d 00            	fsubs	0x7dcc98
  49b7a6: 8b 40 20                     	movl	0x20(%eax), %eax
  49b7a9: d9 53 24                     	fsts	0x24(%ebx)
  49b7ac: 8b 48 38                     	movl	0x38(%eax), %ecx
  49b7af: 8b 49 0c                     	movl	0xc(%ecx), %ecx
  49b7b2: d9 41 0c                     	flds	0xc(%ecx)
  49b7b5: d8 40 24                     	fadds	0x24(%eax)
  49b7b8: d8 61 18                     	fsubs	0x18(%ecx)
  49b7bb: d9 54 24 14                  	fsts	0x14(%esp)
  49b7bf: d8 d9                        	fcomp	%st(1)
  49b7c1: df e0                        	fnstsw	%ax
  49b7c3: dd d8                        	fstp	%st(0)
  49b7c5: f6 c4 01                     	testb	$0x1, %ah
  49b7c8: 75 07                        	jne	0x49b7d1 <.text+0x9a7d1>
  49b7ca: 8b 54 24 14                  	movl	0x14(%esp), %edx
  49b7ce: 89 53 24                     	movl	%edx, 0x24(%ebx)
  49b7d1: 56                           	pushl	%esi
  49b7d2: 8b c5                        	movl	%ebp, %eax
  49b7d4: 8b fb                        	movl	%ebx, %edi
  49b7d6: e8 85 7c ff ff               	calll	0x493460 <.text+0x92460>
  49b7db: 83 c4 04                     	addl	$0x4, %esp
  49b7de: e9 21 06 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b7e3: 56                           	pushl	%esi
  49b7e4: 53                           	pushl	%ebx
  49b7e5: 8b c5                        	movl	%ebp, %eax
  49b7e7: e8 84 f3 fb ff               	calll	0x45ab70 <.text+0x59b70>
  49b7ec: 8b 53 18                     	movl	0x18(%ebx), %edx
  49b7ef: 03 d0                        	addl	%eax, %edx
  49b7f1: eb 0e                        	jmp	0x49b801 <.text+0x9a801>
  49b7f3: 56                           	pushl	%esi
  49b7f4: 53                           	pushl	%ebx
  49b7f5: 8b c5                        	movl	%ebp, %eax
  49b7f7: e8 74 f3 fb ff               	calll	0x45ab70 <.text+0x59b70>
  49b7fc: 8b 53 18                     	movl	0x18(%ebx), %edx
  49b7ff: 2b d0                        	subl	%eax, %edx
  49b801: 56                           	pushl	%esi
  49b802: 8b c5                        	movl	%ebp, %eax
  49b804: 8b fb                        	movl	%ebx, %edi
  49b806: 89 53 18                     	movl	%edx, 0x18(%ebx)
  49b809: e8 52 7c ff ff               	calll	0x493460 <.text+0x92460>
  49b80e: 83 c4 0c                     	addl	$0xc, %esp
  49b811: e9 ee 05 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b816: 8b 53 18                     	movl	0x18(%ebx), %edx
  49b819: 81 c2 00 03 00 00            	addl	$0x300, %edx            # imm = 0x300
  49b81f: 81 e2 00 fe 00 00            	andl	$0xfe00, %edx           # imm = 0xFE00
  49b825: 89 53 18                     	movl	%edx, 0x18(%ebx)
  49b828: 8b 44 24 10                  	movl	0x10(%esp), %eax
  49b82c: 50                           	pushl	%eax
  49b82d: 8b c5                        	movl	%ebp, %eax
  49b82f: 8b f3                        	movl	%ebx, %esi
  49b831: e8 7a 7c ff ff               	calll	0x4934b0 <.text+0x924b0>
  49b836: 83 c4 04                     	addl	$0x4, %esp
  49b839: e9 c2 05 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49b83e: 8b 4b 18                     	movl	0x18(%ebx), %ecx
  49b841: 8b 54 24 10                  	movl	0x10(%esp), %edx
  49b845: 81 e9 00 03 00 00            	subl	$0x300, %ecx            # imm = 0x300
  49b84b: 81 e1 00 fe 00 00            	andl	$0xfe00, %ecx           # imm = 0xFE00
  49b851: 52                           	pushl	%edx
  49b852: 8b c5                        	movl	%ebp, %eax
  49b854: 8b f3                        	movl	%ebx, %esi
  49b856: 89 4b 18                     	movl	%ecx, 0x18(%ebx)
  49b859: e8 52 7c ff ff               	calll	0x4934b0 <.text+0x924b0>
  49b85e: 83 c4 04                     	addl	$0x4, %esp
  49b861: e9 9a 05 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49b866: d9 43 24                     	flds	0x24(%ebx)
  49b869: d8 05 40 d5 7d 00            	fadds	0x7dd540
  49b86f: d9 5b 24                     	fstps	0x24(%ebx)
  49b872: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
  49b876: 51                           	pushl	%ecx
  49b877: 8b c5                        	movl	%ebp, %eax
  49b879: 8b f3                        	movl	%ebx, %esi
  49b87b: e8 30 7c ff ff               	calll	0x4934b0 <.text+0x924b0>
  49b880: 83 c4 04                     	addl	$0x4, %esp
  49b883: e9 78 05 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49b888: 6a 01                        	pushl	$0x1
  49b88a: e8 11 90 f9 ff               	calll	0x4348a0 <.text+0x338a0>
  49b88f: 55                           	pushl	%ebp
  49b890: 56                           	pushl	%esi
  49b891: 53                           	pushl	%ebx
  49b892: e8 39 82 fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b897: 55                           	pushl	%ebp
  49b898: 56                           	pushl	%esi
  49b899: 53                           	pushl	%ebx
  49b89a: e8 b1 7d fa ff               	calll	0x443650 <.text+0x42650>
  49b89f: 55                           	pushl	%ebp
  49b8a0: 56                           	pushl	%esi
  49b8a1: 53                           	pushl	%ebx
  49b8a2: e8 a9 86 fa ff               	calll	0x443f50 <.text+0x42f50>
  49b8a7: 55                           	pushl	%ebp
  49b8a8: 56                           	pushl	%esi
  49b8a9: 53                           	pushl	%ebx
  49b8aa: e8 41 15 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b8af: 55                           	pushl	%ebp
  49b8b0: 56                           	pushl	%esi
  49b8b1: 53                           	pushl	%ebx
  49b8b2: e8 b9 35 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b8b7: 83 c4 40                     	addl	$0x40, %esp
  49b8ba: e9 45 05 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b8bf: 68 29 5c bf 40               	pushl	$0x40bf5c29             # imm = 0x40BF5C29
  49b8c4: 55                           	pushl	%ebp
  49b8c5: 56                           	pushl	%esi
  49b8c6: 53                           	pushl	%ebx
  49b8c7: e8 84 cc fa ff               	calll	0x448550 <.text+0x47550>
  49b8cc: 55                           	pushl	%ebp
  49b8cd: 56                           	pushl	%esi
  49b8ce: 53                           	pushl	%ebx
  49b8cf: e8 7c 86 fa ff               	calll	0x443f50 <.text+0x42f50>
  49b8d4: 55                           	pushl	%ebp
  49b8d5: 56                           	pushl	%esi
  49b8d6: 53                           	pushl	%ebx
  49b8d7: e8 14 15 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b8dc: 83 c4 28                     	addl	$0x28, %esp
  49b8df: 83 f8 02                     	cmpl	$0x2, %eax
  49b8e2: 0f 85 9d 00 00 00            	jne	0x49b985 <.text+0x9a985>
  49b8e8: 8a 43 08                     	movb	0x8(%ebx), %al
  49b8eb: 84 c0                        	testb	%al, %al
  49b8ed: 75 1c                        	jne	0x49b90b <.text+0x9a90b>
  49b8ef: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b8f4: 84 c0                        	testb	%al, %al
  49b8f6: 75 13                        	jne	0x49b90b <.text+0x9a90b>
  49b8f8: 6a 00                        	pushl	$0x0
  49b8fa: 6a 00                        	pushl	$0x0
  49b8fc: 6a 00                        	pushl	$0x0
  49b8fe: 68 c6 04 00 00               	pushl	$0x4c6                  # imm = 0x4C6
  49b903: e8 68 84 f8 ff               	calll	0x423d70 <.text+0x22d70>
  49b908: 83 c4 10                     	addl	$0x10, %esp
  49b90b: c6 03 44                     	movb	$0x44, (%ebx)
  49b90e: 8b 55 64                     	movl	0x64(%ebp), %edx
  49b911: 83 c2 1c                     	addl	$0x1c, %edx
  49b914: 52                           	pushl	%edx
  49b915: 8d 45 44                     	leal	0x44(%ebp), %eax
  49b918: 50                           	pushl	%eax
  49b919: 8d 4e 04                     	leal	0x4(%esi), %ecx
  49b91c: 51                           	pushl	%ecx
  49b91d: e8 ee ca f9 ff               	calll	0x438410 <.text+0x37410>
  49b922: 33 d2                        	xorl	%edx, %edx
  49b924: 3d 00 80 00 00               	cmpl	$0x8000, %eax           # imm = 0x8000
  49b929: 0f 9f c2                     	setg	%dl
  49b92c: 53                           	pushl	%ebx
  49b92d: 83 c2 6d                     	addl	$0x6d, %edx
  49b930: 66 89 95 24 01 00 00         	movw	%dx, 0x124(%ebp)
  49b937: e8 54 4e fa ff               	calll	0x440790 <.text+0x3f790>
  49b93c: 0f bf 05 cc 2d b2 03         	movswl	0x3b22dcc, %eax
  49b943: 0f bf 0d ec 2d b2 03         	movswl	0x3b22dec, %ecx
  49b94a: c1 e0 08                     	shll	$0x8, %eax
  49b94d: 0b c1                        	orl	%ecx, %eax
  49b94f: 83 c4 10                     	addl	$0x10, %esp
  49b952: 3d 02 08 00 00               	cmpl	$0x802, %eax            # imm = 0x802
  49b957: 75 48                        	jne	0x49b9a1 <.text+0x9a9a1>
  49b959: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49b95e: 84 c0                        	testb	%al, %al
  49b960: 75 3f                        	jne	0x49b9a1 <.text+0x9a9a1>
  49b962: 6a 00                        	pushl	$0x0
  49b964: 6a 00                        	pushl	$0x0
  49b966: 6a 00                        	pushl	$0x0
  49b968: 68 eb 00 00 00               	pushl	$0xeb
  49b96d: e8 fe 83 f8 ff               	calll	0x423d70 <.text+0x22d70>
  49b972: 83 c4 10                     	addl	$0x10, %esp
  49b975: 55                           	pushl	%ebp
  49b976: 56                           	pushl	%esi
  49b977: 53                           	pushl	%ebx
  49b978: e8 b3 4f fa ff               	calll	0x440930 <.text+0x3f930>
  49b97d: 83 c4 0c                     	addl	$0xc, %esp
  49b980: e9 7f 04 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b985: 8b 7c 24 10                  	movl	0x10(%esp), %edi
  49b989: 55                           	pushl	%ebp
  49b98a: 57                           	pushl	%edi
  49b98b: 53                           	pushl	%ebx
  49b98c: e8 df 34 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b991: 83 c4 0c                     	addl	$0xc, %esp
  49b994: 8b f5                        	movl	%ebp, %esi
  49b996: 8b c7                        	movl	%edi, %eax
  49b998: 8b cb                        	movl	%ebx, %ecx
  49b99a: e8 d1 63 ff ff               	calll	0x491d70 <.text+0x90d70>
  49b99f: 8b f7                        	movl	%edi, %esi
  49b9a1: 55                           	pushl	%ebp
  49b9a2: 56                           	pushl	%esi
  49b9a3: 53                           	pushl	%ebx
  49b9a4: e8 87 4f fa ff               	calll	0x440930 <.text+0x3f930>
  49b9a9: 83 c4 0c                     	addl	$0xc, %esp
  49b9ac: e9 53 04 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b9b1: 55                           	pushl	%ebp
  49b9b2: 56                           	pushl	%esi
  49b9b3: 53                           	pushl	%ebx
  49b9b4: e8 27 73 fa ff               	calll	0x442ce0 <.text+0x41ce0>
  49b9b9: 55                           	pushl	%ebp
  49b9ba: 56                           	pushl	%esi
  49b9bb: 53                           	pushl	%ebx
  49b9bc: e8 8f 85 fa ff               	calll	0x443f50 <.text+0x42f50>
  49b9c1: 55                           	pushl	%ebp
  49b9c2: 56                           	pushl	%esi
  49b9c3: 53                           	pushl	%ebx
  49b9c4: e8 27 14 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49b9c9: 55                           	pushl	%ebp
  49b9ca: 56                           	pushl	%esi
  49b9cb: 53                           	pushl	%ebx
  49b9cc: e8 9f 34 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49b9d1: 55                           	pushl	%ebp
  49b9d2: 56                           	pushl	%esi
  49b9d3: 53                           	pushl	%ebx
  49b9d4: e8 57 4f fa ff               	calll	0x440930 <.text+0x3f930>
  49b9d9: 83 c4 3c                     	addl	$0x3c, %esp
  49b9dc: e9 23 04 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49b9e1: 55                           	pushl	%ebp
  49b9e2: 56                           	pushl	%esi
  49b9e3: 53                           	pushl	%ebx
  49b9e4: e8 e7 80 fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b9e9: 55                           	pushl	%ebp
  49b9ea: 56                           	pushl	%esi
  49b9eb: 53                           	pushl	%ebx
  49b9ec: e8 ff d0 fa ff               	calll	0x448af0 <.text+0x47af0>
  49b9f1: e9 dd 03 00 00               	jmp	0x49bdd3 <.text+0x9add3>
  49b9f6: 55                           	pushl	%ebp
  49b9f7: 56                           	pushl	%esi
  49b9f8: 53                           	pushl	%ebx
  49b9f9: e8 d2 80 fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49b9fe: 55                           	pushl	%ebp
  49b9ff: 56                           	pushl	%esi
  49ba00: 53                           	pushl	%ebx
  49ba01: e8 0a d3 fa ff               	calll	0x448d10 <.text+0x47d10>
  49ba06: 55                           	pushl	%ebp
  49ba07: 56                           	pushl	%esi
  49ba08: 53                           	pushl	%ebx
  49ba09: e8 42 85 fa ff               	calll	0x443f50 <.text+0x42f50>
  49ba0e: 55                           	pushl	%ebp
  49ba0f: 56                           	pushl	%esi
  49ba10: 53                           	pushl	%ebx
  49ba11: e8 da 13 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49ba16: 83 c4 30                     	addl	$0x30, %esp
  49ba19: 85 c0                        	testl	%eax, %eax
  49ba1b: 74 09                        	je	0x49ba26 <.text+0x9aa26>
  49ba1d: 66 c7 85 24 01 00 00 70 00   	movw	$0x70, 0x124(%ebp)
  49ba26: 55                           	pushl	%ebp
  49ba27: 56                           	pushl	%esi
  49ba28: 53                           	pushl	%ebx
  49ba29: e8 42 34 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49ba2e: 83 c4 0c                     	addl	$0xc, %esp
  49ba31: e9 ce 03 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49ba36: 8d 54 24 10                  	leal	0x10(%esp), %edx
  49ba3a: 52                           	pushl	%edx
  49ba3b: 6a 00                        	pushl	$0x0
  49ba3d: 53                           	pushl	%ebx
  49ba3e: e8 7d 03 fa ff               	calll	0x43bdc0 <.text+0x3adc0>
  49ba43: 83 c4 0c                     	addl	$0xc, %esp
  49ba46: 85 c0                        	testl	%eax, %eax
  49ba48: 0f 84 b7 00 00 00            	je	0x49bb05 <.text+0x9ab05>
  49ba4e: d9 44 24 10                  	flds	0x10(%esp)
  49ba52: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  49ba58: df e0                        	fnstsw	%ax
  49ba5a: f6 c4 44                     	testb	$0x44, %ah
  49ba5d: 0f 8b a2 00 00 00            	jnp	0x49bb05 <.text+0x9ab05>
  49ba63: e8 57 89 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49ba68: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  49ba6c: db 44 24 14                  	fildl	0x14(%esp)
  49ba70: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49ba76: d8 0d f0 07 7e 00            	fmuls	0x7e07f0
  49ba7c: e8 bf 88 1a 00               	calll	0x644340 <.text+0x243340>
  49ba81: 8b f8                        	movl	%eax, %edi
  49ba83: 8b cf                        	movl	%edi, %ecx
  49ba85: e8 06 c8 2e 00               	calll	0x788290 <.text+0x387290>
  49ba8a: d8 4c 24 10                  	fmuls	0x10(%esp)
  49ba8e: 8b cf                        	movl	%edi, %ecx
  49ba90: d9 5c 24 1c                  	fstps	0x1c(%esp)
  49ba94: e8 27 c9 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49ba99: d8 4c 24 10                  	fmuls	0x10(%esp)
  49ba9d: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  49baa1: 50                           	pushl	%eax
  49baa2: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  49baaa: d9 5c 24 28                  	fstps	0x28(%esp)
  49baae: e8 ed a2 f9 ff               	calll	0x435da0 <.text+0x34da0>
  49bab3: 83 c4 04                     	addl	$0x4, %esp
  49bab6: e8 04 89 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49babb: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  49babf: db 44 24 14                  	fildl	0x14(%esp)
  49bac3: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49bac9: d8 1d 5c ce 7d 00            	fcomps	0x7dce5c
  49bacf: df e0                        	fnstsw	%ax
  49bad1: f6 c4 41                     	testb	$0x41, %ah
  49bad4: 75 2f                        	jne	0x49bb05 <.text+0x9ab05>
  49bad6: d9 44 24 24                  	flds	0x24(%esp)
  49bada: 51                           	pushl	%ecx
  49badb: d8 0d 68 d5 7d 00            	fmuls	0x7dd568
  49bae1: 8b 4c 24 24                  	movl	0x24(%esp), %ecx
  49bae5: 33 d2                        	xorl	%edx, %edx
  49bae7: 8a 53 08                     	movb	0x8(%ebx), %dl
  49baea: d9 1c 24                     	fstps	(%esp)
  49baed: 51                           	pushl	%ecx
  49baee: d9 44 24 24                  	flds	0x24(%esp)
  49baf2: 51                           	pushl	%ecx
  49baf3: d8 0d 98 cc 7d 00            	fmuls	0x7dcc98
  49baf9: d9 1c 24                     	fstps	(%esp)
  49bafc: 52                           	pushl	%edx
  49bafd: e8 4e 5c fa ff               	calll	0x441750 <.text+0x40750>
  49bb02: 83 c4 10                     	addl	$0x10, %esp
  49bb05: 55                           	pushl	%ebp
  49bb06: 56                           	pushl	%esi
  49bb07: 53                           	pushl	%ebx
  49bb08: e8 93 7d fa ff               	calll	0x4438a0 <.text+0x428a0>
  49bb0d: 55                           	pushl	%ebp
  49bb0e: 56                           	pushl	%esi
  49bb0f: 53                           	pushl	%ebx
  49bb10: e8 db 12 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49bb15: 8a 43 04                     	movb	0x4(%ebx), %al
  49bb18: 83 c4 18                     	addl	$0x18, %esp
  49bb1b: a8 01                        	testb	$0x1, %al
  49bb1d: 74 07                        	je	0x49bb26 <.text+0x9ab26>
  49bb1f: c7 46 08 00 00 00 00         	movl	$0x0, 0x8(%esi)
  49bb26: 55                           	pushl	%ebp
  49bb27: 56                           	pushl	%esi
  49bb28: 53                           	pushl	%ebx
  49bb29: e8 42 33 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49bb2e: 83 c4 0c                     	addl	$0xc, %esp
  49bb31: d9 45 38                     	flds	0x38(%ebp)
  49bb34: d8 9d c4 00 00 00            	fcomps	0xc4(%ebp)
  49bb3a: df e0                        	fnstsw	%ax
  49bb3c: f6 c4 01                     	testb	$0x1, %ah
  49bb3f: 0f 85 bf 02 00 00            	jne	0x49be04 <.text+0x9ae04>
  49bb45: 53                           	pushl	%ebx
  49bb46: e8 05 85 ff ff               	calll	0x494050 <.text+0x93050>
  49bb4b: 8a 55 7c                     	movb	0x7c(%ebp), %dl
  49bb4e: 80 ca 01                     	orb	$0x1, %dl
  49bb51: 53                           	pushl	%ebx
  49bb52: 88 55 7c                     	movb	%dl, 0x7c(%ebp)
  49bb55: e8 56 85 ff ff               	calll	0x4940b0 <.text+0x930b0>
  49bb5a: 83 c4 08                     	addl	$0x8, %esp
  49bb5d: e9 a2 02 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49bb62: 8d 44 24 10                  	leal	0x10(%esp), %eax
  49bb66: 50                           	pushl	%eax
  49bb67: 6a 00                        	pushl	$0x0
  49bb69: 53                           	pushl	%ebx
  49bb6a: e8 51 02 fa ff               	calll	0x43bdc0 <.text+0x3adc0>
  49bb6f: 83 c4 0c                     	addl	$0xc, %esp
  49bb72: 85 c0                        	testl	%eax, %eax
  49bb74: 74 64                        	je	0x49bbda <.text+0x9abda>
  49bb76: d9 44 24 10                  	flds	0x10(%esp)
  49bb7a: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  49bb80: df e0                        	fnstsw	%ax
  49bb82: f6 c4 44                     	testb	$0x44, %ah
  49bb85: 7b 53                        	jnp	0x49bbda <.text+0x9abda>
  49bb87: e8 33 88 1a 00               	calll	0x6443bf <.text+0x2433bf>
  49bb8c: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  49bb90: db 44 24 14                  	fildl	0x14(%esp)
  49bb94: d8 0d 3c d5 7d 00            	fmuls	0x7dd53c
  49bb9a: d8 0d f0 07 7e 00            	fmuls	0x7e07f0
  49bba0: e8 9b 87 1a 00               	calll	0x644340 <.text+0x243340>
  49bba5: 8b f8                        	movl	%eax, %edi
  49bba7: 8b cf                        	movl	%edi, %ecx
  49bba9: e8 e2 c6 2e 00               	calll	0x788290 <.text+0x387290>
  49bbae: d8 4c 24 10                  	fmuls	0x10(%esp)
  49bbb2: 8b cf                        	movl	%edi, %ecx
  49bbb4: d9 5c 24 1c                  	fstps	0x1c(%esp)
  49bbb8: e8 03 c8 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49bbbd: d8 4c 24 10                  	fmuls	0x10(%esp)
  49bbc1: 8d 4c 24 1c                  	leal	0x1c(%esp), %ecx
  49bbc5: 51                           	pushl	%ecx
  49bbc6: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  49bbce: d9 5c 24 28                  	fstps	0x28(%esp)
  49bbd2: e8 c9 a1 f9 ff               	calll	0x435da0 <.text+0x34da0>
  49bbd7: 83 c4 04                     	addl	$0x4, %esp
  49bbda: 55                           	pushl	%ebp
  49bbdb: 56                           	pushl	%esi
  49bbdc: 53                           	pushl	%ebx
  49bbdd: e8 ee 7e fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49bbe2: 55                           	pushl	%ebp
  49bbe3: 56                           	pushl	%esi
  49bbe4: 53                           	pushl	%ebx
  49bbe5: e8 66 7a fa ff               	calll	0x443650 <.text+0x42650>
  49bbea: 55                           	pushl	%ebp
  49bbeb: 56                           	pushl	%esi
  49bbec: 53                           	pushl	%ebx
  49bbed: e8 5e 83 fa ff               	calll	0x443f50 <.text+0x42f50>
  49bbf2: 55                           	pushl	%ebp
  49bbf3: 56                           	pushl	%esi
  49bbf4: 53                           	pushl	%ebx
  49bbf5: e8 f6 11 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49bbfa: 55                           	pushl	%ebp
  49bbfb: 56                           	pushl	%esi
  49bbfc: 53                           	pushl	%ebx
  49bbfd: e8 6e 32 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49bc02: d9 45 38                     	flds	0x38(%ebp)
  49bc05: d8 9d c4 00 00 00            	fcomps	0xc4(%ebp)
  49bc0b: 83 c4 3c                     	addl	$0x3c, %esp
  49bc0e: df e0                        	fnstsw	%ax
  49bc10: f6 c4 01                     	testb	$0x1, %ah
  49bc13: 0f 85 eb 01 00 00            	jne	0x49be04 <.text+0x9ae04>
  49bc19: 53                           	pushl	%ebx
  49bc1a: e8 31 84 ff ff               	calll	0x494050 <.text+0x93050>
  49bc1f: 8a 55 7c                     	movb	0x7c(%ebp), %dl
  49bc22: 80 ca 01                     	orb	$0x1, %dl
  49bc25: 53                           	pushl	%ebx
  49bc26: 88 55 7c                     	movb	%dl, 0x7c(%ebp)
  49bc29: e8 82 84 ff ff               	calll	0x4940b0 <.text+0x930b0>
  49bc2e: 83 c4 08                     	addl	$0x8, %esp
  49bc31: e9 ce 01 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49bc36: 55                           	pushl	%ebp
  49bc37: 56                           	pushl	%esi
  49bc38: 53                           	pushl	%ebx
  49bc39: e8 32 06 fb ff               	calll	0x44c270 <.text+0x4b270>
  49bc3e: e9 48 f4 ff ff               	jmp	0x49b08b <.text+0x9a08b>
  49bc43: 55                           	pushl	%ebp
  49bc44: 56                           	pushl	%esi
  49bc45: 53                           	pushl	%ebx
  49bc46: e8 25 06 fb ff               	calll	0x44c270 <.text+0x4b270>
  49bc4b: 55                           	pushl	%ebp
  49bc4c: 56                           	pushl	%esi
  49bc4d: 53                           	pushl	%ebx
  49bc4e: e8 fd 82 fa ff               	calll	0x443f50 <.text+0x42f50>
  49bc53: 55                           	pushl	%ebp
  49bc54: 56                           	pushl	%esi
  49bc55: 53                           	pushl	%ebx
  49bc56: e8 95 11 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49bc5b: 83 c4 24                     	addl	$0x24, %esp
  49bc5e: 83 f8 02                     	cmpl	$0x2, %eax
  49bc61: 75 29                        	jne	0x49bc8c <.text+0x9ac8c>
  49bc63: c6 03 4b                     	movb	$0x4b, (%ebx)
  49bc66: 66 c7 85 24 01 00 00 86 00   	movw	$0x86, 0x124(%ebp)
  49bc6f: d9 45 38                     	flds	0x38(%ebp)
  49bc72: d8 0d a8 01 7e 00            	fmuls	0x7e01a8
  49bc78: d9 5d 38                     	fstps	0x38(%ebp)
  49bc7b: d9 45 40                     	flds	0x40(%ebp)
  49bc7e: d8 0d a8 01 7e 00            	fmuls	0x7e01a8
  49bc84: d9 5d 40                     	fstps	0x40(%ebp)
  49bc87: e9 78 01 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49bc8c: 55                           	pushl	%ebp
  49bc8d: 56                           	pushl	%esi
  49bc8e: 53                           	pushl	%ebx
  49bc8f: e8 dc 31 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49bc94: 53                           	pushl	%ebx
  49bc95: 8b cd                        	movl	%ebp, %ecx
  49bc97: 8b d6                        	movl	%esi, %edx
  49bc99: e8 82 5b ff ff               	calll	0x491820 <.text+0x90820>
  49bc9e: 83 c4 10                     	addl	$0x10, %esp
  49bca1: e9 5e 01 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49bca6: 55                           	pushl	%ebp
  49bca7: 56                           	pushl	%esi
  49bca8: 53                           	pushl	%ebx
  49bca9: e8 a2 6d fa ff               	calll	0x442a50 <.text+0x41a50>
  49bcae: 55                           	pushl	%ebp
  49bcaf: 56                           	pushl	%esi
  49bcb0: 53                           	pushl	%ebx
  49bcb1: e8 fa 84 fa ff               	calll	0x4441b0 <.text+0x431b0>
  49bcb6: 55                           	pushl	%ebp
  49bcb7: 56                           	pushl	%esi
  49bcb8: 53                           	pushl	%ebx
  49bcb9: e8 42 24 fa ff               	calll	0x43e100 <.text+0x3d100>
  49bcbe: 53                           	pushl	%ebx
  49bcbf: 8b cd                        	movl	%ebp, %ecx
  49bcc1: 8b d6                        	movl	%esi, %edx
  49bcc3: e8 58 5b ff ff               	calll	0x491820 <.text+0x90820>
  49bcc8: 53                           	pushl	%ebx
  49bcc9: e8 b2 83 ff ff               	calll	0x494080 <.text+0x93080>
  49bcce: 8a 45 7c                     	movb	0x7c(%ebp), %al
  49bcd1: 83 c4 2c                     	addl	$0x2c, %esp
  49bcd4: 0c 01                        	orb	$0x1, %al
  49bcd6: 88 45 7c                     	movb	%al, 0x7c(%ebp)
  49bcd9: e9 26 01 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49bcde: 55                           	pushl	%ebp
  49bcdf: 56                           	pushl	%esi
  49bce0: 53                           	pushl	%ebx
  49bce1: e8 ea 7d fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49bce6: 55                           	pushl	%ebp
  49bce7: 56                           	pushl	%esi
  49bce8: 53                           	pushl	%ebx
  49bce9: e8 d2 fc fa ff               	calll	0x44b9c0 <.text+0x4a9c0>
  49bcee: e9 e0 00 00 00               	jmp	0x49bdd3 <.text+0x9add3>
  49bcf3: 8b 74 24 10                  	movl	0x10(%esp), %esi
  49bcf7: 55                           	pushl	%ebp
  49bcf8: 56                           	pushl	%esi
  49bcf9: 53                           	pushl	%ebx
  49bcfa: e8 d1 7d fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49bcff: 56                           	pushl	%esi
  49bd00: 8b fd                        	movl	%ebp, %edi
  49bd02: 8b f3                        	movl	%ebx, %esi
  49bd04: e8 f7 65 ff ff               	calll	0x492300 <.text+0x91300>
  49bd09: 8b 74 24 20                  	movl	0x20(%esp), %esi
  49bd0d: 55                           	pushl	%ebp
  49bd0e: 56                           	pushl	%esi
  49bd0f: 53                           	pushl	%ebx
  49bd10: e8 db 10 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49bd15: 55                           	pushl	%ebp
  49bd16: 56                           	pushl	%esi
  49bd17: 53                           	pushl	%ebx
  49bd18: e8 53 31 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49bd1d: 53                           	pushl	%ebx
  49bd1e: e8 5d 83 ff ff               	calll	0x494080 <.text+0x93080>
  49bd23: 8a 4d 7c                     	movb	0x7c(%ebp), %cl
  49bd26: 68 20 fb 55 00               	pushl	$0x55fb20               # imm = 0x55FB20
  49bd2b: 80 c9 01                     	orb	$0x1, %cl
  49bd2e: 6a 05                        	pushl	$0x5
  49bd30: 6a 02                        	pushl	$0x2
  49bd32: 88 4d 7c                     	movb	%cl, 0x7c(%ebp)
  49bd35: e8 26 fb f6 ff               	calll	0x40b860 <.text+0xa860>
  49bd3a: 83 c4 38                     	addl	$0x38, %esp
  49bd3d: 85 c0                        	testl	%eax, %eax
  49bd3f: 0f 84 bb 00 00 00            	je	0x49be00 <.text+0x9ae00>
  49bd45: 8b 50 20                     	movl	0x20(%eax), %edx
  49bd48: 8a 43 08                     	movb	0x8(%ebx), %al
  49bd4b: 88 42 08                     	movb	%al, 0x8(%edx)
  49bd4e: e9 ad 00 00 00               	jmp	0x49be00 <.text+0x9ae00>
  49bd53: 53                           	pushl	%ebx
  49bd54: e8 47 c7 f7 ff               	calll	0x4184a0 <.text+0x174a0>
  49bd59: 83 c4 04                     	addl	$0x4, %esp
  49bd5c: e9 a3 00 00 00               	jmp	0x49be04 <.text+0x9ae04>
  49bd61: f6 85 2e 01 00 00 01         	testb	$0x1, 0x12e(%ebp)
  49bd68: 74 41                        	je	0x49bdab <.text+0x9adab>
  49bd6a: 8b 4b 38                     	movl	0x38(%ebx), %ecx
  49bd6d: d9 85 ec 01 00 00            	flds	0x1ec(%ebp)
  49bd73: 8b 49 0c                     	movl	0xc(%ecx), %ecx
  49bd76: d8 59 0c                     	fcomps	0xc(%ecx)
  49bd79: df e0                        	fnstsw	%ax
  49bd7b: f6 c4 41                     	testb	$0x41, %ah
  49bd7e: 75 2b                        	jne	0x49bdab <.text+0x9adab>
  49bd80: f6 85 e4 01 00 00 02         	testb	$0x2, 0x1e4(%ebp)
  49bd87: 74 22                        	je	0x49bdab <.text+0x9adab>
  49bd89: d9 85 ec 01 00 00            	flds	0x1ec(%ebp)
  49bd8f: d8 61 0c                     	fsubs	0xc(%ecx)
  49bd92: d9 55 3c                     	fsts	0x3c(%ebp)
  49bd95: d8 1d 50 d5 7d 00            	fcomps	0x7dd550
  49bd9b: df e0                        	fnstsw	%ax
  49bd9d: f6 c4 41                     	testb	$0x41, %ah
  49bda0: 75 21                        	jne	0x49bdc3 <.text+0x9adc3>
  49bda2: c7 45 3c cd cc cc 3e         	movl	$0x3ecccccd, 0x3c(%ebp) # imm = 0x3ECCCCCD
  49bda9: eb 18                        	jmp	0x49bdc3 <.text+0x9adc3>
  49bdab: c7 45 3c 00 00 00 00         	movl	$0x0, 0x3c(%ebp)
  49bdb2: eb 0f                        	jmp	0x49bdc3 <.text+0x9adc3>
  49bdb4: f6 43 04 03                  	testb	$0x3, 0x4(%ebx)
  49bdb8: 74 09                        	je	0x49bdc3 <.text+0x9adc3>
  49bdba: 66 c7 85 24 01 00 00 57 00   	movw	$0x57, 0x124(%ebp)
  49bdc3: 55                           	pushl	%ebp
  49bdc4: 56                           	pushl	%esi
  49bdc5: 53                           	pushl	%ebx
  49bdc6: e8 05 7d fa ff               	calll	0x443ad0 <.text+0x42ad0>
  49bdcb: 55                           	pushl	%ebp
  49bdcc: 56                           	pushl	%esi
  49bdcd: 53                           	pushl	%ebx
  49bdce: e8 7d d0 fa ff               	calll	0x448e50 <.text+0x47e50>
  49bdd3: 55                           	pushl	%ebp
  49bdd4: 56                           	pushl	%esi
  49bdd5: 53                           	pushl	%ebx
  49bdd6: e8 75 81 fa ff               	calll	0x443f50 <.text+0x42f50>
  49bddb: 55                           	pushl	%ebp
  49bddc: 56                           	pushl	%esi
  49bddd: 53                           	pushl	%ebx
  49bdde: e8 0d 10 fb ff               	calll	0x44cdf0 <.text+0x4bdf0>
  49bde3: 55                           	pushl	%ebp
  49bde4: 56                           	pushl	%esi
  49bde5: 53                           	pushl	%ebx
  49bde6: e8 85 30 fa ff               	calll	0x43ee70 <.text+0x3de70>
  49bdeb: 83 c4 3c                     	addl	$0x3c, %esp
  49bdee: eb 14                        	jmp	0x49be04 <.text+0x9ae04>
  49bdf0: 8b 54 24 10                  	movl	0x10(%esp), %edx
  49bdf4: 55                           	pushl	%ebp
  49bdf5: 52                           	pushl	%edx
  49bdf6: 8b f3                        	movl	%ebx, %esi
  49bdf8: e8 73 63 ff ff               	calll	0x492170 <.text+0x91170>
  49bdfd: 83 c4 08                     	addl	$0x8, %esp
  49be00: 8b 74 24 10                  	movl	0x10(%esp), %esi
  49be04: 8d 85 20 01 00 00            	leal	0x120(%ebp), %eax
  49be0a: 50                           	pushl	%eax
  49be0b: e8 f0 e9 fa ff               	calll	0x44a800 <.text+0x49800>
  49be10: 33 c9                        	xorl	%ecx, %ecx
  49be12: 66 8b 4b 04                  	movw	0x4(%ebx), %cx
  49be16: 83 c4 04                     	addl	$0x4, %esp
  49be19: 81 e1 00 01 00 00            	andl	$0x100, %ecx            # imm = 0x100
  49be1f: 66 85 c9                     	testw	%cx, %cx
  49be22: 75 6e                        	jne	0x49be92 <.text+0x9ae92>
  49be24: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49be29: 84 c0                        	testb	%al, %al
  49be2b: 75 5a                        	jne	0x49be87 <.text+0x9ae87>
  49be2d: f6 45 04 02                  	testb	$0x2, 0x4(%ebp)
  49be31: a1 80 6e 8f 03               	movl	0x38f6e80, %eax
  49be36: 74 19                        	je	0x49be51 <.text+0x9ae51>
  49be38: 8b 50 4c                     	movl	0x4c(%eax), %edx
  49be3b: 8b 80 e8 00 00 00            	movl	0xe8(%eax), %eax
  49be41: 89 42 30                     	movl	%eax, 0x30(%edx)
  49be44: a1 80 6e 8f 03               	movl	0x38f6e80, %eax
  49be49: 8b 90 f0 00 00 00            	movl	0xf0(%eax), %edx
  49be4f: eb 11                        	jmp	0x49be62 <.text+0x9ae62>
  49be51: 8b 50 50                     	movl	0x50(%eax), %edx
  49be54: 8b 48 4c                     	movl	0x4c(%eax), %ecx
  49be57: 89 51 30                     	movl	%edx, 0x30(%ecx)
  49be5a: a1 80 6e 8f 03               	movl	0x38f6e80, %eax
  49be5f: 8b 50 3c                     	movl	0x3c(%eax), %edx
  49be62: 8b 48 38                     	movl	0x38(%eax), %ecx
  49be65: 89 51 30                     	movl	%edx, 0x30(%ecx)
  49be68: f6 45 04 01                  	testb	$0x1, 0x4(%ebp)
  49be6c: a1 80 6e 8f 03               	movl	0x38f6e80, %eax
  49be71: 8b 88 f8 00 00 00            	movl	0xf8(%eax), %ecx
  49be77: 74 08                        	je	0x49be81 <.text+0x9ae81>
  49be79: 8b 90 fc 00 00 00            	movl	0xfc(%eax), %edx
  49be7f: eb 03                        	jmp	0x49be84 <.text+0x9ae84>
  49be81: 8b 50 0c                     	movl	0xc(%eax), %edx
  49be84: 89 51 30                     	movl	%edx, 0x30(%ecx)
  49be87: 55                           	pushl	%ebp
  49be88: 56                           	pushl	%esi
  49be89: 53                           	pushl	%ebx
  49be8a: e8 01 3c fa ff               	calll	0x43fa90 <.text+0x3ea90>
  49be8f: 83 c4 0c                     	addl	$0xc, %esp
  49be92: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49be97: 84 c0                        	testb	%al, %al
  49be99: 74 5a                        	je	0x49bef5 <.text+0x9aef5>
  49be9b: 66 81 bd 24 01 00 00 92 00   	cmpw	$0x92, 0x124(%ebp)
  49bea4: 75 32                        	jne	0x49bed8 <.text+0x9aed8>
  49bea6: 33 c0                        	xorl	%eax, %eax
  49bea8: 89 85 68 01 00 00            	movl	%eax, 0x168(%ebp)
  49beae: 89 85 74 01 00 00            	movl	%eax, 0x174(%ebp)
  49beb4: 8a 0d 44 5d c5 03            	movb	0x3c55d44, %cl
  49beba: 84 c9                        	testb	%cl, %cl
  49bebc: 75 37                        	jne	0x49bef5 <.text+0x9aef5>
  49bebe: 50                           	pushl	%eax
  49bebf: 6a 01                        	pushl	$0x1
  49bec1: 53                           	pushl	%ebx
  49bec2: 68 09 03 00 00               	pushl	$0x309                  # imm = 0x309
  49bec7: e8 54 7f f8 ff               	calll	0x423e20 <.text+0x22e20>
  49becc: 83 c4 10                     	addl	$0x10, %esp
  49becf: c6 05 44 5d c5 03 01         	movb	$0x1, 0x3c55d44
  49bed6: eb 1d                        	jmp	0x49bef5 <.text+0x9aef5>
  49bed8: 80 3d 44 5d c5 03 01         	cmpb	$0x1, 0x3c55d44
  49bedf: 75 0d                        	jne	0x49beee <.text+0x9aeee>
  49bee1: 68 09 03 00 00               	pushl	$0x309                  # imm = 0x309
  49bee6: e8 25 83 f8 ff               	calll	0x424210 <.text+0x23210>
  49beeb: 83 c4 04                     	addl	$0x4, %esp
  49beee: c6 05 44 5d c5 03 00         	movb	$0x0, 0x3c55d44
  49bef5: 56                           	pushl	%esi
  49bef6: 8b cd                        	movl	%ebp, %ecx
  49bef8: 8b fb                        	movl	%ebx, %edi
  49befa: e8 f1 56 ff ff               	calll	0x4915f0 <.text+0x905f0>
  49beff: 8b 44 24 30                  	movl	0x30(%esp), %eax
  49bf03: 50                           	pushl	%eax
  49bf04: e8 b7 89 ff ff               	calll	0x4948c0 <.text+0x938c0>
  49bf09: 8b 45 68                     	movl	0x68(%ebp), %eax
  49bf0c: 83 c4 08                     	addl	$0x8, %esp
  49bf0f: 85 c0                        	testl	%eax, %eax
  49bf11: 5f                           	popl	%edi
  49bf12: 74 0d                        	je	0x49bf21 <.text+0x9af21>
  49bf14: 56                           	pushl	%esi
  49bf15: 8b c5                        	movl	%ebp, %eax
  49bf17: 8b cb                        	movl	%ebx, %ecx
  49bf19: e8 c2 96 ff ff               	calll	0x4955e0 <.text+0x945e0>
  49bf1e: 83 c4 04                     	addl	$0x4, %esp
  49bf21: d0 65 7c                     	shlb	0x7c(%ebp)
  49bf24: 8d 8d dc 01 00 00            	leal	0x1dc(%ebp), %ecx
  49bf2a: 51                           	pushl	%ecx
  49bf2b: 53                           	pushl	%ebx
  49bf2c: e8 6f 32 00 00               	calll	0x49f1a0 <.text+0x9e1a0>
  49bf31: 66 8b 85 26 01 00 00         	movw	0x126(%ebp), %ax
  49bf38: 83 c4 08                     	addl	$0x8, %esp
  49bf3b: 66 3d 4b 00                  	cmpw	$0x4b, %ax
  49bf3f: 74 49                        	je	0x49bf8a <.text+0x9af8a>
  49bf41: 66 3d 8f 00                  	cmpw	$0x8f, %ax
  49bf45: 74 43                        	je	0x49bf8a <.text+0x9af8a>
  49bf47: 8a 03                        	movb	(%ebx), %al
  49bf49: 3c 37                        	cmpb	$0x37, %al
  49bf4b: 7c 04                        	jl	0x49bf51 <.text+0x9af51>
  49bf4d: 3c 3c                        	cmpb	$0x3c, %al
  49bf4f: 7e 39                        	jle	0x49bf8a <.text+0x9af8a>
  49bf51: 53                           	pushl	%ebx
  49bf52: e8 29 03 f8 ff               	calll	0x41c280 <.text+0x1b280>
  49bf57: 83 c4 04                     	addl	$0x4, %esp
  49bf5a: 8d 4d 38                     	leal	0x38(%ebp), %ecx
  49bf5d: e8 5e bd 2e 00               	calll	0x787cc0 <.text+0x386cc0>
  49bf62: d8 85 10 01 00 00            	fadds	0x110(%ebp)
  49bf68: 8b 53 38                     	movl	0x38(%ebx), %edx
  49bf6b: 51                           	pushl	%ecx
  49bf6c: d9 1c 24                     	fstps	(%esp)
  49bf6f: 52                           	pushl	%edx
  49bf70: e8 bb 05 f8 ff               	calll	0x41c530 <.text+0x1b530>
  49bf75: 83 c4 08                     	addl	$0x8, %esp
  49bf78: 55                           	pushl	%ebp
  49bf79: 56                           	pushl	%esi
  49bf7a: 53                           	pushl	%ebx
  49bf7b: e8 90 af fa ff               	calll	0x446f10 <.text+0x45f10>
  49bf80: 83 c4 0c                     	addl	$0xc, %esp
  49bf83: 5e                           	popl	%esi
  49bf84: 5d                           	popl	%ebp
  49bf85: 5b                           	popl	%ebx
  49bf86: 83 c4 18                     	addl	$0x18, %esp
  49bf89: c3                           	retl
  49bf8a: 8b 43 38                     	movl	0x38(%ebx), %eax
  49bf8d: 66 c7 80 a2 00 00 00 ff ff   	movw	$0xffff, 0xa2(%eax)     # imm = 0xFFFF
  49bf96: 8b 4b 38                     	movl	0x38(%ebx), %ecx
  49bf99: 55                           	pushl	%ebp
  49bf9a: c7 81 a4 00 00 00 00 00 00 00	movl	$0x0, 0xa4(%ecx)
  49bfa4: 8b 43 38                     	movl	0x38(%ebx), %eax
  49bfa7: 80 60 04 e0                  	andb	$-0x20, 0x4(%eax)
  49bfab: 56                           	pushl	%esi
  49bfac: 53                           	pushl	%ebx
  49bfad: e8 5e af fa ff               	calll	0x446f10 <.text+0x45f10>
  49bfb2: 83 c4 0c                     	addl	$0xc, %esp
  49bfb5: 5e                           	popl	%esi
  49bfb6: 5d                           	popl	%ebp
  49bfb7: 5b                           	popl	%ebx
  49bfb8: 83 c4 18                     	addl	$0x18, %esp
  49bfbb: c3                           	retl
  49bfbc: 30 ab 49 00 2d ad            	xorb	%ch, -0x52d2ffb7(%ebx)
  49bfc2: 49                           	decl	%ecx
  49bfc3: 00 69 ad                     	addb	%ch, -0x53(%ecx)
  49bfc6: 49                           	decl	%ecx
  49bfc7: 00 04 ae                     	addb	%al, (%esi,%ebp,4)
  49bfca: 49                           	decl	%ecx
  49bfcb: 00 2f                        	addb	%ch, (%edi)
  49bfcd: ae                           	scasb	%es:(%edi), %al
  49bfce: 49                           	decl	%ecx
  49bfcf: 00 bb ae 49 00 c8            	addb	%bh, -0x37ffb652(%ebx)
  49bfd5: ae                           	scasb	%es:(%edi), %al
  49bfd6: 49                           	decl	%ecx
  49bfd7: 00 4b af                     	addb	%cl, -0x51(%ebx)
  49bfda: 49                           	decl	%ecx
  49bfdb: 00 24 b3                     	addb	%ah, (%ebx,%esi,4)
  49bfde: 49                           	decl	%ecx
  49bfdf: 00 71 af                     	addb	%dh, -0x51(%ecx)
  49bfe2: 49                           	decl	%ecx
  49bfe3: 00 7e af                     	addb	%bh, -0x51(%esi)
  49bfe6: 49                           	decl	%ecx
  49bfe7: 00 9f af 49 00 24            	addb	%bl, 0x240049af(%edi)
  49bfed: b3 49                        	movb	$0x49, %bl
  49bfef: 00 83 b0 49 00 ac            	addb	%al, -0x53ffb650(%ebx)
  49bff5: af                           	scasl	%es:(%edi), %eax
  49bff6: 49                           	decl	%ecx
  49bff7: 00 f3                        	addb	%dh, %bl
  49bff9: af                           	scasl	%es:(%edi), %eax
  49bffa: 49                           	decl	%ecx
  49bffb: 00 72 b0                     	addb	%dh, -0x50(%edx)
  49bffe: 49                           	decl	%ecx
  49bfff: 00 83 b0 49 00 83            	addb	%al, -0x7cffb650(%ebx)
  49c005: b0 49                        	movb	$0x49, %al
  49c007: 00 ab b0 49 00 04            	addb	%ch, 0x40049b0(%ebx)
  49c00d: be 49 00 1e b1               	movl	$0xb11e0049, %esi       # imm = 0xB11E0049
  49c012: 49                           	decl	%ecx
  49c013: 00 1e                        	addb	%bl, (%esi)
  49c015: b1 49                        	movb	$0x49, %cl
  49c017: 00 4d b2                     	addb	%cl, -0x4e(%ebp)
  49c01a: 49                           	decl	%ecx
  49c01b: 00 62 b2                     	addb	%ah, -0x4e(%edx)
  49c01e: 49                           	decl	%ecx
  49c01f: 00 8c b2 49 00 77 b2         	addb	%cl, -0x4d88ffb7(%edx,%esi,4)
  49c026: 49                           	decl	%ecx
  49c027: 00 83 b0 49 00 e2            	addb	%al, -0x1dffb650(%ebx)
  49c02d: b2 49                        	movb	$0x49, %dl
  49c02f: 00 24 b3                     	addb	%ah, (%ebx,%esi,4)
  49c032: 49                           	decl	%ecx
  49c033: 00 71 af                     	addb	%dh, -0x51(%ecx)
  49c036: 49                           	decl	%ecx
  49c037: 00 7e af                     	addb	%bh, -0x51(%esi)
  49c03a: 49                           	decl	%ecx
  49c03b: 00 66 b3                     	addb	%ah, -0x4d(%esi)
  49c03e: 49                           	decl	%ecx
  49c03f: 00 be b3 49 00 f6            	addb	%bh, -0x9ffb64d(%esi)
  49c045: b3 49                        	movb	$0x49, %bl
  49c047: 00 06                        	addb	%al, (%esi)
  49c049: b4 49                        	movb	$0x49, %ah
  49c04b: 00 c0                        	addb	%al, %al
  49c04d: b0 49                        	movb	$0x49, %al
  49c04f: 00 c0                        	addb	%al, %al
  49c051: b0 49                        	movb	$0x49, %al
  49c053: 00 de                        	addb	%bl, %dh
  49c055: b0 49                        	movb	$0x49, %al
  49c057: 00 de                        	addb	%bl, %dh
  49c059: b0 49                        	movb	$0x49, %al
  49c05b: 00 fc                        	addb	%bh, %ah
  49c05d: b0 49                        	movb	$0x49, %al
  49c05f: 00 04 be                     	addb	%al, (%esi,%edi,4)
  49c062: 49                           	decl	%ecx
  49c063: 00 1e                        	addb	%bl, (%esi)
  49c065: b1 49                        	movb	$0x49, %cl
  49c067: 00 2b                        	addb	%ch, (%ebx)
  49c069: b1 49                        	movb	$0x49, %cl
  49c06b: 00 00                        	addb	%al, (%eax)
  49c06d: b2 49                        	movb	$0x49, %dl
  49c06f: 00 04 be                     	addb	%al, (%esi,%edi,4)
  49c072: 49                           	decl	%ecx
  49c073: 00 38                        	addb	%bh, (%eax)
  49c075: b4 49                        	movb	$0x49, %ah
  49c077: 00 06                        	addb	%al, (%esi)
  49c079: b5 49                        	movb	$0x49, %ch
  49c07b: 00 af b6 49 00 ca            	addb	%ch, -0x35ffb64a(%edi)
  49c081: b5 49                        	movb	$0x49, %ch
  49c083: 00 71 b7                     	addb	%dh, -0x49(%ecx)
  49c086: 49                           	decl	%ecx
  49c087: 00 d1                        	addb	%dl, %cl
  49c089: b7 49                        	movb	$0x49, %bh
  49c08b: 00 9a b7 49 00 e3            	addb	%bl, -0x1cffb649(%edx)
  49c091: b7 49                        	movb	$0x49, %bh
  49c093: 00 f3                        	addb	%dh, %bl
  49c095: b7 49                        	movb	$0x49, %bh
  49c097: 00 28                        	addb	%ch, (%eax)
  49c099: b8 49 00 72 b8               	movl	$0xb8720049, %eax       # imm = 0xB8720049
  49c09e: 49                           	decl	%ecx
  49c09f: 00 16                        	addb	%dl, (%esi)
  49c0a1: b8 49 00 3e b8               	movl	$0xb83e0049, %eax       # imm = 0xB83E0049
  49c0a6: 49                           	decl	%ecx
  49c0a7: 00 28                        	addb	%ch, (%eax)
  49c0a9: b8 49 00 66 b8               	movl	$0xb8660049, %eax       # imm = 0xB8660049
  49c0ae: 49                           	decl	%ecx
  49c0af: 00 88 b8 49 00 bf            	addb	%cl, -0x40ffb648(%eax)
  49c0b5: b8 49 00 04 be               	movl	$0xbe040049, %eax       # imm = 0xBE040049
  49c0ba: 49                           	decl	%ecx
  49c0bb: 00 b1 b9 49 00 e1            	addb	%dh, -0x1effb647(%ecx)
  49c0c1: b9 49 00 e1 b9               	movl	$0xb9e10049, %ecx       # imm = 0xB9E10049
  49c0c6: 49                           	decl	%ecx
  49c0c7: 00 f6                        	addb	%dh, %dh
  49c0c9: b9 49 00 b1 b9               	movl	$0xb9b10049, %ecx       # imm = 0xB9B10049
  49c0ce: 49                           	decl	%ecx
  49c0cf: 00 36                        	addb	%dh, (%esi)
  49c0d1: ba 49 00 62 bb               	movl	$0xbb620049, %edx       # imm = 0xBB620049
  49c0d6: 49                           	decl	%ecx
  49c0d7: 00 36                        	addb	%dh, (%esi)
  49c0d9: bc 49 00 4d b2               	movl	$0xb24d0049, %esp       # imm = 0xB24D0049
  49c0de: 49                           	decl	%ecx
  49c0df: 00 4d b2                     	addb	%cl, -0x4e(%ebp)
  49c0e2: 49                           	decl	%ecx
  49c0e3: 00 4d b2                     	addb	%cl, -0x4e(%ebp)
  49c0e6: 49                           	decl	%ecx
  49c0e7: 00 4d b2                     	addb	%cl, -0x4e(%ebp)
  49c0ea: 49                           	decl	%ecx
  49c0eb: 00 43 bc                     	addb	%al, -0x44(%ebx)
  49c0ee: 49                           	decl	%ecx
  49c0ef: 00 a6 bc 49 00 de            	addb	%ah, -0x21ffb644(%esi)
  49c0f5: bc 49 00 7e af               	movl	$0xaf7e0049, %esp       # imm = 0xAF7E0049
  49c0fa: 49                           	decl	%ecx
  49c0fb: 00 71 af                     	addb	%dh, -0x51(%ecx)
  49c0fe: 49                           	decl	%ecx
  49c0ff: 00 f3                        	addb	%dh, %bl
  49c101: bc 49 00 de bc               	movl	$0xbcde0049, %esp       # imm = 0xBCDE0049
  49c106: 49                           	decl	%ecx
  49c107: 00 83 b0 49 00 53            	addb	%al, 0x530049b0(%ebx)
  49c10d: bd 49 00 61 bd               	movl	$0xbd610049, %ebp       # imm = 0xBD610049
  49c112: 49                           	decl	%ecx
  49c113: 00 b4 bd 49 00 f0 bd         	addb	%dh, -0x420fffb7(%ebp,%edi,4)
  49c11a: 49                           	decl	%ecx
  49c11b: 00 90 90 90 90 56            	addb	%dl, 0x56909090(%eax)
