
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  40b420: 8b 44 24 04                  	movl	0x4(%esp), %eax
  40b424: 8b 40 0c                     	movl	0xc(%eax), %eax
  40b427: 85 c0                        	testl	%eax, %eax
  40b429: 74 28                        	je	0x40b453 <.text+0xa453>
  40b42b: 56                           	pushl	%esi
  40b42c: 8d 64 24 00                  	leal	(%esp), %esp
  40b430: 8b 48 30                     	movl	0x30(%eax), %ecx
  40b433: 8b 30                        	movl	(%eax), %esi
  40b435: 50                           	pushl	%eax
  40b436: 89 0d f8 db ab 03            	movl	%ecx, 0x3abdbf8
  40b43c: ff 50 10                     	calll	*0x10(%eax)
  40b43f: 83 c4 04                     	addl	$0x4, %esp
  40b442: 85 f6                        	testl	%esi, %esi
  40b444: c7 05 f8 db ab 03 00 00 00 00	movl	$0x0, 0x3abdbf8
  40b44e: 8b c6                        	movl	%esi, %eax
  40b450: 75 de                        	jne	0x40b430 <.text+0xa430>
  40b452: 5e                           	popl	%esi
  40b453: c3                           	retl
