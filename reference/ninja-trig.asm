
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  788290: 51                           	pushl	%ecx
  788291: 8b d1                        	movl	%ecx, %edx
  788293: c1 f9 04                     	sarl	$0x4, %ecx
  788296: 81 e1 ff 0f 00 00            	andl	$0xfff, %ecx            # imm = 0xFFF
  78829c: 8b c1                        	movl	%ecx, %eax
  78829e: 83 e2 0f                     	andl	$0xf, %edx
  7882a1: 25 00 0c 00 00               	andl	$0xc00, %eax            # imm = 0xC00
  7882a6: 85 d2                        	testl	%edx, %edx
  7882a8: 75 5d                        	jne	0x788307 <.text+0x387307>
  7882aa: 3d 00 08 00 00               	cmpl	$0x800, %eax            # imm = 0x800
  7882af: 7f 37                        	jg	0x7882e8 <.text+0x3872e8>
  7882b1: 74 2a                        	je	0x7882dd <.text+0x3872dd>
  7882b3: 85 c0                        	testl	%eax, %eax
  7882b5: 74 1d                        	je	0x7882d4 <.text+0x3872d4>
  7882b7: 3d 00 04 00 00               	cmpl	$0x400, %eax            # imm = 0x400
  7882bc: 0f 85 f6 00 00 00            	jne	0x7883b8 <.text+0x3873b8>
  7882c2: 8d 04 8d 00 00 00 00         	leal	(,%ecx,4), %eax
  7882c9: b9 78 cf 88 00               	movl	$0x88cf78, %ecx         # imm = 0x88CF78
  7882ce: 2b c8                        	subl	%eax, %ecx
  7882d0: d9 01                        	flds	(%ecx)
  7882d2: 59                           	popl	%ecx
  7882d3: c3                           	retl
  7882d4: d9 04 8d 78 af 88 00         	flds	0x88af78(,%ecx,4)
  7882db: 59                           	popl	%ecx
  7882dc: c3                           	retl
  7882dd: d9 04 8d 78 8f 88 00         	flds	0x888f78(,%ecx,4)
  7882e4: d9 e0                        	fchs
  7882e6: 59                           	popl	%ecx
  7882e7: c3                           	retl
  7882e8: 3d 00 0c 00 00               	cmpl	$0xc00, %eax            # imm = 0xC00
  7882ed: 0f 85 c5 00 00 00            	jne	0x7883b8 <.text+0x3873b8>
  7882f3: 8d 14 8d 00 00 00 00         	leal	(,%ecx,4), %edx
  7882fa: b8 78 ef 88 00               	movl	$0x88ef78, %eax         # imm = 0x88EF78
  7882ff: 2b c2                        	subl	%edx, %eax
  788301: d9 00                        	flds	(%eax)
  788303: d9 e0                        	fchs
  788305: 59                           	popl	%ecx
  788306: c3                           	retl
  788307: 3d 00 08 00 00               	cmpl	$0x800, %eax            # imm = 0x800
  78830c: 56                           	pushl	%esi
  78830d: 7f 53                        	jg	0x788362 <.text+0x387362>
  78830f: 74 3d                        	je	0x78834e <.text+0x38734e>
  788311: 85 c0                        	testl	%eax, %eax
  788313: 74 29                        	je	0x78833e <.text+0x38733e>
  788315: 3d 00 04 00 00               	cmpl	$0x400, %eax            # imm = 0x400
  78831a: 75 73                        	jne	0x78838f <.text+0x38738f>
  78831c: 8d 04 8d 00 00 00 00         	leal	(,%ecx,4), %eax
  788323: be 78 cf 88 00               	movl	$0x88cf78, %esi         # imm = 0x88CF78
  788328: 2b f0                        	subl	%eax, %esi
  78832a: d9 06                        	flds	(%esi)
  78832c: 8d 0c 8d 04 00 00 00         	leal	0x4(,%ecx,4), %ecx
  788333: b8 78 cf 88 00               	movl	$0x88cf78, %eax         # imm = 0x88CF78
  788338: 2b c1                        	subl	%ecx, %eax
  78833a: d9 00                        	flds	(%eax)
  78833c: eb 59                        	jmp	0x788397 <.text+0x387397>
  78833e: d9 04 8d 78 af 88 00         	flds	0x88af78(,%ecx,4)
  788345: d9 04 8d 7c af 88 00         	flds	0x88af7c(,%ecx,4)
  78834c: eb 49                        	jmp	0x788397 <.text+0x387397>
  78834e: d9 04 8d 78 8f 88 00         	flds	0x888f78(,%ecx,4)
  788355: d9 e0                        	fchs
  788357: d9 04 8d 7c 8f 88 00         	flds	0x888f7c(,%ecx,4)
  78835e: d9 e0                        	fchs
  788360: eb 35                        	jmp	0x788397 <.text+0x387397>
  788362: 3d 00 0c 00 00               	cmpl	$0xc00, %eax            # imm = 0xC00
  788367: 75 26                        	jne	0x78838f <.text+0x38738f>
  788369: 8d 04 8d 00 00 00 00         	leal	(,%ecx,4), %eax
  788370: be 78 ef 88 00               	movl	$0x88ef78, %esi         # imm = 0x88EF78
  788375: 2b f0                        	subl	%eax, %esi
  788377: d9 06                        	flds	(%esi)
  788379: 8d 0c 8d 04 00 00 00         	leal	0x4(,%ecx,4), %ecx
  788380: b8 78 ef 88 00               	movl	$0x88ef78, %eax         # imm = 0x88EF78
  788385: d9 e0                        	fchs
  788387: 2b c1                        	subl	%ecx, %eax
  788389: d9 00                        	flds	(%eax)
  78838b: d9 e0                        	fchs
  78838d: eb 08                        	jmp	0x788397 <.text+0x387397>
  78838f: d9 44 24 04                  	flds	0x4(%esp)
  788393: d9 44 24 04                  	flds	0x4(%esp)
  788397: 85 d2                        	testl	%edx, %edx
  788399: d8 e1                        	fsub	%st(1), %st
  78839b: 89 54 24 04                  	movl	%edx, 0x4(%esp)
  78839f: db 44 24 04                  	fildl	0x4(%esp)
  7883a3: 7d 06                        	jge	0x7883ab <.text+0x3873ab>
  7883a5: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  7883ab: d8 0d 20 01 7e 00            	fmuls	0x7e0120
  7883b1: 5e                           	popl	%esi
  7883b2: de c9                        	fmulp	%st, %st(1)
  7883b4: de c1                        	faddp	%st, %st(1)
  7883b6: 59                           	popl	%ecx
  7883b7: c3                           	retl
  7883b8: d9 04 24                     	flds	(%esp)
  7883bb: 59                           	popl	%ecx
  7883bc: c3                           	retl
  7883bd: cc                           	int3
  7883be: cc                           	int3
  7883bf: cc                           	int3
  7883c0: 51                           	pushl	%ecx
  7883c1: 8b d1                        	movl	%ecx, %edx
  7883c3: c1 f9 04                     	sarl	$0x4, %ecx
  7883c6: 81 e1 ff 0f 00 00            	andl	$0xfff, %ecx            # imm = 0xFFF
  7883cc: 8b c1                        	movl	%ecx, %eax
  7883ce: 83 e2 0f                     	andl	$0xf, %edx
  7883d1: 25 00 0c 00 00               	andl	$0xc00, %eax            # imm = 0xC00
  7883d6: 85 d2                        	testl	%edx, %edx
  7883d8: 75 5d                        	jne	0x788437 <.text+0x387437>
  7883da: 3d 00 08 00 00               	cmpl	$0x800, %eax            # imm = 0x800
  7883df: 7f 42                        	jg	0x788423 <.text+0x387423>
  7883e1: 74 2c                        	je	0x78840f <.text+0x38740f>
  7883e3: 85 c0                        	testl	%eax, %eax
  7883e5: 74 16                        	je	0x7883fd <.text+0x3873fd>
  7883e7: 3d 00 04 00 00               	cmpl	$0x400, %eax            # imm = 0x400
  7883ec: 0f 85 f6 00 00 00            	jne	0x7884e8 <.text+0x3874e8>
  7883f2: d9 04 8d 78 9f 88 00         	flds	0x889f78(,%ecx,4)
  7883f9: d9 e0                        	fchs
  7883fb: 59                           	popl	%ecx
  7883fc: c3                           	retl
  7883fd: 8d 04 8d 00 00 00 00         	leal	(,%ecx,4), %eax
  788404: b9 78 bf 88 00               	movl	$0x88bf78, %ecx         # imm = 0x88BF78
  788409: 2b c8                        	subl	%eax, %ecx
  78840b: d9 01                        	flds	(%ecx)
  78840d: 59                           	popl	%ecx
  78840e: c3                           	retl
  78840f: 8d 14 8d 00 00 00 00         	leal	(,%ecx,4), %edx
  788416: b8 78 df 88 00               	movl	$0x88df78, %eax         # imm = 0x88DF78
  78841b: 2b c2                        	subl	%edx, %eax
  78841d: d9 00                        	flds	(%eax)
  78841f: d9 e0                        	fchs
  788421: 59                           	popl	%ecx
  788422: c3                           	retl
  788423: 3d 00 0c 00 00               	cmpl	$0xc00, %eax            # imm = 0xC00
  788428: 0f 85 ba 00 00 00            	jne	0x7884e8 <.text+0x3874e8>
  78842e: d9 04 8d 78 7f 88 00         	flds	0x887f78(,%ecx,4)
  788435: 59                           	popl	%ecx
  788436: c3                           	retl
  788437: 3d 00 08 00 00               	cmpl	$0x800, %eax            # imm = 0x800
  78843c: 56                           	pushl	%esi
  78843d: 7f 69                        	jg	0x7884a8 <.text+0x3874a8>
  78843f: 74 41                        	je	0x788482 <.text+0x387482>
  788441: 85 c0                        	testl	%eax, %eax
  788443: 74 1b                        	je	0x788460 <.text+0x387460>
  788445: 3d 00 04 00 00               	cmpl	$0x400, %eax            # imm = 0x400
  78844a: 75 73                        	jne	0x7884bf <.text+0x3874bf>
  78844c: d9 04 8d 78 9f 88 00         	flds	0x889f78(,%ecx,4)
  788453: d9 e0                        	fchs
  788455: d9 04 8d 7c 9f 88 00         	flds	0x889f7c(,%ecx,4)
  78845c: d9 e0                        	fchs
  78845e: eb 67                        	jmp	0x7884c7 <.text+0x3874c7>
  788460: 8d 04 8d 00 00 00 00         	leal	(,%ecx,4), %eax
  788467: be 78 bf 88 00               	movl	$0x88bf78, %esi         # imm = 0x88BF78
  78846c: 2b f0                        	subl	%eax, %esi
  78846e: d9 06                        	flds	(%esi)
  788470: 8d 0c 8d 04 00 00 00         	leal	0x4(,%ecx,4), %ecx
  788477: b8 78 bf 88 00               	movl	$0x88bf78, %eax         # imm = 0x88BF78
  78847c: 2b c1                        	subl	%ecx, %eax
  78847e: d9 00                        	flds	(%eax)
  788480: eb 45                        	jmp	0x7884c7 <.text+0x3874c7>
  788482: 8d 04 8d 00 00 00 00         	leal	(,%ecx,4), %eax
  788489: be 78 df 88 00               	movl	$0x88df78, %esi         # imm = 0x88DF78
  78848e: 2b f0                        	subl	%eax, %esi
  788490: d9 06                        	flds	(%esi)
  788492: 8d 0c 8d 04 00 00 00         	leal	0x4(,%ecx,4), %ecx
  788499: b8 78 df 88 00               	movl	$0x88df78, %eax         # imm = 0x88DF78
  78849e: d9 e0                        	fchs
  7884a0: 2b c1                        	subl	%ecx, %eax
  7884a2: d9 00                        	flds	(%eax)
  7884a4: d9 e0                        	fchs
  7884a6: eb 1f                        	jmp	0x7884c7 <.text+0x3874c7>
  7884a8: 3d 00 0c 00 00               	cmpl	$0xc00, %eax            # imm = 0xC00
  7884ad: 75 10                        	jne	0x7884bf <.text+0x3874bf>
  7884af: d9 04 8d 78 7f 88 00         	flds	0x887f78(,%ecx,4)
  7884b6: d9 04 8d 7c 7f 88 00         	flds	0x887f7c(,%ecx,4)
  7884bd: eb 08                        	jmp	0x7884c7 <.text+0x3874c7>
  7884bf: d9 44 24 04                  	flds	0x4(%esp)
  7884c3: d9 44 24 04                  	flds	0x4(%esp)
  7884c7: 85 d2                        	testl	%edx, %edx
  7884c9: d8 e1                        	fsub	%st(1), %st
  7884cb: 89 54 24 04                  	movl	%edx, 0x4(%esp)
  7884cf: db 44 24 04                  	fildl	0x4(%esp)
  7884d3: 7d 06                        	jge	0x7884db <.text+0x3874db>
  7884d5: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  7884db: d8 0d 20 01 7e 00            	fmuls	0x7e0120
  7884e1: 5e                           	popl	%esi
  7884e2: de c9                        	fmulp	%st, %st(1)
  7884e4: de c1                        	faddp	%st, %st(1)
  7884e6: 59                           	popl	%ecx
  7884e7: c3                           	retl
  7884e8: d9 04 24                     	flds	(%esp)
  7884eb: 59                           	popl	%ecx
  7884ec: c3                           	retl
  7884ed: cc                           	int3
  7884ee: cc                           	int3
  7884ef: cc                           	int3
