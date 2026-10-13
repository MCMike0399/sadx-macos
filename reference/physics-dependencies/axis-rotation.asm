
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  788df0: 83 ec 28                     	subl	$0x28, %esp
  788df3: 8b 02                        	movl	(%edx), %eax
  788df5: 56                           	pushl	%esi
  788df6: 8b f1                        	movl	%ecx, %esi
  788df8: 8b 4a 04                     	movl	0x4(%edx), %ecx
  788dfb: 8b 52 08                     	movl	0x8(%edx), %edx
  788dfe: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  788e02: d9 44 24 14                  	flds	0x14(%esp)
  788e06: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  788e0a: d8 4c 24 14                  	fmuls	0x14(%esp)
  788e0e: 57                           	pushl	%edi
  788e0f: d9 44 24 1c                  	flds	0x1c(%esp)
  788e13: 51                           	pushl	%ecx
  788e14: d8 4c 24 20                  	fmuls	0x20(%esp)
  788e18: 89 44 24 28                  	movl	%eax, 0x28(%esp)
  788e1c: de c1                        	faddp	%st, %st(1)
  788e1e: d9 1c 24                     	fstps	(%esp)
  788e21: e8 ca e3 ff ff               	calll	0x7871f0 <.text+0x3861f0>
  788e26: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  788e2c: d9 44 24 18                  	flds	0x18(%esp)
  788e30: da e9                        	fucompp
  788e32: df e0                        	fnstsw	%ax
  788e34: f6 c4 44                     	testb	$0x44, %ah
  788e37: 7a 07                        	jp	0x788e40 <.text+0x387e40>
  788e39: b9 01 00 00 00               	movl	$0x1, %ecx
  788e3e: eb 02                        	jmp	0x788e42 <.text+0x387e42>
  788e40: 33 c9                        	xorl	%ecx, %ecx
  788e42: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  788e48: d9 44 24 1c                  	flds	0x1c(%esp)
  788e4c: da e9                        	fucompp
  788e4e: df e0                        	fnstsw	%ax
  788e50: f6 c4 44                     	testb	$0x44, %ah
  788e53: 7b 07                        	jnp	0x788e5c <.text+0x387e5c>
  788e55: b8 01 00 00 00               	movl	$0x1, %eax
  788e5a: eb 02                        	jmp	0x788e5e <.text+0x387e5e>
  788e5c: 33 c0                        	xorl	%eax, %eax
  788e5e: 85 c8                        	testl	%ecx, %eax
  788e60: 74 2b                        	je	0x788e8d <.text+0x387e8d>
  788e62: d9 44 24 1c                  	flds	0x1c(%esp)
  788e66: c7 44 24 14 00 00 80 3f      	movl	$0x3f800000, 0x14(%esp) # imm = 0x3F800000
  788e6e: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  788e74: df e0                        	fnstsw	%ax
  788e76: f6 c4 41                     	testb	$0x41, %ah
  788e79: 74 08                        	je	0x788e83 <.text+0x387e83>
  788e7b: c7 44 24 14 00 00 80 bf      	movl	$0xbf800000, 0x14(%esp) # imm = 0xBF800000
  788e83: c7 44 24 0c 00 00 00 00      	movl	$0x0, 0xc(%esp)
  788e8b: eb 5a                        	jmp	0x788ee7 <.text+0x387ee7>
  788e8d: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  788e93: d9 44 24 1c                  	flds	0x1c(%esp)
  788e97: da e9                        	fucompp
  788e99: df e0                        	fnstsw	%ax
  788e9b: f6 c4 44                     	testb	$0x44, %ah
  788e9e: 7a 2d                        	jp	0x788ecd <.text+0x387ecd>
  788ea0: d9 44 24 18                  	flds	0x18(%esp)
  788ea4: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  788eac: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  788eb2: df e0                        	fnstsw	%ax
  788eb4: f6 c4 41                     	testb	$0x41, %ah
  788eb7: 75 0a                        	jne	0x788ec3 <.text+0x387ec3>
  788eb9: c7 44 24 0c 00 00 80 3f      	movl	$0x3f800000, 0xc(%esp)  # imm = 0x3F800000
  788ec1: eb 24                        	jmp	0x788ee7 <.text+0x387ee7>
  788ec3: c7 44 24 0c 00 00 80 bf      	movl	$0xbf800000, 0xc(%esp)  # imm = 0xBF800000
  788ecb: eb 1a                        	jmp	0x788ee7 <.text+0x387ee7>
  788ecd: d9 05 10 cb 7d 00            	flds	0x7dcb10
  788ed3: d8 f1                        	fdiv	%st(1), %st
  788ed5: d9 44 24 1c                  	flds	0x1c(%esp)
  788ed9: d8 c9                        	fmul	%st(1), %st
  788edb: d9 5c 24 14                  	fstps	0x14(%esp)
  788edf: d8 4c 24 18                  	fmuls	0x18(%esp)
  788ee3: d9 5c 24 0c                  	fstps	0xc(%esp)
  788ee7: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  788eed: d9 c1                        	fld	%st(1)
  788eef: da e9                        	fucompp
  788ef1: df e0                        	fnstsw	%ax
  788ef3: f6 c4 44                     	testb	$0x44, %ah
  788ef6: 7a 07                        	jp	0x788eff <.text+0x387eff>
  788ef8: b9 01 00 00 00               	movl	$0x1, %ecx
  788efd: eb 02                        	jmp	0x788f01 <.text+0x387f01>
  788eff: 33 c9                        	xorl	%ecx, %ecx
  788f01: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  788f07: d9 44 24 24                  	flds	0x24(%esp)
  788f0b: da e9                        	fucompp
  788f0d: df e0                        	fnstsw	%ax
  788f0f: f6 c4 44                     	testb	$0x44, %ah
  788f12: 7b 07                        	jnp	0x788f1b <.text+0x387f1b>
  788f14: b8 01 00 00 00               	movl	$0x1, %eax
  788f19: eb 02                        	jmp	0x788f1d <.text+0x387f1d>
  788f1b: 33 c0                        	xorl	%eax, %eax
  788f1d: 85 c8                        	testl	%ecx, %eax
  788f1f: 74 2d                        	je	0x788f4e <.text+0x387f4e>
  788f21: dd d8                        	fstp	%st(0)
  788f23: c7 44 24 10 00 00 80 3f      	movl	$0x3f800000, 0x10(%esp) # imm = 0x3F800000
  788f2b: d9 44 24 24                  	flds	0x24(%esp)
  788f2f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  788f35: df e0                        	fnstsw	%ax
  788f37: f6 c4 41                     	testb	$0x41, %ah
  788f3a: 74 08                        	je	0x788f44 <.text+0x387f44>
  788f3c: c7 44 24 10 00 00 80 bf      	movl	$0xbf800000, 0x10(%esp) # imm = 0xBF800000
  788f44: c7 44 24 08 00 00 00 00      	movl	$0x0, 0x8(%esp)
  788f4c: eb 48                        	jmp	0x788f96 <.text+0x387f96>
  788f4e: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  788f54: d9 44 24 24                  	flds	0x24(%esp)
  788f58: da e9                        	fucompp
  788f5a: df e0                        	fnstsw	%ax
  788f5c: f6 c4 44                     	testb	$0x44, %ah
  788f5f: 7a 29                        	jp	0x788f8a <.text+0x387f8a>
  788f61: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  788f67: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  788f6f: df e0                        	fnstsw	%ax
  788f71: f6 c4 41                     	testb	$0x41, %ah
  788f74: 75 0a                        	jne	0x788f80 <.text+0x387f80>
  788f76: c7 44 24 08 00 00 80 3f      	movl	$0x3f800000, 0x8(%esp)  # imm = 0x3F800000
  788f7e: eb 16                        	jmp	0x788f96 <.text+0x387f96>
  788f80: c7 44 24 08 00 00 80 bf      	movl	$0xbf800000, 0x8(%esp)  # imm = 0xBF800000
  788f88: eb 0c                        	jmp	0x788f96 <.text+0x387f96>
  788f8a: 8b 44 24 24                  	movl	0x24(%esp), %eax
  788f8e: d9 5c 24 08                  	fstps	0x8(%esp)
  788f92: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  788f96: 85 f6                        	testl	%esi, %esi
  788f98: 75 06                        	jne	0x788fa0 <.text+0x387fa0>
  788f9a: 8b 35 18 f8 d0 03            	movl	0x3d0f818, %esi
  788fa0: d9 46 10                     	flds	0x10(%esi)
  788fa3: 8b 4e 18                     	movl	0x18(%esi), %ecx
  788fa6: d9 46 14                     	flds	0x14(%esi)
  788fa9: 8b 56 1c                     	movl	0x1c(%esi), %edx
  788fac: d9 44 24 14                  	flds	0x14(%esp)
  788fb0: 89 4c 24 20                  	movl	%ecx, 0x20(%esp)
  788fb4: d9 e0                        	fchs
  788fb6: 89 54 24 24                  	movl	%edx, 0x24(%esp)
  788fba: d9 54 24 18                  	fsts	0x18(%esp)
  788fbe: 8b 46 08                     	movl	0x8(%esi), %eax
  788fc1: d8 4e 20                     	fmuls	0x20(%esi)
  788fc4: 8b 4e 0c                     	movl	0xc(%esi), %ecx
  788fc7: d9 c2                        	fld	%st(2)
  788fc9: 89 4c 24 1c                  	movl	%ecx, 0x1c(%esp)
  788fcd: d8 4c 24 0c                  	fmuls	0xc(%esp)
  788fd1: de c1                        	faddp	%st, %st(1)
  788fd3: d9 5e 10                     	fstps	0x10(%esi)
  788fd6: d9 44 24 18                  	flds	0x18(%esp)
  788fda: d8 4e 24                     	fmuls	0x24(%esi)
  788fdd: d9 c1                        	fld	%st(1)
  788fdf: d8 4c 24 0c                  	fmuls	0xc(%esp)
  788fe3: de c1                        	faddp	%st, %st(1)
  788fe5: d9 5e 14                     	fstps	0x14(%esi)
  788fe8: d9 44 24 18                  	flds	0x18(%esp)
  788fec: d8 4e 28                     	fmuls	0x28(%esi)
  788fef: d9 44 24 20                  	flds	0x20(%esp)
  788ff3: d8 4c 24 0c                  	fmuls	0xc(%esp)
  788ff7: de c1                        	faddp	%st, %st(1)
  788ff9: d9 5e 18                     	fstps	0x18(%esi)
  788ffc: d9 44 24 18                  	flds	0x18(%esp)
  789000: d8 4e 2c                     	fmuls	0x2c(%esi)
  789003: d9 44 24 24                  	flds	0x24(%esp)
  789007: d8 4c 24 0c                  	fmuls	0xc(%esp)
  78900b: de c1                        	faddp	%st, %st(1)
  78900d: d9 5e 1c                     	fstps	0x1c(%esi)
  789010: d9 44 24 0c                  	flds	0xc(%esp)
  789014: d8 4e 20                     	fmuls	0x20(%esi)
  789017: d9 44 24 18                  	flds	0x18(%esp)
  78901b: d8 cb                        	fmul	%st(3), %st
  78901d: de e9                        	fsubrp	%st, %st(1)
  78901f: d9 5c 24 28                  	fstps	0x28(%esp)
  789023: d9 44 24 0c                  	flds	0xc(%esp)
  789027: d8 4e 24                     	fmuls	0x24(%esi)
  78902a: d9 44 24 18                  	flds	0x18(%esp)
  78902e: d8 ca                        	fmul	%st(2), %st
  789030: de e9                        	fsubrp	%st, %st(1)
  789032: dd da                        	fstp	%st(2)
  789034: dd d8                        	fstp	%st(0)
  789036: d9 44 24 0c                  	flds	0xc(%esp)
  78903a: d8 4e 28                     	fmuls	0x28(%esi)
  78903d: d9 44 24 18                  	flds	0x18(%esp)
  789041: d8 4c 24 20                  	fmuls	0x20(%esp)
  789045: de e9                        	fsubrp	%st, %st(1)
  789047: d9 44 24 0c                  	flds	0xc(%esp)
  78904b: d8 4e 2c                     	fmuls	0x2c(%esi)
  78904e: d9 44 24 18                  	flds	0x18(%esp)
  789052: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  789056: d8 4c 24 24                  	fmuls	0x24(%esp)
  78905a: de e9                        	fsubrp	%st, %st(1)
  78905c: d9 5c 24 2c                  	fstps	0x2c(%esp)
  789060: d9 06                        	flds	(%esi)
  789062: d9 46 04                     	flds	0x4(%esi)
  789065: d9 c1                        	fld	%st(1)
  789067: d8 4c 24 08                  	fmuls	0x8(%esp)
  78906b: d9 44 24 28                  	flds	0x28(%esp)
  78906f: d8 4c 24 10                  	fmuls	0x10(%esp)
  789073: de e9                        	fsubrp	%st, %st(1)
  789075: d9 1e                        	fstps	(%esi)
  789077: d9 44 24 08                  	flds	0x8(%esp)
  78907b: d8 c9                        	fmul	%st(1), %st
  78907d: d9 c4                        	fld	%st(4)
  78907f: d8 4c 24 10                  	fmuls	0x10(%esp)
  789083: de e9                        	fsubrp	%st, %st(1)
  789085: d9 5e 04                     	fstps	0x4(%esi)
  789088: d9 44 24 18                  	flds	0x18(%esp)
  78908c: d8 4c 24 08                  	fmuls	0x8(%esp)
  789090: d9 c3                        	fld	%st(3)
  789092: d8 4c 24 10                  	fmuls	0x10(%esp)
  789096: de e9                        	fsubrp	%st, %st(1)
  789098: d9 5e 08                     	fstps	0x8(%esi)
  78909b: d9 44 24 1c                  	flds	0x1c(%esp)
  78909f: d8 4c 24 08                  	fmuls	0x8(%esp)
  7890a3: 8b 7c 24 34                  	movl	0x34(%esp), %edi
  7890a7: d9 44 24 2c                  	flds	0x2c(%esp)
  7890ab: 8b cf                        	movl	%edi, %ecx
  7890ad: d8 4c 24 10                  	fmuls	0x10(%esp)
  7890b1: de e9                        	fsubrp	%st, %st(1)
  7890b3: d9 5e 0c                     	fstps	0xc(%esi)
  7890b6: d9 44 24 28                  	flds	0x28(%esp)
  7890ba: d8 4c 24 08                  	fmuls	0x8(%esp)
  7890be: d9 ca                        	fxch	%st(2)
  7890c0: d8 4c 24 10                  	fmuls	0x10(%esp)
  7890c4: de c2                        	faddp	%st, %st(2)
  7890c6: d9 c9                        	fxch	%st(1)
  7890c8: d9 5e 20                     	fstps	0x20(%esi)
  7890cb: d9 ca                        	fxch	%st(2)
  7890cd: d8 4c 24 08                  	fmuls	0x8(%esp)
  7890d1: d9 ca                        	fxch	%st(2)
  7890d3: d8 4c 24 10                  	fmuls	0x10(%esp)
  7890d7: de c2                        	faddp	%st, %st(2)
  7890d9: d9 c9                        	fxch	%st(1)
  7890db: d9 5e 24                     	fstps	0x24(%esi)
  7890de: d8 4c 24 08                  	fmuls	0x8(%esp)
  7890e2: d9 44 24 18                  	flds	0x18(%esp)
  7890e6: d8 4c 24 10                  	fmuls	0x10(%esp)
  7890ea: de c1                        	faddp	%st, %st(1)
  7890ec: d9 5e 28                     	fstps	0x28(%esi)
  7890ef: d9 44 24 2c                  	flds	0x2c(%esp)
  7890f3: d8 4c 24 08                  	fmuls	0x8(%esp)
  7890f7: d9 44 24 1c                  	flds	0x1c(%esp)
  7890fb: d8 4c 24 10                  	fmuls	0x10(%esp)
  7890ff: de c1                        	faddp	%st, %st(1)
  789101: d9 5e 2c                     	fstps	0x2c(%esi)
  789104: e8 87 f1 ff ff               	calll	0x788290 <.text+0x387290>
  789109: 8b cf                        	movl	%edi, %ecx
  78910b: d9 5c 24 34                  	fstps	0x34(%esp)
  78910f: e8 ac f2 ff ff               	calll	0x7883c0 <.text+0x3873c0>
  789114: d9 06                        	flds	(%esi)
  789116: d9 46 04                     	flds	0x4(%esi)
  789119: 8b 56 08                     	movl	0x8(%esi), %edx
  78911c: d9 44 24 34                  	flds	0x34(%esp)
  789120: 8b 46 0c                     	movl	0xc(%esi), %eax
  789123: d8 4e 10                     	fmuls	0x10(%esi)
  789126: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  78912a: d9 c3                        	fld	%st(3)
  78912c: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  789130: d8 cb                        	fmul	%st(3), %st
  789132: de c1                        	faddp	%st, %st(1)
  789134: d9 1e                        	fstps	(%esi)
  789136: d9 44 24 34                  	flds	0x34(%esp)
  78913a: d8 4e 14                     	fmuls	0x14(%esi)
  78913d: d9 c3                        	fld	%st(3)
  78913f: d8 ca                        	fmul	%st(2), %st
  789141: de c1                        	faddp	%st, %st(1)
  789143: d9 5e 04                     	fstps	0x4(%esi)
  789146: d9 c2                        	fld	%st(2)
  789148: d8 4c 24 18                  	fmuls	0x18(%esp)
  78914c: d9 44 24 34                  	flds	0x34(%esp)
  789150: d8 4e 18                     	fmuls	0x18(%esi)
  789153: de c1                        	faddp	%st, %st(1)
  789155: d9 5e 08                     	fstps	0x8(%esi)
  789158: d9 44 24 34                  	flds	0x34(%esp)
  78915c: d8 4e 1c                     	fmuls	0x1c(%esi)
  78915f: d9 c3                        	fld	%st(3)
  789161: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  789165: de c1                        	faddp	%st, %st(1)
  789167: d9 5e 0c                     	fstps	0xc(%esi)
  78916a: d9 c2                        	fld	%st(2)
  78916c: d8 4e 10                     	fmuls	0x10(%esi)
  78916f: d9 44 24 34                  	flds	0x34(%esp)
  789173: d8 cb                        	fmul	%st(3), %st
  789175: de e9                        	fsubrp	%st, %st(1)
  789177: d9 5e 10                     	fstps	0x10(%esi)
  78917a: d9 c2                        	fld	%st(2)
  78917c: d8 4e 14                     	fmuls	0x14(%esi)
  78917f: d9 44 24 34                  	flds	0x34(%esp)
  789183: d8 ca                        	fmul	%st(2), %st
  789185: de e9                        	fsubrp	%st, %st(1)
  789187: d9 5e 14                     	fstps	0x14(%esi)
  78918a: dd d8                        	fstp	%st(0)
  78918c: dd d8                        	fstp	%st(0)
  78918e: d9 c0                        	fld	%st(0)

Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  789190: d8 4e 18                     	fmuls	0x18(%esi)
  789193: 8b 4e 08                     	movl	0x8(%esi), %ecx
  789196: d9 44 24 34                  	flds	0x34(%esp)
  78919a: 8b 56 0c                     	movl	0xc(%esi), %edx
  78919d: d8 4c 24 18                  	fmuls	0x18(%esp)
  7891a1: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  7891a5: de e9                        	fsubrp	%st, %st(1)
  7891a7: d9 5e 18                     	fstps	0x18(%esi)
  7891aa: 8b 46 18                     	movl	0x18(%esi), %eax
  7891ad: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  7891b1: d8 4e 1c                     	fmuls	0x1c(%esi)
  7891b4: d9 44 24 34                  	flds	0x34(%esp)
  7891b8: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  7891bc: 89 54 24 1c                  	movl	%edx, 0x1c(%esp)
  7891c0: de e9                        	fsubrp	%st, %st(1)
  7891c2: d9 5e 1c                     	fstps	0x1c(%esi)
  7891c5: 8b 4e 1c                     	movl	0x1c(%esi), %ecx
  7891c8: d9 06                        	flds	(%esi)
  7891ca: 89 4c 24 24                  	movl	%ecx, 0x24(%esp)
  7891ce: d9 46 04                     	flds	0x4(%esi)
  7891d1: d9 44 24 10                  	flds	0x10(%esp)
  7891d5: d9 e0                        	fchs
  7891d7: d9 5c 24 34                  	fstps	0x34(%esp)
  7891db: d9 c1                        	fld	%st(1)
  7891dd: d8 4c 24 08                  	fmuls	0x8(%esp)
  7891e1: d9 44 24 34                  	flds	0x34(%esp)
  7891e5: d8 4e 20                     	fmuls	0x20(%esi)
  7891e8: de e9                        	fsubrp	%st, %st(1)
  7891ea: d9 1e                        	fstps	(%esi)
  7891ec: d9 44 24 08                  	flds	0x8(%esp)
  7891f0: d8 c9                        	fmul	%st(1), %st
  7891f2: d9 44 24 34                  	flds	0x34(%esp)
  7891f6: d8 4e 24                     	fmuls	0x24(%esi)
  7891f9: de e9                        	fsubrp	%st, %st(1)
  7891fb: d9 5e 04                     	fstps	0x4(%esi)
  7891fe: d9 44 24 18                  	flds	0x18(%esp)
  789202: d8 4c 24 08                  	fmuls	0x8(%esp)
  789206: d9 44 24 34                  	flds	0x34(%esp)
  78920a: d8 4e 28                     	fmuls	0x28(%esi)
  78920d: de e9                        	fsubrp	%st, %st(1)
  78920f: d9 5e 08                     	fstps	0x8(%esi)
  789212: d9 44 24 1c                  	flds	0x1c(%esp)
  789216: d8 4c 24 08                  	fmuls	0x8(%esp)
  78921a: d9 44 24 34                  	flds	0x34(%esp)
  78921e: d8 4e 2c                     	fmuls	0x2c(%esi)
  789221: de e9                        	fsubrp	%st, %st(1)
  789223: d9 5e 0c                     	fstps	0xc(%esi)
  789226: d9 44 24 34                  	flds	0x34(%esp)
  78922a: d8 ca                        	fmul	%st(2), %st
  78922c: d9 44 24 08                  	flds	0x8(%esp)
  789230: d8 4e 20                     	fmuls	0x20(%esi)
  789233: de c1                        	faddp	%st, %st(1)
  789235: d9 5e 20                     	fstps	0x20(%esi)
  789238: d9 44 24 34                  	flds	0x34(%esp)
  78923c: d8 c9                        	fmul	%st(1), %st
  78923e: d9 44 24 08                  	flds	0x8(%esp)
  789242: d8 4e 24                     	fmuls	0x24(%esi)
  789245: de c1                        	faddp	%st, %st(1)
  789247: d9 5e 24                     	fstps	0x24(%esi)
  78924a: dd d8                        	fstp	%st(0)
  78924c: dd d8                        	fstp	%st(0)
  78924e: d9 44 24 34                  	flds	0x34(%esp)
  789252: d8 4c 24 18                  	fmuls	0x18(%esp)
  789256: d9 44 24 08                  	flds	0x8(%esp)
  78925a: d8 4e 28                     	fmuls	0x28(%esi)
  78925d: de c1                        	faddp	%st, %st(1)
  78925f: d9 5e 28                     	fstps	0x28(%esi)
  789262: d9 44 24 34                  	flds	0x34(%esp)
  789266: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  78926a: d9 44 24 08                  	flds	0x8(%esp)
  78926e: d8 4e 2c                     	fmuls	0x2c(%esi)
  789271: de c1                        	faddp	%st, %st(1)
  789273: d9 5e 2c                     	fstps	0x2c(%esi)
  789276: d9 46 10                     	flds	0x10(%esi)
  789279: d9 46 14                     	flds	0x14(%esi)
  78927c: d9 44 24 14                  	flds	0x14(%esp)
  789280: d8 4e 20                     	fmuls	0x20(%esi)
  789283: d9 c2                        	fld	%st(2)
  789285: d8 4c 24 0c                  	fmuls	0xc(%esp)
  789289: de c1                        	faddp	%st, %st(1)
  78928b: d9 5e 10                     	fstps	0x10(%esi)
  78928e: d9 44 24 14                  	flds	0x14(%esp)
  789292: 5f                           	popl	%edi
  789293: d8 4e 24                     	fmuls	0x24(%esi)
  789296: d9 c1                        	fld	%st(1)
  789298: d8 4c 24 08                  	fmuls	0x8(%esp)
  78929c: de c1                        	faddp	%st, %st(1)
  78929e: d9 5e 14                     	fstps	0x14(%esi)
  7892a1: d9 44 24 1c                  	flds	0x1c(%esp)
  7892a5: d8 4c 24 08                  	fmuls	0x8(%esp)
  7892a9: d9 44 24 10                  	flds	0x10(%esp)
  7892ad: d8 4e 28                     	fmuls	0x28(%esi)
  7892b0: de c1                        	faddp	%st, %st(1)
  7892b2: d9 5e 18                     	fstps	0x18(%esi)
  7892b5: d9 44 24 10                  	flds	0x10(%esp)
  7892b9: d8 4e 2c                     	fmuls	0x2c(%esi)
  7892bc: d9 44 24 20                  	flds	0x20(%esp)
  7892c0: d8 4c 24 08                  	fmuls	0x8(%esp)
  7892c4: de c1                        	faddp	%st, %st(1)
  7892c6: d9 5e 1c                     	fstps	0x1c(%esi)
  7892c9: d9 44 24 08                  	flds	0x8(%esp)
  7892cd: d8 4e 20                     	fmuls	0x20(%esi)
  7892d0: d9 ca                        	fxch	%st(2)
  7892d2: d8 4c 24 10                  	fmuls	0x10(%esp)
  7892d6: de ea                        	fsubrp	%st, %st(2)
  7892d8: d9 c9                        	fxch	%st(1)
  7892da: d9 5e 20                     	fstps	0x20(%esi)
  7892dd: d9 44 24 08                  	flds	0x8(%esp)
  7892e1: d8 4e 24                     	fmuls	0x24(%esi)
  7892e4: d9 c9                        	fxch	%st(1)
  7892e6: d8 4c 24 10                  	fmuls	0x10(%esp)
  7892ea: de e9                        	fsubrp	%st, %st(1)
  7892ec: d9 5e 24                     	fstps	0x24(%esi)
  7892ef: d9 44 24 08                  	flds	0x8(%esp)
  7892f3: d8 4e 28                     	fmuls	0x28(%esi)
  7892f6: d9 44 24 1c                  	flds	0x1c(%esp)
  7892fa: d8 4c 24 10                  	fmuls	0x10(%esp)
  7892fe: de e9                        	fsubrp	%st, %st(1)
  789300: d9 5e 28                     	fstps	0x28(%esi)
  789303: d9 44 24 08                  	flds	0x8(%esp)
  789307: d8 4e 2c                     	fmuls	0x2c(%esi)
  78930a: d9 44 24 20                  	flds	0x20(%esp)
  78930e: d8 4c 24 10                  	fmuls	0x10(%esp)
  789312: de e9                        	fsubrp	%st, %st(1)
  789314: d9 5e 2c                     	fstps	0x2c(%esi)
  789317: 5e                           	popl	%esi
  789318: 83 c4 28                     	addl	$0x28, %esp
  78931b: c2 04 00                     	retl	$0x4
  78931e: cc                           	int3
  78931f: cc                           	int3
  789320: d9 44 24 04                  	flds	0x4(%esp)
  789324: 83 ec 08                     	subl	$0x8, %esp
  789327: dc 1d 48 00 7e 00            	fcompl	0x7e0048
  78932d: d9 44 24 0c                  	flds	0xc(%esp)
  789331: d9 e1                        	fabs
  789333: df e0                        	fnstsw	%ax
  789335: dd 1c 24                     	fstpl	(%esp)
  789338: f6 c4 05                     	testb	$0x5, %ah
  78933b: 7a 0d                        	jp	0x78934a <.text+0x38834a>
  78933d: e8 ae b6 eb ff               	calll	0x6449f0 <.text+0x2439f0>
  789342: d9 e0                        	fchs
  789344: 83 c4 08                     	addl	$0x8, %esp
  789347: c2 04 00                     	retl	$0x4
  78934a: e8 a1 b6 eb ff               	calll	0x6449f0 <.text+0x2439f0>
  78934f: 83 c4 08                     	addl	$0x8, %esp
  789352: c2 04 00                     	retl	$0x4
  789355: cc                           	int3
  789356: cc                           	int3
  789357: cc                           	int3
  789358: cc                           	int3
  789359: cc                           	int3
  78935a: cc                           	int3
  78935b: cc                           	int3
  78935c: cc                           	int3
  78935d: cc                           	int3
  78935e: cc                           	int3
  78935f: cc                           	int3
  789360: 56                           	pushl	%esi
  789361: 57                           	pushl	%edi
  789362: 8b f9                        	movl	%ecx, %edi
  789364: d9 47 08                     	flds	0x8(%edi)
  789367: 8b f2                        	movl	%edx, %esi
  789369: d8 66 08                     	fsubs	0x8(%esi)
  78936c: 51                           	pushl	%ecx
  78936d: d9 47 04                     	flds	0x4(%edi)
  789370: d8 66 04                     	fsubs	0x4(%esi)
  789373: d9 07                        	flds	(%edi)
  789375: d8 26                        	fsubs	(%esi)
  789377: d9 c0                        	fld	%st(0)
  789379: d8 c9                        	fmul	%st(1), %st
  78937b: d9 c2                        	fld	%st(2)
  78937d: d8 cb                        	fmul	%st(3), %st
  78937f: de c1                        	faddp	%st, %st(1)
  789381: d9 c3                        	fld	%st(3)
  789383: d8 cc                        	fmul	%st(4), %st
  789385: de c1                        	faddp	%st, %st(1)
  789387: d9 1c 24                     	fstps	(%esp)
  78938a: dd d8                        	fstp	%st(0)
  78938c: dd d8                        	fstp	%st(0)
  78938e: dd d8                        	fstp	%st(0)
  789390: e8 5b de ff ff               	calll	0x7871f0 <.text+0x3861f0>
  789395: d9 47 0c                     	flds	0xc(%edi)
  789398: 5f                           	popl	%edi
  789399: d8 46 0c                     	fadds	0xc(%esi)
  78939c: 5e                           	popl	%esi
  78939d: d9 c9                        	fxch	%st(1)
  78939f: de d9                        	fcompp
  7893a1: df e0                        	fnstsw	%ax
  7893a3: f6 c4 05                     	testb	$0x5, %ah
  7893a6: 7a 06                        	jp	0x7893ae <.text+0x3883ae>
  7893a8: b8 01 00 00 00               	movl	$0x1, %eax
  7893ad: c3                           	retl
  7893ae: 33 c0                        	xorl	%eax, %eax
  7893b0: c3                           	retl
  7893b1: cc                           	int3
  7893b2: cc                           	int3
  7893b3: cc                           	int3
  7893b4: cc                           	int3
  7893b5: cc                           	int3
  7893b6: cc                           	int3
  7893b7: cc                           	int3
  7893b8: cc                           	int3
  7893b9: cc                           	int3
  7893ba: cc                           	int3
  7893bb: cc                           	int3
  7893bc: cc                           	int3
  7893bd: cc                           	int3
  7893be: cc                           	int3
  7893bf: cc                           	int3
  7893c0: 83 ec 08                     	subl	$0x8, %esp
  7893c3: 53                           	pushl	%ebx
  7893c4: 55                           	pushl	%ebp
  7893c5: 56                           	pushl	%esi
  7893c6: 8b f2                        	movl	%edx, %esi
  7893c8: d9 46 14                     	flds	0x14(%esi)
  7893cb: 57                           	pushl	%edi
  7893cc: d9 46 0c                     	flds	0xc(%esi)
  7893cf: 51                           	pushl	%ecx
  7893d0: d9 c0                        	fld	%st(0)
  7893d2: 8b f9                        	movl	%ecx, %edi
  7893d4: d8 c9                        	fmul	%st(1), %st
  7893d6: d9 c2                        	fld	%st(2)
  7893d8: d8 cb                        	fmul	%st(3), %st
  7893da: de c1                        	faddp	%st, %st(1)
  7893dc: d9 1c 24                     	fstps	(%esp)
  7893df: dd d8                        	fstp	%st(0)
  7893e1: dd d8                        	fstp	%st(0)
  7893e3: e8 08 de ff ff               	calll	0x7871f0 <.text+0x3861f0>
  7893e8: d9 5c 24 10                  	fstps	0x10(%esp)
  7893ec: d9 46 10                     	flds	0x10(%esi)
  7893ef: 51                           	pushl	%ecx
  7893f0: d9 c0                        	fld	%st(0)
  7893f2: d8 c9                        	fmul	%st(1), %st
  7893f4: d9 44 24 14                  	flds	0x14(%esp)
  7893f8: d8 4c 24 14                  	fmuls	0x14(%esp)
  7893fc: de c1                        	faddp	%st, %st(1)
  7893fe: d9 1c 24                     	fstps	(%esp)
  789401: dd d8                        	fstp	%st(0)
  789403: e8 e8 dd ff ff               	calll	0x7871f0 <.text+0x3861f0>
  789408: d9 5c 24 14                  	fstps	0x14(%esp)
  78940c: d9 46 10                     	flds	0x10(%esi)
  78940f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  789415: df e0                        	fnstsw	%ax
  789417: f6 c4 01                     	testb	$0x1, %ah
  78941a: 75 22                        	jne	0x78943e <.text+0x38843e>
  78941c: d9 44 24 10                  	flds	0x10(%esp)
  789420: d8 74 24 14                  	fdivs	0x14(%esp)
  789424: e8 c7 bc eb ff               	calll	0x6450f0 <.text+0x2440f0>
  789429: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  78942f: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  789435: e8 06 af eb ff               	calll	0x644340 <.text+0x243340>
  78943a: 8b e8                        	movl	%eax, %ebp
  78943c: eb 04                        	jmp	0x789442 <.text+0x388442>
  78943e: 8b 6c 24 14                  	movl	0x14(%esp), %ebp
  789442: d9 46 10                     	flds	0x10(%esi)
  789445: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  78944b: df e0                        	fnstsw	%ax
  78944d: f6 c4 05                     	testb	$0x5, %ah
  789450: 7a 20                        	jp	0x789472 <.text+0x388472>
  789452: d9 44 24 10                  	flds	0x10(%esp)
  789456: d8 74 24 14                  	fdivs	0x14(%esp)
  78945a: e8 91 bc eb ff               	calll	0x6450f0 <.text+0x2440f0>
  78945f: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  789465: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  78946b: e8 d0 ae eb ff               	calll	0x644340 <.text+0x243340>
  789470: 8b e8                        	movl	%eax, %ebp
  789472: d9 46 0c                     	flds	0xc(%esi)
  789475: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  78947b: df e0                        	fnstsw	%ax
  78947d: f6 c4 01                     	testb	$0x1, %ah
  789480: 75 21                        	jne	0x7894a3 <.text+0x3884a3>
  789482: d9 46 14                     	flds	0x14(%esi)
  789485: d8 74 24 10                  	fdivs	0x10(%esp)
  789489: e8 62 bc eb ff               	calll	0x6450f0 <.text+0x2440f0>
  78948e: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  789494: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  78949a: e8 a1 ae eb ff               	calll	0x644340 <.text+0x243340>
  78949f: 8b d8                        	movl	%eax, %ebx
  7894a1: eb 04                        	jmp	0x7894a7 <.text+0x3884a7>
  7894a3: 8b 5c 24 14                  	movl	0x14(%esp), %ebx
  7894a7: d9 46 0c                     	flds	0xc(%esi)
  7894aa: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  7894b0: df e0                        	fnstsw	%ax
  7894b2: f6 c4 05                     	testb	$0x5, %ah
  7894b5: 7a 1f                        	jp	0x7894d6 <.text+0x3884d6>
  7894b7: d9 46 14                     	flds	0x14(%esi)
  7894ba: d8 74 24 10                  	fdivs	0x10(%esp)
  7894be: e8 2d bc eb ff               	calll	0x6450f0 <.text+0x2440f0>
  7894c3: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  7894c9: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  7894cf: e8 6c ae eb ff               	calll	0x644340 <.text+0x243340>
  7894d4: 8b d8                        	movl	%eax, %ebx
  7894d6: 85 ff                        	testl	%edi, %edi
  7894d8: 75 06                        	jne	0x7894e0 <.text+0x3884e0>
  7894da: 8b 3d 18 f8 d0 03            	movl	0x3d0f818, %edi
  7894e0: 8b 46 08                     	movl	0x8(%esi), %eax
  7894e3: 8b 4e 04                     	movl	0x4(%esi), %ecx
  7894e6: 8b 16                        	movl	(%esi), %edx
  7894e8: 50                           	pushl	%eax
  7894e9: 51                           	pushl	%ecx
  7894ea: 52                           	pushl	%edx
  7894eb: 8b cf                        	movl	%edi, %ecx
  7894ed: e8 ee b6 ff ff               	calll	0x784be0 <.text+0x383be0>
  7894f2: 8b d3                        	movl	%ebx, %edx
  7894f4: 8b cf                        	movl	%edi, %ecx
  7894f6: e8 25 b9 ff ff               	calll	0x784e20 <.text+0x383e20>
  7894fb: 8b d5                        	movl	%ebp, %edx
  7894fd: 8b cf                        	movl	%edi, %ecx
  7894ff: e8 dc b9 ff ff               	calll	0x784ee0 <.text+0x383ee0>
