
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  40f070: 8b 4c 24 04                  	movl	0x4(%esp), %ecx
  40f074: 8b 14 8d 7c e7 b0 03         	movl	0x3b0e77c(,%ecx,4), %edx
  40f07b: 53                           	pushl	%ebx
  40f07c: 33 db                        	xorl	%ebx, %ebx
  40f07e: 33 c0                        	xorl	%eax, %eax
  40f080: 3b d3                        	cmpl	%ebx, %edx
  40f082: 0f 84 e1 00 00 00            	je	0x40f169 <.text+0xe169>
  40f088: 56                           	pushl	%esi
  40f089: 66 8b 72 1e                  	movw	0x1e(%edx), %si
  40f08d: 66 83 fe 4e                  	cmpw	$0x4e, %si
  40f091: 7e 05                        	jle	0x40f098 <.text+0xe098>
  40f093: b8 20 00 00 00               	movl	$0x20, %eax
  40f098: 66 83 fe b2                  	cmpw	$-0x4e, %si
  40f09c: 7d 03                        	jge	0x40f0a1 <.text+0xe0a1>
  40f09e: 83 c8 10                     	orl	$0x10, %eax
  40f0a1: 66 8b 72 1c                  	movw	0x1c(%edx), %si
  40f0a5: 66 83 fe 4e                  	cmpw	$0x4e, %si
  40f0a9: 7e 05                        	jle	0x40f0b0 <.text+0xe0b0>
  40f0ab: 0d 80 00 00 00               	orl	$0x80, %eax
  40f0b0: 66 83 fe b2                  	cmpw	$-0x4e, %si
  40f0b4: 7d 03                        	jge	0x40f0b9 <.text+0xe0b9>
  40f0b6: 83 c8 40                     	orl	$0x40, %eax
  40f0b9: 8b 34 8d a8 e3 b0 03         	movl	0x3b0e3a8(,%ecx,4), %esi
  40f0c0: f7 d6                        	notl	%esi
  40f0c2: 23 f0                        	andl	%eax, %esi
  40f0c4: 89 34 8d 54 e3 b0 03         	movl	%esi, 0x3b0e354(,%ecx,4)
  40f0cb: 89 04 8d a8 e3 b0 03         	movl	%eax, 0x3b0e3a8(,%ecx,4)
  40f0d2: 8b 72 08                     	movl	0x8(%edx), %esi
  40f0d5: 0b f0                        	orl	%eax, %esi
  40f0d7: 89 34 8d a8 e3 b0 03         	movl	%esi, 0x3b0e3a8(,%ecx,4)
  40f0de: 8b 42 10                     	movl	0x10(%edx), %eax
  40f0e1: 8b 34 8d 54 e3 b0 03         	movl	0x3b0e354(,%ecx,4), %esi
  40f0e8: 0b f0                        	orl	%eax, %esi
  40f0ea: 8b c6                        	movl	%esi, %eax
  40f0ec: a8 f0                        	testb	$-0x10, %al
  40f0ee: 89 34 8d 54 e3 b0 03         	movl	%esi, 0x3b0e354(,%ecx,4)
  40f0f5: 5e                           	popl	%esi
  40f0f6: 75 36                        	jne	0x40f12e <.text+0xe12e>
  40f0f8: 8b 14 8d a8 e3 b0 03         	movl	0x3b0e3a8(,%ecx,4), %edx
  40f0ff: 81 e2 f0 00 00 00            	andl	$0xf0, %edx
  40f105: 74 27                        	je	0x40f12e <.text+0xe12e>
  40f107: 8b 04 8d bc e3 b0 03         	movl	0x3b0e3bc(,%ecx,4), %eax
  40f10e: 40                           	incl	%eax
  40f10f: 83 f8 1e                     	cmpl	$0x1e, %eax
  40f112: 89 04 8d bc e3 b0 03         	movl	%eax, 0x3b0e3bc(,%ecx,4)
  40f119: 7e 1a                        	jle	0x40f135 <.text+0xe135>
  40f11b: 83 c0 fa                     	addl	$-0x6, %eax
  40f11e: 89 04 8d bc e3 b0 03         	movl	%eax, 0x3b0e3bc(,%ecx,4)
  40f125: 09 14 8d 54 e3 b0 03         	orl	%edx, 0x3b0e354(,%ecx,4)
  40f12c: eb 07                        	jmp	0x40f135 <.text+0xe135>
  40f12e: 89 1c 8d bc e3 b0 03         	movl	%ebx, 0x3b0e3bc(,%ecx,4)
  40f135: 38 1d b0 9f 90 00            	cmpb	%bl, 0x909fb0
  40f13b: 75 10                        	jne	0x40f14d <.text+0xe14d>
  40f13d: 89 1c 8d e0 e7 b0 03         	movl	%ebx, 0x3b0e7e0(,%ecx,4)
  40f144: 89 1c 8d cc e3 b0 03         	movl	%ebx, 0x3b0e3cc(,%ecx,4)
  40f14b: 5b                           	popl	%ebx
  40f14c: c3                           	retl
  40f14d: 8b 14 8d a8 e3 b0 03         	movl	0x3b0e3a8(,%ecx,4), %edx
  40f154: 8b 04 8d 54 e3 b0 03         	movl	0x3b0e354(,%ecx,4), %eax
  40f15b: 89 14 8d e0 e7 b0 03         	movl	%edx, 0x3b0e7e0(,%ecx,4)
  40f162: 89 04 8d cc e3 b0 03         	movl	%eax, 0x3b0e3cc(,%ecx,4)
  40f169: 5b                           	popl	%ebx
  40f16a: c3                           	retl
  40f16b: 90                           	nop
  40f16c: 90                           	nop
  40f16d: 90                           	nop
  40f16e: 90                           	nop
  40f16f: 90                           	nop
