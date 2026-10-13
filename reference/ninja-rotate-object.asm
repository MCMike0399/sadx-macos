
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  7931a0: 83 ec 14                     	subl	$0x14, %esp
  7931a3: 85 d2                        	testl	%edx, %edx
  7931a5: 56                           	pushl	%esi
  7931a6: 8b 35 18 f8 d0 03            	movl	0x3d0f818, %esi
  7931ac: 57                           	pushl	%edi
  7931ad: 8b f9                        	movl	%ecx, %edi
  7931af: 0f 84 28 02 00 00            	je	0x7933dd <.text+0x3923dd>
  7931b5: 8b 4f 04                     	movl	0x4(%edi), %ecx
  7931b8: 85 c9                        	testl	%ecx, %ecx
  7931ba: 0f 84 ab 00 00 00            	je	0x79326b <.text+0x39226b>
  7931c0: e8 cb 50 ff ff               	calll	0x788290 <.text+0x387290>
  7931c5: d9 5c 24 08                  	fstps	0x8(%esp)
  7931c9: 8b 4f 04                     	movl	0x4(%edi), %ecx
  7931cc: e8 ef 51 ff ff               	calll	0x7883c0 <.text+0x3873c0>
  7931d1: d9 06                        	flds	(%esi)
  7931d3: d9 46 04                     	flds	0x4(%esi)
  7931d6: 8b 46 08                     	movl	0x8(%esi), %eax
  7931d9: d9 c1                        	fld	%st(1)
  7931db: 8b 4e 0c                     	movl	0xc(%esi), %ecx
  7931de: d8 cb                        	fmul	%st(3), %st
  7931e0: 89 44 24 0c                  	movl	%eax, 0xc(%esp)
  7931e4: d9 44 24 08                  	flds	0x8(%esp)
  7931e8: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  7931ec: d8 4e 20                     	fmuls	0x20(%esi)
  7931ef: de e9                        	fsubrp	%st, %st(1)
  7931f1: d9 1e                        	fstps	(%esi)
  7931f3: d9 c0                        	fld	%st(0)
  7931f5: d8 cb                        	fmul	%st(3), %st
  7931f7: d9 44 24 08                  	flds	0x8(%esp)
  7931fb: d8 4e 24                     	fmuls	0x24(%esi)
  7931fe: de e9                        	fsubrp	%st, %st(1)
  793200: d9 5e 04                     	fstps	0x4(%esi)
  793203: d9 44 24 0c                  	flds	0xc(%esp)
  793207: d8 cb                        	fmul	%st(3), %st
  793209: d9 44 24 08                  	flds	0x8(%esp)
  79320d: d8 4e 28                     	fmuls	0x28(%esi)
  793210: de e9                        	fsubrp	%st, %st(1)
  793212: d9 5e 08                     	fstps	0x8(%esi)
  793215: d9 44 24 10                  	flds	0x10(%esp)
  793219: d8 cb                        	fmul	%st(3), %st
  79321b: d9 44 24 08                  	flds	0x8(%esp)
  79321f: d8 4e 2c                     	fmuls	0x2c(%esi)
  793222: de e9                        	fsubrp	%st, %st(1)
  793224: d9 5e 0c                     	fstps	0xc(%esi)
  793227: d9 c2                        	fld	%st(2)
  793229: d8 4e 20                     	fmuls	0x20(%esi)
  79322c: d9 ca                        	fxch	%st(2)
  79322e: d8 4c 24 08                  	fmuls	0x8(%esp)
  793232: de c2                        	faddp	%st, %st(2)
  793234: d9 c9                        	fxch	%st(1)
  793236: d9 5e 20                     	fstps	0x20(%esi)
  793239: d9 c1                        	fld	%st(1)
  79323b: d8 4e 24                     	fmuls	0x24(%esi)
  79323e: d9 c9                        	fxch	%st(1)
  793240: d8 4c 24 08                  	fmuls	0x8(%esp)
  793244: de c1                        	faddp	%st, %st(1)
  793246: d9 5e 24                     	fstps	0x24(%esi)
  793249: d9 c0                        	fld	%st(0)
  79324b: d8 4e 28                     	fmuls	0x28(%esi)
  79324e: d9 44 24 0c                  	flds	0xc(%esp)
  793252: d8 4c 24 08                  	fmuls	0x8(%esp)
  793256: de c1                        	faddp	%st, %st(1)
  793258: d9 5e 28                     	fstps	0x28(%esi)
  79325b: d8 4e 2c                     	fmuls	0x2c(%esi)
  79325e: d9 44 24 10                  	flds	0x10(%esp)
