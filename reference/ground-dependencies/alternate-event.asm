
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  447510: 83 ec 24                     	subl	$0x24, %esp
  447513: d9 05 f8 f0 b0 03            	flds	0x3b0f0f8
  447519: 53                           	pushl	%ebx
  44751a: 55                           	pushl	%ebp
  44751b: 8b 6c 24 34                  	movl	0x34(%esp), %ebp
  44751f: 56                           	pushl	%esi
  447520: 8b 74 24 3c                  	movl	0x3c(%esp), %esi
  447524: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  44752a: 8d 45 10                     	leal	0x10(%ebp), %eax
  44752d: 8b 08                        	movl	(%eax), %ecx
  44752f: 8b 50 04                     	movl	0x4(%eax), %edx
  447532: 8b 40 08                     	movl	0x8(%eax), %eax
  447535: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  447539: d8 44 24 0c                  	fadds	0xc(%esp)
  44753d: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  447541: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  447545: d9 5c 24 0c                  	fstps	0xc(%esp)
  447549: 57                           	pushl	%edi
  44754a: d9 05 fc f0 b0 03            	flds	0x3b0f0fc
  447550: 8b 7c 24 38                  	movl	0x38(%esp), %edi
  447554: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  44755a: 8b 5f 3c                     	movl	0x3c(%edi), %ebx
  44755d: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  447565: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44756d: d8 44 24 14                  	fadds	0x14(%esp)
  447571: d9 5c 24 14                  	fstps	0x14(%esp)
  447575: d9 05 00 f1 b0 03            	flds	0x3b0f100
  44757b: d8 8e 14 01 00 00            	fmuls	0x114(%esi)
  447581: d8 44 24 18                  	fadds	0x18(%esp)
  447585: d9 5c 24 18                  	fstps	0x18(%esp)
  447589: 0f b6 43 24                  	movzbl	0x24(%ebx), %eax
  44758d: 48                           	decl	%eax
  44758e: 83 f8 03                     	cmpl	$0x3, %eax
  447591: 0f 87 15 02 00 00            	ja	0x4477ac <.text+0x467ac>
  447597: ff 24 85 80 7b 44 00         	jmpl	*0x447b80(,%eax,4)
  44759e: 8d 4b 28                     	leal	0x28(%ebx), %ecx
  4475a1: 8b 11                        	movl	(%ecx), %edx
  4475a3: 89 54 24 28                  	movl	%edx, 0x28(%esp)
  4475a7: 8b 41 04                     	movl	0x4(%ecx), %eax
  4475aa: 89 44 24 2c                  	movl	%eax, 0x2c(%esp)
  4475ae: 8b 49 08                     	movl	0x8(%ecx), %ecx
  4475b1: 89 4c 24 30                  	movl	%ecx, 0x30(%esp)
  4475b5: 8d 57 20                     	leal	0x20(%edi), %edx
  4475b8: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  4475bc: e8 4f 00 34 00               	calll	0x787610 <.text+0x386610>
  4475c1: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  4475c5: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  4475cd: e8 ee 06 34 00               	calll	0x787cc0 <.text+0x386cc0>
  4475d2: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  4475d8: df e0                        	fnstsw	%ax
  4475da: f6 c4 41                     	testb	$0x41, %ah
  4475dd: 7a 14                        	jp	0x4475f3 <.text+0x465f3>
  4475df: 33 c0                        	xorl	%eax, %eax
  4475e1: 89 46 40                     	movl	%eax, 0x40(%esi)
  4475e4: 89 46 3c                     	movl	%eax, 0x3c(%esi)
  4475e7: 89 46 38                     	movl	%eax, 0x38(%esi)
  4475ea: c6 43 24 01                  	movb	$0x1, 0x24(%ebx)
  4475ee: e9 bd 01 00 00               	jmp	0x4477b0 <.text+0x467b0>
  4475f3: d9 44 24 30                  	flds	0x30(%esp)
  4475f7: d9 44 24 28                  	flds	0x28(%esp)
  4475fb: d9 f3                        	fpatan
  4475fd: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  447603: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  447609: e8 32 cd 1f 00               	calll	0x644340 <.text+0x243340>
  44760e: 25 ff ff 00 00               	andl	$0xffff, %eax           # imm = 0xFFFF
  447613: 89 45 20                     	movl	%eax, 0x20(%ebp)
  447616: d9 43 34                     	flds	0x34(%ebx)
  447619: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44761f: df e0                        	fnstsw	%ax
  447621: f6 c4 44                     	testb	$0x44, %ah
  447624: 7a 08                        	jp	0x44762e <.text+0x4662e>
  447626: dd d8                        	fstp	%st(0)
  447628: d9 86 c4 00 00 00            	flds	0xc4(%esi)
  44762e: d9 43 38                     	flds	0x38(%ebx)
  447631: d9 54 24 38                  	fsts	0x38(%esp)
  447635: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44763b: df e0                        	fnstsw	%ax
  44763d: f6 c4 44                     	testb	$0x44, %ah
  447640: 7a 0a                        	jp	0x44764c <.text+0x4664c>
  447642: 8b 96 d8 00 00 00            	movl	0xd8(%esi), %edx
  447648: 89 54 24 38                  	movl	%edx, 0x38(%esp)
  44764c: d8 66 38                     	fsubs	0x38(%esi)
  44764f: d9 54 24 1c                  	fsts	0x1c(%esp)
  447653: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447659: df e0                        	fnstsw	%ax
  44765b: f6 c4 41                     	testb	$0x41, %ah
  44765e: 0f 85 c6 00 00 00            	jne	0x44772a <.text+0x4672a>
  447664: d9 44 24 1c                  	flds	0x1c(%esp)
  447668: d8 5c 24 38                  	fcomps	0x38(%esp)
  44766c: df e0                        	fnstsw	%ax
  44766e: f6 c4 41                     	testb	$0x41, %ah
  447671: 0f 85 39 01 00 00            	jne	0x4477b0 <.text+0x467b0>
  447677: 8b 44 24 38                  	movl	0x38(%esp), %eax
  44767b: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  44767f: e9 2c 01 00 00               	jmp	0x4477b0 <.text+0x467b0>
  447684: 8d 4b 28                     	leal	0x28(%ebx), %ecx
  447687: 8b 11                        	movl	(%ecx), %edx
  447689: 89 54 24 28                  	movl	%edx, 0x28(%esp)
  44768d: 8b 41 04                     	movl	0x4(%ecx), %eax
  447690: 89 44 24 2c                  	movl	%eax, 0x2c(%esp)
  447694: 8b 49 08                     	movl	0x8(%ecx), %ecx
  447697: 89 4c 24 30                  	movl	%ecx, 0x30(%esp)
  44769b: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  44769f: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  4476a7: e8 14 06 34 00               	calll	0x787cc0 <.text+0x386cc0>
  4476ac: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  4476b2: df e0                        	fnstsw	%ax
  4476b4: f6 c4 44                     	testb	$0x44, %ah
  4476b7: 7a 16                        	jp	0x4476cf <.text+0x466cf>
  4476b9: 33 c0                        	xorl	%eax, %eax
  4476bb: dd d8                        	fstp	%st(0)
  4476bd: 89 46 40                     	movl	%eax, 0x40(%esi)
  4476c0: 89 46 3c                     	movl	%eax, 0x3c(%esi)
  4476c3: 89 46 38                     	movl	%eax, 0x38(%esi)
  4476c6: c6 43 24 01                  	movb	$0x1, 0x24(%ebx)
  4476ca: e9 e1 00 00 00               	jmp	0x4477b0 <.text+0x467b0>
  4476cf: d9 44 24 30                  	flds	0x30(%esp)
  4476d3: d9 44 24 28                  	flds	0x28(%esp)
  4476d7: d9 f3                        	fpatan
  4476d9: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  4476df: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  4476e5: e8 56 cc 1f 00               	calll	0x644340 <.text+0x243340>
  4476ea: 25 ff ff 00 00               	andl	$0xffff, %eax           # imm = 0xFFFF
  4476ef: 89 45 20                     	movl	%eax, 0x20(%ebp)
  4476f2: d8 66 38                     	fsubs	0x38(%esi)
  4476f5: d9 54 24 1c                  	fsts	0x1c(%esp)
  4476f9: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4476ff: df e0                        	fnstsw	%ax
  447701: f6 c4 41                     	testb	$0x41, %ah
  447704: 75 24                        	jne	0x44772a <.text+0x4672a>
  447706: d9 44 24 1c                  	flds	0x1c(%esp)
  44770a: d8 9e d8 00 00 00            	fcomps	0xd8(%esi)
  447710: df e0                        	fnstsw	%ax
  447712: f6 c4 41                     	testb	$0x41, %ah
  447715: 0f 85 95 00 00 00            	jne	0x4477b0 <.text+0x467b0>
  44771b: 8b 96 d8 00 00 00            	movl	0xd8(%esi), %edx
  447721: 89 54 24 1c                  	movl	%edx, 0x1c(%esp)
  447725: e9 86 00 00 00               	jmp	0x4477b0 <.text+0x467b0>
  44772a: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  447732: eb 7c                        	jmp	0x4477b0 <.text+0x467b0>
  447734: 8d 43 28                     	leal	0x28(%ebx), %eax
  447737: 8b 08                        	movl	(%eax), %ecx
  447739: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  44773d: 8b 50 04                     	movl	0x4(%eax), %edx
  447740: 89 54 24 2c                  	movl	%edx, 0x2c(%esp)
  447744: 8b 40 08                     	movl	0x8(%eax), %eax
  447747: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  44774b: 89 44 24 30                  	movl	%eax, 0x30(%esp)
  44774f: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  447757: e8 64 05 34 00               	calll	0x787cc0 <.text+0x386cc0>
  44775c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447762: df e0                        	fnstsw	%ax
  447764: f6 c4 44                     	testb	$0x44, %ah
  447767: 7a 11                        	jp	0x44777a <.text+0x4677a>
  447769: 33 c0                        	xorl	%eax, %eax
  44776b: 89 46 40                     	movl	%eax, 0x40(%esi)
  44776e: 89 46 3c                     	movl	%eax, 0x3c(%esi)
  447771: 89 46 38                     	movl	%eax, 0x38(%esi)
  447774: c6 43 24 01                  	movb	$0x1, 0x24(%ebx)
  447778: eb 36                        	jmp	0x4477b0 <.text+0x467b0>
  44777a: d9 44 24 30                  	flds	0x30(%esp)
  44777e: d9 44 24 28                  	flds	0x28(%esp)
  447782: d9 f3                        	fpatan
  447784: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  44778a: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  447790: e8 ab cb 1f 00               	calll	0x644340 <.text+0x243340>
  447795: 25 ff ff 00 00               	andl	$0xffff, %eax           # imm = 0xFFFF
  44779a: 8d 54 24 28                  	leal	0x28(%esp), %edx
  44779e: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  4477a2: 89 45 20                     	movl	%eax, 0x20(%ebp)
  4477a5: e8 c6 d9 33 00               	calll	0x785170 <.text+0x384170>
  4477aa: eb 04                        	jmp	0x4477b0 <.text+0x467b0>
  4477ac: 80 67 05 bf                  	andb	$-0x41, 0x5(%edi)
  4477b0: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  4477b4: 51                           	pushl	%ecx
  4477b5: 57                           	pushl	%edi
  4477b6: e8 45 74 ff ff               	calll	0x43ec00 <.text+0x3dc00>
  4477bb: d9 46 38                     	flds	0x38(%esi)
  4477be: d8 9e ac 00 00 00            	fcomps	0xac(%esi)
  4477c4: 83 c4 08                     	addl	$0x8, %esp
  4477c7: d9 46 38                     	flds	0x38(%esi)
  4477ca: df e0                        	fnstsw	%ax
  4477cc: f6 c4 41                     	testb	$0x41, %ah
  4477cf: 75 0e                        	jne	0x4477df <.text+0x467df>
  4477d1: d8 a6 ac 00 00 00            	fsubs	0xac(%esi)
  4477d7: d8 8e f0 00 00 00            	fmuls	0xf0(%esi)
  4477dd: eb 16                        	jmp	0x4477f5 <.text+0x467f5>
  4477df: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4477e5: df e0                        	fnstsw	%ax
  4477e7: f6 c4 05                     	testb	$0x5, %ah
  4477ea: 7a 11                        	jp	0x4477fd <.text+0x467fd>
  4477ec: d9 86 f0 00 00 00            	flds	0xf0(%esi)
  4477f2: d8 4e 38                     	fmuls	0x38(%esi)
  4477f5: d8 44 24 10                  	fadds	0x10(%esp)
  4477f9: d9 5c 24 10                  	fstps	0x10(%esp)
  4477fd: d9 86 f4 00 00 00            	flds	0xf4(%esi)
  447803: 8b 55 20                     	movl	0x20(%ebp), %edx
  447806: d8 4e 3c                     	fmuls	0x3c(%esi)
  447809: 52                           	pushl	%edx
  44780a: 55                           	pushl	%ebp
  44780b: 8b c6                        	movl	%esi, %eax
  44780d: d8 44 24 1c                  	fadds	0x1c(%esp)
  447811: d9 5c 24 1c                  	fstps	0x1c(%esp)
  447815: d9 86 f8 00 00 00            	flds	0xf8(%esi)
  44781b: d8 4e 40                     	fmuls	0x40(%esi)
  44781e: d8 44 24 20                  	fadds	0x20(%esp)
  447822: d9 5c 24 20                  	fstps	0x20(%esp)
  447826: e8 c5 c5 ff ff               	calll	0x443df0 <.text+0x42df0>
  44782b: d9 46 38                     	flds	0x38(%esi)
  44782e: 83 c4 08                     	addl	$0x8, %esp
  447831: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447837: df e0                        	fnstsw	%ax
  447839: f6 c4 44                     	testb	$0x44, %ah
  44783c: 0f 8a a6 00 00 00            	jp	0x4478e8 <.text+0x468e8>
  447842: d9 86 04 01 00 00            	flds	0x104(%esi)
  447848: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44784b: d8 4c 24 14                  	fmuls	0x14(%esp)
  44784f: d9 5c 24 38                  	fstps	0x38(%esp)
  447853: d9 44 24 10                  	flds	0x10(%esp)
  447857: d8 44 24 1c                  	fadds	0x1c(%esp)
  44785b: d9 5c 24 10                  	fstps	0x10(%esp)
  44785f: d9 44 24 1c                  	flds	0x1c(%esp)
  447863: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447869: df e0                        	fnstsw	%ax
  44786b: f6 c4 44                     	testb	$0x44, %ah
  44786e: 0f 8a fa 01 00 00            	jp	0x447a6e <.text+0x46a6e>
  447874: d9 44 24 10                  	flds	0x10(%esp)
  447878: d8 5c 24 38                  	fcomps	0x38(%esp)
  44787c: df e0                        	fnstsw	%ax
  44787e: f6 c4 05                     	testb	$0x5, %ah
  447881: 7a 19                        	jp	0x44789c <.text+0x4689c>
  447883: d9 44 24 38                  	flds	0x38(%esp)
  447887: d9 e0                        	fchs
  447889: d8 5c 24 10                  	fcomps	0x10(%esp)
  44788d: df e0                        	fnstsw	%ax
  44788f: f6 c4 05                     	testb	$0x5, %ah
  447892: 7a 08                        	jp	0x44789c <.text+0x4689c>
  447894: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  44789c: d9 44 24 1c                  	flds	0x1c(%esp)
  4478a0: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4478a6: df e0                        	fnstsw	%ax
  4478a8: f6 c4 44                     	testb	$0x44, %ah
  4478ab: 0f 8a bd 01 00 00            	jp	0x447a6e <.text+0x46a6e>
  4478b1: d9 44 24 10                  	flds	0x10(%esp)
  4478b5: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  4478bb: df e0                        	fnstsw	%ax
  4478bd: f6 c4 05                     	testb	$0x5, %ah
  4478c0: 0f 8a a8 01 00 00            	jp	0x447a6e <.text+0x46a6e>
  4478c6: d9 44 24 10                  	flds	0x10(%esp)
  4478ca: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  4478d0: df e0                        	fnstsw	%ax
  4478d2: f6 c4 41                     	testb	$0x41, %ah
  4478d5: 0f 85 93 01 00 00            	jne	0x447a6e <.text+0x46a6e>
  4478db: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  4478e3: e9 86 01 00 00               	jmp	0x447a6e <.text+0x46a6e>
  4478e8: d9 44 24 1c                  	flds	0x1c(%esp)
  4478ec: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4478f2: df e0                        	fnstsw	%ax
  4478f4: f6 c4 05                     	testb	$0x5, %ah
  4478f7: 7a 6b                        	jp	0x447964 <.text+0x46964>
  4478f9: d9 46 38                     	flds	0x38(%esi)
  4478fc: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447902: df e0                        	fnstsw	%ax
  447904: f6 c4 41                     	testb	$0x41, %ah
  447907: 75 2c                        	jne	0x447935 <.text+0x46935>
  447909: d9 44 24 10                  	flds	0x10(%esp)
  44790d: d8 44 24 1c                  	fadds	0x1c(%esp)
  447911: d9 44 24 1c                  	flds	0x1c(%esp)
  447915: d8 c9                        	fmul	%st(1), %st
  447917: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44791d: df e0                        	fnstsw	%ax
  44791f: f6 c4 05                     	testb	$0x5, %ah
  447922: 0f 8a 42 01 00 00            	jp	0x447a6a <.text+0x46a6a>
  447928: dd d8                        	fstp	%st(0)
  44792a: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  447930: e9 35 01 00 00               	jmp	0x447a6a <.text+0x46a6a>
  447935: d9 86 04 01 00 00            	flds	0x104(%esi)
  44793b: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44793e: d8 4c 24 14                  	fmuls	0x14(%esp)
  447942: d8 44 24 1c                  	fadds	0x1c(%esp)
  447946: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  44794c: df e0                        	fnstsw	%ax
  44794e: f6 c4 01                     	testb	$0x1, %ah
  447951: 0f 85 13 01 00 00            	jne	0x447a6a <.text+0x46a6a>
  447957: dd d8                        	fstp	%st(0)
  447959: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44795f: e9 06 01 00 00               	jmp	0x447a6a <.text+0x46a6a>
  447964: d9 86 04 01 00 00            	flds	0x104(%esi)
  44796a: d8 4d 3c                     	fmuls	0x3c(%ebp)
  44796d: d8 4c 24 14                  	fmuls	0x14(%esp)
  447971: d9 5c 24 38                  	fstps	0x38(%esp)
  447975: d9 46 38                     	flds	0x38(%esi)
  447978: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44797e: df e0                        	fnstsw	%ax
  447980: f6 c4 05                     	testb	$0x5, %ah
  447983: 7a 50                        	jp	0x4479d5 <.text+0x469d5>
  447985: d9 44 24 10                  	flds	0x10(%esp)
  447989: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  44798f: df e0                        	fnstsw	%ax
  447991: f6 c4 05                     	testb	$0x5, %ah
  447994: 7a 3f                        	jp	0x4479d5 <.text+0x469d5>
  447996: d9 44 24 10                  	flds	0x10(%esp)
  44799a: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  4479a0: df e0                        	fnstsw	%ax
  4479a2: f6 c4 41                     	testb	$0x41, %ah
  4479a5: 75 2e                        	jne	0x4479d5 <.text+0x469d5>
  4479a7: d9 44 24 10                  	flds	0x10(%esp)
  4479ab: d8 44 24 1c                  	fadds	0x1c(%esp)
  4479af: d9 54 24 10                  	fsts	0x10(%esp)
  4479b3: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  4479b7: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4479bd: df e0                        	fnstsw	%ax
  4479bf: f6 c4 05                     	testb	$0x5, %ah
  4479c2: 0f 8a a6 00 00 00            	jp	0x447a6e <.text+0x46a6e>
  4479c8: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  4479d0: e9 99 00 00 00               	jmp	0x447a6e <.text+0x46a6e>
  4479d5: d9 44 24 38                  	flds	0x38(%esp)
  4479d9: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4479df: df e0                        	fnstsw	%ax
  4479e1: f6 c4 41                     	testb	$0x41, %ah
  4479e4: 75 2f                        	jne	0x447a15 <.text+0x46a15>
  4479e6: d9 44 24 38                  	flds	0x38(%esp)
  4479ea: d8 5c 24 1c                  	fcomps	0x1c(%esp)
  4479ee: df e0                        	fnstsw	%ax
  4479f0: f6 c4 05                     	testb	$0x5, %ah
  4479f3: 7a 20                        	jp	0x447a15 <.text+0x46a15>
  4479f5: d9 44 24 10                  	flds	0x10(%esp)
  4479f9: d8 44 24 1c                  	fadds	0x1c(%esp)
  4479fd: d8 54 24 38                  	fcoms	0x38(%esp)
  447a01: df e0                        	fnstsw	%ax
  447a03: f6 c4 41                     	testb	$0x41, %ah
  447a06: 75 62                        	jne	0x447a6a <.text+0x46a6a>
  447a08: d8 64 24 38                  	fsubs	0x38(%esp)
  447a0c: d9 5d 38                     	fstps	0x38(%ebp)
  447a0f: d9 44 24 38                  	flds	0x38(%esp)
  447a13: eb 55                        	jmp	0x447a6a <.text+0x46a6a>
  447a15: d9 44 24 1c                  	flds	0x1c(%esp)
  447a19: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447a1f: df e0                        	fnstsw	%ax
  447a21: f6 c4 44                     	testb	$0x44, %ah
  447a24: 7a 3c                        	jp	0x447a62 <.text+0x46a62>
  447a26: d9 46 38                     	flds	0x38(%esi)
  447a29: d8 9e c0 00 00 00            	fcomps	0xc0(%esi)
  447a2f: df e0                        	fnstsw	%ax
  447a31: f6 c4 41                     	testb	$0x41, %ah
  447a34: 7a 2c                        	jp	0x447a62 <.text+0x46a62>
  447a36: d9 44 24 10                  	flds	0x10(%esp)
  447a3a: d8 1d 4c 01 7e 00            	fcomps	0x7e014c
  447a40: df e0                        	fnstsw	%ax
  447a42: f6 c4 05                     	testb	$0x5, %ah
  447a45: 7a 1b                        	jp	0x447a62 <.text+0x46a62>
  447a47: d9 44 24 10                  	flds	0x10(%esp)
  447a4b: d8 1d 48 01 7e 00            	fcomps	0x7e0148
  447a51: df e0                        	fnstsw	%ax
  447a53: f6 c4 41                     	testb	$0x41, %ah
  447a56: 75 0a                        	jne	0x447a62 <.text+0x46a62>
  447a58: c7 44 24 10 00 00 00 00      	movl	$0x0, 0x10(%esp)
  447a60: eb 0c                        	jmp	0x447a6e <.text+0x46a6e>
  447a62: d9 44 24 10                  	flds	0x10(%esp)
  447a66: d8 44 24 1c                  	fadds	0x1c(%esp)
  447a6a: d9 5c 24 10                  	fstps	0x10(%esp)
  447a6e: d9 46 40                     	flds	0x40(%esi)
  447a71: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447a77: df e0                        	fnstsw	%ax
  447a79: f6 c4 44                     	testb	$0x44, %ah
  447a7c: 7a 3c                        	jp	0x447aba <.text+0x46aba>
  447a7e: d9 86 04 01 00 00            	flds	0x104(%esi)
  447a84: d8 4d 3c                     	fmuls	0x3c(%ebp)
  447a87: d8 4c 24 14                  	fmuls	0x14(%esp)
  447a8b: d9 44 24 18                  	flds	0x18(%esp)
  447a8f: d8 d9                        	fcomp	%st(1)
  447a91: df e0                        	fnstsw	%ax
  447a93: f6 c4 05                     	testb	$0x5, %ah
  447a96: 0f 8a c1 00 00 00            	jp	0x447b5d <.text+0x46b5d>
  447a9c: d9 e0                        	fchs
  447a9e: d8 5c 24 18                  	fcomps	0x18(%esp)
  447aa2: df e0                        	fnstsw	%ax
  447aa4: f6 c4 05                     	testb	$0x5, %ah
  447aa7: 0f 8a b2 00 00 00            	jp	0x447b5f <.text+0x46b5f>
  447aad: c7 44 24 18 00 00 00 00      	movl	$0x0, 0x18(%esp)
  447ab5: e9 a5 00 00 00               	jmp	0x447b5f <.text+0x46b5f>
  447aba: d9 44 24 14                  	flds	0x14(%esp)
  447abe: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447ac4: df e0                        	fnstsw	%ax
  447ac6: f6 c4 05                     	testb	$0x5, %ah
  447ac9: 7a 0f                        	jp	0x447ada <.text+0x46ada>
  447acb: d9 86 00 01 00 00            	flds	0x100(%esi)
  447ad1: d8 4d 3c                     	fmuls	0x3c(%ebp)
  447ad4: d8 4c 24 14                  	fmuls	0x14(%esp)
  447ad8: eb 06                        	jmp	0x447ae0 <.text+0x46ae0>
  447ada: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  447ae0: d9 44 24 18                  	flds	0x18(%esp)
  447ae4: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447aea: df e0                        	fnstsw	%ax
  447aec: f6 c4 41                     	testb	$0x41, %ah
  447aef: 75 08                        	jne	0x447af9 <.text+0x46af9>
  447af1: d9 e0                        	fchs
  447af3: d9 5c 24 24                  	fstps	0x24(%esp)
  447af7: eb 19                        	jmp	0x447b12 <.text+0x46b12>
  447af9: d9 44 24 18                  	flds	0x18(%esp)
  447afd: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447b03: df e0                        	fnstsw	%ax
  447b05: f6 c4 05                     	testb	$0x5, %ah
  447b08: 7a 06                        	jp	0x447b10 <.text+0x46b10>
  447b0a: d9 5c 24 24                  	fstps	0x24(%esp)
  447b0e: eb 02                        	jmp	0x447b12 <.text+0x46b12>
  447b10: dd d8                        	fstp	%st(0)
  447b12: d9 44 24 18                  	flds	0x18(%esp)
  447b16: d8 44 24 24                  	fadds	0x24(%esp)
  447b1a: d9 44 24 18                  	flds	0x18(%esp)
  447b1e: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447b24: df e0                        	fnstsw	%ax
  447b26: f6 c4 44                     	testb	$0x44, %ah
  447b29: 7b 2c                        	jnp	0x447b57 <.text+0x46b57>
  447b2b: d9 44 24 24                  	flds	0x24(%esp)
  447b2f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447b35: df e0                        	fnstsw	%ax
  447b37: f6 c4 44                     	testb	$0x44, %ah
  447b3a: 7b 1b                        	jnp	0x447b57 <.text+0x46b57>
  447b3c: d9 44 24 18                  	flds	0x18(%esp)
  447b40: d8 c9                        	fmul	%st(1), %st
  447b42: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447b48: df e0                        	fnstsw	%ax
  447b4a: f6 c4 05                     	testb	$0x5, %ah
  447b4d: 7a 08                        	jp	0x447b57 <.text+0x46b57>
  447b4f: dd d8                        	fstp	%st(0)
  447b51: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  447b57: d9 5c 24 18                  	fstps	0x18(%esp)
  447b5b: eb 02                        	jmp	0x447b5f <.text+0x46b5f>
  447b5d: dd d8                        	fstp	%st(0)
  447b5f: 8b 44 24 10                  	movl	0x10(%esp), %eax
  447b63: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  447b67: 8b 54 24 18                  	movl	0x18(%esp), %edx
  447b6b: 83 c6 2c                     	addl	$0x2c, %esi
  447b6e: 89 06                        	movl	%eax, (%esi)
  447b70: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  447b73: 5f                           	popl	%edi
  447b74: 89 56 08                     	movl	%edx, 0x8(%esi)
  447b77: 5e                           	popl	%esi
  447b78: 5d                           	popl	%ebp
  447b79: 5b                           	popl	%ebx
  447b7a: 83 c4 24                     	addl	$0x24, %esp
  447b7d: c3                           	retl
