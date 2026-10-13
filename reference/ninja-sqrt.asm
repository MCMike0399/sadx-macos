
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  7871f0: d9 44 24 04                  	flds	0x4(%esp)
  7871f4: dc 1d 48 00 7e 00            	fcompl	0x7e0048
  7871fa: df e0                        	fnstsw	%ax
  7871fc: f6 c4 05                     	testb	$0x5, %ah
  7871ff: 7a 11                        	jp	0x787212 <.text+0x386212>
  787201: d9 44 24 04                  	flds	0x4(%esp)
  787205: d8 0d 0c cb 7d 00            	fmuls	0x7dcb0c
  78720b: d9 fa                        	fsqrt
  78720d: d9 e0                        	fchs
  78720f: c2 04 00                     	retl	$0x4
  787212: d9 44 24 04                  	flds	0x4(%esp)
  787216: d9 fa                        	fsqrt
  787218: c2 04 00                     	retl	$0x4
