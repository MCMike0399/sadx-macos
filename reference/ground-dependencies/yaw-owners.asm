
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  443c50: 83 ec 0c                     	subl	$0xc, %esp
  443c53: 53                           	pushl	%ebx
  443c54: 8d 5f 38                     	leal	0x38(%edi), %ebx
  443c57: 8b c3                        	movl	%ebx, %eax
  443c59: 8b 08                        	movl	(%eax), %ecx
  443c5b: 8b 50 04                     	movl	0x4(%eax), %edx
  443c5e: 8b 40 08                     	movl	0x8(%eax), %eax
  443c61: 55                           	pushl	%ebp
  443c62: 8b 6c 24 1c                  	movl	0x1c(%esp), %ebp
  443c66: 89 4c 24 08                  	movl	%ecx, 0x8(%esp)
  443c6a: 8d 4c 24 08                  	leal	0x8(%esp), %ecx
  443c6e: 51                           	pushl	%ecx
  443c6f: 56                           	pushl	%esi
  443c70: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  443c74: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  443c78: e8 13 b0 ff ff               	calll	0x43ec90 <.text+0x3dc90>
  443c7d: 8b 56 18                     	movl	0x18(%esi), %edx
  443c80: 55                           	pushl	%ebp
  443c81: 52                           	pushl	%edx
  443c82: e8 29 47 ff ff               	calll	0x4383b0 <.text+0x373b0>
  443c87: 83 c4 10                     	addl	$0x10, %esp
  443c8a: 3d 00 20 00 00               	cmpl	$0x2000, %eax           # imm = 0x2000
  443c8f: 7e 0c                        	jle	0x443c9d <.text+0x42c9d>
  443c91: 8b 46 18                     	movl	0x18(%esi), %eax
  443c94: 68 00 08 00 00               	pushl	$0x800                  # imm = 0x800
  443c99: 55                           	pushl	%ebp
  443c9a: 50                           	pushl	%eax
  443c9b: eb 1b                        	jmp	0x443cb8 <.text+0x42cb8>
  443c9d: 3d 00 10 00 00               	cmpl	$0x1000, %eax           # imm = 0x1000
  443ca2: 7e 0b                        	jle	0x443caf <.text+0x42caf>
  443ca4: 8b 4e 18                     	movl	0x18(%esi), %ecx
  443ca7: c1 f8 02                     	sarl	$0x2, %eax
  443caa: 50                           	pushl	%eax
  443cab: 55                           	pushl	%ebp
  443cac: 51                           	pushl	%ecx
  443cad: eb 09                        	jmp	0x443cb8 <.text+0x42cb8>
  443caf: 8b 56 18                     	movl	0x18(%esi), %edx
  443cb2: c1 f8 03                     	sarl	$0x3, %eax
  443cb5: 50                           	pushl	%eax
  443cb6: 55                           	pushl	%ebp
  443cb7: 52                           	pushl	%edx
  443cb8: e8 93 46 ff ff               	calll	0x438350 <.text+0x37350>
  443cbd: 89 46 18                     	movl	%eax, 0x18(%esi)
  443cc0: 83 c4 0c                     	addl	$0xc, %esp
  443cc3: 8d 44 24 08                  	leal	0x8(%esp), %eax
  443cc7: 50                           	pushl	%eax
  443cc8: 56                           	pushl	%esi
  443cc9: e8 32 af ff ff               	calll	0x43ec00 <.text+0x3dc00>
  443cce: 8a 46 04                     	movb	0x4(%esi), %al
  443cd1: 83 c4 08                     	addl	$0x8, %esp
  443cd4: a8 03                        	testb	$0x3, %al
  443cd6: 75 4c                        	jne	0x443d24 <.text+0x42d24>
  443cd8: d9 44 24 08                  	flds	0x8(%esp)
  443cdc: 5d                           	popl	%ebp
  443cdd: d8 0d f0 ff 7d 00            	fmuls	0x7dfff0
  443ce3: d9 03                        	flds	(%ebx)
  443ce5: d8 0d 68 d5 7d 00            	fmuls	0x7dd568
  443ceb: de c1                        	faddp	%st, %st(1)
  443ced: d9 1b                        	fstps	(%ebx)
  443cef: 5b                           	popl	%ebx
  443cf0: d9 47 3c                     	flds	0x3c(%edi)
  443cf3: d8 0d 68 d5 7d 00            	fmuls	0x7dd568
  443cf9: d9 44 24 04                  	flds	0x4(%esp)
  443cfd: d8 0d f0 ff 7d 00            	fmuls	0x7dfff0
  443d03: de c1                        	faddp	%st, %st(1)
  443d05: d9 5f 3c                     	fstps	0x3c(%edi)
  443d08: d9 47 40                     	flds	0x40(%edi)
  443d0b: d8 0d 68 d5 7d 00            	fmuls	0x7dd568
  443d11: d9 44 24 08                  	flds	0x8(%esp)
  443d15: d8 0d f0 ff 7d 00            	fmuls	0x7dfff0
  443d1b: de c1                        	faddp	%st, %st(1)
  443d1d: d9 5f 40                     	fstps	0x40(%edi)
  443d20: 83 c4 0c                     	addl	$0xc, %esp
  443d23: c3                           	retl
  443d24: 6a 00                        	pushl	$0x0
  443d26: 6a 00                        	pushl	$0x0
  443d28: 56                           	pushl	%esi
  443d29: e8 92 80 ff ff               	calll	0x43bdc0 <.text+0x3adc0>
  443d2e: 8b 4c 24 24                  	movl	0x24(%esp), %ecx
  443d32: 83 c4 0c                     	addl	$0xc, %esp
  443d35: 85 c0                        	testl	%eax, %eax
  443d37: 75 23                        	jne	0x443d5c <.text+0x42d5c>
  443d39: d9 05 70 00 7e 00            	flds	0x7e0070
  443d3f: d9 41 3c                     	flds	0x3c(%ecx)
  443d42: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  443d48: df e0                        	fnstsw	%ax
  443d4a: f6 c4 05                     	testb	$0x5, %ah
  443d4d: 7a 61                        	jp	0x443db0 <.text+0x42db0>
  443d4f: dd d8                        	fstp	%st(0)
  443d51: d9 41 3c                     	flds	0x3c(%ecx)
  443d54: d8 0d 70 00 7e 00            	fmuls	0x7e0070
  443d5a: eb 54                        	jmp	0x443db0 <.text+0x42db0>
  443d5c: d9 47 1c                     	flds	0x1c(%edi)
  443d5f: d8 1d 50 d5 7d 00            	fcomps	0x7dd550
  443d65: df e0                        	fnstsw	%ax
  443d67: f6 c4 41                     	testb	$0x41, %ah
  443d6a: 75 23                        	jne	0x443d8f <.text+0x42d8f>
  443d6c: d9 05 18 01 7e 00            	flds	0x7e0118
  443d72: d9 41 3c                     	flds	0x3c(%ecx)
  443d75: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  443d7b: df e0                        	fnstsw	%ax
  443d7d: f6 c4 05                     	testb	$0x5, %ah
  443d80: 7a 2e                        	jp	0x443db0 <.text+0x42db0>
  443d82: dd d8                        	fstp	%st(0)
  443d84: d9 41 3c                     	flds	0x3c(%ecx)
  443d87: d8 0d 18 01 7e 00            	fmuls	0x7e0118
  443d8d: eb 21                        	jmp	0x443db0 <.text+0x42db0>
  443d8f: d9 05 98 cc 7d 00            	flds	0x7dcc98
  443d95: d9 41 3c                     	flds	0x3c(%ecx)
  443d98: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  443d9e: df e0                        	fnstsw	%ax
  443da0: f6 c4 05                     	testb	$0x5, %ah
  443da3: 7a 0b                        	jp	0x443db0 <.text+0x42db0>
  443da5: dd d8                        	fstp	%st(0)
  443da7: d9 41 3c                     	flds	0x3c(%ecx)
  443daa: d8 0d 98 cc 7d 00            	fmuls	0x7dcc98
  443db0: d9 05 10 cb 7d 00            	flds	0x7dcb10
  443db6: 5d                           	popl	%ebp
  443db7: d8 e1                        	fsub	%st(1), %st
  443db9: d9 44 24 04                  	flds	0x4(%esp)
  443dbd: d8 c9                        	fmul	%st(1), %st
  443dbf: d9 c2                        	fld	%st(2)
  443dc1: d8 0b                        	fmuls	(%ebx)
  443dc3: de c1                        	faddp	%st, %st(1)
  443dc5: d9 1b                        	fstps	(%ebx)
  443dc7: 5b                           	popl	%ebx
  443dc8: d9 c1                        	fld	%st(1)
  443dca: d8 4f 3c                     	fmuls	0x3c(%edi)
  443dcd: d9 c1                        	fld	%st(1)
  443dcf: d8 4c 24 04                  	fmuls	0x4(%esp)
  443dd3: de c1                        	faddp	%st, %st(1)
  443dd5: d9 5f 3c                     	fstps	0x3c(%edi)
  443dd8: d8 4c 24 08                  	fmuls	0x8(%esp)
  443ddc: d9 c9                        	fxch	%st(1)
  443dde: d8 4f 40                     	fmuls	0x40(%edi)
  443de1: de c1                        	faddp	%st, %st(1)
  443de3: d9 5f 40                     	fstps	0x40(%edi)
  443de6: 83 c4 0c                     	addl	$0xc, %esp
  443de9: c3                           	retl
  443dea: 90                           	nop
  443deb: 90                           	nop
  443dec: 90                           	nop
  443ded: 90                           	nop
  443dee: 90                           	nop
  443def: 90                           	nop
  443df0: 83 ec 0c                     	subl	$0xc, %esp
  443df3: 56                           	pushl	%esi
  443df4: 8b f0                        	movl	%eax, %esi
  443df6: 83 c6 38                     	addl	$0x38, %esi
  443df9: 8b c6                        	movl	%esi, %eax
  443dfb: 8b 08                        	movl	(%eax), %ecx
  443dfd: 8b 50 04                     	movl	0x4(%eax), %edx
  443e00: 8b 40 08                     	movl	0x8(%eax), %eax
  443e03: 89 4c 24 04                  	movl	%ecx, 0x4(%esp)
  443e07: 8d 4c 24 04                  	leal	0x4(%esp), %ecx
  443e0b: 51                           	pushl	%ecx
  443e0c: 57                           	pushl	%edi
  443e0d: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  443e11: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  443e15: e8 76 ae ff ff               	calll	0x43ec90 <.text+0x3dc90>
  443e1a: 8b 54 24 20                  	movl	0x20(%esp), %edx
  443e1e: 8b 47 18                     	movl	0x18(%edi), %eax
  443e21: 68 00 20 00 00               	pushl	$0x2000                 # imm = 0x2000
  443e26: 52                           	pushl	%edx
  443e27: 50                           	pushl	%eax
  443e28: e8 23 45 ff ff               	calll	0x438350 <.text+0x37350>
  443e2d: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  443e31: 51                           	pushl	%ecx
  443e32: 57                           	pushl	%edi
  443e33: 89 47 18                     	movl	%eax, 0x18(%edi)
  443e36: e8 c5 ad ff ff               	calll	0x43ec00 <.text+0x3dc00>
  443e3b: 8b 54 24 20                  	movl	0x20(%esp), %edx
  443e3f: 8b 44 24 24                  	movl	0x24(%esp), %eax
  443e43: 8b 4c 24 28                  	movl	0x28(%esp), %ecx
  443e47: 89 16                        	movl	%edx, (%esi)
  443e49: 89 46 04                     	movl	%eax, 0x4(%esi)
  443e4c: 83 c4 1c                     	addl	$0x1c, %esp
  443e4f: 89 4e 08                     	movl	%ecx, 0x8(%esi)
  443e52: 5e                           	popl	%esi
  443e53: 83 c4 0c                     	addl	$0xc, %esp
  443e56: c3                           	retl
  443e57: 90                           	nop
  443e58: 90                           	nop
  443e59: 90                           	nop
  443e5a: 90                           	nop
  443e5b: 90                           	nop
  443e5c: 90                           	nop
  443e5d: 90                           	nop
  443e5e: 90                           	nop
  443e5f: 90                           	nop
  443e60: 83 ec 0c                     	subl	$0xc, %esp
  443e63: 57                           	pushl	%edi
  443e64: 8d 7e 38                     	leal	0x38(%esi), %edi
  443e67: 8b c7                        	movl	%edi, %eax
  443e69: 8b 08                        	movl	(%eax), %ecx
  443e6b: 8b 50 04                     	movl	0x4(%eax), %edx
  443e6e: 8b 40 08                     	movl	0x8(%eax), %eax
  443e71: 89 4c 24 04                  	movl	%ecx, 0x4(%esp)
  443e75: 8d 4c 24 04                  	leal	0x4(%esp), %ecx
  443e79: 51                           	pushl	%ecx
  443e7a: 53                           	pushl	%ebx
  443e7b: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  443e7f: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  443e83: e8 08 ae ff ff               	calll	0x43ec90 <.text+0x3dc90>
  443e88: d9 07                        	flds	(%edi)
  443e8a: d8 9e d0 00 00 00            	fcomps	0xd0(%esi)
  443e90: 83 c4 08                     	addl	$0x8, %esp
  443e93: b9 00 01 00 00               	movl	$0x100, %ecx            # imm = 0x100
  443e98: df e0                        	fnstsw	%ax
  443e9a: f6 c4 41                     	testb	$0x41, %ah
  443e9d: 75 29                        	jne	0x443ec8 <.text+0x42ec8>
  443e9f: d9 07                        	flds	(%edi)
  443ea1: 51                           	pushl	%ecx
  443ea2: d8 a6 d0 00 00 00            	fsubs	0xd0(%esi)
  443ea8: d8 0d 20 01 7e 00            	fmuls	0x7e0120
  443eae: d9 1c 24                     	fstps	(%esp)
  443eb1: e8 3a 33 34 00               	calll	0x7871f0 <.text+0x3861f0>
  443eb6: d8 0d 68 ce 7d 00            	fmuls	0x7dce68
  443ebc: e8 7f 04 20 00               	calll	0x644340 <.text+0x243340>
  443ec1: b9 00 01 00 00               	movl	$0x100, %ecx            # imm = 0x100
  443ec6: 2b c8                        	subl	%eax, %ecx
  443ec8: 8b 54 24 18                  	movl	0x18(%esp), %edx
  443ecc: 8b 43 18                     	movl	0x18(%ebx), %eax
  443ecf: 51                           	pushl	%ecx
  443ed0: 52                           	pushl	%edx
  443ed1: 50                           	pushl	%eax
  443ed2: e8 79 44 ff ff               	calll	0x438350 <.text+0x37350>
  443ed7: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  443edb: 51                           	pushl	%ecx
  443edc: 53                           	pushl	%ebx
  443edd: 89 43 18                     	movl	%eax, 0x18(%ebx)
  443ee0: e8 1b ad ff ff               	calll	0x43ec00 <.text+0x3dc00>
  443ee5: d9 46 1c                     	flds	0x1c(%esi)
  443ee8: d8 1d 50 d5 7d 00            	fcomps	0x7dd550
  443eee: 83 c4 14                     	addl	$0x14, %esp
  443ef1: df e0                        	fnstsw	%ax
  443ef3: f6 c4 41                     	testb	$0x41, %ah
  443ef6: 75 0e                        	jne	0x443f06 <.text+0x42f06>
  443ef8: d9 05 18 01 7e 00            	flds	0x7e0118
  443efe: d9 05 1c 01 7e 00            	flds	0x7e011c
  443f04: eb 0c                        	jmp	0x443f12 <.text+0x42f12>
  443f06: d9 05 98 cc 7d 00            	flds	0x7dcc98
  443f0c: d9 05 98 cc 7d 00            	flds	0x7dcc98
  443f12: d9 44 24 04                  	flds	0x4(%esp)
  443f16: d8 c9                        	fmul	%st(1), %st
  443f18: d9 c2                        	fld	%st(2)
  443f1a: d8 0f                        	fmuls	(%edi)
  443f1c: de c1                        	faddp	%st, %st(1)
  443f1e: d9 1f                        	fstps	(%edi)
  443f20: 5f                           	popl	%edi
  443f21: d9 c1                        	fld	%st(1)
  443f23: d8 4e 3c                     	fmuls	0x3c(%esi)
  443f26: d9 44 24 04                  	flds	0x4(%esp)
  443f2a: d8 ca                        	fmul	%st(2), %st
  443f2c: de c1                        	faddp	%st, %st(1)
  443f2e: d9 5e 3c                     	fstps	0x3c(%esi)
  443f31: d9 c9                        	fxch	%st(1)
  443f33: d8 4e 40                     	fmuls	0x40(%esi)
  443f36: d9 44 24 08                  	flds	0x8(%esp)
  443f3a: d8 ca                        	fmul	%st(2), %st
  443f3c: de c1                        	faddp	%st, %st(1)
  443f3e: d9 5e 40                     	fstps	0x40(%esi)
  443f41: dd d8                        	fstp	%st(0)
  443f43: 83 c4 0c                     	addl	$0xc, %esp
  443f46: c3                           	retl
  443f47: 90                           	nop
  443f48: 90                           	nop
  443f49: 90                           	nop
  443f4a: 90                           	nop
  443f4b: 90                           	nop
  443f4c: 90                           	nop
  443f4d: 90                           	nop
  443f4e: 90                           	nop
  443f4f: 90                           	nop
