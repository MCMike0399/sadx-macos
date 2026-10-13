
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43b580: 51                           	pushl	%ecx
  43b581: 0f bf 05 10 d5 b2 03         	movswl	0x3b2d510, %eax
  43b588: 53                           	pushl	%ebx
  43b589: 55                           	pushl	%ebp
  43b58a: 8d 04 40                     	leal	(%eax,%eax,2), %eax
  43b58d: 33 d2                        	xorl	%edx, %edx
  43b58f: 8d 04 85 24 2d b3 03         	leal	0x3b32d24(,%eax,4), %eax
  43b596: 3d 30 2d b3 03               	cmpl	$0x3b32d30, %eax        # imm = 0x3B32D30
  43b59b: 56                           	pushl	%esi
  43b59c: 57                           	pushl	%edi
  43b59d: bb 20 f7 b2 03               	movl	$0x3b2f720, %ebx        # imm = 0x3B2F720
  43b5a2: 66 89 15 24 27 b3 03         	movw	%dx, 0x3b32724
  43b5a9: 89 15 38 6d b3 03            	movl	%edx, 0x3b36d38
  43b5af: 72 6b                        	jb	0x43b61c <.text+0x3a61c>
  43b5b1: 8b cb                        	movl	%ebx, %ecx
  43b5b3: f7 d9                        	negl	%ecx
  43b5b5: 8d b4 19 28 27 b3 03         	leal	0x3b32728(%ecx,%ebx), %esi
  43b5bc: 8b f8                        	movl	%eax, %edi
  43b5be: 8b 2f                        	movl	(%edi), %ebp
  43b5c0: 89 2e                        	movl	%ebp, (%esi)
  43b5c2: 8b 6f 04                     	movl	0x4(%edi), %ebp
  43b5c5: 8b 7f 08                     	movl	0x8(%edi), %edi
  43b5c8: 89 6e 04                     	movl	%ebp, 0x4(%esi)
  43b5cb: 89 7e 08                     	movl	%edi, 0x8(%esi)
  43b5ce: 8b 78 04                     	movl	0x4(%eax), %edi
  43b5d1: 8b 35 38 6d b3 03            	movl	0x3b36d38, %esi
  43b5d7: 89 bc 19 24 f7 b2 03         	movl	%edi, 0x3b2f724(%ecx,%ebx)
  43b5de: 8b 78 08                     	movl	0x8(%eax), %edi
  43b5e1: 89 bc 19 28 f7 b2 03         	movl	%edi, 0x3b2f728(%ecx,%ebx)
  43b5e8: 8b 38                        	movl	(%eax), %edi
  43b5ea: 46                           	incl	%esi
  43b5eb: 89 3b                        	movl	%edi, (%ebx)
  43b5ed: 83 c3 0c                     	addl	$0xc, %ebx
  43b5f0: 66 42                        	incw	%dx
  43b5f2: 81 fe 80 00 00 00            	cmpl	$0x80, %esi
  43b5f8: 89 35 38 6d b3 03            	movl	%esi, 0x3b36d38
  43b5fe: 0f 8d b5 00 00 00            	jge	0x43b6b9 <.text+0x3a6b9>
  43b604: 66 81 fa 00 04               	cmpw	$0x400, %dx             # imm = 0x400
  43b609: 7d 0a                        	jge	0x43b615 <.text+0x3a615>
  43b60b: 83 e8 0c                     	subl	$0xc, %eax
  43b60e: 3d 30 2d b3 03               	cmpl	$0x3b32d30, %eax        # imm = 0x3B32D30
  43b613: 73 a0                        	jae	0x43b5b5 <.text+0x3a5b5>
  43b615: 66 89 15 24 27 b3 03         	movw	%dx, 0x3b32724
  43b61c: 83 3d 60 54 91 00 01         	cmpl	$0x1, 0x915460
  43b623: c7 05 3c 6d b3 03 00 00 00 00	movl	$0x0, 0x3b36d3c
  43b62d: bd 18 e5 b2 03               	movl	$0x3b2e518, %ebp        # imm = 0x3B2E518
  43b632: 0f 85 7b 00 00 00            	jne	0x43b6b3 <.text+0x3a6b3>
  43b638: 8b 0d 18 f7 b2 03            	movl	0x3b2f718, %ecx
  43b63e: 85 c9                        	testl	%ecx, %ecx
  43b640: 74 71                        	je	0x43b6b3 <.text+0x3a6b3>
  43b642: 8b 41 0c                     	movl	0xc(%ecx), %eax
  43b645: 0f bf 09                     	movswl	(%ecx), %ecx
  43b648: 66 85 c9                     	testw	%cx, %cx
  43b64b: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  43b64f: 7e 62                        	jle	0x43b6b3 <.text+0x3a6b3>
  43b651: f7 40 20 03 00 40 00         	testl	$0x400003, 0x20(%eax)   # imm = 0x400003
  43b658: 74 4c                        	je	0x43b6a6 <.text+0x3a6a6>
  43b65a: 8b fd                        	movl	%ebp, %edi
  43b65c: b9 09 00 00 00               	movl	$0x9, %ecx
  43b661: 8b f0                        	movl	%eax, %esi
  43b663: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  43b665: 8b 70 20                     	movl	0x20(%eax), %esi
  43b668: 8b 0d 3c 6d b3 03            	movl	0x3b36d3c, %ecx
  43b66e: 89 33                        	movl	%esi, (%ebx)
  43b670: c7 43 08 00 00 00 00         	movl	$0x0, 0x8(%ebx)
  43b677: 8b 70 18                     	movl	0x18(%eax), %esi
  43b67a: 83 c5 24                     	addl	$0x24, %ebp
  43b67d: 41                           	incl	%ecx
  43b67e: 89 73 04                     	movl	%esi, 0x4(%ebx)
  43b681: 83 c3 0c                     	addl	$0xc, %ebx
  43b684: 66 42                        	incw	%dx
  43b686: 81 f9 80 00 00 00            	cmpl	$0x80, %ecx
  43b68c: 89 0d 3c 6d b3 03            	movl	%ecx, 0x3b36d3c
  43b692: 66 89 15 24 27 b3 03         	movw	%dx, 0x3b32724
  43b699: 7d 18                        	jge	0x43b6b3 <.text+0x3a6b3>
  43b69b: 66 81 fa 00 04               	cmpw	$0x400, %dx             # imm = 0x400
  43b6a0: 7d 11                        	jge	0x43b6b3 <.text+0x3a6b3>
  43b6a2: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
  43b6a6: 83 c0 24                     	addl	$0x24, %eax
  43b6a9: 49                           	decl	%ecx
  43b6aa: 66 85 c9                     	testw	%cx, %cx
  43b6ad: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  43b6b1: 7f 9e                        	jg	0x43b651 <.text+0x3a651>
  43b6b3: 5f                           	popl	%edi
  43b6b4: 5e                           	popl	%esi
  43b6b5: 5d                           	popl	%ebp
  43b6b6: 5b                           	popl	%ebx
  43b6b7: 59                           	popl	%ecx
  43b6b8: c3                           	retl
  43b6b9: 68 30 00 7e 00               	pushl	$0x7e0030               # imm = 0x7E0030
  43b6be: 66 89 15 24 27 b3 03         	movw	%dx, 0x3b32724
  43b6c5: e8 36 59 fc ff               	calll	0x401000 <.text>
  43b6ca: 66 8b 15 24 27 b3 03         	movw	0x3b32724, %dx
  43b6d1: 83 c4 04                     	addl	$0x4, %esp
  43b6d4: e9 43 ff ff ff               	jmp	0x43b61c <.text+0x3a61c>
