
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  781e20: 53                           	pushl	%ebx
  781e21: 56                           	pushl	%esi
  781e22: 57                           	pushl	%edi
  781e23: 8b fa                        	movl	%edx, %edi
  781e25: 8b f1                        	movl	%ecx, %esi
  781e27: 8b 1e                        	movl	(%esi), %ebx
  781e29: 8b c3                        	movl	%ebx, %eax
  781e2b: 83 e0 07                     	andl	$0x7, %eax
  781e2e: 3c 07                        	cmpb	$0x7, %al
  781e30: 75 1f                        	jne	0x781e51 <.text+0x380e51>
  781e32: f6 c3 08                     	testb	$0x8, %bl
  781e35: 75 09                        	jne	0x781e40 <.text+0x380e40>
  781e37: 8b 4e 04                     	movl	0x4(%esi), %ecx
  781e3a: 51                           	pushl	%ecx
  781e3b: ff d7                        	calll	*%edi
  781e3d: 83 c4 04                     	addl	$0x4, %esp
  781e40: f6 c3 10                     	testb	$0x10, %bl
  781e43: 75 5f                        	jne	0x781ea4 <.text+0x380ea4>
  781e45: 8b 4e 2c                     	movl	0x2c(%esi), %ecx
  781e48: 8b d7                        	movl	%edi, %edx
  781e4a: e8 d1 ff ff ff               	calll	0x781e20 <.text+0x380e20>
  781e4f: eb 53                        	jmp	0x781ea4 <.text+0x380ea4>
  781e51: e8 2a fe ff ff               	calll	0x781c80 <.text+0x380c80>
  781e56: f6 c3 01                     	testb	$0x1, %bl
  781e59: 75 08                        	jne	0x781e63 <.text+0x380e63>
  781e5b: 8d 4e 08                     	leal	0x8(%esi), %ecx
  781e5e: e8 ad fd ff ff               	calll	0x781c10 <.text+0x380c10>
  781e63: f6 c3 02                     	testb	$0x2, %bl
  781e66: 75 0d                        	jne	0x781e75 <.text+0x380e75>
  781e68: 8b d3                        	movl	%ebx, %edx
  781e6a: 83 e2 20                     	andl	$0x20, %edx
  781e6d: 8d 4e 14                     	leal	0x14(%esi), %ecx
  781e70: e8 2b 13 01 00               	calll	0x7931a0 <.text+0x3921a0>
  781e75: f6 c3 04                     	testb	$0x4, %bl
  781e78: 75 08                        	jne	0x781e82 <.text+0x380e82>
  781e7a: 8d 4e 20                     	leal	0x20(%esi), %ecx
  781e7d: e8 7e f8 ff ff               	calll	0x781700 <.text+0x380700>
  781e82: f6 c3 08                     	testb	$0x8, %bl
  781e85: 75 09                        	jne	0x781e90 <.text+0x380e90>
  781e87: 8b 56 04                     	movl	0x4(%esi), %edx
  781e8a: 52                           	pushl	%edx
  781e8b: ff d7                        	calll	*%edi
  781e8d: 83 c4 04                     	addl	$0x4, %esp
  781e90: f6 c3 10                     	testb	$0x10, %bl
  781e93: 75 0a                        	jne	0x781e9f <.text+0x380e9f>
  781e95: 8b 4e 2c                     	movl	0x2c(%esi), %ecx
  781e98: 8b d7                        	movl	%edi, %edx
  781e9a: e8 81 ff ff ff               	calll	0x781e20 <.text+0x380e20>
  781e9f: e8 3c f8 ff ff               	calll	0x7816e0 <.text+0x3806e0>
  781ea4: 8b 76 30                     	movl	0x30(%esi), %esi
  781ea7: 85 f6                        	testl	%esi, %esi
  781ea9: 0f 85 78 ff ff ff            	jne	0x781e27 <.text+0x380e27>
  781eaf: 5f                           	popl	%edi
  781eb0: 5e                           	popl	%esi
  781eb1: 5b                           	popl	%ebx
  781eb2: c3                           	retl
  781eb3: cc                           	int3
  781eb4: cc                           	int3
  781eb5: cc                           	int3
  781eb6: cc                           	int3
  781eb7: cc                           	int3
  781eb8: cc                           	int3
  781eb9: cc                           	int3
  781eba: cc                           	int3
  781ebb: cc                           	int3
  781ebc: cc                           	int3
  781ebd: cc                           	int3
  781ebe: cc                           	int3
  781ebf: cc                           	int3
  781ec0: 83 ec 14                     	subl	$0x14, %esp
  781ec3: 0f b6 41 03                  	movzbl	0x3(%ecx), %eax
  781ec7: 0f b6 51 02                  	movzbl	0x2(%ecx), %edx
  781ecb: 89 04 24                     	movl	%eax, (%esp)
  781ece: 0f b6 41 01                  	movzbl	0x1(%ecx), %eax
  781ed2: db 04 24                     	fildl	(%esp)
  781ed5: 0f b6 09                     	movzbl	(%ecx), %ecx
  781ed8: d8 0d c4 b2 7d 00            	fmuls	0x7db2c4
  781ede: 89 14 24                     	movl	%edx, (%esp)
  781ee1: db 04 24                     	fildl	(%esp)
  781ee4: 89 04 24                     	movl	%eax, (%esp)
  781ee7: d8 0d c4 b2 7d 00            	fmuls	0x7db2c4
  781eed: d9 5c 24 08                  	fstps	0x8(%esp)
  781ef1: db 04 24                     	fildl	(%esp)
  781ef4: 89 0c 24                     	movl	%ecx, (%esp)
  781ef7: d8 0d c4 b2 7d 00            	fmuls	0x7db2c4
  781efd: d9 5c 24 0c                  	fstps	0xc(%esp)
  781f01: db 04 24                     	fildl	(%esp)
  781f04: d8 0d c4 b2 7d 00            	fmuls	0x7db2c4
  781f0a: d9 5c 24 10                  	fstps	0x10(%esp)
  781f0e: d8 05 f0 f7 d0 03            	fadds	0x3d0f7f0
  781f14: d9 05 10 cb 7d 00            	flds	0x7dcb10
  781f1a: d8 d9                        	fcomp	%st(1)
  781f1c: df e0                        	fnstsw	%ax
  781f1e: f6 c4 05                     	testb	$0x5, %ah
  781f21: 7a 0a                        	jp	0x781f2d <.text+0x380f2d>
  781f23: dd d8                        	fstp	%st(0)
  781f25: d9 05 10 cb 7d 00            	flds	0x7dcb10
  781f2b: eb 15                        	jmp	0x781f42 <.text+0x380f42>
  781f2d: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  781f33: df e0                        	fnstsw	%ax
  781f35: f6 c4 05                     	testb	$0x5, %ah
  781f38: 7a 08                        	jp	0x781f42 <.text+0x380f42>
  781f3a: dd d8                        	fstp	%st(0)
  781f3c: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  781f42: d9 05 f4 f7 d0 03            	flds	0x3d0f7f4
  781f48: d8 44 24 08                  	fadds	0x8(%esp)
  781f4c: d9 05 10 cb 7d 00            	flds	0x7dcb10
  781f52: d8 d9                        	fcomp	%st(1)
  781f54: df e0                        	fnstsw	%ax
  781f56: f6 c4 05                     	testb	$0x5, %ah
  781f59: 7a 0a                        	jp	0x781f65 <.text+0x380f65>
  781f5b: dd d8                        	fstp	%st(0)
  781f5d: d9 05 10 cb 7d 00            	flds	0x7dcb10
  781f63: eb 15                        	jmp	0x781f7a <.text+0x380f7a>
  781f65: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  781f6b: df e0                        	fnstsw	%ax
  781f6d: f6 c4 05                     	testb	$0x5, %ah
  781f70: 7a 08                        	jp	0x781f7a <.text+0x380f7a>
  781f72: dd d8                        	fstp	%st(0)
  781f74: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  781f7a: d9 05 f8 f7 d0 03            	flds	0x3d0f7f8
  781f80: d8 44 24 0c                  	fadds	0xc(%esp)
  781f84: d9 05 10 cb 7d 00            	flds	0x7dcb10
  781f8a: d8 d9                        	fcomp	%st(1)
  781f8c: df e0                        	fnstsw	%ax
  781f8e: f6 c4 05                     	testb	$0x5, %ah
  781f91: 7a 0a                        	jp	0x781f9d <.text+0x380f9d>
  781f93: dd d8                        	fstp	%st(0)
  781f95: d9 05 10 cb 7d 00            	flds	0x7dcb10
  781f9b: eb 15                        	jmp	0x781fb2 <.text+0x380fb2>
  781f9d: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
