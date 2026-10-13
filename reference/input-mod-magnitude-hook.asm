
_reference/sadx/updated-pc-reference/unpacked/input/sadx-input-mod/sadx-input-mod.dll:	file format coff-i386

Disassembly of section .text:

10001000 <.text>:
10004b00: 80 3d b0 9f 90 00 00         	cmpb	$0x0, 0x909fb0
10004b07: 74 62                        	je	0x10004b6b <.text+0x3b6b>
10004b09: 56                           	pushl	%esi
10004b0a: 33 f6                        	xorl	%esi, %esi
10004b0c: 33 c9                        	xorl	%ecx, %ecx
10004b0e: 66 90                        	nop
10004b10: 80 be 3c e7 00 10 00         	cmpb	$0x0, 0x1000e73c(%esi)
10004b17: 74 45                        	je	0x10004b5e <.text+0x3b5e>
10004b19: 80 b9 8c e9 00 10 00         	cmpb	$0x0, 0x1000e98c(%ecx)
10004b20: 75 0d                        	jne	0x10004b2f <.text+0x3b2f>
10004b22: 80 b9 9c e9 00 10 00         	cmpb	$0x0, 0x1000e99c(%ecx)
10004b29: 74 33                        	je	0x10004b5e <.text+0x3b5e>
10004b2b: 85 f6                        	testl	%esi, %esi
10004b2d: 75 2f                        	jne	0x10004b5e <.text+0x3b5e>
10004b2f: 0f bf 81 6c e9 00 10         	movswl	0x1000e96c(%ecx), %eax
10004b36: 99                           	cltd
10004b37: 33 c2                        	xorl	%edx, %eax
10004b39: 2b c2                        	subl	%edx, %eax
10004b3b: 83 f8 0c                     	cmpl	$0xc, %eax
10004b3e: 7f 11                        	jg	0x10004b51 <.text+0x3b51>
10004b40: 0f bf 81 6e e9 00 10         	movswl	0x1000e96e(%ecx), %eax
10004b47: 99                           	cltd
10004b48: 33 c2                        	xorl	%edx, %eax
10004b4a: 2b c2                        	subl	%edx, %eax
10004b4c: 83 f8 0c                     	cmpl	$0xc, %eax
10004b4f: 7e 0d                        	jle	0x10004b5e <.text+0x3b5e>
10004b51: 8b 81 94 e9 00 10            	movl	0x1000e994(%ecx), %eax
10004b57: 89 04 f5 a4 e7 b0 03         	movl	%eax, 0x3b0e7a4(,%esi,8)
10004b5e: 83 c1 60                     	addl	$0x60, %ecx
10004b61: 46                           	incl	%esi
10004b62: 81 f9 00 03 00 00            	cmpl	$0x300, %ecx            # imm = 0x300
10004b68: 72 a6                        	jb	0x10004b10 <.text+0x3b10>
10004b6a: 5e                           	popl	%esi
10004b6b: c3                           	retl
10004b6c: cc                           	int3
10004b6d: cc                           	int3
10004b6e: cc                           	int3
10004b6f: cc                           	int3
10004b70: e8 8b ff ff ff               	calll	0x10004b00 <.text+0x3b00>
10004b75: c3                           	retl
