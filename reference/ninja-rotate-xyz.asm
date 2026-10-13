
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  781770: 83 ec 08                     	subl	$0x8, %esp
  781773: 85 c9                        	testl	%ecx, %ecx
  781775: 53                           	pushl	%ebx
  781776: 56                           	pushl	%esi
  781777: 8b 35 18 f8 d0 03            	movl	0x3d0f818, %esi
  78177d: 8b da                        	movl	%edx, %ebx
  78177f: 74 02                        	je	0x781783 <.text+0x380783>
  781781: 8b f1                        	movl	%ecx, %esi
  781783: 57                           	pushl	%edi
  781784: 8b 7c 24 1c                  	movl	0x1c(%esp), %edi
  781788: 85 ff                        	testl	%edi, %edi
  78178a: 0f 84 ac 00 00 00            	je	0x78183c <.text+0x38083c>
  781790: 8b cf                        	movl	%edi, %ecx
  781792: e8 f9 6a 00 00               	calll	0x788290 <.text+0x387290>
  781797: d9 5c 24 1c                  	fstps	0x1c(%esp)
  78179b: 8b cf                        	movl	%edi, %ecx
  78179d: e8 1e 6c 00 00               	calll	0x7883c0 <.text+0x3873c0>
  7817a2: d9 06                        	flds	(%esi)
  7817a4: d9 46 04                     	flds	0x4(%esi)
  7817a7: 8b 46 08                     	movl	0x8(%esi), %eax
  7817aa: d9 44 24 1c                  	flds	0x1c(%esp)
  7817ae: 8b 4e 0c                     	movl	0xc(%esi), %ecx
  7817b1: d8 4e 10                     	fmuls	0x10(%esi)
  7817b4: 89 44 24 0c                  	movl	%eax, 0xc(%esp)
  7817b8: d9 c2                        	fld	%st(2)
  7817ba: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  7817be: d8 cc                        	fmul	%st(4), %st
  7817c0: de c1                        	faddp	%st, %st(1)
  7817c2: d9 1e                        	fstps	(%esi)
  7817c4: d9 44 24 1c                  	flds	0x1c(%esp)
  7817c8: d8 4e 14                     	fmuls	0x14(%esi)
  7817cb: d9 c1                        	fld	%st(1)
  7817cd: d8 cc                        	fmul	%st(4), %st
  7817cf: de c1                        	faddp	%st, %st(1)
  7817d1: d9 5e 04                     	fstps	0x4(%esi)
  7817d4: d9 44 24 1c                  	flds	0x1c(%esp)
  7817d8: d8 4e 18                     	fmuls	0x18(%esi)
  7817db: d9 44 24 0c                  	flds	0xc(%esp)
  7817df: d8 cc                        	fmul	%st(4), %st
  7817e1: de c1                        	faddp	%st, %st(1)
  7817e3: d9 5e 08                     	fstps	0x8(%esi)
  7817e6: d9 44 24 1c                  	flds	0x1c(%esp)
  7817ea: d8 4e 1c                     	fmuls	0x1c(%esi)
  7817ed: d9 44 24 10                  	flds	0x10(%esp)
  7817f1: d8 cc                        	fmul	%st(4), %st
  7817f3: de c1                        	faddp	%st, %st(1)
  7817f5: d9 5e 0c                     	fstps	0xc(%esi)
  7817f8: d9 c2                        	fld	%st(2)
  7817fa: d8 4e 10                     	fmuls	0x10(%esi)
  7817fd: d9 ca                        	fxch	%st(2)
  7817ff: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  781803: de ea                        	fsubrp	%st, %st(2)
  781805: d9 c9                        	fxch	%st(1)
  781807: d9 5e 10                     	fstps	0x10(%esi)
  78180a: d9 c1                        	fld	%st(1)
  78180c: d8 4e 14                     	fmuls	0x14(%esi)
  78180f: d9 c9                        	fxch	%st(1)
  781811: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  781815: de e9                        	fsubrp	%st, %st(1)
  781817: d9 5e 14                     	fstps	0x14(%esi)
  78181a: d9 c0                        	fld	%st(0)
  78181c: d8 4e 18                     	fmuls	0x18(%esi)
  78181f: d9 44 24 0c                  	flds	0xc(%esp)
  781823: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  781827: de e9                        	fsubrp	%st, %st(1)
  781829: d9 5e 18                     	fstps	0x18(%esi)
  78182c: d8 4e 1c                     	fmuls	0x1c(%esi)
  78182f: d9 44 24 10                  	flds	0x10(%esp)
  781833: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  781837: de e9                        	fsubrp	%st, %st(1)
  781839: d9 5e 1c                     	fstps	0x1c(%esi)
  78183c: 8b 7c 24 18                  	movl	0x18(%esp), %edi
  781840: 85 ff                        	testl	%edi, %edi
  781842: 0f 84 ac 00 00 00            	je	0x7818f4 <.text+0x3808f4>
  781848: 8b cf                        	movl	%edi, %ecx
  78184a: e8 41 6a 00 00               	calll	0x788290 <.text+0x387290>
  78184f: d9 5c 24 1c                  	fstps	0x1c(%esp)
  781853: 8b cf                        	movl	%edi, %ecx
  781855: e8 66 6b 00 00               	calll	0x7883c0 <.text+0x3873c0>
  78185a: d9 06                        	flds	(%esi)
  78185c: d9 46 04                     	flds	0x4(%esi)
  78185f: 8b 56 08                     	movl	0x8(%esi), %edx
  781862: d9 c1                        	fld	%st(1)
  781864: 8b 46 0c                     	movl	0xc(%esi), %eax
  781867: d8 cb                        	fmul	%st(3), %st
  781869: 89 54 24 0c                  	movl	%edx, 0xc(%esp)
  78186d: d9 44 24 1c                  	flds	0x1c(%esp)
  781871: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  781875: d8 4e 20                     	fmuls	0x20(%esi)
  781878: de e9                        	fsubrp	%st, %st(1)
  78187a: d9 1e                        	fstps	(%esi)
  78187c: d9 c0                        	fld	%st(0)
  78187e: d8 cb                        	fmul	%st(3), %st
  781880: d9 44 24 1c                  	flds	0x1c(%esp)
  781884: d8 4e 24                     	fmuls	0x24(%esi)
  781887: de e9                        	fsubrp	%st, %st(1)
  781889: d9 5e 04                     	fstps	0x4(%esi)
  78188c: d9 44 24 0c                  	flds	0xc(%esp)
  781890: d8 cb                        	fmul	%st(3), %st
  781892: d9 44 24 1c                  	flds	0x1c(%esp)
  781896: d8 4e 28                     	fmuls	0x28(%esi)
  781899: de e9                        	fsubrp	%st, %st(1)
  78189b: d9 5e 08                     	fstps	0x8(%esi)
  78189e: d9 44 24 10                  	flds	0x10(%esp)
  7818a2: d8 cb                        	fmul	%st(3), %st
  7818a4: d9 44 24 1c                  	flds	0x1c(%esp)
  7818a8: d8 4e 2c                     	fmuls	0x2c(%esi)
  7818ab: de e9                        	fsubrp	%st, %st(1)
  7818ad: d9 5e 0c                     	fstps	0xc(%esi)
  7818b0: d9 c2                        	fld	%st(2)
  7818b2: d8 4e 20                     	fmuls	0x20(%esi)
  7818b5: d9 ca                        	fxch	%st(2)
  7818b7: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  7818bb: de c2                        	faddp	%st, %st(2)
  7818bd: d9 c9                        	fxch	%st(1)
  7818bf: d9 5e 20                     	fstps	0x20(%esi)
  7818c2: d9 c1                        	fld	%st(1)
  7818c4: d8 4e 24                     	fmuls	0x24(%esi)
  7818c7: d9 c9                        	fxch	%st(1)
  7818c9: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  7818cd: de c1                        	faddp	%st, %st(1)
  7818cf: d9 5e 24                     	fstps	0x24(%esi)
  7818d2: d9 44 24 0c                  	flds	0xc(%esp)
  7818d6: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  7818da: d9 c1                        	fld	%st(1)
  7818dc: d8 4e 28                     	fmuls	0x28(%esi)
  7818df: de c1                        	faddp	%st, %st(1)
  7818e1: d9 5e 28                     	fstps	0x28(%esi)
  7818e4: d8 4e 2c                     	fmuls	0x2c(%esi)
  7818e7: d9 44 24 10                  	flds	0x10(%esp)
  7818eb: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  7818ef: de c1                        	faddp	%st, %st(1)
  7818f1: d9 5e 2c                     	fstps	0x2c(%esi)
  7818f4: 85 db                        	testl	%ebx, %ebx
  7818f6: 5f                           	popl	%edi
  7818f7: 0f 84 ae 00 00 00            	je	0x7819ab <.text+0x3809ab>
  7818fd: 8b cb                        	movl	%ebx, %ecx
  7818ff: e8 8c 69 00 00               	calll	0x788290 <.text+0x387290>
  781904: d9 5c 24 18                  	fstps	0x18(%esp)
  781908: 8b cb                        	movl	%ebx, %ecx
  78190a: e8 b1 6a 00 00               	calll	0x7883c0 <.text+0x3873c0>
  78190f: d9 46 10                     	flds	0x10(%esi)
  781912: d9 46 14                     	flds	0x14(%esi)
  781915: 8b 4e 18                     	movl	0x18(%esi), %ecx
  781918: d9 44 24 18                  	flds	0x18(%esp)
  78191c: 8b 56 1c                     	movl	0x1c(%esi), %edx
  78191f: d8 4e 20                     	fmuls	0x20(%esi)
  781922: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  781926: d9 c2                        	fld	%st(2)
  781928: 89 54 24 0c                  	movl	%edx, 0xc(%esp)
  78192c: d8 cc                        	fmul	%st(4), %st
  78192e: de c1                        	faddp	%st, %st(1)
  781930: d9 5e 10                     	fstps	0x10(%esi)
  781933: d9 44 24 18                  	flds	0x18(%esp)
  781937: d8 4e 24                     	fmuls	0x24(%esi)
  78193a: d9 c1                        	fld	%st(1)
  78193c: d8 cc                        	fmul	%st(4), %st
  78193e: de c1                        	faddp	%st, %st(1)
  781940: d9 5e 14                     	fstps	0x14(%esi)
  781943: d9 44 24 14                  	flds	0x14(%esp)
  781947: d8 cb                        	fmul	%st(3), %st
  781949: d9 44 24 18                  	flds	0x18(%esp)
  78194d: d8 4e 28                     	fmuls	0x28(%esi)
  781950: de c1                        	faddp	%st, %st(1)
  781952: d9 5e 18                     	fstps	0x18(%esi)
  781955: d9 44 24 18                  	flds	0x18(%esp)
  781959: d8 4e 2c                     	fmuls	0x2c(%esi)
  78195c: d9 44 24 0c                  	flds	0xc(%esp)
  781960: d8 cc                        	fmul	%st(4), %st
  781962: de c1                        	faddp	%st, %st(1)
  781964: d9 5e 1c                     	fstps	0x1c(%esi)
  781967: d9 c2                        	fld	%st(2)
  781969: d8 4e 20                     	fmuls	0x20(%esi)
  78196c: d9 ca                        	fxch	%st(2)
  78196e: d8 4c 24 18                  	fmuls	0x18(%esp)
  781972: de ea                        	fsubrp	%st, %st(2)
  781974: d9 c9                        	fxch	%st(1)
  781976: d9 5e 20                     	fstps	0x20(%esi)
  781979: d9 c1                        	fld	%st(1)
  78197b: d8 4e 24                     	fmuls	0x24(%esi)
  78197e: d9 c9                        	fxch	%st(1)
  781980: d8 4c 24 18                  	fmuls	0x18(%esp)
  781984: de e9                        	fsubrp	%st, %st(1)
  781986: d9 5e 24                     	fstps	0x24(%esi)
  781989: d9 c0                        	fld	%st(0)
  78198b: d8 4e 28                     	fmuls	0x28(%esi)
  78198e: d9 44 24 14                  	flds	0x14(%esp)
