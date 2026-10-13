
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  648b47: 55                           	pushl	%ebp
  648b48: 8b ec                        	movl	%esp, %ebp
  648b4a: 83 c4 e0                     	addl	$-0x20, %esp
  648b4d: 89 45 e0                     	movl	%eax, -0x20(%ebp)
  648b50: dd 5d f8                     	fstpl	-0x8(%ebp)
  648b53: 89 4d e4                     	movl	%ecx, -0x1c(%ebp)
  648b56: 8b 45 10                     	movl	0x10(%ebp), %eax
  648b59: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  648b5c: 89 45 e8                     	movl	%eax, -0x18(%ebp)
  648b5f: 89 4d ec                     	movl	%ecx, -0x14(%ebp)
  648b62: 8d 45 08                     	leal	0x8(%ebp), %eax
  648b65: 8d 4d e0                     	leal	-0x20(%ebp), %ecx
  648b68: 50                           	pushl	%eax
  648b69: 51                           	pushl	%ecx
  648b6a: 52                           	pushl	%edx
  648b6b: e8 5b 4f 00 00               	calll	0x64dacb <.text+0x24cacb>
  648b70: 83 c4 0c                     	addl	$0xc, %esp
  648b73: dd 45 f8                     	fldl	-0x8(%ebp)
  648b76: 66 81 7d 08 7f 02            	cmpw	$0x27f, 0x8(%ebp)       # imm = 0x27F
  648b7c: 74 03                        	je	0x648b81 <.text+0x247b81>
  648b7e: d9 6d 08                     	fldcw	0x8(%ebp)
  648b81: c9                           	leave
  648b82: c3                           	retl
  648b83: cc                           	int3
  648b84: cc                           	int3
  648b85: cc                           	int3
  648b86: cc                           	int3
  648b87: cc                           	int3
  648b88: cc                           	int3
  648b89: cc                           	int3
  648b8a: cc                           	int3
  648b8b: cc                           	int3
  648b8c: cc                           	int3
  648b8d: cc                           	int3
  648b8e: cc                           	int3
  648b8f: cc                           	int3
  648b90: d9 c0                        	fld	%st(0)
  648b92: d9 fc                        	frndint
  648b94: dc e1                        	fsub	%st, %st(1)
  648b96: d9 c9                        	fxch	%st(1)
  648b98: d9 e0                        	fchs
  648b9a: d9 f0                        	f2xm1
  648b9c: d9 e8                        	fld1
  648b9e: de c1                        	faddp	%st, %st(1)
  648ba0: d9 fd                        	fscale
  648ba2: dd d9                        	fstp	%st(1)
  648ba4: c3                           	retl
  648ba5: 8b 54 24 04                  	movl	0x4(%esp), %edx
  648ba9: 81 e2 00 03 00 00            	andl	$0x300, %edx            # imm = 0x300
  648baf: 83 ca 7f                     	orl	$0x7f, %edx
  648bb2: 66 89 54 24 06               	movw	%dx, 0x6(%esp)
  648bb7: d9 6c 24 06                  	fldcw	0x6(%esp)
  648bbb: c3                           	retl
  648bbc: a9 00 00 08 00               	testl	$0x80000, %eax          # imm = 0x80000
  648bc1: 74 06                        	je	0x648bc9 <.text+0x247bc9>
  648bc3: b8 07 00 00 00               	movl	$0x7, %eax
  648bc8: c3                           	retl
  648bc9: dc 05 f0 4d 84 00            	faddl	0x844df0
  648bcf: b8 01 00 00 00               	movl	$0x1, %eax
  648bd4: c3                           	retl
  648bd5: 8b 42 04                     	movl	0x4(%edx), %eax
  648bd8: 25 00 00 f0 7f               	andl	$0x7ff00000, %eax       # imm = 0x7FF00000
  648bdd: 3d 00 00 f0 7f               	cmpl	$0x7ff00000, %eax       # imm = 0x7FF00000
  648be2: 74 03                        	je	0x648be7 <.text+0x247be7>
  648be4: dd 02                        	fldl	(%edx)
  648be6: c3                           	retl
  648be7: 8b 42 04                     	movl	0x4(%edx), %eax
  648bea: 83 ec 0a                     	subl	$0xa, %esp
  648bed: 0d 00 00 ff 7f               	orl	$0x7fff0000, %eax       # imm = 0x7FFF0000
  648bf2: 89 44 24 06                  	movl	%eax, 0x6(%esp)
  648bf6: 8b 42 04                     	movl	0x4(%edx), %eax
  648bf9: 8b 0a                        	movl	(%edx), %ecx
  648bfb: 0f a4 c8 0b                  	shldl	$0xb, %ecx, %eax
  648bff: c1 e1 0b                     	shll	$0xb, %ecx
  648c02: 89 44 24 04                  	movl	%eax, 0x4(%esp)
  648c06: 89 0c 24                     	movl	%ecx, (%esp)
  648c09: db 2c 24                     	fldt	(%esp)
  648c0c: 83 c4 0a                     	addl	$0xa, %esp
  648c0f: a9 00 00 00 00               	testl	$0x0, %eax
  648c14: 8b 42 04                     	movl	0x4(%edx), %eax
  648c17: c3                           	retl
  648c18: 8b 44 24 08                  	movl	0x8(%esp), %eax
  648c1c: 25 00 00 f0 7f               	andl	$0x7ff00000, %eax       # imm = 0x7FF00000
  648c21: 3d 00 00 f0 7f               	cmpl	$0x7ff00000, %eax       # imm = 0x7FF00000
  648c26: 74 01                        	je	0x648c29 <.text+0x247c29>
  648c28: c3                           	retl
  648c29: 8b 44 24 08                  	movl	0x8(%esp), %eax
  648c2d: c3                           	retl
  648c2e: 66 81 3c 24 7f 02            	cmpw	$0x27f, (%esp)          # imm = 0x27F
  648c34: 74 03                        	je	0x648c39 <.text+0x247c39>
  648c36: d9 2c 24                     	fldcw	(%esp)
  648c39: 5a                           	popl	%edx
  648c3a: c3                           	retl
  648c3b: 66 8b 04 24                  	movw	(%esp), %ax
  648c3f: 66 3d 7f 02                  	cmpw	$0x27f, %ax             # imm = 0x27F
  648c43: 74 1e                        	je	0x648c63 <.text+0x247c63>
  648c45: 66 83 e0 20                  	andw	$0x20, %ax
  648c49: 74 15                        	je	0x648c60 <.text+0x247c60>
  648c4b: 9b                           	wait
  648c4c: df e0                        	fnstsw	%ax
  648c4e: 66 83 e0 20                  	andw	$0x20, %ax
  648c52: 74 0c                        	je	0x648c60 <.text+0x247c60>
  648c54: b8 08 00 00 00               	movl	$0x8, %eax
  648c59: e8 e9 fe ff ff               	calll	0x648b47 <.text+0x247b47>
  648c5e: 5a                           	popl	%edx
  648c5f: c3                           	retl
  648c60: d9 2c 24                     	fldcw	(%esp)
  648c63: 5a                           	popl	%edx
  648c64: c3                           	retl
