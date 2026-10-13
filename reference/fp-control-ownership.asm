
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  643e5f: e8 c3 ff ff ff               	calll	0x643e27 <.text+0x242e27>
  643e64: e8 b9 24 00 00               	calll	0x646322 <.text+0x245322>
  643e69: a3 ac 37 c8 03               	movl	%eax, 0x3c837ac
  643e6e: e8 5d 24 00 00               	calll	0x6462d0 <.text+0x2452d0>
  643e73: db e2                        	fnclex
  643e75: c3                           	retl


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  6462d0: 68 00 00 03 00               	pushl	$0x30000                # imm = 0x30000
  6462d5: 68 00 00 01 00               	pushl	$0x10000                # imm = 0x10000
  6462da: e8 9c 62 00 00               	calll	0x64c57b <.text+0x24b57b>
  6462df: 59                           	popl	%ecx
  6462e0: 59                           	popl	%ecx
  6462e1: c3                           	retl


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  64c429: 33 c0                        	xorl	%eax, %eax
  64c42b: f6 c3 01                     	testb	$0x1, %bl
  64c42e: 74 03                        	je	0x64c433 <.text+0x24b433>
  64c430: 6a 10                        	pushl	$0x10
  64c432: 58                           	popl	%eax
  64c433: f6 c3 04                     	testb	$0x4, %bl
  64c436: 74 03                        	je	0x64c43b <.text+0x24b43b>
  64c438: 83 c8 08                     	orl	$0x8, %eax
  64c43b: f6 c3 08                     	testb	$0x8, %bl
  64c43e: 74 03                        	je	0x64c443 <.text+0x24b443>
  64c440: 83 c8 04                     	orl	$0x4, %eax
  64c443: f6 c3 10                     	testb	$0x10, %bl
  64c446: 74 03                        	je	0x64c44b <.text+0x24b44b>
  64c448: 83 c8 02                     	orl	$0x2, %eax
  64c44b: f6 c3 20                     	testb	$0x20, %bl
  64c44e: 74 03                        	je	0x64c453 <.text+0x24b453>
  64c450: 83 c8 01                     	orl	$0x1, %eax
  64c453: f6 c3 02                     	testb	$0x2, %bl
  64c456: 74 05                        	je	0x64c45d <.text+0x24b45d>
  64c458: 0d 00 00 08 00               	orl	$0x80000, %eax          # imm = 0x80000
  64c45d: 55                           	pushl	%ebp
  64c45e: 0f b7 d3                     	movzwl	%bx, %edx
  64c461: 56                           	pushl	%esi
  64c462: 8b ca                        	movl	%edx, %ecx
  64c464: be 00 0c 00 00               	movl	$0xc00, %esi            # imm = 0xC00
  64c469: 23 ce                        	andl	%esi, %ecx
  64c46b: 57                           	pushl	%edi
  64c46c: bf 00 03 00 00               	movl	$0x300, %edi            # imm = 0x300
  64c471: bd 00 02 00 00               	movl	$0x200, %ebp            # imm = 0x200
  64c476: 74 21                        	je	0x64c499 <.text+0x24b499>
  64c478: 81 f9 00 04 00 00            	cmpl	$0x400, %ecx            # imm = 0x400
  64c47e: 74 14                        	je	0x64c494 <.text+0x24b494>
  64c480: 81 f9 00 08 00 00            	cmpl	$0x800, %ecx            # imm = 0x800
  64c486: 74 08                        	je	0x64c490 <.text+0x24b490>
  64c488: 3b ce                        	cmpl	%esi, %ecx
  64c48a: 75 0d                        	jne	0x64c499 <.text+0x24b499>
  64c48c: 0b c7                        	orl	%edi, %eax
  64c48e: eb 09                        	jmp	0x64c499 <.text+0x24b499>
  64c490: 0b c5                        	orl	%ebp, %eax
  64c492: eb 05                        	jmp	0x64c499 <.text+0x24b499>
  64c494: 0d 00 01 00 00               	orl	$0x100, %eax            # imm = 0x100
  64c499: 23 d7                        	andl	%edi, %edx
  64c49b: 74 0b                        	je	0x64c4a8 <.text+0x24b4a8>
  64c49d: 3b d5                        	cmpl	%ebp, %edx
  64c49f: 75 0c                        	jne	0x64c4ad <.text+0x24b4ad>
  64c4a1: 0d 00 00 01 00               	orl	$0x10000, %eax          # imm = 0x10000
  64c4a6: eb 05                        	jmp	0x64c4ad <.text+0x24b4ad>
  64c4a8: 0d 00 00 02 00               	orl	$0x20000, %eax          # imm = 0x20000
  64c4ad: f6 c7 10                     	testb	$0x10, %bh
  64c4b0: 5f                           	popl	%edi
  64c4b1: 5e                           	popl	%esi
  64c4b2: 5d                           	popl	%ebp
  64c4b3: 74 05                        	je	0x64c4ba <.text+0x24b4ba>
  64c4b5: 0d 00 00 04 00               	orl	$0x40000, %eax          # imm = 0x40000
  64c4ba: c3                           	retl


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  64c4bb: 33 c0                        	xorl	%eax, %eax
  64c4bd: f6 c3 10                     	testb	$0x10, %bl
  64c4c0: 74 01                        	je	0x64c4c3 <.text+0x24b4c3>
  64c4c2: 40                           	incl	%eax
  64c4c3: f6 c3 08                     	testb	$0x8, %bl
  64c4c6: 74 03                        	je	0x64c4cb <.text+0x24b4cb>
  64c4c8: 83 c8 04                     	orl	$0x4, %eax
  64c4cb: f6 c3 04                     	testb	$0x4, %bl
  64c4ce: 74 03                        	je	0x64c4d3 <.text+0x24b4d3>
  64c4d0: 83 c8 08                     	orl	$0x8, %eax
  64c4d3: f6 c3 02                     	testb	$0x2, %bl
  64c4d6: 74 03                        	je	0x64c4db <.text+0x24b4db>
  64c4d8: 83 c8 10                     	orl	$0x10, %eax
  64c4db: f6 c3 01                     	testb	$0x1, %bl
  64c4de: 74 03                        	je	0x64c4e3 <.text+0x24b4e3>
  64c4e0: 83 c8 20                     	orl	$0x20, %eax
  64c4e3: f7 c3 00 00 08 00            	testl	$0x80000, %ebx          # imm = 0x80000
  64c4e9: 74 03                        	je	0x64c4ee <.text+0x24b4ee>
  64c4eb: 83 c8 02                     	orl	$0x2, %eax
  64c4ee: 8b cb                        	movl	%ebx, %ecx
  64c4f0: ba 00 03 00 00               	movl	$0x300, %edx            # imm = 0x300
  64c4f5: 23 ca                        	andl	%edx, %ecx
  64c4f7: 56                           	pushl	%esi
  64c4f8: be 00 02 00 00               	movl	$0x200, %esi            # imm = 0x200
  64c4fd: 74 23                        	je	0x64c522 <.text+0x24b522>
  64c4ff: 81 f9 00 01 00 00            	cmpl	$0x100, %ecx            # imm = 0x100
  64c505: 74 16                        	je	0x64c51d <.text+0x24b51d>
  64c507: 3b ce                        	cmpl	%esi, %ecx
  64c509: 74 0b                        	je	0x64c516 <.text+0x24b516>
  64c50b: 3b ca                        	cmpl	%edx, %ecx
  64c50d: 75 13                        	jne	0x64c522 <.text+0x24b522>
  64c50f: 0d 00 0c 00 00               	orl	$0xc00, %eax            # imm = 0xC00
  64c514: eb 0c                        	jmp	0x64c522 <.text+0x24b522>
  64c516: 0d 00 08 00 00               	orl	$0x800, %eax            # imm = 0x800
  64c51b: eb 05                        	jmp	0x64c522 <.text+0x24b522>
  64c51d: 0d 00 04 00 00               	orl	$0x400, %eax            # imm = 0x400
  64c522: 8b cb                        	movl	%ebx, %ecx
  64c524: 81 e1 00 00 03 00            	andl	$0x30000, %ecx          # imm = 0x30000
  64c52a: 74 0c                        	je	0x64c538 <.text+0x24b538>
  64c52c: 81 f9 00 00 01 00            	cmpl	$0x10000, %ecx          # imm = 0x10000
  64c532: 75 06                        	jne	0x64c53a <.text+0x24b53a>
  64c534: 0b c6                        	orl	%esi, %eax
  64c536: eb 02                        	jmp	0x64c53a <.text+0x24b53a>
  64c538: 0b c2                        	orl	%edx, %eax
  64c53a: f7 c3 00 00 04 00            	testl	$0x40000, %ebx          # imm = 0x40000
  64c540: 5e                           	popl	%esi
  64c541: 74 05                        	je	0x64c548 <.text+0x24b548>
  64c543: 0d 00 10 00 00               	orl	$0x1000, %eax           # imm = 0x1000
  64c548: c3                           	retl


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  64c549: 55                           	pushl	%ebp
  64c54a: 8b ec                        	movl	%esp, %ebp
  64c54c: 51                           	pushl	%ecx
  64c54d: 53                           	pushl	%ebx
  64c54e: 9b                           	wait
  64c54f: d9 7d fc                     	fnstcw	-0x4(%ebp)
  64c552: 8b 5d fc                     	movl	-0x4(%ebp), %ebx
  64c555: e8 cf fe ff ff               	calll	0x64c429 <.text+0x24b429>
  64c55a: 8b d8                        	movl	%eax, %ebx
  64c55c: 8b 45 0c                     	movl	0xc(%ebp), %eax
  64c55f: f7 d0                        	notl	%eax
  64c561: 23 d8                        	andl	%eax, %ebx
  64c563: 8b 45 08                     	movl	0x8(%ebp), %eax
  64c566: 23 45 0c                     	andl	0xc(%ebp), %eax
  64c569: 0b d8                        	orl	%eax, %ebx
  64c56b: e8 4b ff ff ff               	calll	0x64c4bb <.text+0x24b4bb>
  64c570: 89 45 0c                     	movl	%eax, 0xc(%ebp)
  64c573: d9 6d 0c                     	fldcw	0xc(%ebp)
  64c576: 8b c3                        	movl	%ebx, %eax
  64c578: 5b                           	popl	%ebx
  64c579: c9                           	leave
  64c57a: c3                           	retl


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  64c57b: 8b 44 24 08                  	movl	0x8(%esp), %eax
  64c57f: 25 ff ff f7 ff               	andl	$0xfff7ffff, %eax       # imm = 0xFFF7FFFF
  64c584: 50                           	pushl	%eax
  64c585: ff 74 24 08                  	pushl	0x8(%esp)
  64c589: e8 bb ff ff ff               	calll	0x64c549 <.text+0x24b549>
  64c58e: 59                           	popl	%ecx
  64c58f: 59                           	popl	%ecx
  64c590: c3                           	retl


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  64b1fd: 55                           	pushl	%ebp
  64b1fe: 8b ec                        	movl	%esp, %ebp
  64b200: 51                           	pushl	%ecx
  64b201: 9b                           	wait
  64b202: d9 7d fc                     	fnstcw	-0x4(%ebp)
  64b205: 8b 45 0c                     	movl	0xc(%ebp), %eax
  64b208: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  64b20b: 23 4d 0c                     	andl	0xc(%ebp), %ecx
  64b20e: f7 d0                        	notl	%eax
  64b210: 23 45 fc                     	andl	-0x4(%ebp), %eax
  64b213: 0b c1                        	orl	%ecx, %eax
  64b215: 89 45 0c                     	movl	%eax, 0xc(%ebp)
  64b218: d9 6d 0c                     	fldcw	0xc(%ebp)
  64b21b: 0f bf 45 fc                  	movswl	-0x4(%ebp), %eax
  64b21f: c9                           	leave
  64b220: c3                           	retl


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  794000: a1 b0 28 d1 03               	movl	0x3d128b0, %eax
  794005: 85 c0                        	testl	%eax, %eax
  794007: 75 37                        	jne	0x794040 <.text+0x393040>
  794009: a1 60 1f d1 03               	movl	0x3d11f60, %eax
  79400e: 8b 08                        	movl	(%eax), %ecx
  794010: 68 b0 28 d1 03               	pushl	$0x3d128b0              # imm = 0x3D128B0
  794015: 68 c0 fd d0 03               	pushl	$0x3d0fdc0              # imm = 0x3D0FDC0
  79401a: 52                           	pushl	%edx
  79401b: 8b 15 30 fd d0 03            	movl	0x3d0fd30, %edx
  794021: 52                           	pushl	%edx
  794022: 8b 54 24 14                  	movl	0x14(%esp), %edx
  794026: 52                           	pushl	%edx
  794027: 8b 15 fc fd d0 03            	movl	0x3d0fdfc, %edx
  79402d: 52                           	pushl	%edx
  79402e: 50                           	pushl	%eax
  79402f: ff 51 3c                     	calll	*0x3c(%ecx)
  794032: 85 c0                        	testl	%eax, %eax
  794034: 7d 0a                        	jge	0x794040 <.text+0x393040>
  794036: c7 05 b0 28 d1 03 00 00 00 00	movl	$0x0, 0x3d128b0
  794040: c2 04 00                     	retl	$0x4


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  794060: 8b 15 cc 5d 8a 03            	movl	0x38a5dcc, %edx
  794066: 81 ec d4 00 00 00            	subl	$0xd4, %esp
  79406c: 56                           	pushl	%esi
  79406d: 8b b4 24 e0 00 00 00         	movl	0xe0(%esp), %esi
  794074: 57                           	pushl	%edi
  794075: 33 c0                        	xorl	%eax, %eax
  794077: b9 0d 00 00 00               	movl	$0xd, %ecx
  79407c: bf c0 fd d0 03               	movl	$0x3d0fdc0, %edi        # imm = 0x3D0FDC0
  794081: f3 ab                        	rep		stosl	%eax, %es:(%edi)
  794083: 8b 0d c8 5d 8a 03            	movl	0x38a5dc8, %ecx
  794089: a1 c4 5d 8a 03               	movl	0x38a5dc4, %eax
  79408e: 85 c0                        	testl	%eax, %eax
  794090: 89 0d c0 fd d0 03            	movl	%ecx, 0x3d0fdc0
  794096: 8b 8c 24 e0 00 00 00         	movl	0xe0(%esp), %ecx
  79409d: 89 15 c4 fd d0 03            	movl	%edx, 0x3d0fdc4
  7940a3: a3 dc fd d0 03               	movl	%eax, 0x3d0fddc
  7940a8: 8b 11                        	movl	(%ecx), %edx
  7940aa: bf 01 00 00 00               	movl	$0x1, %edi
  7940af: 89 15 c8 fd d0 03            	movl	%edx, 0x3d0fdc8
  7940b5: 8b 15 30 fd d0 03            	movl	0x3d0fd30, %edx
  7940bb: 89 3d cc fd d0 03            	movl	%edi, 0x3d0fdcc
  7940c1: 89 3d e0 fd d0 03            	movl	%edi, 0x3d0fde0
  7940c7: 8b 0e                        	movl	(%esi), %ecx
  7940c9: 89 0d e4 fd d0 03            	movl	%ecx, 0x3d0fde4
  7940cf: 89 3d d4 fd d0 03            	movl	%edi, 0x3d0fdd4
  7940d5: 89 15 d8 fd d0 03            	movl	%edx, 0x3d0fdd8
  7940db: 75 0f                        	jne	0x7940ec <.text+0x3930ec>
  7940dd: a3 ec fd d0 03               	movl	%eax, 0x3d0fdec
  7940e2: c7 05 f0 fd d0 03 00 00 00 80	movl	$0x80000000, 0x3d0fdf0  # imm = 0x80000000
  7940ec: a1 b0 28 d1 03               	movl	0x3d128b0, %eax
  7940f1: 85 c0                        	testl	%eax, %eax
  7940f3: 0f 85 9b 00 00 00            	jne	0x794194 <.text+0x393194>
  7940f9: a1 60 1f d1 03               	movl	0x3d11f60, %eax
  7940fe: 8b 08                        	movl	(%eax), %ecx
  794100: 8d 54 24 08                  	leal	0x8(%esp), %edx
  794104: 52                           	pushl	%edx
  794105: 8b 15 fc fd d0 03            	movl	0x3d0fdfc, %edx
  79410b: 57                           	pushl	%edi
  79410c: 52                           	pushl	%edx
  79410d: 50                           	pushl	%eax
  79410e: ff 51 34                     	calll	*0x34(%ecx)
  794111: 85 c0                        	testl	%eax, %eax
  794113: 75 21                        	jne	0x794136 <.text+0x393136>
  794115: 57                           	pushl	%edi
  794116: ba 40 00 00 00               	movl	$0x40, %edx
  79411b: e8 e0 fe ff ff               	calll	0x794000 <.text+0x393000>
  794120: 57                           	pushl	%edi
  794121: ba 80 00 00 00               	movl	$0x80, %edx
  794126: e8 d5 fe ff ff               	calll	0x794000 <.text+0x393000>
  79412b: 57                           	pushl	%edi
  79412c: ba 20 00 00 00               	movl	$0x20, %edx
  794131: e8 ca fe ff ff               	calll	0x794000 <.text+0x393000>
  794136: a1 b0 28 d1 03               	movl	0x3d128b0, %eax
  79413b: 85 c0                        	testl	%eax, %eax
  79413d: 75 55                        	jne	0x794194 <.text+0x393194>
  79413f: a1 60 1f d1 03               	movl	0x3d11f60, %eax
  794144: 8b 08                        	movl	(%eax), %ecx
  794146: 8d 54 24 08                  	leal	0x8(%esp), %edx
  79414a: 52                           	pushl	%edx
  79414b: 8b 15 fc fd d0 03            	movl	0x3d0fdfc, %edx
  794151: 6a 02                        	pushl	$0x2
  794153: 52                           	pushl	%edx
  794154: 50                           	pushl	%eax
  794155: ff 51 34                     	calll	*0x34(%ecx)
  794158: 85 c0                        	testl	%eax, %eax
  79415a: 75 24                        	jne	0x794180 <.text+0x393180>
  79415c: 6a 02                        	pushl	$0x2
  79415e: ba 40 00 00 00               	movl	$0x40, %edx
  794163: e8 98 fe ff ff               	calll	0x794000 <.text+0x393000>
  794168: 6a 02                        	pushl	$0x2
  79416a: ba 80 00 00 00               	movl	$0x80, %edx
  79416f: e8 8c fe ff ff               	calll	0x794000 <.text+0x393000>
  794174: 6a 02                        	pushl	$0x2
  794176: ba 20 00 00 00               	movl	$0x20, %edx
  79417b: e8 80 fe ff ff               	calll	0x794000 <.text+0x393000>
  794180: a1 b0 28 d1 03               	movl	0x3d128b0, %eax
  794185: 85 c0                        	testl	%eax, %eax
  794187: 75 0b                        	jne	0x794194 <.text+0x393194>
  794189: 5f                           	popl	%edi
  79418a: 33 c0                        	xorl	%eax, %eax
  79418c: 5e                           	popl	%esi
  79418d: 81 c4 d4 00 00 00            	addl	$0xd4, %esp
  794193: c3                           	retl
  794194: 8b 0e                        	movl	(%esi), %ecx
  794196: 8b 15 9c 28 d1 03            	movl	0x3d1289c, %edx
  79419c: b8 50 af 88 00               	movl	$0x88af50, %eax         # imm = 0x88AF50
  7941a1: 39 08                        	cmpl	%ecx, (%eax)


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  78ea10: 51                           	pushl	%ecx
  78ea11: 55                           	pushl	%ebp
  78ea12: 8b 69 0c                     	movl	0xc(%ecx), %ebp
  78ea15: 83 fd 02                     	cmpl	$0x2, %ebp
  78ea18: 89 6c 24 04                  	movl	%ebp, 0x4(%esp)
  78ea1c: 0f 8e 88 00 00 00            	jle	0x78eaaa <.text+0x38daaa>
  78ea22: 85 ed                        	testl	%ebp, %ebp
  78ea24: 53                           	pushl	%ebx
  78ea25: 8b 59 04                     	movl	0x4(%ecx), %ebx
  78ea28: 56                           	pushl	%esi
  78ea29: 8b 35 ac 28 d1 03            	movl	0x3d128ac, %esi
  78ea2f: 57                           	pushl	%edi
  78ea30: 8b 39                        	movl	(%ecx), %edi
  78ea32: 74 3d                        	je	0x78ea71 <.text+0x38da71>
  78ea34: 8b 44 24 18                  	movl	0x18(%esp), %eax
  78ea38: 50                           	pushl	%eax
  78ea39: e8 a2 f1 ff ff               	calll	0x78dbe0 <.text+0x38cbe0>
  78ea3e: d9 07                        	flds	(%edi)
  78ea40: d8 05 98 cc 7d 00            	fadds	0x7dcc98
  78ea46: 83 c6 14                     	addl	$0x14, %esi
  78ea49: 83 c7 08                     	addl	$0x8, %edi
  78ea4c: 83 c3 04                     	addl	$0x4, %ebx
  78ea4f: 4d                           	decl	%ebp
  78ea50: d9 5e ec                     	fstps	-0x14(%esi)
  78ea53: d9 47 fc                     	flds	-0x4(%edi)
  78ea56: c7 46 f8 00 00 80 3f         	movl	$0x3f800000, -0x8(%esi) # imm = 0x3F800000
  78ea5d: d8 05 98 cc 7d 00            	fadds	0x7dcc98
  78ea63: d9 5e f0                     	fstps	-0x10(%esi)
  78ea66: d9 5e f4                     	fstps	-0xc(%esi)
  78ea69: 8b 4b fc                     	movl	-0x4(%ebx), %ecx
  78ea6c: 89 4e fc                     	movl	%ecx, -0x4(%esi)
  78ea6f: 75 c3                        	jne	0x78ea34 <.text+0x38da34>
  78ea71: e8 2a d1 ff ff               	calll	0x78bba0 <.text+0x38aba0>
  78ea76: a1 b0 28 d1 03               	movl	0x3d128b0, %eax
  78ea7b: 8b 10                        	movl	(%eax), %edx
  78ea7d: 6a 44                        	pushl	$0x44
  78ea7f: 50                           	pushl	%eax
  78ea80: ff 92 30 01 00 00            	calll	*0x130(%edx)
  78ea86: 8b 15 ac 28 d1 03            	movl	0x3d128ac, %edx
  78ea8c: a1 b0 28 d1 03               	movl	0x3d128b0, %eax
  78ea91: 8b 08                        	movl	(%eax), %ecx
  78ea93: 6a 14                        	pushl	$0x14
  78ea95: 52                           	pushl	%edx
  78ea96: 8b 54 24 18                  	movl	0x18(%esp), %edx
  78ea9a: 83 c2 fe                     	addl	$-0x2, %edx
  78ea9d: 52                           	pushl	%edx
  78ea9e: 6a 06                        	pushl	$0x6
  78eaa0: 50                           	pushl	%eax
  78eaa1: ff 91 20 01 00 00            	calll	*0x120(%ecx)
  78eaa7: 5f                           	popl	%edi
  78eaa8: 5e                           	popl	%esi
  78eaa9: 5b                           	popl	%ebx
  78eaaa: 5d                           	popl	%ebp
  78eaab: 59                           	popl	%ecx
  78eaac: c2 04 00                     	retl	$0x4


Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  77dfc0: 83 ec 10                     	subl	$0x10, %esp
  77dfc3: 8b 44 24 14                  	movl	0x14(%esp), %eax
  77dfc7: 8b 08                        	movl	(%eax), %ecx
  77dfc9: 8b 50 04                     	movl	0x4(%eax), %edx
  77dfcc: 89 0c 24                     	movl	%ecx, (%esp)
  77dfcf: 8b 48 08                     	movl	0x8(%eax), %ecx
  77dfd2: 89 54 24 04                  	movl	%edx, 0x4(%esp)
  77dfd6: 8b 50 0c                     	movl	0xc(%eax), %edx
  77dfd9: 8b 44 24 18                  	movl	0x18(%esp), %eax
  77dfdd: 83 f8 02                     	cmpl	$0x2, %eax
  77dfe0: 89 54 24 0c                  	movl	%edx, 0xc(%esp)
  77dfe4: 89 4c 24 08                  	movl	%ecx, 0x8(%esp)
  77dfe8: 89 44 24 0c                  	movl	%eax, 0xc(%esp)
  77dfec: 7e 5c                        	jle	0x77e04a <.text+0x37d04a>
  77dfee: d9 05 0c cb 7d 00            	flds	0x7dcb0c
  77dff4: d8 74 24 1c                  	fdivs	0x1c(%esp)
  77dff8: d9 5c 24 1c                  	fstps	0x1c(%esp)
  77dffc: d9 05 10 cb 7d 00            	flds	0x7dcb10
  77e002: d8 5c 24 1c                  	fcomps	0x1c(%esp)
  77e006: df e0                        	fnstsw	%ax
  77e008: f6 c4 05                     	testb	$0x5, %ah
  77e00b: 7a 08                        	jp	0x77e015 <.text+0x37d015>
  77e00d: c7 44 24 1c 00 00 80 3f      	movl	$0x3f800000, 0x1c(%esp) # imm = 0x3F800000
  77e015: 8b 4c 24 20                  	movl	0x20(%esp), %ecx
  77e019: 85 c9                        	testl	%ecx, %ecx
  77e01b: 78 08                        	js	0x77e025 <.text+0x37d025>
  77e01d: c7 44 24 08 00 00 00 00      	movl	$0x0, 0x8(%esp)
  77e025: 83 e1 40                     	andl	$0x40, %ecx
  77e028: e8 13 39 01 00               	calll	0x791940 <.text+0x390940>
  77e02d: e8 6e db 00 00               	calll	0x78bba0 <.text+0x38aba0>
  77e032: e8 49 d7 00 00               	calll	0x78b780 <.text+0x38a780>
  77e037: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  77e03b: 50                           	pushl	%eax
  77e03c: 8d 4c 24 04                  	leal	0x4(%esp), %ecx
  77e040: e8 cb 09 01 00               	calll	0x78ea10 <.text+0x38da10>
  77e045: e8 b6 d7 00 00               	calll	0x78b800 <.text+0x38a800>
  77e04a: 83 c4 10                     	addl	$0x10, %esp
  77e04d: c3                           	retl
