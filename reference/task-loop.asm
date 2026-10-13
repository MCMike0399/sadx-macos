
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  40b0c0: 8b 44 24 04                  	movl	0x4(%esp), %eax
  40b0c4: 83 f8 08                     	cmpl	$0x8, %eax
  40b0c7: 75 05                        	jne	0x40b0ce <.text+0xa0ce>
  40b0c9: b8 07 00 00 00               	movl	$0x7, %eax
  40b0ce: 55                           	pushl	%ebp
  40b0cf: 56                           	pushl	%esi
  40b0d0: 8b 34 85 c4 db ab 03         	movl	0x3abdbc4(,%eax,4), %esi
  40b0d7: 33 ed                        	xorl	%ebp, %ebp
  40b0d9: 3b f5                        	cmpl	%ebp, %esi
  40b0db: 74 45                        	je	0x40b122 <.text+0xa122>
  40b0dd: 53                           	pushl	%ebx
  40b0de: 57                           	pushl	%edi
  40b0df: 90                           	nop
  40b0e0: 8b 1e                        	movl	(%esi), %ebx
  40b0e2: 3b de                        	cmpl	%esi, %ebx
  40b0e4: 8b 7e 10                     	movl	0x10(%esi), %edi
  40b0e7: 75 0d                        	jne	0x40b0f6 <.text+0xa0f6>
  40b0e9: 68 c0 cb 7d 00               	pushl	$0x7dcbc0               # imm = 0x7DCBC0
  40b0ee: e8 0d 5f ff ff               	calll	0x401000 <.text>
  40b0f3: 83 c4 04                     	addl	$0x4, %esp
  40b0f6: 3b fd                        	cmpl	%ebp, %edi
  40b0f8: 89 3d f4 db ab 03            	movl	%edi, 0x3abdbf4
  40b0fe: 74 14                        	je	0x40b114 <.text+0xa114>
  40b100: 8b 46 30                     	movl	0x30(%esi), %eax
  40b103: 56                           	pushl	%esi
  40b104: a3 f8 db ab 03               	movl	%eax, 0x3abdbf8
  40b109: ff d7                        	calll	*%edi
  40b10b: 83 c4 04                     	addl	$0x4, %esp
  40b10e: 89 2d f8 db ab 03            	movl	%ebp, 0x3abdbf8
  40b114: 3b dd                        	cmpl	%ebp, %ebx
  40b116: 89 2d f4 db ab 03            	movl	%ebp, 0x3abdbf4
  40b11c: 8b f3                        	movl	%ebx, %esi
  40b11e: 75 c0                        	jne	0x40b0e0 <.text+0xa0e0>
  40b120: 5f                           	popl	%edi
  40b121: 5b                           	popl	%ebx
  40b122: 5e                           	popl	%esi
  40b123: 5d                           	popl	%ebp
  40b124: c3                           	retl
