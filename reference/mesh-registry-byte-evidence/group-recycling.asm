
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  451800: 55                           	pushl	%ebp
  451801: 8b ec                        	movl	%esp, %ebp
  451803: 83 ec 14                     	subl	$0x14, %esp
  451806: 8b 45 08                     	movl	0x8(%ebp), %eax
  451809: 8b 48 10                     	movl	0x10(%eax), %ecx
  45180c: 89 4d f8                     	movl	%ecx, -0x8(%ebp)
  45180f: 8b 55 08                     	movl	0x8(%ebp), %edx
  451812: 8b 42 14                     	movl	0x14(%edx), %eax
  451815: 89 45 fc                     	movl	%eax, -0x4(%ebp)
  451818: 83 7d f8 00                  	cmpl	$0x0, -0x8(%ebp)
  45181c: 74 19                        	je	0x451837 <.text+0x50837>
  45181e: 83 7d fc 00                  	cmpl	$0x0, -0x4(%ebp)
  451822: 74 13                        	je	0x451837 <.text+0x50837>
  451824: 8b 4d fc                     	movl	-0x4(%ebp), %ecx
  451827: 8b 15 58 85 c4 03            	movl	0x3c48558, %edx
  45182d: 89 11                        	movl	%edx, (%ecx)
  45182f: 8b 45 f8                     	movl	-0x8(%ebp), %eax
  451832: a3 58 85 c4 03               	movl	%eax, 0x3c48558
  451837: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  45183a: c7 41 08 00 00 00 00         	movl	$0x0, 0x8(%ecx)
  451841: 8b 55 08                     	movl	0x8(%ebp), %edx
  451844: 52                           	pushl	%edx
  451845: e8 16 00 00 00               	calll	0x451860 <.text+0x50860>
  45184a: 83 c4 04                     	addl	$0x4, %esp
  45184d: 8b e5                        	movl	%ebp, %esp
  45184f: 5d                           	popl	%ebp
  451850: c3                           	retl
  451851: cc                           	int3
  451852: cc                           	int3
  451853: cc                           	int3
  451854: cc                           	int3
  451855: cc                           	int3
  451856: cc                           	int3
  451857: cc                           	int3
  451858: cc                           	int3
  451859: cc                           	int3
  45185a: cc                           	int3
  45185b: cc                           	int3
  45185c: cc                           	int3
  45185d: cc                           	int3
  45185e: cc                           	int3
  45185f: cc                           	int3
  451860: 55                           	pushl	%ebp
  451861: 8b ec                        	movl	%esp, %ebp
  451863: 83 ec 0c                     	subl	$0xc, %esp
  451866: 8b 45 08                     	movl	0x8(%ebp), %eax
  451869: 8b 08                        	movl	(%eax), %ecx
  45186b: 89 4d f8                     	movl	%ecx, -0x8(%ebp)
  45186e: 8b 55 08                     	movl	0x8(%ebp), %edx
  451871: 8b 42 04                     	movl	0x4(%edx), %eax
  451874: 89 45 fc                     	movl	%eax, -0x4(%ebp)
  451877: 83 7d f8 00                  	cmpl	$0x0, -0x8(%ebp)
  45187b: 74 09                        	je	0x451886 <.text+0x50886>
  45187d: 8b 4d f8                     	movl	-0x8(%ebp), %ecx
  451880: 8b 55 fc                     	movl	-0x4(%ebp), %edx
  451883: 89 51 04                     	movl	%edx, 0x4(%ecx)
  451886: 8b 45 fc                     	movl	-0x4(%ebp), %eax
  451889: 83 e0 01                     	andl	$0x1, %eax
  45188c: 85 c0                        	testl	%eax, %eax
  45188e: 74 12                        	je	0x4518a2 <.text+0x508a2>
  451890: 8b 4d fc                     	movl	-0x4(%ebp), %ecx
  451893: f7 d1                        	notl	%ecx
  451895: 89 4d f4                     	movl	%ecx, -0xc(%ebp)
  451898: 8b 55 f4                     	movl	-0xc(%ebp), %edx
  45189b: 8b 45 f8                     	movl	-0x8(%ebp), %eax
  45189e: 89 02                        	movl	%eax, (%edx)
  4518a0: eb 08                        	jmp	0x4518aa <.text+0x508aa>
  4518a2: 8b 4d fc                     	movl	-0x4(%ebp), %ecx
  4518a5: 8b 55 f8                     	movl	-0x8(%ebp), %edx
  4518a8: 89 11                        	movl	%edx, (%ecx)
  4518aa: 8b 45 08                     	movl	0x8(%ebp), %eax
  4518ad: 8b 0d 54 85 c4 03            	movl	0x3c48554, %ecx
  4518b3: 89 08                        	movl	%ecx, (%eax)
  4518b5: 8b 55 08                     	movl	0x8(%ebp), %edx
  4518b8: 89 15 54 85 c4 03            	movl	%edx, 0x3c48554
  4518be: 8b e5                        	movl	%ebp, %esp
  4518c0: 5d                           	popl	%ebp
  4518c1: c3                           	retl
  4518c2: cc                           	int3
  4518c3: cc                           	int3
  4518c4: cc                           	int3
  4518c5: cc                           	int3
  4518c6: cc                           	int3
  4518c7: cc                           	int3
  4518c8: cc                           	int3
  4518c9: cc                           	int3
  4518ca: cc                           	int3
  4518cb: cc                           	int3
  4518cc: cc                           	int3
  4518cd: cc                           	int3
  4518ce: cc                           	int3
  4518cf: cc                           	int3
  4518d0: 55                           	pushl	%ebp
  4518d1: 8b ec                        	movl	%esp, %ebp
  4518d3: 83 ec 08                     	subl	$0x8, %esp
  4518d6: c7 45 f8 50 81 b4 03         	movl	$0x3b48150, -0x8(%ebp)  # imm = 0x3B48150
  4518dd: c7 45 fc 00 00 00 00         	movl	$0x0, -0x4(%ebp)
  4518e4: eb 12                        	jmp	0x4518f8 <.text+0x508f8>
  4518e6: 8b 45 fc                     	movl	-0x4(%ebp), %eax
  4518e9: 83 c0 01                     	addl	$0x1, %eax
  4518ec: 89 45 fc                     	movl	%eax, -0x4(%ebp)
  4518ef: 8b 4d f8                     	movl	-0x8(%ebp), %ecx
  4518f2: 83 c1 04                     	addl	$0x4, %ecx
  4518f5: 89 4d f8                     	movl	%ecx, -0x8(%ebp)
  4518f8: 81 7d fc 00 01 00 00         	cmpl	$0x100, -0x4(%ebp)      # imm = 0x100
  4518ff: 7d 0b                        	jge	0x45190c <.text+0x5090c>
  451901: 8b 55 f8                     	movl	-0x8(%ebp), %edx
  451904: c7 02 00 00 00 00            	movl	$0x0, (%edx)
  45190a: eb da                        	jmp	0x4518e6 <.text+0x508e6>
  45190c: 8b e5                        	movl	%ebp, %esp
  45190e: 5d                           	popl	%ebp
  45190f: c3                           	retl
