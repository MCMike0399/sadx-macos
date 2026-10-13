
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  44a800: 51                           	pushl	%ecx
  44a801: 53                           	pushl	%ebx
  44a802: 55                           	pushl	%ebp
  44a803: 56                           	pushl	%esi
  44a804: 8b 74 24 14                  	movl	0x14(%esp), %esi
  44a808: 66 8b 4e 06                  	movw	0x6(%esi), %cx
  44a80c: 66 8b 5e 04                  	movw	0x4(%esi), %bx
  44a810: 33 ed                        	xorl	%ebp, %ebp
  44a812: 66 3b cb                     	cmpw	%bx, %cx
  44a815: 57                           	pushl	%edi
  44a816: 66 89 4e 08                  	movw	%cx, 0x8(%esi)
  44a81a: 66 89 6e 0e                  	movw	%bp, 0xe(%esi)
  44a81e: ba 01 00 00 00               	movl	$0x1, %edx
  44a823: 74 1d                        	je	0x44a842 <.text+0x49842>
  44a825: 66 8b 06                     	movw	(%esi), %ax
  44a828: 66 3d 02 00                  	cmpw	$0x2, %ax
  44a82c: 74 14                        	je	0x44a842 <.text+0x49842>
  44a82e: 66 3b c5                     	cmpw	%bp, %ax
  44a831: 74 0f                        	je	0x44a842 <.text+0x49842>
  44a833: 66 3d 0e 00                  	cmpw	$0xe, %ax
  44a837: 74 09                        	je	0x44a842 <.text+0x49842>
  44a839: 66 89 16                     	movw	%dx, (%esi)
  44a83c: 66 89 6e 0c                  	movw	%bp, 0xc(%esi)
  44a840: eb 04                        	jmp	0x44a846 <.text+0x49846>
  44a842: 66 ff 46 0c                  	incw	0xc(%esi)
  44a846: 0f bf 06                     	movswl	(%esi), %eax
  44a849: 83 f8 0c                     	cmpl	$0xc, %eax
  44a84c: 0f 87 9d 05 00 00            	ja	0x44adef <.text+0x49def>
  44a852: ff 24 85 48 ae 44 00         	jmpl	*0x44ae48(,%eax,4)
  44a859: 6a 08                        	pushl	$0x8
  44a85b: 66 89 5e 06                  	movw	%bx, 0x6(%esi)
  44a85f: 66 89 16                     	movw	%dx, (%esi)
  44a862: 89 6e 14                     	movl	%ebp, 0x14(%esi)
  44a865: 89 6e 10                     	movl	%ebp, 0x10(%esi)
  44a868: e8 b3 09 fc ff               	calll	0x40b220 <.text+0xa220>
  44a86d: 8b 56 20                     	movl	0x20(%esi), %edx
  44a870: 0f b7 cb                     	movzwl	%bx, %ecx
  44a873: 89 46 28                     	movl	%eax, 0x28(%esi)
  44a876: c1 e1 04                     	shll	$0x4, %ecx
  44a879: 8b 0c 11                     	movl	(%ecx,%edx), %ecx
  44a87c: 8b 11                        	movl	(%ecx), %edx
  44a87e: 6a 0c                        	pushl	$0xc
  44a880: 89 10                        	movl	%edx, (%eax)
  44a882: e8 99 09 fc ff               	calll	0x40b220 <.text+0xa220>
  44a887: 8b 4e 28                     	movl	0x28(%esi), %ecx
  44a88a: 89 41 04                     	movl	%eax, 0x4(%ecx)
  44a88d: 8b 56 28                     	movl	0x28(%esi), %edx
  44a890: 8b 4a 04                     	movl	0x4(%edx), %ecx
  44a893: b8 02 00 00 00               	movl	$0x2, %eax
  44a898: 89 41 04                     	movl	%eax, 0x4(%ecx)
  44a89b: 8b 56 28                     	movl	0x28(%esi), %edx
  44a89e: 8b 4a 04                     	movl	0x4(%edx), %ecx
  44a8a1: 66 c7 41 08 03 00            	movw	$0x3, 0x8(%ecx)
  44a8a7: 8b 56 28                     	movl	0x28(%esi), %edx
  44a8aa: 8b 4a 04                     	movl	0x4(%edx), %ecx
  44a8ad: 6a 18                        	pushl	$0x18
  44a8af: 6a 6f                        	pushl	$0x6f
  44a8b1: 66 89 41 0a                  	movw	%ax, 0xa(%ecx)
  44a8b5: e8 16 0a fc ff               	calll	0x40b2d0 <.text+0xa2d0>
  44a8ba: 8b 56 28                     	movl	0x28(%esi), %edx
  44a8bd: 8b 4a 04                     	movl	0x4(%edx), %ecx
  44a8c0: 83 c4 10                     	addl	$0x10, %esp
  44a8c3: 5f                           	popl	%edi
  44a8c4: 5e                           	popl	%esi
  44a8c5: 5d                           	popl	%ebp
  44a8c6: 89 01                        	movl	%eax, (%ecx)
  44a8c8: 5b                           	popl	%ebx
  44a8c9: 59                           	popl	%ecx
  44a8ca: c3                           	retl
  44a8cb: 8b 46 20                     	movl	0x20(%esi), %eax
  44a8ce: 0f b7 fb                     	movzwl	%bx, %edi
  44a8d1: c1 e7 04                     	shll	$0x4, %edi
  44a8d4: ba 00 00 80 3f               	movl	$0x3f800000, %edx       # imm = 0x3F800000
  44a8d9: 39 54 38 08                  	cmpl	%edx, 0x8(%eax,%edi)
  44a8dd: 74 1e                        	je	0x44a8fd <.text+0x498fd>
  44a8df: 0f b7 c9                     	movzwl	%cx, %ecx
  44a8e2: c1 e1 04                     	shll	$0x4, %ecx
  44a8e5: 8b 0c 01                     	movl	(%ecx,%eax), %ecx
  44a8e8: 8b 04 38                     	movl	(%eax,%edi), %eax
  44a8eb: 8b 00                        	movl	(%eax), %eax
  44a8ed: 3b 01                        	cmpl	(%ecx), %eax
  44a8ef: 75 0c                        	jne	0x44a8fd <.text+0x498fd>
  44a8f1: 8b d6                        	movl	%esi, %edx
  44a8f3: e8 58 c0 ff ff               	calll	0x446950 <.text+0x45950>
  44a8f8: 89 6e 10                     	movl	%ebp, 0x10(%esi)
  44a8fb: eb 03                        	jmp	0x44a900 <.text+0x49900>
  44a8fd: 89 56 10                     	movl	%edx, 0x10(%esi)
  44a900: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44a903: 66 89 5e 06                  	movw	%bx, 0x6(%esi)
  44a907: 66 c7 06 02 00               	movw	$0x2, (%esi)
  44a90c: 66 0f b6 54 0f 06            	movzbw	0x6(%edi,%ecx), %dx
  44a912: 66 89 56 0a                  	movw	%dx, 0xa(%esi)
  44a916: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44a919: 0f b7 c3                     	movzwl	%bx, %eax
  44a91c: c1 e0 04                     	shll	$0x4, %eax
  44a91f: 03 c8                        	addl	%eax, %ecx
  44a921: d9 41 08                     	flds	0x8(%ecx)
  44a924: d8 46 10                     	fadds	0x10(%esi)
  44a927: d9 56 10                     	fsts	0x10(%esi)
  44a92a: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  44a930: df e0                        	fnstsw	%ax
  44a932: f6 c4 01                     	testb	$0x1, %ah
  44a935: 0f 85 04 05 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44a93b: 66 0f be 51 05               	movsbw	0x5(%ecx), %dx
  44a940: 66 89 16                     	movw	%dx, (%esi)
  44a943: e9 74 03 00 00               	jmp	0x44acbc <.text+0x49cbc>
  44a948: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44a94b: 0f b7 c3                     	movzwl	%bx, %eax
  44a94e: c1 e0 04                     	shll	$0x4, %eax
  44a951: 03 c1                        	addl	%ecx, %eax
  44a953: 8b 08                        	movl	(%eax), %ecx
  44a955: d9 40 0c                     	flds	0xc(%eax)
  44a958: d8 46 10                     	fadds	0x10(%esi)
  44a95b: d9 54 24 18                  	fsts	0x18(%esp)
  44a95f: d9 5e 10                     	fstps	0x10(%esi)
  44a962: 8b 51 04                     	movl	0x4(%ecx), %edx
  44a965: 8b 42 04                     	movl	0x4(%edx), %eax
  44a968: db 42 04                     	fildl	0x4(%edx)
  44a96b: 85 c0                        	testl	%eax, %eax
  44a96d: 7d 06                        	jge	0x44a975 <.text+0x49975>
  44a96f: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44a975: d8 54 24 18                  	fcoms	0x18(%esp)
  44a979: df e0                        	fnstsw	%ax
  44a97b: f6 c4 41                     	testb	$0x41, %ah
  44a97e: 7a 27                        	jp	0x44a9a7 <.text+0x499a7>
  44a980: d9 46 10                     	flds	0x10(%esi)
  44a983: d8 e1                        	fsub	%st(1), %st
  44a985: d9 5e 10                     	fstps	0x10(%esi)
  44a988: 8b 51 04                     	movl	0x4(%ecx), %edx
  44a98b: 8b 42 04                     	movl	0x4(%edx), %eax
  44a98e: db 42 04                     	fildl	0x4(%edx)
  44a991: 85 c0                        	testl	%eax, %eax
  44a993: 7d 06                        	jge	0x44a99b <.text+0x4999b>
  44a995: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44a99b: dd d9                        	fstp	%st(1)
  44a99d: d8 56 10                     	fcoms	0x10(%esi)
  44a9a0: df e0                        	fnstsw	%ax
  44a9a2: f6 c4 41                     	testb	$0x41, %ah
  44a9a5: 7b d9                        	jnp	0x44a980 <.text+0x49980>
  44a9a7: dd d8                        	fstp	%st(0)
  44a9a9: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44a9af: d8 5e 10                     	fcomps	0x10(%esi)
  44a9b2: df e0                        	fnstsw	%ax
  44a9b4: f6 c4 41                     	testb	$0x41, %ah
  44a9b7: 0f 85 82 04 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44a9bd: 8d 49 00                     	leal	(%ecx), %ecx
  44a9c0: 8b 51 04                     	movl	0x4(%ecx), %edx
  44a9c3: 8b 42 04                     	movl	0x4(%edx), %eax
  44a9c6: db 42 04                     	fildl	0x4(%edx)
  44a9c9: 85 c0                        	testl	%eax, %eax
  44a9cb: 7d 06                        	jge	0x44a9d3 <.text+0x499d3>
  44a9cd: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44a9d3: d8 46 10                     	fadds	0x10(%esi)
  44a9d6: d9 5e 10                     	fstps	0x10(%esi)
  44a9d9: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44a9df: d8 5e 10                     	fcomps	0x10(%esi)
  44a9e2: df e0                        	fnstsw	%ax
  44a9e4: f6 c4 41                     	testb	$0x41, %ah
  44a9e7: 74 d7                        	je	0x44a9c0 <.text+0x499c0>
  44a9e9: 5f                           	popl	%edi
  44a9ea: 5e                           	popl	%esi
  44a9eb: 5d                           	popl	%ebp
  44a9ec: 5b                           	popl	%ebx
  44a9ed: 59                           	popl	%ecx
  44a9ee: c3                           	retl
  44a9ef: 8b 56 20                     	movl	0x20(%esi), %edx
  44a9f2: 0f b7 cb                     	movzwl	%bx, %ecx
  44a9f5: c1 e1 04                     	shll	$0x4, %ecx
  44a9f8: 8d 04 11                     	leal	(%ecx,%edx), %eax
  44a9fb: 8b 56 18                     	movl	0x18(%esi), %edx
  44a9fe: 8b 08                        	movl	(%eax), %ecx
  44aa00: d9 02                        	flds	(%edx)
  44aa02: d8 48 0c                     	fmuls	0xc(%eax)
  44aa05: d8 46 10                     	fadds	0x10(%esi)
  44aa08: d9 54 24 18                  	fsts	0x18(%esp)
  44aa0c: d9 5e 10                     	fstps	0x10(%esi)
  44aa0f: 8b 41 04                     	movl	0x4(%ecx), %eax
  44aa12: 8b 50 04                     	movl	0x4(%eax), %edx
  44aa15: db 40 04                     	fildl	0x4(%eax)
  44aa18: 85 d2                        	testl	%edx, %edx
  44aa1a: 7d 06                        	jge	0x44aa22 <.text+0x49a22>
  44aa1c: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44aa22: d8 54 24 18                  	fcoms	0x18(%esp)
  44aa26: df e0                        	fnstsw	%ax
  44aa28: f6 c4 41                     	testb	$0x41, %ah
  44aa2b: 7a 2a                        	jp	0x44aa57 <.text+0x49a57>
  44aa2d: 8d 49 00                     	leal	(%ecx), %ecx
  44aa30: d9 46 10                     	flds	0x10(%esi)
  44aa33: d8 e1                        	fsub	%st(1), %st
  44aa35: d9 5e 10                     	fstps	0x10(%esi)
  44aa38: 8b 41 04                     	movl	0x4(%ecx), %eax
  44aa3b: 8b 50 04                     	movl	0x4(%eax), %edx
  44aa3e: db 40 04                     	fildl	0x4(%eax)
  44aa41: 85 d2                        	testl	%edx, %edx
  44aa43: 7d 06                        	jge	0x44aa4b <.text+0x49a4b>
  44aa45: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44aa4b: dd d9                        	fstp	%st(1)
  44aa4d: d8 56 10                     	fcoms	0x10(%esi)
  44aa50: df e0                        	fnstsw	%ax
  44aa52: f6 c4 41                     	testb	$0x41, %ah
  44aa55: 7b d9                        	jnp	0x44aa30 <.text+0x49a30>
  44aa57: dd d8                        	fstp	%st(0)
  44aa59: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44aa5f: d8 5e 10                     	fcomps	0x10(%esi)
  44aa62: df e0                        	fnstsw	%ax
  44aa64: f6 c4 41                     	testb	$0x41, %ah
  44aa67: 0f 85 d2 03 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44aa6d: 8d 49 00                     	leal	(%ecx), %ecx
  44aa70: 8b 41 04                     	movl	0x4(%ecx), %eax
  44aa73: 8b 50 04                     	movl	0x4(%eax), %edx
  44aa76: db 40 04                     	fildl	0x4(%eax)
  44aa79: 85 d2                        	testl	%edx, %edx
  44aa7b: 7d 06                        	jge	0x44aa83 <.text+0x49a83>
  44aa7d: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44aa83: d8 46 10                     	fadds	0x10(%esi)
  44aa86: d9 5e 10                     	fstps	0x10(%esi)
  44aa89: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44aa8f: d8 5e 10                     	fcomps	0x10(%esi)
  44aa92: df e0                        	fnstsw	%ax
  44aa94: f6 c4 41                     	testb	$0x41, %ah
  44aa97: 74 d7                        	je	0x44aa70 <.text+0x49a70>
  44aa99: 5f                           	popl	%edi
  44aa9a: 5e                           	popl	%esi
  44aa9b: 5d                           	popl	%ebp
  44aa9c: 5b                           	popl	%ebx
  44aa9d: 59                           	popl	%ecx
  44aa9e: c3                           	retl
  44aa9f: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44aaa2: 8b 56 1c                     	movl	0x1c(%esi), %edx
  44aaa5: d9 02                        	flds	(%edx)
  44aaa7: 0f b7 c3                     	movzwl	%bx, %eax
  44aaaa: c1 e0 04                     	shll	$0x4, %eax
  44aaad: 03 c1                        	addl	%ecx, %eax
  44aaaf: 8b 08                        	movl	(%eax), %ecx
  44aab1: d8 48 0c                     	fmuls	0xc(%eax)
  44aab4: d8 46 10                     	fadds	0x10(%esi)
  44aab7: d9 54 24 18                  	fsts	0x18(%esp)
  44aabb: d9 5e 10                     	fstps	0x10(%esi)
  44aabe: 8b 41 04                     	movl	0x4(%ecx), %eax
  44aac1: 8b 50 04                     	movl	0x4(%eax), %edx
  44aac4: db 40 04                     	fildl	0x4(%eax)
  44aac7: 85 d2                        	testl	%edx, %edx
  44aac9: 7d 06                        	jge	0x44aad1 <.text+0x49ad1>
  44aacb: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44aad1: d8 54 24 18                  	fcoms	0x18(%esp)
  44aad5: df e0                        	fnstsw	%ax
  44aad7: f6 c4 41                     	testb	$0x41, %ah
  44aada: 7a 2b                        	jp	0x44ab07 <.text+0x49b07>
  44aadc: 8d 64 24 00                  	leal	(%esp), %esp
  44aae0: d9 46 10                     	flds	0x10(%esi)
  44aae3: d8 e1                        	fsub	%st(1), %st
  44aae5: d9 5e 10                     	fstps	0x10(%esi)
  44aae8: 8b 41 04                     	movl	0x4(%ecx), %eax
  44aaeb: 8b 50 04                     	movl	0x4(%eax), %edx
  44aaee: db 40 04                     	fildl	0x4(%eax)
  44aaf1: 85 d2                        	testl	%edx, %edx
  44aaf3: 7d 06                        	jge	0x44aafb <.text+0x49afb>
  44aaf5: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44aafb: dd d9                        	fstp	%st(1)
  44aafd: d8 56 10                     	fcoms	0x10(%esi)
  44ab00: df e0                        	fnstsw	%ax
  44ab02: f6 c4 41                     	testb	$0x41, %ah
  44ab05: 7b d9                        	jnp	0x44aae0 <.text+0x49ae0>
  44ab07: dd d8                        	fstp	%st(0)
  44ab09: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44ab0f: d8 5e 10                     	fcomps	0x10(%esi)
  44ab12: df e0                        	fnstsw	%ax
  44ab14: f6 c4 41                     	testb	$0x41, %ah
  44ab17: 0f 85 22 03 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44ab1d: 8d 49 00                     	leal	(%ecx), %ecx
  44ab20: 8b 41 04                     	movl	0x4(%ecx), %eax
  44ab23: 8b 50 04                     	movl	0x4(%eax), %edx
  44ab26: db 40 04                     	fildl	0x4(%eax)
  44ab29: 85 d2                        	testl	%edx, %edx
  44ab2b: 7d 06                        	jge	0x44ab33 <.text+0x49b33>
  44ab2d: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44ab33: d8 46 10                     	fadds	0x10(%esi)
  44ab36: d9 5e 10                     	fstps	0x10(%esi)
  44ab39: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44ab3f: d8 5e 10                     	fcomps	0x10(%esi)
  44ab42: df e0                        	fnstsw	%ax
  44ab44: f6 c4 41                     	testb	$0x41, %ah
  44ab47: 74 d7                        	je	0x44ab20 <.text+0x49b20>
  44ab49: 5f                           	popl	%edi
  44ab4a: 5e                           	popl	%esi
  44ab4b: 5d                           	popl	%ebp
  44ab4c: 5b                           	popl	%ebx
  44ab4d: 59                           	popl	%ecx
  44ab4e: c3                           	retl
  44ab4f: 8b 56 1c                     	movl	0x1c(%esi), %edx
  44ab52: d9 02                        	flds	(%edx)
  44ab54: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44ab57: d8 46 10                     	fadds	0x10(%esi)
  44ab5a: 0f b7 c3                     	movzwl	%bx, %eax
  44ab5d: c1 e0 04                     	shll	$0x4, %eax
  44ab60: 8b 04 08                     	movl	(%eax,%ecx), %eax
  44ab63: d9 56 10                     	fsts	0x10(%esi)
  44ab66: 8b 40 04                     	movl	0x4(%eax), %eax
  44ab69: 8b 48 04                     	movl	0x4(%eax), %ecx
  44ab6c: db 40 04                     	fildl	0x4(%eax)
  44ab6f: 85 c9                        	testl	%ecx, %ecx
  44ab71: 7d 06                        	jge	0x44ab79 <.text+0x49b79>
  44ab73: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44ab79: d8 25 10 cb 7d 00            	fsubs	0x7dcb10
  44ab7f: d8 d9                        	fcomp	%st(1)
  44ab81: df e0                        	fnstsw	%ax
  44ab83: dd d8                        	fstp	%st(0)
  44ab85: f6 c4 41                     	testb	$0x41, %ah
  44ab88: 0f 8a b1 02 00 00            	jp	0x44ae3f <.text+0x49e3f>
  44ab8e: 5f                           	popl	%edi
  44ab8f: 8b ce                        	movl	%esi, %ecx
  44ab91: 5e                           	popl	%esi
  44ab92: 5d                           	popl	%ebp
  44ab93: 8b d3                        	movl	%ebx, %edx
  44ab95: 5b                           	popl	%ebx
  44ab96: 83 c4 04                     	addl	$0x4, %esp
  44ab99: e9 72 bd ff ff               	jmp	0x446910 <.text+0x45910>
  44ab9e: 8b 46 20                     	movl	0x20(%esi), %eax
  44aba1: 0f b7 d3                     	movzwl	%bx, %edx
  44aba4: c1 e2 04                     	shll	$0x4, %edx
  44aba7: 8b 0c 10                     	movl	(%eax,%edx), %ecx
  44abaa: 03 c2                        	addl	%edx, %eax
  44abac: d9 40 0c                     	flds	0xc(%eax)
  44abaf: d8 46 10                     	fadds	0x10(%esi)
  44abb2: d9 56 10                     	fsts	0x10(%esi)
  44abb5: 8b 49 04                     	movl	0x4(%ecx), %ecx
  44abb8: 8b 51 04                     	movl	0x4(%ecx), %edx
  44abbb: db 41 04                     	fildl	0x4(%ecx)
  44abbe: 85 d2                        	testl	%edx, %edx
  44abc0: eb af                        	jmp	0x44ab71 <.text+0x49b71>
  44abc2: d9 46 10                     	flds	0x10(%esi)
  44abc5: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44abc8: 0f b7 c3                     	movzwl	%bx, %eax
  44abcb: c1 e0 04                     	shll	$0x4, %eax
  44abce: d8 64 08 0c                  	fsubs	0xc(%eax,%ecx)
  44abd2: d9 56 10                     	fsts	0x10(%esi)
  44abd5: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44abdb: d8 d9                        	fcomp	%st(1)
  44abdd: df e0                        	fnstsw	%ax
  44abdf: dd d8                        	fstp	%st(0)
  44abe1: f6 c4 41                     	testb	$0x41, %ah
  44abe4: 0f 85 55 02 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44abea: 5f                           	popl	%edi
  44abeb: 8b ce                        	movl	%esi, %ecx
  44abed: 5e                           	popl	%esi
  44abee: 5d                           	popl	%ebp
  44abef: 8b d3                        	movl	%ebx, %edx
  44abf1: 5b                           	popl	%ebx
  44abf2: 83 c4 04                     	addl	$0x4, %esp
  44abf5: e9 16 bd ff ff               	jmp	0x446910 <.text+0x45910>
  44abfa: 8b 56 20                     	movl	0x20(%esi), %edx
  44abfd: 0f b7 c3                     	movzwl	%bx, %eax
  44ac00: c1 e0 04                     	shll	$0x4, %eax
  44ac03: 8d 0c 10                     	leal	(%eax,%edx), %ecx
  44ac06: 8b 01                        	movl	(%ecx), %eax
  44ac08: d9 41 0c                     	flds	0xc(%ecx)
  44ac0b: d8 46 10                     	fadds	0x10(%esi)
  44ac0e: d9 56 10                     	fsts	0x10(%esi)
  44ac11: 8b 40 04                     	movl	0x4(%eax), %eax
  44ac14: 8b 40 04                     	movl	0x4(%eax), %eax
  44ac17: 85 c0                        	testl	%eax, %eax
  44ac19: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  44ac1d: db 44 24 18                  	fildl	0x18(%esp)
  44ac21: 7d 06                        	jge	0x44ac29 <.text+0x49c29>
  44ac23: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44ac29: d8 25 10 cb 7d 00            	fsubs	0x7dcb10
  44ac2f: d8 d9                        	fcomp	%st(1)
  44ac31: df e0                        	fnstsw	%ax
  44ac33: dd d8                        	fstp	%st(0)
  44ac35: f6 c4 41                     	testb	$0x41, %ah
  44ac38: 0f 8a 01 02 00 00            	jp	0x44ae3f <.text+0x49e3f>
  44ac3e: 66 89 6e 0c                  	movw	%bp, 0xc(%esi)
  44ac42: 81 79 08 00 00 80 3f         	cmpl	$0x3f800000, 0x8(%ecx)  # imm = 0x3F800000
  44ac49: 74 2b                        	je	0x44ac76 <.text+0x49c76>
  44ac4b: 66 8b 46 0a                  	movw	0xa(%esi), %ax
  44ac4f: 8b 09                        	movl	(%ecx), %ecx
  44ac51: 8b 09                        	movl	(%ecx), %ecx
  44ac53: 0f b7 f8                     	movzwl	%ax, %edi
  44ac56: c1 e7 04                     	shll	$0x4, %edi
  44ac59: 8b 14 17                     	movl	(%edi,%edx), %edx
  44ac5c: 3b 0a                        	cmpl	(%edx), %ecx
  44ac5e: 75 16                        	jne	0x44ac76 <.text+0x49c76>
  44ac60: 8b d8                        	movl	%eax, %ebx
  44ac62: 66 0f b6 d3                  	movzbw	%bl, %dx
  44ac66: 66 89 56 04                  	movw	%dx, 0x4(%esi)
  44ac6a: 8b d6                        	movl	%esi, %edx
  44ac6c: e8 df bc ff ff               	calll	0x446950 <.text+0x45950>
  44ac71: 89 6e 10                     	movl	%ebp, 0x10(%esi)
  44ac74: eb 07                        	jmp	0x44ac7d <.text+0x49c7d>
  44ac76: c7 46 10 00 00 80 3f         	movl	$0x3f800000, 0x10(%esi) # imm = 0x3F800000
  44ac7d: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44ac80: 0f b7 c3                     	movzwl	%bx, %eax
  44ac83: c1 e0 04                     	shll	$0x4, %eax
  44ac86: 03 c8                        	addl	%eax, %ecx
  44ac88: 66 89 5e 06                  	movw	%bx, 0x6(%esi)
  44ac8c: 66 0f b6 51 06               	movzbw	0x6(%ecx), %dx
  44ac91: 66 89 56 0a                  	movw	%dx, 0xa(%esi)
  44ac95: 66 c7 06 02 00               	movw	$0x2, (%esi)
  44ac9a: d9 41 08                     	flds	0x8(%ecx)
  44ac9d: d8 46 10                     	fadds	0x10(%esi)
  44aca0: d9 56 10                     	fsts	0x10(%esi)
  44aca3: d8 1d 10 cb 7d 00            	fcomps	0x7dcb10
  44aca9: df e0                        	fnstsw	%ax
  44acab: f6 c4 01                     	testb	$0x1, %ah
  44acae: 0f 85 8b 01 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44acb4: 66 0f be 41 05               	movsbw	0x5(%ecx), %ax
  44acb9: 66 89 06                     	movw	%ax, (%esi)
  44acbc: 8b c6                        	movl	%esi, %eax
  44acbe: e8 2d 4d ff ff               	calll	0x43f9f0 <.text+0x3e9f0>
  44acc3: 5f                           	popl	%edi
  44acc4: 8b ce                        	movl	%esi, %ecx
  44acc6: 5e                           	popl	%esi
  44acc7: 5d                           	popl	%ebp
  44acc8: 5b                           	popl	%ebx
  44acc9: 83 c4 04                     	addl	$0x4, %esp
  44accc: e9 5f 45 ff ff               	jmp	0x43f230 <.text+0x3e230>
  44acd1: 8b 46 20                     	movl	0x20(%esi), %eax
  44acd4: 0f b7 cb                     	movzwl	%bx, %ecx
  44acd7: c1 e1 04                     	shll	$0x4, %ecx
  44acda: 03 c1                        	addl	%ecx, %eax
  44acdc: 8b 08                        	movl	(%eax), %ecx
  44acde: d9 40 0c                     	flds	0xc(%eax)
  44ace1: d8 46 10                     	fadds	0x10(%esi)
  44ace4: d9 56 10                     	fsts	0x10(%esi)
  44ace7: 8b 49 04                     	movl	0x4(%ecx), %ecx
  44acea: 8b 41 04                     	movl	0x4(%ecx), %eax
  44aced: db 41 04                     	fildl	0x4(%ecx)
  44acf0: 85 c0                        	testl	%eax, %eax
  44acf2: 7d 06                        	jge	0x44acfa <.text+0x49cfa>
  44acf4: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44acfa: d8 25 10 cb 7d 00            	fsubs	0x7dcb10
  44ad00: d9 54 24 18                  	fsts	0x18(%esp)
  44ad04: d8 d9                        	fcomp	%st(1)
  44ad06: df e0                        	fnstsw	%ax
  44ad08: dd d8                        	fstp	%st(0)
  44ad0a: f6 c4 41                     	testb	$0x41, %ah
  44ad0d: 0f 8a 2c 01 00 00            	jp	0x44ae3f <.text+0x49e3f>
  44ad13: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  44ad17: 5f                           	popl	%edi
  44ad18: 89 4e 10                     	movl	%ecx, 0x10(%esi)
  44ad1b: 66 89 56 0e                  	movw	%dx, 0xe(%esi)
  44ad1f: 5e                           	popl	%esi
  44ad20: 5d                           	popl	%ebp
  44ad21: 5b                           	popl	%ebx
  44ad22: 59                           	popl	%ecx
  44ad23: c3                           	retl
  44ad24: d9 46 10                     	flds	0x10(%esi)
  44ad27: 8b 4e 20                     	movl	0x20(%esi), %ecx
  44ad2a: 0f b7 c3                     	movzwl	%bx, %eax
  44ad2d: c1 e0 04                     	shll	$0x4, %eax
  44ad30: d8 64 08 0c                  	fsubs	0xc(%eax,%ecx)
  44ad34: d9 56 10                     	fsts	0x10(%esi)
  44ad37: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44ad3d: d8 d9                        	fcomp	%st(1)
  44ad3f: df e0                        	fnstsw	%ax
  44ad41: dd d8                        	fstp	%st(0)
  44ad43: f6 c4 41                     	testb	$0x41, %ah
  44ad46: 0f 85 f3 00 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44ad4c: 5f                           	popl	%edi
  44ad4d: 89 6e 10                     	movl	%ebp, 0x10(%esi)
  44ad50: 66 89 56 0e                  	movw	%dx, 0xe(%esi)
  44ad54: 5e                           	popl	%esi
  44ad55: 5d                           	popl	%ebp
  44ad56: 5b                           	popl	%ebx
  44ad57: 59                           	popl	%ecx
  44ad58: c3                           	retl
  44ad59: 8b 46 20                     	movl	0x20(%esi), %eax
  44ad5c: 0f b7 d3                     	movzwl	%bx, %edx
  44ad5f: c1 e2 04                     	shll	$0x4, %edx
  44ad62: 8b 0c 02                     	movl	(%edx,%eax), %ecx
  44ad65: 8b 51 04                     	movl	0x4(%ecx), %edx
  44ad68: 8b 42 04                     	movl	0x4(%edx), %eax
  44ad6b: 85 c0                        	testl	%eax, %eax
  44ad6d: db 42 04                     	fildl	0x4(%edx)
  44ad70: 7d 06                        	jge	0x44ad78 <.text+0x49d78>
  44ad72: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44ad78: d8 56 10                     	fcoms	0x10(%esi)
  44ad7b: df e0                        	fnstsw	%ax
  44ad7d: f6 c4 41                     	testb	$0x41, %ah
  44ad80: 7a 27                        	jp	0x44ada9 <.text+0x49da9>
  44ad82: d9 46 10                     	flds	0x10(%esi)
  44ad85: d8 e1                        	fsub	%st(1), %st
  44ad87: d9 5e 10                     	fstps	0x10(%esi)
  44ad8a: 8b 51 04                     	movl	0x4(%ecx), %edx
  44ad8d: 8b 42 04                     	movl	0x4(%edx), %eax
  44ad90: db 42 04                     	fildl	0x4(%edx)
  44ad93: 85 c0                        	testl	%eax, %eax
  44ad95: 7d 06                        	jge	0x44ad9d <.text+0x49d9d>
  44ad97: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44ad9d: dd d9                        	fstp	%st(1)
  44ad9f: d8 56 10                     	fcoms	0x10(%esi)
  44ada2: df e0                        	fnstsw	%ax
  44ada4: f6 c4 41                     	testb	$0x41, %ah
  44ada7: 7b d9                        	jnp	0x44ad82 <.text+0x49d82>
  44ada9: dd d8                        	fstp	%st(0)
  44adab: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44adb1: d8 5e 10                     	fcomps	0x10(%esi)
  44adb4: df e0                        	fnstsw	%ax
  44adb6: f6 c4 41                     	testb	$0x41, %ah
  44adb9: 0f 85 80 00 00 00            	jne	0x44ae3f <.text+0x49e3f>
  44adbf: 90                           	nop
  44adc0: 8b 51 04                     	movl	0x4(%ecx), %edx
  44adc3: 8b 42 04                     	movl	0x4(%edx), %eax
  44adc6: db 42 04                     	fildl	0x4(%edx)
  44adc9: 85 c0                        	testl	%eax, %eax
  44adcb: 7d 06                        	jge	0x44add3 <.text+0x49dd3>
  44adcd: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  44add3: d8 46 10                     	fadds	0x10(%esi)
  44add6: d9 5e 10                     	fstps	0x10(%esi)
  44add9: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  44addf: d8 5e 10                     	fcomps	0x10(%esi)
  44ade2: df e0                        	fnstsw	%ax
  44ade4: f6 c4 41                     	testb	$0x41, %ah
  44ade7: 74 d7                        	je	0x44adc0 <.text+0x49dc0>
  44ade9: 5f                           	popl	%edi
  44adea: 5e                           	popl	%esi
  44adeb: 5d                           	popl	%ebp
  44adec: 5b                           	popl	%ebx
  44aded: 59                           	popl	%ecx
  44adee: c3                           	retl
  44adef: 8b c6                        	movl	%esi, %eax
  44adf1: e8 fa 4b ff ff               	calll	0x43f9f0 <.text+0x3e9f0>
  44adf6: 8b 46 28                     	movl	0x28(%esi), %eax
  44adf9: 3b c5                        	cmpl	%ebp, %eax
  44adfb: 74 42                        	je	0x44ae3f <.text+0x49e3f>
  44adfd: 8b 48 04                     	movl	0x4(%eax), %ecx
  44ae00: 8b 01                        	movl	(%ecx), %eax
  44ae02: 3b c5                        	cmpl	%ebp, %eax
  44ae04: 74 11                        	je	0x44ae17 <.text+0x49e17>
  44ae06: 50                           	pushl	%eax
  44ae07: e8 04 05 fc ff               	calll	0x40b310 <.text+0xa310>
  44ae0c: 8b 56 28                     	movl	0x28(%esi), %edx
  44ae0f: 8b 42 04                     	movl	0x4(%edx), %eax
  44ae12: 83 c4 04                     	addl	$0x4, %esp
  44ae15: 89 28                        	movl	%ebp, (%eax)
  44ae17: 8b 4e 28                     	movl	0x28(%esi), %ecx
  44ae1a: 8b 41 04                     	movl	0x4(%ecx), %eax
  44ae1d: 3b c5                        	cmpl	%ebp, %eax
  44ae1f: 74 0f                        	je	0x44ae30 <.text+0x49e30>
  44ae21: 50                           	pushl	%eax
  44ae22: e8 e9 04 fc ff               	calll	0x40b310 <.text+0xa310>
  44ae27: 8b 56 28                     	movl	0x28(%esi), %edx
  44ae2a: 83 c4 04                     	addl	$0x4, %esp
  44ae2d: 89 6a 04                     	movl	%ebp, 0x4(%edx)
  44ae30: 8b 46 28                     	movl	0x28(%esi), %eax
  44ae33: 50                           	pushl	%eax
  44ae34: e8 d7 04 fc ff               	calll	0x40b310 <.text+0xa310>
  44ae39: 83 c4 04                     	addl	$0x4, %esp
  44ae3c: 89 6e 28                     	movl	%ebp, 0x28(%esi)
  44ae3f: 5f                           	popl	%edi
  44ae40: 5e                           	popl	%esi
