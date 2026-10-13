
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  40fdc0: e8 eb 48 00 00               	calll	0x4146b0 <.text+0x136b0>
  40fdc5: 83 f8 01                     	cmpl	$0x1, %eax
  40fdc8: 75 16                        	jne	0x40fde0 <.text+0xede0>
  40fdca: c7 05 94 9f 90 00 02 00 00 00	movl	$0x2, 0x909f94
  40fdd4: c7 05 a4 9f 90 00 02 04 00 00	movl	$0x402, 0x909fa4        # imm = 0x402
  40fdde: eb 0f                        	jmp	0x40fdef <.text+0xedef>
  40fde0: b8 02 04 00 00               	movl	$0x402, %eax            # imm = 0x402
  40fde5: a3 94 9f 90 00               	movl	%eax, 0x909f94
  40fdea: a3 a4 9f 90 00               	movl	%eax, 0x909fa4
  40fdef: a0 b0 9f 90 00               	movb	0x909fb0, %al
  40fdf4: 53                           	pushl	%ebx
  40fdf5: 33 db                        	xorl	%ebx, %ebx
  40fdf7: 3c 01                        	cmpb	$0x1, %al
  40fdf9: 0f 85 ad 00 00 00            	jne	0x40feac <.text+0xeeac>
  40fdff: 55                           	pushl	%ebp
  40fe00: 56                           	pushl	%esi
  40fe01: 33 ed                        	xorl	%ebp, %ebp
  40fe03: 57                           	pushl	%edi
  40fe04: 83 ca ff                     	orl	$-0x1, %edx
  40fe07: 3b eb                        	cmpl	%ebx, %ebp
  40fe09: 75 1d                        	jne	0x40fe28 <.text+0xee28>
  40fe0b: 80 3d b4 9f 90 00 01         	cmpb	$0x1, 0x909fb4
  40fe12: 75 36                        	jne	0x40fe4a <.text+0xee4a>
  40fe14: 8b 35 7c e7 b0 03            	movl	0x3b0e77c, %esi
  40fe1a: b9 0d 00 00 00               	movl	$0xd, %ecx
  40fe1f: bf f0 e7 b0 03               	movl	$0x3b0e7f0, %edi        # imm = 0x3B0E7F0
  40fe24: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  40fe26: eb 75                        	jmp	0x40fe9d <.text+0xee9d>
  40fe28: 83 fd 01                     	cmpl	$0x1, %ebp
  40fe2b: 75 1d                        	jne	0x40fe4a <.text+0xee4a>
  40fe2d: 80 3d b5 9f 90 00 01         	cmpb	$0x1, 0x909fb5
  40fe34: 75 14                        	jne	0x40fe4a <.text+0xee4a>
  40fe36: 8b 35 80 e7 b0 03            	movl	0x3b0e780, %esi
  40fe3c: b9 0d 00 00 00               	movl	$0xd, %ecx
  40fe41: bf 24 e8 b0 03               	movl	$0x3b0e824, %edi        # imm = 0x3B0E824
  40fe46: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  40fe48: eb 53                        	jmp	0x40fe9d <.text+0xee9d>
  40fe4a: 8b c5                        	movl	%ebp, %eax
  40fe4c: 6b c0 34                     	imull	$0x34, %eax, %eax
  40fe4f: 8b 88 f8 e7 b0 03            	movl	0x3b0e7f8(%eax), %ecx
  40fe55: 89 88 1c e8 b0 03            	movl	%ecx, 0x3b0e81c(%eax)
  40fe5b: 89 98 f8 e7 b0 03            	movl	%ebx, 0x3b0e7f8(%eax)
  40fe61: 89 90 fc e7 b0 03            	movl	%edx, 0x3b0e7fc(%eax)
  40fe67: 89 98 00 e8 b0 03            	movl	%ebx, 0x3b0e800(%eax)
  40fe6d: 89 98 04 e8 b0 03            	movl	%ebx, 0x3b0e804(%eax)
  40fe73: 66 89 98 0c e8 b0 03         	movw	%bx, 0x3b0e80c(%eax)
  40fe7a: 66 89 98 0e e8 b0 03         	movw	%bx, 0x3b0e80e(%eax)
  40fe81: 66 89 98 0a e8 b0 03         	movw	%bx, 0x3b0e80a(%eax)
  40fe88: 66 89 98 08 e8 b0 03         	movw	%bx, 0x3b0e808(%eax)
  40fe8f: 66 89 98 10 e8 b0 03         	movw	%bx, 0x3b0e810(%eax)
  40fe96: 66 89 98 12 e8 b0 03         	movw	%bx, 0x3b0e812(%eax)
  40fe9d: 45                           	incl	%ebp
  40fe9e: 83 fd 08                     	cmpl	$0x8, %ebp
  40fea1: 0f 8c 60 ff ff ff            	jl	0x40fe07 <.text+0xee07>
  40fea7: 5f                           	popl	%edi
  40fea8: 5e                           	popl	%esi
  40fea9: 5d                           	popl	%ebp
  40feaa: eb 3a                        	jmp	0x40fee6 <.text+0xeee6>
  40feac: b8 f8 e7 b0 03               	movl	$0x3b0e7f8, %eax        # imm = 0x3B0E7F8
  40feb1: 83 ca ff                     	orl	$-0x1, %edx
  40feb4: 8b 08                        	movl	(%eax), %ecx
  40feb6: 89 48 24                     	movl	%ecx, 0x24(%eax)
  40feb9: 89 18                        	movl	%ebx, (%eax)
  40febb: 89 50 04                     	movl	%edx, 0x4(%eax)
  40febe: 89 58 08                     	movl	%ebx, 0x8(%eax)
  40fec1: 89 58 0c                     	movl	%ebx, 0xc(%eax)
  40fec4: 66 89 58 14                  	movw	%bx, 0x14(%eax)
  40fec8: 66 89 58 16                  	movw	%bx, 0x16(%eax)
  40fecc: 66 89 58 12                  	movw	%bx, 0x12(%eax)
  40fed0: 66 89 58 10                  	movw	%bx, 0x10(%eax)
  40fed4: 66 89 58 18                  	movw	%bx, 0x18(%eax)
  40fed8: 66 89 58 1a                  	movw	%bx, 0x1a(%eax)
  40fedc: 83 c0 34                     	addl	$0x34, %eax
  40fedf: 3d 98 e9 b0 03               	cmpl	$0x3b0e998, %eax        # imm = 0x3B0E998
  40fee4: 7c ce                        	jl	0x40feb4 <.text+0xeeb4>
  40fee6: e8 85 f2 ff ff               	calll	0x40f170 <.text+0xe170>
  40feeb: eb 03                        	jmp	0x40fef0 <.text+0xeef0>
  40feed: 8d 49 00                     	leal	(%ecx), %ecx
  40fef0: 53                           	pushl	%ebx
  40fef1: e8 7a f1 ff ff               	calll	0x40f070 <.text+0xe070>
  40fef6: 83 c4 04                     	addl	$0x4, %esp
  40fef9: 43                           	incl	%ebx
  40fefa: 83 fb 04                     	cmpl	$0x4, %ebx
  40fefd: 7c f1                        	jl	0x40fef0 <.text+0xeef0>
  40feff: 5b                           	popl	%ebx
  40ff00: c3                           	retl
  40ff01: 90                           	nop
  40ff02: 90                           	nop
  40ff03: 90                           	nop
  40ff04: 90                           	nop
  40ff05: 90                           	nop
  40ff06: 90                           	nop
  40ff07: 90                           	nop
  40ff08: 90                           	nop
  40ff09: 90                           	nop
  40ff0a: 90                           	nop
  40ff0b: 90                           	nop
  40ff0c: 90                           	nop
  40ff0d: 90                           	nop
  40ff0e: 90                           	nop
  40ff0f: 90                           	nop
  40ff10: a1 2c ef b0 03               	movl	0x3b0ef2c, %eax
  40ff15: c3                           	retl
  40ff16: 90                           	nop
  40ff17: 90                           	nop
  40ff18: 90                           	nop
  40ff19: 90                           	nop
  40ff1a: 90                           	nop
  40ff1b: 90                           	nop
  40ff1c: 90                           	nop
  40ff1d: 90                           	nop
  40ff1e: 90                           	nop
  40ff1f: 90                           	nop
  40ff20: 8b 44 24 04                  	movl	0x4(%esp), %eax
  40ff24: 85 c0                        	testl	%eax, %eax
  40ff26: 7d 08                        	jge	0x40ff30 <.text+0xef30>
  40ff28: 33 c0                        	xorl	%eax, %eax
  40ff2a: a3 28 ef b0 03               	movl	%eax, 0x3b0ef28
  40ff2f: c3                           	retl
  40ff30: 83 f8 03                     	cmpl	$0x3, %eax
  40ff33: 7e 05                        	jle	0x40ff3a <.text+0xef3a>
  40ff35: b8 03 00 00 00               	movl	$0x3, %eax
  40ff3a: a3 28 ef b0 03               	movl	%eax, 0x3b0ef28
  40ff3f: c3                           	retl
  40ff40: a1 28 ef b0 03               	movl	0x3b0ef28, %eax
  40ff45: c3                           	retl
  40ff46: 90                           	nop
  40ff47: 90                           	nop
  40ff48: 90                           	nop
  40ff49: 90                           	nop
  40ff4a: 90                           	nop
  40ff4b: 90                           	nop
  40ff4c: 90                           	nop
  40ff4d: 90                           	nop
  40ff4e: 90                           	nop
  40ff4f: 90                           	nop
  40ff50: a1 2c ef b0 03               	movl	0x3b0ef2c, %eax
  40ff55: 83 ec 08                     	subl	$0x8, %esp
  40ff58: 85 c0                        	testl	%eax, %eax
  40ff5a: 53                           	pushl	%ebx
  40ff5b: 55                           	pushl	%ebp
  40ff5c: 56                           	pushl	%esi
  40ff5d: 57                           	pushl	%edi
  40ff5e: 0f 84 d1 00 00 00            	je	0x410035 <.text+0xf035>
  40ff64: 8b 1d 8c b0 7d 00            	movl	0x7db08c, %ebx
  40ff6a: 8b 2d 88 b0 7d 00            	movl	0x7db088, %ebp
  40ff70: 6a ff                        	pushl	$-0x1
  40ff72: 6a 00                        	pushl	$0x0
  40ff74: 68 e0 ec b0 03               	pushl	$0x3b0ece0              # imm = 0x3B0ECE0
  40ff79: 6a 24                        	pushl	$0x24
  40ff7b: ff d3                        	calll	*%ebx
  40ff7d: 8b f8                        	movl	%eax, %edi
  40ff7f: a1 2c ef b0 03               	movl	0x3b0ef2c, %eax
  40ff84: 85 c0                        	testl	%eax, %eax
  40ff86: 0f 84 a9 00 00 00            	je	0x410035 <.text+0xf035>
  40ff8c: 68 c4 ec b0 03               	pushl	$0x3b0ecc4              # imm = 0x3B0ECC4
  40ff91: ff 15 84 b0 7d 00            	calll	*0x7db084
  40ff97: 85 ff                        	testl	%edi, %edi
  40ff99: 0f 8c 82 00 00 00            	jl	0x410021 <.text+0xf021>
  40ff9f: 8b 0c bd 30 ec b0 03         	movl	0x3b0ec30(,%edi,4), %ecx
  40ffa6: 85 c9                        	testl	%ecx, %ecx
  40ffa8: 8d 34 bd 30 ec b0 03         	leal	0x3b0ec30(,%edi,4), %esi
  40ffaf: 74 70                        	je	0x410021 <.text+0xf021>
  40ffb1: 83 ff 24                     	cmpl	$0x24, %edi
  40ffb4: 7d 6b                        	jge	0x410021 <.text+0xf021>
  40ffb6: 8d 44 24 14                  	leal	0x14(%esp), %eax
  40ffba: 50                           	pushl	%eax
  40ffbb: 8d 54 24 14                  	leal	0x14(%esp), %edx
  40ffbf: 52                           	pushl	%edx
  40ffc0: e8 2b d9 03 00               	calll	0x44d8f0 <.text+0x4c8f0>
  40ffc5: 8b 0e                        	movl	(%esi), %ecx
  40ffc7: 8b 41 24                     	movl	0x24(%ecx), %eax
  40ffca: 8b 54 24 10                  	movl	0x10(%esp), %edx
  40ffce: 3b c2                        	cmpl	%edx, %eax
