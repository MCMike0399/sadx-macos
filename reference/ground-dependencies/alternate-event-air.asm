
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  447b90: 83 ec 24                     	subl	$0x24, %esp
  447b93: d9 05 f8 f0 b0 03            	flds	0x3b0f0f8
  447b99: 53                           	pushl	%ebx
  447b9a: 55                           	pushl	%ebp
  447b9b: 8b 6c 24 34                  	movl	0x34(%esp), %ebp
  447b9f: 56                           	pushl	%esi
  447ba0: 8b 74 24 3c                  	movl	0x3c(%esp), %esi
  447ba4: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  447baa: 8d 45 10                     	leal	0x10(%ebp), %eax
  447bad: 8b 08                        	movl	(%eax), %ecx
  447baf: 8b 50 04                     	movl	0x4(%eax), %edx
  447bb2: 8b 40 08                     	movl	0x8(%eax), %eax
  447bb5: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  447bb9: d8 44 24 18                  	fadds	0x18(%esp)
  447bbd: 89 54 24 1c                  	movl	%edx, 0x1c(%esp)
  447bc1: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  447bc5: d9 5c 24 18                  	fstps	0x18(%esp)
  447bc9: 57                           	pushl	%edi
  447bca: d9 05 fc f0 b0 03            	flds	0x3b0f0fc
  447bd0: 8b 7c 24 38                  	movl	0x38(%esp), %edi
  447bd4: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  447bda: 8b 5f 3c                     	movl	0x3c(%edi), %ebx
  447bdd: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%esp)
  447be5: d8 44 24 20                  	fadds	0x20(%esp)
  447be9: d9 5c 24 20                  	fstps	0x20(%esp)
  447bed: d9 05 00 f1 b0 03            	flds	0x3b0f100
  447bf3: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  447bf9: d8 44 24 24                  	fadds	0x24(%esp)
  447bfd: d9 5c 24 24                  	fstps	0x24(%esp)
  447c01: 0f b6 43 24                  	movzbl	0x24(%ebx), %eax
  447c05: 48                           	decl	%eax
  447c06: 83 f8 03                     	cmpl	$0x3, %eax
  447c09: 0f 87 c9 01 00 00            	ja	0x447dd8 <.text+0x46dd8>
  447c0f: ff 24 85 54 7e 44 00         	jmpl	*0x447e54(,%eax,4)
  447c16: 8d 4b 28                     	leal	0x28(%ebx), %ecx
  447c19: 8b 11                        	movl	(%ecx), %edx
  447c1b: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  447c1f: 8b 41 04                     	movl	0x4(%ecx), %eax
  447c22: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  447c26: 8b 49 08                     	movl	0x8(%ecx), %ecx
  447c29: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  447c2d: 8d 57 20                     	leal	0x20(%edi), %edx
  447c30: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  447c34: e8 d7 f9 33 00               	calll	0x787610 <.text+0x386610>
  447c39: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  447c3d: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  447c45: e8 76 00 34 00               	calll	0x787cc0 <.text+0x386cc0>
  447c4a: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  447c50: df e0                        	fnstsw	%ax
  447c52: f6 c4 41                     	testb	$0x41, %ah
  447c55: 0f 8b 9c 00 00 00            	jnp	0x447cf7 <.text+0x46cf7>
  447c5b: d9 44 24 18                  	flds	0x18(%esp)
  447c5f: d9 44 24 10                  	flds	0x10(%esp)
  447c63: d9 f3                        	fpatan
  447c65: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  447c6b: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  447c71: e8 ca c6 1f 00               	calll	0x644340 <.text+0x243340>
  447c76: 25 ff ff 00 00               	andl	$0xffff, %eax           # imm = 0xFFFF
  447c7b: 89 45 20                     	movl	%eax, 0x20(%ebp)
  447c7e: d9 86 c4 00 00 00            	flds	0xc4(%esi)
  447c84: d8 66 38                     	fsubs	0x38(%esi)
  447c87: d9 54 24 28                  	fsts	0x28(%esp)
  447c8b: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447c91: df e0                        	fnstsw	%ax
  447c93: f6 c4 41                     	testb	$0x41, %ah
  447c96: 0f 85 c7 00 00 00            	jne	0x447d63 <.text+0x46d63>
  447c9c: d9 44 24 28                  	flds	0x28(%esp)
  447ca0: d8 9e d8 00 00 00            	fcomps	0xd8(%esi)
  447ca6: df e0                        	fnstsw	%ax
  447ca8: f6 c4 41                     	testb	$0x41, %ah
  447cab: 0f 85 2b 01 00 00            	jne	0x447ddc <.text+0x46ddc>
  447cb1: 8b 96 d8 00 00 00            	movl	0xd8(%esi), %edx
  447cb7: 89 54 24 28                  	movl	%edx, 0x28(%esp)
  447cbb: e9 1c 01 00 00               	jmp	0x447ddc <.text+0x46ddc>
  447cc0: 8d 43 28                     	leal	0x28(%ebx), %eax
  447cc3: 8b 08                        	movl	(%eax), %ecx
  447cc5: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  447cc9: 8b 50 04                     	movl	0x4(%eax), %edx
  447ccc: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  447cd0: 8b 40 08                     	movl	0x8(%eax), %eax
  447cd3: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  447cd7: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  447cdb: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  447ce3: e8 d8 ff 33 00               	calll	0x787cc0 <.text+0x386cc0>
  447ce8: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  447cee: df e0                        	fnstsw	%ax
  447cf0: f6 c4 44                     	testb	$0x44, %ah
  447cf3: 7a 16                        	jp	0x447d0b <.text+0x46d0b>
  447cf5: dd d8                        	fstp	%st(0)
  447cf7: 33 c0                        	xorl	%eax, %eax
  447cf9: 89 46 40                     	movl	%eax, 0x40(%esi)
  447cfc: 89 46 3c                     	movl	%eax, 0x3c(%esi)
  447cff: 89 46 38                     	movl	%eax, 0x38(%esi)
  447d02: c6 43 24 01                  	movb	$0x1, 0x24(%ebx)
  447d06: e9 d1 00 00 00               	jmp	0x447ddc <.text+0x46ddc>
  447d0b: d9 44 24 18                  	flds	0x18(%esp)
  447d0f: d9 44 24 10                  	flds	0x10(%esp)
  447d13: d9 f3                        	fpatan
  447d15: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  447d1b: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  447d21: e8 1a c6 1f 00               	calll	0x644340 <.text+0x243340>
  447d26: 25 ff ff 00 00               	andl	$0xffff, %eax           # imm = 0xFFFF
  447d2b: 89 45 20                     	movl	%eax, 0x20(%ebp)
  447d2e: d8 66 38                     	fsubs	0x38(%esi)
  447d31: d9 54 24 28                  	fsts	0x28(%esp)
  447d35: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447d3b: df e0                        	fnstsw	%ax
  447d3d: f6 c4 41                     	testb	$0x41, %ah
  447d40: 75 21                        	jne	0x447d63 <.text+0x46d63>
  447d42: d9 44 24 28                  	flds	0x28(%esp)
  447d46: d8 9e d8 00 00 00            	fcomps	0xd8(%esi)
  447d4c: df e0                        	fnstsw	%ax
  447d4e: f6 c4 41                     	testb	$0x41, %ah
  447d51: 0f 85 85 00 00 00            	jne	0x447ddc <.text+0x46ddc>
  447d57: 8b 8e d8 00 00 00            	movl	0xd8(%esi), %ecx
  447d5d: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  447d61: eb 79                        	jmp	0x447ddc <.text+0x46ddc>
  447d63: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%esp)
  447d6b: eb 6f                        	jmp	0x447ddc <.text+0x46ddc>
  447d6d: 8d 53 28                     	leal	0x28(%ebx), %edx
  447d70: 8b 02                        	movl	(%edx), %eax
  447d72: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  447d76: 8b 4a 04                     	movl	0x4(%edx), %ecx
  447d79: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  447d7d: 8b 52 08                     	movl	0x8(%edx), %edx
  447d80: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  447d84: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  447d88: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  447d90: e8 2b ff 33 00               	calll	0x787cc0 <.text+0x386cc0>
  447d95: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447d9b: df e0                        	fnstsw	%ax
  447d9d: f6 c4 44                     	testb	$0x44, %ah
  447da0: 0f 8b 51 ff ff ff            	jnp	0x447cf7 <.text+0x46cf7>
  447da6: d9 44 24 18                  	flds	0x18(%esp)
  447daa: d9 44 24 10                  	flds	0x10(%esp)
  447dae: d9 f3                        	fpatan
  447db0: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  447db6: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  447dbc: e8 7f c5 1f 00               	calll	0x644340 <.text+0x243340>
  447dc1: 25 ff ff 00 00               	andl	$0xffff, %eax           # imm = 0xFFFF
  447dc6: 8d 54 24 10                  	leal	0x10(%esp), %edx
  447dca: 8d 4c 24 1c                  	leal	0x1c(%esp), %ecx
  447dce: 89 45 20                     	movl	%eax, 0x20(%ebp)
  447dd1: e8 9a d3 33 00               	calll	0x785170 <.text+0x384170>
  447dd6: eb 04                        	jmp	0x447ddc <.text+0x46ddc>
  447dd8: 80 67 05 bf                  	andb	$-0x41, 0x5(%edi)
  447ddc: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  447de0: 50                           	pushl	%eax
  447de1: 57                           	pushl	%edi
  447de2: e8 19 6e ff ff               	calll	0x43ec00 <.text+0x3dc00>
  447de7: d9 86 ec 00 00 00            	flds	0xec(%esi)
  447ded: d8 4e 38                     	fmuls	0x38(%esi)
  447df0: 8b 4d 20                     	movl	0x20(%ebp), %ecx
  447df3: 51                           	pushl	%ecx
  447df4: 55                           	pushl	%ebp
  447df5: d8 44 24 2c                  	fadds	0x2c(%esp)
  447df9: 8b c6                        	movl	%esi, %eax
  447dfb: d9 5c 24 2c                  	fstps	0x2c(%esp)
  447dff: d9 86 f4 00 00 00            	flds	0xf4(%esi)
  447e05: d8 4e 3c                     	fmuls	0x3c(%esi)
  447e08: d8 44 24 30                  	fadds	0x30(%esp)
  447e0c: d9 5c 24 30                  	fstps	0x30(%esp)
  447e10: d9 86 f8 00 00 00            	flds	0xf8(%esi)
  447e16: d8 4e 40                     	fmuls	0x40(%esi)
  447e19: d8 44 24 34                  	fadds	0x34(%esp)
  447e1d: d9 5c 24 34                  	fstps	0x34(%esp)
  447e21: e8 ca bf ff ff               	calll	0x443df0 <.text+0x42df0>
  447e26: d9 44 24 2c                  	flds	0x2c(%esp)
  447e2a: 8b 44 24 30                  	movl	0x30(%esp), %eax
  447e2e: d8 44 24 38                  	fadds	0x38(%esp)
  447e32: 8b 4c 24 34                  	movl	0x34(%esp), %ecx
  447e36: 83 c4 10                     	addl	$0x10, %esp
  447e39: 83 c6 2c                     	addl	$0x2c, %esi
  447e3c: d9 5c 24 1c                  	fstps	0x1c(%esp)
  447e40: 8b 54 24 1c                  	movl	0x1c(%esp), %edx
  447e44: 89 16                        	movl	%edx, (%esi)
  447e46: 89 46 04                     	movl	%eax, 0x4(%esi)
  447e49: 5f                           	popl	%edi
  447e4a: 89 4e 08                     	movl	%ecx, 0x8(%esi)
  447e4d: 5e                           	popl	%esi
  447e4e: 5d                           	popl	%ebp
  447e4f: 5b                           	popl	%ebx
  447e50: 83 c4 24                     	addl	$0x24, %esp
  447e53: c3                           	retl
  447e54: dc 7d 44                     	fdivrl	0x44(%ebp)
  447e57: 00 16                        	addb	%dl, (%esi)
  447e59: 7c 44                        	jl	0x447e9f <.text+0x46e9f>
  447e5b: 00 c0                        	addb	%al, %al
  447e5d: 7c 44                        	jl	0x447ea3 <.text+0x46ea3>
  447e5f: 00 6d 7d                     	addb	%ch, 0x7d(%ebp)
  447e62: 44                           	incl	%esp
  447e63: 00 90 90 90 90 90            	addb	%dl, -0x6f6f6f70(%eax)
  447e69: 90                           	nop
  447e6a: 90                           	nop
  447e6b: 90                           	nop
  447e6c: 90                           	nop
  447e6d: 90                           	nop
  447e6e: 90                           	nop
  447e6f: 90                           	nop
  447e70: 83 ec 0c                     	subl	$0xc, %esp
  447e73: 53                           	pushl	%ebx
  447e74: 8b 5c 24 18                  	movl	0x18(%esp), %ebx
  447e78: 8d 43 10                     	leal	0x10(%ebx), %eax
  447e7b: 8b 08                        	movl	(%eax), %ecx
  447e7d: 8b 50 04                     	movl	0x4(%eax), %edx
