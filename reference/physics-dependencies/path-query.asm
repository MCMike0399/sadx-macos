
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  49c210: 8b 44 24 04                  	movl	0x4(%esp), %eax
  49c214: 83 ec 3c                     	subl	$0x3c, %esp
  49c217: 55                           	pushl	%ebp
  49c218: 8b 68 08                     	movl	0x8(%eax), %ebp
  49c21b: 56                           	pushl	%esi
  49c21c: 8d 0c 92                     	leal	(%edx,%edx,4), %ecx
  49c21f: 8d 74 8d 00                  	leal	(%ebp,%ecx,4), %esi
  49c223: 57                           	pushl	%edi
  49c224: b9 05 00 00 00               	movl	$0x5, %ecx
  49c229: 8d 7c 24 20                  	leal	0x20(%esp), %edi
  49c22d: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  49c22f: 8b 74 24 4c                  	movl	0x4c(%esp), %esi
  49c233: 0f b6 0e                     	movzbl	(%esi), %ecx
  49c236: 8d 42 ff                     	leal	-0x1(%edx), %eax
  49c239: 42                           	incl	%edx
  49c23a: 83 e9 00                     	subl	$0x0, %ecx
  49c23d: 74 1a                        	je	0x49c259 <.text+0x9b259>
  49c23f: 49                           	decl	%ecx
  49c240: 75 2a                        	jne	0x49c26c <.text+0x9b26c>
  49c242: 85 c0                        	testl	%eax, %eax
  49c244: 7d 07                        	jge	0x49c24d <.text+0x9b24d>
  49c246: 0f b7 46 02                  	movzwl	0x2(%esi), %eax
  49c24a: 48                           	decl	%eax
  49c24b: eb 1f                        	jmp	0x49c26c <.text+0x9b26c>
  49c24d: 0f b7 4e 02                  	movzwl	0x2(%esi), %ecx
  49c251: 3b d1                        	cmpl	%ecx, %edx
  49c253: 7c 17                        	jl	0x49c26c <.text+0x9b26c>
  49c255: 33 d2                        	xorl	%edx, %edx
  49c257: eb 13                        	jmp	0x49c26c <.text+0x9b26c>
  49c259: 85 c0                        	testl	%eax, %eax
  49c25b: 7d 04                        	jge	0x49c261 <.text+0x9b261>
  49c25d: 33 c0                        	xorl	%eax, %eax
  49c25f: eb 0b                        	jmp	0x49c26c <.text+0x9b26c>
  49c261: 0f b7 4e 02                  	movzwl	0x2(%esi), %ecx
  49c265: 3b d1                        	cmpl	%ecx, %edx
  49c267: 7c 03                        	jl	0x49c26c <.text+0x9b26c>
  49c269: 8d 51 ff                     	leal	-0x1(%ecx), %edx
  49c26c: 8d 04 80                     	leal	(%eax,%eax,4), %eax
  49c26f: 8d 74 85 00                  	leal	(%ebp,%eax,4), %esi
  49c273: b9 05 00 00 00               	movl	$0x5, %ecx
  49c278: 8d 7c 24 0c                  	leal	0xc(%esp), %edi
  49c27c: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  49c27e: 8d 0c 92                     	leal	(%edx,%edx,4), %ecx
  49c281: 8d 74 8d 00                  	leal	(%ebp,%ecx,4), %esi
  49c285: b9 05 00 00 00               	movl	$0x5, %ecx
  49c28a: 8d 7c 24 34                  	leal	0x34(%esp), %edi
  49c28e: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  49c290: 8b 6c 24 20                  	movl	0x20(%esp), %ebp
  49c294: 8b 7c 24 0c                  	movl	0xc(%esp), %edi
  49c298: 0f bf f5                     	movswl	%bp, %esi
  49c29b: 0f bf d7                     	movswl	%di, %edx
  49c29e: 56                           	pushl	%esi
  49c29f: 52                           	pushl	%edx
  49c2a0: e8 fb c0 f9 ff               	calll	0x4383a0 <.text+0x373a0>
  49c2a5: d1 f8                        	sarl	%eax
  49c2a7: 03 c7                        	addl	%edi, %eax
  49c2a9: 0f bf 7c 24 2a               	movswl	0x2a(%esp), %edi
  49c2ae: 66 89 03                     	movw	%ax, (%ebx)
  49c2b1: 0f bf 44 24 16               	movswl	0x16(%esp), %eax
  49c2b6: 57                           	pushl	%edi
  49c2b7: 50                           	pushl	%eax
  49c2b8: e8 e3 c0 f9 ff               	calll	0x4383a0 <.text+0x373a0>
  49c2bd: 8b 4c 24 1e                  	movl	0x1e(%esp), %ecx
  49c2c1: 8b 54 24 3c                  	movl	0x3c(%esp), %edx
  49c2c5: d1 f8                        	sarl	%eax
  49c2c7: 03 c1                        	addl	%ecx, %eax
  49c2c9: 8b 4c 24 38                  	movl	0x38(%esp), %ecx
  49c2cd: 89 4b 08                     	movl	%ecx, 0x8(%ebx)
  49c2d0: 0f bf 4c 24 44               	movswl	0x44(%esp), %ecx
  49c2d5: 66 89 43 02                  	movw	%ax, 0x2(%ebx)
  49c2d9: 8b 44 24 40                  	movl	0x40(%esp), %eax
  49c2dd: 51                           	pushl	%ecx
  49c2de: 56                           	pushl	%esi
  49c2df: 89 53 0c                     	movl	%edx, 0xc(%ebx)
  49c2e2: 89 43 10                     	movl	%eax, 0x10(%ebx)
  49c2e5: e8 b6 c0 f9 ff               	calll	0x4383a0 <.text+0x373a0>
  49c2ea: 0f bf 54 24 4e               	movswl	0x4e(%esp), %edx
  49c2ef: d1 f8                        	sarl	%eax
  49c2f1: 52                           	pushl	%edx
  49c2f2: 03 c5                        	addl	%ebp, %eax
  49c2f4: 57                           	pushl	%edi
  49c2f5: 66 89 43 14                  	movw	%ax, 0x14(%ebx)
  49c2f9: e8 a2 c0 f9 ff               	calll	0x4383a0 <.text+0x373a0>
  49c2fe: 8b 4c 24 42                  	movl	0x42(%esp), %ecx
  49c302: 8b 54 24 64                  	movl	0x64(%esp), %edx
  49c306: 83 c4 20                     	addl	$0x20, %esp
  49c309: d1 f8                        	sarl	%eax
  49c30b: 03 c1                        	addl	%ecx, %eax
  49c30d: 8b 4c 24 40                  	movl	0x40(%esp), %ecx
  49c311: 5f                           	popl	%edi
  49c312: 66 89 43 16                  	movw	%ax, 0x16(%ebx)
  49c316: 8b 44 24 38                  	movl	0x38(%esp), %eax
  49c31a: 5e                           	popl	%esi
  49c31b: 89 43 1c                     	movl	%eax, 0x1c(%ebx)
  49c31e: 89 4b 20                     	movl	%ecx, 0x20(%ebx)
  49c321: 89 53 24                     	movl	%edx, 0x24(%ebx)
  49c324: 5d                           	popl	%ebp
  49c325: 83 c4 3c                     	addl	$0x3c, %esp
  49c328: c3                           	retl
  49c329: 90                           	nop
  49c32a: 90                           	nop
  49c32b: 90                           	nop
  49c32c: 90                           	nop
  49c32d: 90                           	nop
  49c32e: 90                           	nop
  49c32f: 90                           	nop
  49c330: 83 ec 38                     	subl	$0x38, %esp
  49c333: 53                           	pushl	%ebx
  49c334: 8b 5c 24 40                  	movl	0x40(%esp), %ebx
  49c338: 0f b6 03                     	movzbl	(%ebx), %eax
  49c33b: 83 e8 00                     	subl	$0x0, %eax
  49c33e: 56                           	pushl	%esi
  49c33f: 8b 74 24 48                  	movl	0x48(%esp), %esi
  49c343: 74 18                        	je	0x49c35d <.text+0x9b35d>
  49c345: 48                           	decl	%eax
  49c346: 75 48                        	jne	0x49c390 <.text+0x9b390>
  49c348: 8b 43 04                     	movl	0x4(%ebx), %eax
  49c34b: 8b 4e 10                     	movl	0x10(%esi), %ecx
  49c34e: 50                           	pushl	%eax
  49c34f: 51                           	pushl	%ecx
  49c350: e8 6b d0 f9 ff               	calll	0x4393c0 <.text+0x383c0>
  49c355: d9 5e 10                     	fstps	0x10(%esi)
  49c358: 83 c4 08                     	addl	$0x8, %esp
  49c35b: eb 33                        	jmp	0x49c390 <.text+0x9b390>
  49c35d: d9 46 10                     	flds	0x10(%esi)
  49c360: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  49c366: df e0                        	fnstsw	%ax
  49c368: f6 c4 05                     	testb	$0x5, %ah
  49c36b: 0f 8b 5e 02 00 00            	jnp	0x49c5cf <.text+0x9b5cf>
  49c371: d9 46 10                     	flds	0x10(%esi)
  49c374: d8 0d 7c d5 7d 00            	fmuls	0x7dd57c
  49c37a: d9 43 04                     	flds	0x4(%ebx)
  49c37d: d8 0d 7c d5 7d 00            	fmuls	0x7dd57c
  49c383: de d9                        	fcompp
  49c385: df e0                        	fnstsw	%ax
  49c387: f6 c4 05                     	testb	$0x5, %ah
  49c38a: 0f 8b 3f 02 00 00            	jnp	0x49c5cf <.text+0x9b5cf>
  49c390: 0f b7 4b 02                  	movzwl	0x2(%ebx), %ecx
  49c394: 33 d2                        	xorl	%edx, %edx
  49c396: 85 c9                        	testl	%ecx, %ecx
  49c398: 57                           	pushl	%edi
  49c399: 8b 7b 08                     	movl	0x8(%ebx), %edi
  49c39c: 89 54 24 48                  	movl	%edx, 0x48(%esp)
  49c3a0: 7e 43                        	jle	0x49c3e5 <.text+0x9b3e5>
  49c3a2: d9 47 04                     	flds	0x4(%edi)
  49c3a5: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  49c3ab: df e0                        	fnstsw	%ax
  49c3ad: f6 c4 44                     	testb	$0x44, %ah
  49c3b0: 7a 0a                        	jp	0x49c3bc <.text+0x9b3bc>
  49c3b2: c7 44 24 0c 00 00 80 3f      	movl	$0x3f800000, 0xc(%esp)  # imm = 0x3F800000
  49c3ba: eb 07                        	jmp	0x49c3c3 <.text+0x9b3c3>
  49c3bc: 8b 47 04                     	movl	0x4(%edi), %eax
  49c3bf: 89 44 24 0c                  	movl	%eax, 0xc(%esp)
  49c3c3: d9 44 24 0c                  	flds	0xc(%esp)
  49c3c7: d8 44 24 48                  	fadds	0x48(%esp)
  49c3cb: d8 56 10                     	fcoms	0x10(%esi)
  49c3ce: df e0                        	fnstsw	%ax
  49c3d0: f6 c4 41                     	testb	$0x41, %ah
  49c3d3: 74 0e                        	je	0x49c3e3 <.text+0x9b3e3>
  49c3d5: 42                           	incl	%edx
  49c3d6: d9 5c 24 48                  	fstps	0x48(%esp)
  49c3da: 83 c7 14                     	addl	$0x14, %edi
  49c3dd: 3b d1                        	cmpl	%ecx, %edx
  49c3df: 7c c1                        	jl	0x49c3a2 <.text+0x9b3a2>
  49c3e1: eb 02                        	jmp	0x49c3e5 <.text+0x9b3e5>
  49c3e3: dd d8                        	fstp	%st(0)
  49c3e5: 56                           	pushl	%esi
  49c3e6: 53                           	pushl	%ebx
  49c3e7: 8d 5c 24 24                  	leal	0x24(%esp), %ebx
  49c3eb: e8 20 fe ff ff               	calll	0x49c210 <.text+0x9b210>
  49c3f0: d9 46 10                     	flds	0x10(%esi)
  49c3f3: d8 64 24 50                  	fsubs	0x50(%esp)
  49c3f7: 0f bf 0f                     	movswl	(%edi), %ecx
  49c3fa: 0f bf 44 24 38               	movswl	0x38(%esp), %eax
  49c3ff: d8 74 24 14                  	fdivs	0x14(%esp)
  49c403: 89 0e                        	movl	%ecx, (%esi)
  49c405: 0f bf 57 02                  	movswl	0x2(%edi), %edx
  49c409: 0f bf 7c 24 24               	movswl	0x24(%esp), %edi
  49c40e: 50                           	pushl	%eax
  49c40f: 57                           	pushl	%edi
  49c410: 89 56 04                     	movl	%edx, 0x4(%esi)
  49c413: d9 5c 24 58                  	fstps	0x58(%esp)
  49c417: e8 84 bf f9 ff               	calll	0x4383a0 <.text+0x373a0>
  49c41c: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  49c420: db 44 24 1c                  	fildl	0x1c(%esp)
  49c424: d8 4c 24 58                  	fmuls	0x58(%esp)
  49c428: e8 13 7f 1a 00               	calll	0x644340 <.text+0x243340>
  49c42d: 0f bf 4c 24 42               	movswl	0x42(%esp), %ecx
  49c432: 03 c7                        	addl	%edi, %eax
  49c434: 0f bf 7c 24 2e               	movswl	0x2e(%esp), %edi
  49c439: 51                           	pushl	%ecx
  49c43a: 57                           	pushl	%edi
  49c43b: 89 46 08                     	movl	%eax, 0x8(%esi)
  49c43e: e8 5d bf f9 ff               	calll	0x4383a0 <.text+0x373a0>
  49c443: 89 44 24 24                  	movl	%eax, 0x24(%esp)
  49c447: 83 c4 18                     	addl	$0x18, %esp
  49c44a: db 44 24 0c                  	fildl	0xc(%esp)
  49c44e: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c452: e8 e9 7e 1a 00               	calll	0x644340 <.text+0x243340>
  49c457: d9 44 24 38                  	flds	0x38(%esp)
  49c45b: d8 64 24 24                  	fsubs	0x24(%esp)
  49c45f: 8b 4e 04                     	movl	0x4(%esi), %ecx
  49c462: 03 c7                        	addl	%edi, %eax
  49c464: 89 46 0c                     	movl	%eax, 0xc(%esi)
  49c467: d9 5c 24 10                  	fstps	0x10(%esp)
  49c46b: d9 44 24 3c                  	flds	0x3c(%esp)
  49c46f: d8 64 24 28                  	fsubs	0x28(%esp)
  49c473: d9 5c 24 14                  	fstps	0x14(%esp)
  49c477: d9 44 24 40                  	flds	0x40(%esp)
  49c47b: d8 64 24 2c                  	fsubs	0x2c(%esp)
  49c47f: d9 5c 24 18                  	fstps	0x18(%esp)
  49c483: d9 44 24 10                  	flds	0x10(%esp)
  49c487: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c48b: d8 44 24 24                  	fadds	0x24(%esp)
  49c48f: d9 56 20                     	fsts	0x20(%esi)
  49c492: d9 44 24 14                  	flds	0x14(%esp)
  49c496: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c49a: d8 44 24 28                  	fadds	0x28(%esp)
  49c49e: d9 56 24                     	fsts	0x24(%esi)
  49c4a1: d9 44 24 18                  	flds	0x18(%esp)
  49c4a5: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c4a9: d8 44 24 2c                  	fadds	0x2c(%esp)
  49c4ad: d9 54 24 48                  	fsts	0x48(%esp)
  49c4b1: 8b 54 24 48                  	movl	0x48(%esp), %edx
  49c4b5: d9 5e 28                     	fstps	0x28(%esi)
  49c4b8: d9 c9                        	fxch	%st(1)
  49c4ba: 89 56 1c                     	movl	%edx, 0x1c(%esi)
  49c4bd: d9 5e 14                     	fstps	0x14(%esi)
  49c4c0: d9 5e 18                     	fstps	0x18(%esi)
  49c4c3: e8 c8 bd 2e 00               	calll	0x788290 <.text+0x387290>
  49c4c8: 8b 0e                        	movl	(%esi), %ecx
  49c4ca: d9 5c 24 48                  	fstps	0x48(%esp)
  49c4ce: e8 ed be 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49c4d3: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c4d7: 8b 4e 04                     	movl	0x4(%esi), %ecx
  49c4da: d9 e0                        	fchs
  49c4dc: d9 5e 2c                     	fstps	0x2c(%esi)
  49c4df: e8 dc be 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49c4e4: 8b 0e                        	movl	(%esi), %ecx
  49c4e6: d9 5c 24 48                  	fstps	0x48(%esp)
  49c4ea: e8 d1 be 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49c4ef: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c4f3: 8b 0e                        	movl	(%esi), %ecx
  49c4f5: d9 5e 30                     	fstps	0x30(%esi)
  49c4f8: e8 93 bd 2e 00               	calll	0x788290 <.text+0x387290>
  49c4fd: d9 5e 34                     	fstps	0x34(%esi)
  49c500: 8b 4e 0c                     	movl	0xc(%esi), %ecx
  49c503: e8 88 bd 2e 00               	calll	0x788290 <.text+0x387290>
  49c508: d9 5c 24 48                  	fstps	0x48(%esp)
  49c50c: 8b 4e 08                     	movl	0x8(%esi), %ecx
  49c50f: e8 ac be 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49c514: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c518: 8b 4e 0c                     	movl	0xc(%esi), %ecx
  49c51b: d9 e0                        	fchs
  49c51d: d9 5e 38                     	fstps	0x38(%esi)
  49c520: e8 9b be 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49c525: 8b 4e 08                     	movl	0x8(%esi), %ecx
  49c528: d9 5c 24 48                  	fstps	0x48(%esp)
  49c52c: e8 8f be 2e 00               	calll	0x7883c0 <.text+0x3873c0>
  49c531: d8 4c 24 48                  	fmuls	0x48(%esp)
  49c535: 8b 4e 08                     	movl	0x8(%esi), %ecx
  49c538: d9 5e 3c                     	fstps	0x3c(%esi)
  49c53b: e8 50 bd 2e 00               	calll	0x788290 <.text+0x387290>
  49c540: d9 5e 40                     	fstps	0x40(%esi)
  49c543: 5f                           	popl	%edi
  49c544: d9 44 24 14                  	flds	0x14(%esp)
  49c548: d8 4c 24 14                  	fmuls	0x14(%esp)
  49c54c: d9 44 24 10                  	flds	0x10(%esp)
  49c550: d8 4c 24 10                  	fmuls	0x10(%esp)
  49c554: de c1                        	faddp	%st, %st(1)
  49c556: d9 44 24 0c                  	flds	0xc(%esp)
  49c55a: d8 4c 24 0c                  	fmuls	0xc(%esp)
  49c55e: de c1                        	faddp	%st, %st(1)
  49c560: d9 fa                        	fsqrt
  49c562: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  49c568: df e0                        	fnstsw	%ax
  49c56a: f6 c4 44                     	testb	$0x44, %ah
  49c56d: 7a 1c                        	jp	0x49c58b <.text+0x9b58b>
  49c56f: dd d8                        	fstp	%st(0)
  49c571: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  49c579: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  49c581: c7 44 24 0c 00 00 00 00      	movl	$0x0, 0xc(%esp)
  49c589: eb 22                        	jmp	0x49c5ad <.text+0x9b5ad>
  49c58b: d8 3d 10 cb 7d 00            	fdivrs	0x7dcb10
  49c591: d9 44 24 0c                  	flds	0xc(%esp)
  49c595: d8 c9                        	fmul	%st(1), %st
  49c597: d9 5c 24 0c                  	fstps	0xc(%esp)
  49c59b: d9 44 24 10                  	flds	0x10(%esp)
  49c59f: d8 c9                        	fmul	%st(1), %st
  49c5a1: d9 5c 24 10                  	fstps	0x10(%esp)
  49c5a5: d8 4c 24 14                  	fmuls	0x14(%esp)
  49c5a9: d9 5c 24 14                  	fstps	0x14(%esp)
  49c5ad: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  49c5b1: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
  49c5b5: 8b 54 24 14                  	movl	0x14(%esp), %edx
  49c5b9: 83 c6 44                     	addl	$0x44, %esi
  49c5bc: 89 06                        	movl	%eax, (%esi)
  49c5be: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  49c5c1: 89 56 08                     	movl	%edx, 0x8(%esi)
  49c5c4: 5e                           	popl	%esi
  49c5c5: b8 01 00 00 00               	movl	$0x1, %eax
  49c5ca: 5b                           	popl	%ebx
  49c5cb: 83 c4 38                     	addl	$0x38, %esp
  49c5ce: c3                           	retl
  49c5cf: 5e                           	popl	%esi
  49c5d0: 33 c0                        	xorl	%eax, %eax
  49c5d2: 5b                           	popl	%ebx
  49c5d3: 83 c4 38                     	addl	$0x38, %esp
  49c5d6: c3                           	retl
  49c5d7: 90                           	nop
  49c5d8: 90                           	nop
  49c5d9: 90                           	nop
  49c5da: 90                           	nop
  49c5db: 90                           	nop
  49c5dc: 90                           	nop
  49c5dd: 90                           	nop
  49c5de: 90                           	nop
  49c5df: 90                           	nop
  49c5e0: 8b 44 24 04                  	movl	0x4(%esp), %eax
  49c5e4: 8b 48 08                     	movl	0x8(%eax), %ecx
  49c5e7: 0f b7 50 02                  	movzwl	0x2(%eax), %edx
  49c5eb: d9 41 04                     	flds	0x4(%ecx)
  49c5ee: 56                           	pushl	%esi
  49c5ef: 8d 71 04                     	leal	0x4(%ecx), %esi
  49c5f2: 33 c9                        	xorl	%ecx, %ecx
  49c5f4: 85 d2                        	testl	%edx, %edx
  49c5f6: 76 27                        	jbe	0x49c61f <.text+0x9b61f>
  49c5f8: d9 44 24 0c                  	flds	0xc(%esp)
  49c5fc: d8 d9                        	fcomp	%st(1)
  49c5fe: df e0                        	fnstsw	%ax
  49c600: f6 c4 41                     	testb	$0x41, %ah
  49c603: 75 0b                        	jne	0x49c610 <.text+0x9b610>
  49c605: d8 46 14                     	fadds	0x14(%esi)
  49c608: 83 c6 14                     	addl	$0x14, %esi
  49c60b: 41                           	incl	%ecx
  49c60c: 3b ca                        	cmpl	%edx, %ecx
  49c60e: 72 e8                        	jb	0x49c5f8 <.text+0x9b5f8>
  49c610: 8b 44 24 10                  	movl	0x10(%esp), %eax
  49c614: dd d8                        	fstp	%st(0)
  49c616: 89 08                        	movl	%ecx, (%eax)
  49c618: b8 01 00 00 00               	movl	$0x1, %eax
  49c61d: 5e                           	popl	%esi
  49c61e: c3                           	retl
  49c61f: 8b 54 24 10                  	movl	0x10(%esp), %edx
  49c623: dd d8                        	fstp	%st(0)
  49c625: 89 0a                        	movl	%ecx, (%edx)
  49c627: b8 01 00 00 00               	movl	$0x1, %eax
  49c62c: 5e                           	popl	%esi
  49c62d: c3                           	retl
  49c62e: 90                           	nop
  49c62f: 90                           	nop
