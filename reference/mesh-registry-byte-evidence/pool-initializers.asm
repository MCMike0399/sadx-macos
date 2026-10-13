
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  451030: 55                           	pushl	%ebp
  451031: 8b ec                        	movl	%esp, %ebp
  451033: 83 ec 0c                     	subl	$0xc, %esp
  451036: c7 05 f0 2f b4 03 00 00 00 00	movl	$0x0, 0x3b42ff0
  451040: c7 05 f4 2f b4 03 00 00 00 00	movl	$0x0, 0x3b42ff4
  45104a: c7 05 50 85 c4 03 00 00 00 00	movl	$0x0, 0x3c48550
  451054: c7 05 00 70 b4 03 00 00 00 00	movl	$0x0, 0x3b47000
  45105e: c7 45 fc 00 01 00 00         	movl	$0x100, -0x4(%ebp)      # imm = 0x100
  451065: c7 45 f8 00 30 b4 03         	movl	$0x3b43000, -0x8(%ebp)  # imm = 0x3B43000
  45106c: eb 12                        	jmp	0x451080 <.text+0x50080>
  45106e: 8b 45 fc                     	movl	-0x4(%ebp), %eax
  451071: 83 e8 01                     	subl	$0x1, %eax
  451074: 89 45 fc                     	movl	%eax, -0x4(%ebp)
  451077: 8b 4d f8                     	movl	-0x8(%ebp), %ecx
  45107a: 83 c1 40                     	addl	$0x40, %ecx
  45107d: 89 4d f8                     	movl	%ecx, -0x8(%ebp)
  451080: 83 7d fc 00                  	cmpl	$0x0, -0x4(%ebp)
  451084: 7e 0c                        	jle	0x451092 <.text+0x50092>
  451086: 8b 55 f8                     	movl	-0x8(%ebp), %edx
  451089: c7 42 08 00 00 00 00         	movl	$0x0, 0x8(%edx)
  451090: eb dc                        	jmp	0x45106e <.text+0x5006e>
  451092: c7 45 fc 00 40 00 00         	movl	$0x4000, -0x4(%ebp)     # imm = 0x4000
  451099: c7 45 f4 50 85 b4 03         	movl	$0x3b48550, -0xc(%ebp)  # imm = 0x3B48550
  4510a0: eb 12                        	jmp	0x4510b4 <.text+0x500b4>
  4510a2: 8b 45 fc                     	movl	-0x4(%ebp), %eax
  4510a5: 83 e8 01                     	subl	$0x1, %eax
  4510a8: 89 45 fc                     	movl	%eax, -0x4(%ebp)
  4510ab: 8b 4d f4                     	movl	-0xc(%ebp), %ecx
  4510ae: 83 c1 40                     	addl	$0x40, %ecx
  4510b1: 89 4d f4                     	movl	%ecx, -0xc(%ebp)
  4510b4: 83 7d fc 00                  	cmpl	$0x0, -0x4(%ebp)
  4510b8: 7e 0c                        	jle	0x4510c6 <.text+0x500c6>
  4510ba: 8b 55 f4                     	movl	-0xc(%ebp), %edx
  4510bd: c7 42 14 00 00 00 00         	movl	$0x0, 0x14(%edx)
  4510c4: eb dc                        	jmp	0x4510a2 <.text+0x500a2>
  4510c6: e8 15 00 00 00               	calll	0x4510e0 <.text+0x500e0>
  4510cb: b8 01 00 00 00               	movl	$0x1, %eax
  4510d0: 8b e5                        	movl	%ebp, %esp
  4510d2: 5d                           	popl	%ebp
  4510d3: c3                           	retl
  4510d4: cc                           	int3
  4510d5: cc                           	int3
  4510d6: cc                           	int3
  4510d7: cc                           	int3
  4510d8: cc                           	int3
  4510d9: cc                           	int3
  4510da: cc                           	int3
  4510db: cc                           	int3
  4510dc: cc                           	int3
  4510dd: cc                           	int3
  4510de: cc                           	int3
  4510df: cc                           	int3
  4510e0: 55                           	pushl	%ebp
  4510e1: 8b ec                        	movl	%esp, %ebp
  4510e3: 68 00 40 00 00               	pushl	$0x4000                 # imm = 0x4000
  4510e8: 68 50 85 b4 03               	pushl	$0x3b48550              # imm = 0x3B48550
  4510ed: e8 1e 00 00 00               	calll	0x451110 <.text+0x50110>
  4510f2: 83 c4 08                     	addl	$0x8, %esp
  4510f5: 68 00 01 00 00               	pushl	$0x100                  # imm = 0x100
  4510fa: 68 00 30 b4 03               	pushl	$0x3b43000              # imm = 0x3B43000
  4510ff: e8 5c 00 00 00               	calll	0x451160 <.text+0x50160>
  451104: 83 c4 08                     	addl	$0x8, %esp
  451107: 5d                           	popl	%ebp
  451108: c3                           	retl
  451109: cc                           	int3
  45110a: cc                           	int3
  45110b: cc                           	int3
  45110c: cc                           	int3
  45110d: cc                           	int3
  45110e: cc                           	int3
  45110f: cc                           	int3
  451110: 55                           	pushl	%ebp
  451111: 8b ec                        	movl	%esp, %ebp
  451113: 51                           	pushl	%ecx
  451114: 8b 45 08                     	movl	0x8(%ebp), %eax
  451117: a3 58 85 c4 03               	movl	%eax, 0x3c48558
  45111c: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  45111f: 83 c1 40                     	addl	$0x40, %ecx
  451122: 89 4d fc                     	movl	%ecx, -0x4(%ebp)
  451125: eb 1b                        	jmp	0x451142 <.text+0x50142>
  451127: 8b 55 0c                     	movl	0xc(%ebp), %edx
  45112a: 83 ea 01                     	subl	$0x1, %edx
  45112d: 89 55 0c                     	movl	%edx, 0xc(%ebp)
  451130: 8b 45 fc                     	movl	-0x4(%ebp), %eax
  451133: 83 c0 40                     	addl	$0x40, %eax
  451136: 89 45 fc                     	movl	%eax, -0x4(%ebp)
  451139: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  45113c: 83 c1 40                     	addl	$0x40, %ecx
  45113f: 89 4d 08                     	movl	%ecx, 0x8(%ebp)
  451142: 83 7d 0c 01                  	cmpl	$0x1, 0xc(%ebp)
  451146: 7e 0a                        	jle	0x451152 <.text+0x50152>
  451148: 8b 55 08                     	movl	0x8(%ebp), %edx
  45114b: 8b 45 fc                     	movl	-0x4(%ebp), %eax
  45114e: 89 02                        	movl	%eax, (%edx)
  451150: eb d5                        	jmp	0x451127 <.text+0x50127>
  451152: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  451155: c7 01 00 00 00 00            	movl	$0x0, (%ecx)
  45115b: 8b e5                        	movl	%ebp, %esp
  45115d: 5d                           	popl	%ebp
  45115e: c3                           	retl
  45115f: cc                           	int3
  451160: 55                           	pushl	%ebp
  451161: 8b ec                        	movl	%esp, %ebp
  451163: 51                           	pushl	%ecx
  451164: 8b 45 08                     	movl	0x8(%ebp), %eax
  451167: a3 54 85 c4 03               	movl	%eax, 0x3c48554
  45116c: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  45116f: 83 c1 40                     	addl	$0x40, %ecx
  451172: 89 4d fc                     	movl	%ecx, -0x4(%ebp)
  451175: eb 1b                        	jmp	0x451192 <.text+0x50192>
  451177: 8b 55 0c                     	movl	0xc(%ebp), %edx
  45117a: 83 ea 01                     	subl	$0x1, %edx
  45117d: 89 55 0c                     	movl	%edx, 0xc(%ebp)
  451180: 8b 45 08                     	movl	0x8(%ebp), %eax
  451183: 83 c0 40                     	addl	$0x40, %eax
  451186: 89 45 08                     	movl	%eax, 0x8(%ebp)
  451189: 8b 4d fc                     	movl	-0x4(%ebp), %ecx
  45118c: 83 c1 40                     	addl	$0x40, %ecx
  45118f: 89 4d fc                     	movl	%ecx, -0x4(%ebp)
  451192: 83 7d 0c 01                  	cmpl	$0x1, 0xc(%ebp)
  451196: 7e 0a                        	jle	0x4511a2 <.text+0x501a2>
  451198: 8b 55 08                     	movl	0x8(%ebp), %edx
  45119b: 8b 45 fc                     	movl	-0x4(%ebp), %eax
  45119e: 89 02                        	movl	%eax, (%edx)
  4511a0: eb d5                        	jmp	0x451177 <.text+0x50177>
  4511a2: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  4511a5: c7 01 00 00 00 00            	movl	$0x0, (%ecx)
  4511ab: c7 05 50 85 c4 03 00 00 00 00	movl	$0x0, 0x3c48550
  4511b5: 8b e5                        	movl	%ebp, %esp
  4511b7: 5d                           	popl	%ebp
  4511b8: c3                           	retl
  4511b9: cc                           	int3
  4511ba: cc                           	int3
  4511bb: cc                           	int3
  4511bc: cc                           	int3
  4511bd: cc                           	int3
  4511be: cc                           	int3
  4511bf: cc                           	int3
  4511c0: 55                           	pushl	%ebp
  4511c1: 8b ec                        	movl	%esp, %ebp
  4511c3: c7 05 5c 85 c4 03 01 00 00 00	movl	$0x1, 0x3c4855c
  4511cd: 5d                           	popl	%ebp
  4511ce: c3                           	retl
  4511cf: cc                           	int3
