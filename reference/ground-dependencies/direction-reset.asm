
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43c130: 81 ec 8c 00 00 00            	subl	$0x8c, %esp
  43c136: 56                           	pushl	%esi
  43c137: 8b f0                        	movl	%eax, %esi
  43c139: d9 46 38                     	flds	0x38(%esi)
  43c13c: d8 9e d0 00 00 00            	fcomps	0xd0(%esi)
  43c142: df e0                        	fnstsw	%ax
  43c144: f6 c4 05                     	testb	$0x5, %ah
  43c147: 7a 1a                        	jp	0x43c163 <.text+0x3b163>
  43c149: 6a 00                        	pushl	$0x0
  43c14b: 6a 00                        	pushl	$0x0
  43c14d: 57                           	pushl	%edi
  43c14e: e8 6d fc ff ff               	calll	0x43bdc0 <.text+0x3adc0>
  43c153: 83 c4 0c                     	addl	$0xc, %esp
  43c156: 85 c0                        	testl	%eax, %eax
  43c158: 74 09                        	je	0x43c163 <.text+0x3b163>
  43c15a: 8b 86 d0 00 00 00            	movl	0xd0(%esi), %eax
  43c160: 89 46 38                     	movl	%eax, 0x38(%esi)
  43c163: d9 46 38                     	flds	0x38(%esi)
  43c166: 8d 4c 24 40                  	leal	0x40(%esp), %ecx
  43c16a: d8 46 18                     	fadds	0x18(%esi)
  43c16d: 51                           	pushl	%ecx
  43c16e: d9 56 18                     	fsts	0x18(%esi)
  43c171: 8b 57 10                     	movl	0x10(%edi), %edx
  43c174: 52                           	pushl	%edx
  43c175: d9 5c 24 58                  	fstps	0x58(%esp)
  43c179: e8 b2 01 06 00               	calll	0x49c330 <.text+0x9b330>
  43c17e: 83 c4 08                     	addl	$0x8, %esp
  43c181: 85 c0                        	testl	%eax, %eax
  43c183: 0f 84 2a 01 00 00            	je	0x43c2b3 <.text+0x3b2b3>
  43c189: 8b 46 64                     	movl	0x64(%esi), %eax
  43c18c: d9 44 24 60                  	flds	0x60(%esp)
  43c190: 8b 48 1c                     	movl	0x1c(%eax), %ecx
  43c193: d8 67 20                     	fsubs	0x20(%edi)
  43c196: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  43c19a: 8b 50 20                     	movl	0x20(%eax), %edx
  43c19d: 89 54 24 2c                  	movl	%edx, 0x2c(%esp)
  43c1a1: 8b 40 24                     	movl	0x24(%eax), %eax
  43c1a4: d9 5c 24 04                  	fstps	0x4(%esp)
  43c1a8: d9 44 24 64                  	flds	0x64(%esp)
  43c1ac: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c1b0: d8 67 24                     	fsubs	0x24(%edi)
  43c1b3: 51                           	pushl	%ecx
  43c1b4: 8d 54 24 20                  	leal	0x20(%esp), %edx
  43c1b8: 8d 4c 24 08                  	leal	0x8(%esp), %ecx
  43c1bc: d9 5c 24 0c                  	fstps	0xc(%esp)
  43c1c0: 89 44 24 34                  	movl	%eax, 0x34(%esp)
  43c1c4: d9 44 24 6c                  	flds	0x6c(%esp)
  43c1c8: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%esp)
  43c1d0: d8 67 28                     	fsubs	0x28(%edi)
  43c1d3: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  43c1db: c7 44 24 20 00 00 00 00      	movl	$0x0, 0x20(%esp)
  43c1e3: d9 5c 24 10                  	fstps	0x10(%esp)
  43c1e7: e8 54 cb 34 00               	calll	0x788d40 <.text+0x387d40>
  43c1ec: 8d 53 04                     	leal	0x4(%ebx), %edx
  43c1ef: dd d8                        	fstp	%st(0)
  43c1f1: 8b 02                        	movl	(%edx), %eax
  43c1f3: 8b 4a 04                     	movl	0x4(%edx), %ecx
  43c1f6: 8b 52 08                     	movl	0x8(%edx), %edx
  43c1f9: 89 44 24 04                  	movl	%eax, 0x4(%esp)
  43c1fd: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43c201: 89 4c 24 08                  	movl	%ecx, 0x8(%esp)
  43c205: 89 54 24 0c                  	movl	%edx, 0xc(%esp)
  43c209: 50                           	pushl	%eax
  43c20a: 8d 54 24 20                  	leal	0x20(%esp), %edx
  43c20e: 8d 4c 24 08                  	leal	0x8(%esp), %ecx
  43c212: e8 29 cb 34 00               	calll	0x788d40 <.text+0x387d40>
  43c217: dd d8                        	fstp	%st(0)
  43c219: 8d 4c 24 34                  	leal	0x34(%esp), %ecx
  43c21d: e8 9e ba 34 00               	calll	0x787cc0 <.text+0x386cc0>
  43c222: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  43c228: df e0                        	fnstsw	%ax
  43c22a: f6 c4 44                     	testb	$0x44, %ah
  43c22d: 0f 8b 80 00 00 00            	jnp	0x43c2b3 <.text+0x3b2b3>
  43c233: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c237: e8 84 ba 34 00               	calll	0x787cc0 <.text+0x386cc0>
  43c23c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  43c242: df e0                        	fnstsw	%ax
  43c244: f6 c4 44                     	testb	$0x44, %ah
  43c247: 7b 6a                        	jnp	0x43c2b3 <.text+0x3b2b3>
  43c249: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c24d: 51                           	pushl	%ecx
  43c24e: e8 6d c8 fd ff               	calll	0x418ac0 <.text+0x17ac0>
  43c253: dd d8                        	fstp	%st(0)
  43c255: 8d 54 24 38                  	leal	0x38(%esp), %edx
  43c259: 52                           	pushl	%edx
  43c25a: e8 61 c8 fd ff               	calll	0x418ac0 <.text+0x17ac0>
  43c25f: dd d8                        	fstp	%st(0)
  43c261: 8b 46 64                     	movl	0x64(%esi), %eax
  43c264: 83 c0 1c                     	addl	$0x1c, %eax
  43c267: 50                           	pushl	%eax
  43c268: 8d 4c 24 40                  	leal	0x40(%esp), %ecx
  43c26c: 51                           	pushl	%ecx
  43c26d: 8d 54 24 20                  	leal	0x20(%esp), %edx
  43c271: 52                           	pushl	%edx
  43c272: e8 99 c1 ff ff               	calll	0x438410 <.text+0x37410>
  43c277: 8b 77 18                     	movl	0x18(%edi), %esi
  43c27a: 03 f0                        	addl	%eax, %esi
  43c27c: 8b 43 20                     	movl	0x20(%ebx), %eax
  43c27f: 50                           	pushl	%eax
  43c280: 81 e6 ff ff 00 00            	andl	$0xffff, %esi           # imm = 0xFFFF
  43c286: 56                           	pushl	%esi
  43c287: e8 24 c1 ff ff               	calll	0x4383b0 <.text+0x373b0>
  43c28c: 83 c4 1c                     	addl	$0x1c, %esp
  43c28f: 3d 00 40 00 00               	cmpl	$0x4000, %eax           # imm = 0x4000
  43c294: 7e 0f                        	jle	0x43c2a5 <.text+0x3b2a5>
  43c296: b9 00 80 ff ff               	movl	$0xffff8000, %ecx       # imm = 0xFFFF8000
  43c29b: 2b ce                        	subl	%esi, %ecx
  43c29d: 81 e1 ff ff 00 00            	andl	$0xffff, %ecx           # imm = 0xFFFF
  43c2a3: 8b f1                        	movl	%ecx, %esi
  43c2a5: 89 73 20                     	movl	%esi, 0x20(%ebx)
  43c2a8: 89 77 18                     	movl	%esi, 0x18(%edi)
  43c2ab: 5e                           	popl	%esi
  43c2ac: 81 c4 8c 00 00 00            	addl	$0x8c, %esp
  43c2b2: c3                           	retl
  43c2b3: 80 67 05 df                  	andb	$-0x21, 0x5(%edi)
  43c2b7: 5e                           	popl	%esi
  43c2b8: 81 c4 8c 00 00 00            	addl	$0x8c, %esp
  43c2be: c3                           	retl
  43c2bf: 90                           	nop
