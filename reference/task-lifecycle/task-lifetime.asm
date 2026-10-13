
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  40b570: 56                           	pushl	%esi
  40b571: 8b 74 24 08                  	movl	0x8(%esp), %esi
  40b575: 56                           	pushl	%esi
  40b576: e8 35 fa ff ff               	calll	0x40afb0 <.text+0x9fb0>
  40b57b: 83 c4 04                     	addl	$0x4, %esp
  40b57e: 85 c0                        	testl	%eax, %eax
  40b580: 0f 84 32 01 00 00            	je	0x40b6b8 <.text+0xa6b8>
  40b586: 8b 46 18                     	movl	0x18(%esi), %eax
  40b589: 55                           	pushl	%ebp
  40b58a: 33 ed                        	xorl	%ebp, %ebp
  40b58c: 3b c5                        	cmpl	%ebp, %eax
  40b58e: 74 06                        	je	0x40b596 <.text+0xa596>
  40b590: 56                           	pushl	%esi
  40b591: ff d0                        	calll	*%eax
  40b593: 83 c4 04                     	addl	$0x4, %esp
  40b596: 39 6e 0c                     	cmpl	%ebp, 0xc(%esi)
  40b599: 53                           	pushl	%ebx
  40b59a: 8b 5e 04                     	movl	0x4(%esi), %ebx
  40b59d: 57                           	pushl	%edi
  40b59e: 8b 3e                        	movl	(%esi), %edi
  40b5a0: 89 6e 10                     	movl	%ebp, 0x10(%esi)
  40b5a3: 89 6e 14                     	movl	%ebp, 0x14(%esi)
  40b5a6: 89 6e 18                     	movl	%ebp, 0x18(%esi)
  40b5a9: 74 09                        	je	0x40b5b4 <.text+0xa5b4>
  40b5ab: 56                           	pushl	%esi
  40b5ac: e8 2f 02 00 00               	calll	0x40b7e0 <.text+0xa7e0>
  40b5b1: 83 c4 04                     	addl	$0x4, %esp
  40b5b4: 3b fd                        	cmpl	%ebp, %edi
  40b5b6: 74 0b                        	je	0x40b5c3 <.text+0xa5c3>
  40b5b8: 3b dd                        	cmpl	%ebp, %ebx
  40b5ba: 74 0f                        	je	0x40b5cb <.text+0xa5cb>
  40b5bc: 89 5f 04                     	movl	%ebx, 0x4(%edi)
  40b5bf: 89 3b                        	movl	%edi, (%ebx)
  40b5c1: eb 45                        	jmp	0x40b608 <.text+0xa608>
  40b5c3: 3b dd                        	cmpl	%ebp, %ebx
  40b5c5: 74 04                        	je	0x40b5cb <.text+0xa5cb>
  40b5c7: 89 2b                        	movl	%ebp, (%ebx)
  40b5c9: eb 3d                        	jmp	0x40b608 <.text+0xa608>
  40b5cb: 39 6e 08                     	cmpl	%ebp, 0x8(%esi)
  40b5ce: 74 0f                        	je	0x40b5df <.text+0xa5df>
  40b5d0: 3b fd                        	cmpl	%ebp, %edi
  40b5d2: 74 03                        	je	0x40b5d7 <.text+0xa5d7>
  40b5d4: 89 6f 04                     	movl	%ebp, 0x4(%edi)
  40b5d7: 8b 46 08                     	movl	0x8(%esi), %eax
  40b5da: 89 78 0c                     	movl	%edi, 0xc(%eax)
  40b5dd: eb 29                        	jmp	0x40b608 <.text+0xa608>
  40b5df: 3b fd                        	cmpl	%ebp, %edi
  40b5e1: 74 03                        	je	0x40b5e6 <.text+0xa5e6>
  40b5e3: 89 6f 04                     	movl	%ebp, 0x4(%edi)
  40b5e6: 33 c0                        	xorl	%eax, %eax
  40b5e8: eb 06                        	jmp	0x40b5f0 <.text+0xa5f0>
  40b5ea: 8d 9b 00 00 00 00            	leal	(%ebx), %ebx
  40b5f0: 3b 34 85 c4 db ab 03         	cmpl	0x3abdbc4(,%eax,4), %esi
  40b5f7: 74 08                        	je	0x40b601 <.text+0xa601>
  40b5f9: 40                           	incl	%eax
  40b5fa: 83 f8 08                     	cmpl	$0x8, %eax
  40b5fd: 7c f1                        	jl	0x40b5f0 <.text+0xa5f0>
  40b5ff: eb 07                        	jmp	0x40b608 <.text+0xa608>
  40b601: 89 3c 85 c4 db ab 03         	movl	%edi, 0x3abdbc4(,%eax,4)
  40b608: 8b 46 20                     	movl	0x20(%esi), %eax
  40b60b: 3b c5                        	cmpl	%ebp, %eax
  40b60d: 5f                           	popl	%edi
  40b60e: 5b                           	popl	%ebx
  40b60f: 74 2e                        	je	0x40b63f <.text+0xa63f>
  40b611: 39 68 38                     	cmpl	%ebp, 0x38(%eax)
  40b614: 74 09                        	je	0x40b61f <.text+0xa61f>
  40b616: 50                           	pushl	%eax
  40b617: e8 c4 0e 01 00               	calll	0x41c4e0 <.text+0x1b4e0>
  40b61c: 83 c4 04                     	addl	$0x4, %esp
  40b61f: 8b 46 20                     	movl	0x20(%esi), %eax
  40b622: 39 68 3c                     	cmpl	%ebp, 0x3c(%eax)
  40b625: 74 09                        	je	0x40b630 <.text+0xa630>
  40b627: 50                           	pushl	%eax
  40b628: e8 73 d1 00 00               	calll	0x4187a0 <.text+0x177a0>
  40b62d: 83 c4 04                     	addl	$0x4, %esp
  40b630: 8b 4e 20                     	movl	0x20(%esi), %ecx
  40b633: 51                           	pushl	%ecx
  40b634: e8 d7 fc ff ff               	calll	0x40b310 <.text+0xa310>
  40b639: 83 c4 04                     	addl	$0x4, %esp
  40b63c: 89 6e 20                     	movl	%ebp, 0x20(%esi)
  40b63f: 8b 46 28                     	movl	0x28(%esi), %eax
  40b642: 3b c5                        	cmpl	%ebp, %eax
  40b644: 74 0c                        	je	0x40b652 <.text+0xa652>
  40b646: 50                           	pushl	%eax
  40b647: e8 c4 fc ff ff               	calll	0x40b310 <.text+0xa310>
  40b64c: 83 c4 04                     	addl	$0x4, %esp
  40b64f: 89 6e 28                     	movl	%ebp, 0x28(%esi)
  40b652: 8b 46 24                     	movl	0x24(%esi), %eax
  40b655: 3b c5                        	cmpl	%ebp, %eax
  40b657: 74 0c                        	je	0x40b665 <.text+0xa665>
  40b659: 50                           	pushl	%eax
  40b65a: e8 b1 fc ff ff               	calll	0x40b310 <.text+0xa310>
  40b65f: 83 c4 04                     	addl	$0x4, %esp
  40b662: 89 6e 24                     	movl	%ebp, 0x24(%esi)
  40b665: 8b 46 2c                     	movl	0x2c(%esi), %eax
  40b668: 3b c5                        	cmpl	%ebp, %eax
  40b66a: 74 0c                        	je	0x40b678 <.text+0xa678>
  40b66c: 50                           	pushl	%eax
  40b66d: e8 9e fc ff ff               	calll	0x40b310 <.text+0xa310>
  40b672: 83 c4 04                     	addl	$0x4, %esp
  40b675: 89 6e 2c                     	movl	%ebp, 0x2c(%esi)
  40b678: 8b 46 1c                     	movl	0x1c(%esi), %eax
  40b67b: 3b c5                        	cmpl	%ebp, %eax
  40b67d: 74 0d                        	je	0x40b68c <.text+0xa68c>
  40b67f: 89 68 04                     	movl	%ebp, 0x4(%eax)
  40b682: 8b 46 1c                     	movl	0x1c(%esi), %eax
  40b685: 80 60 02 fe                  	andb	$-0x2, 0x2(%eax)
  40b689: 89 6e 1c                     	movl	%ebp, 0x1c(%esi)
  40b68c: a1 ec db ab 03               	movl	0x3abdbec, %eax
  40b691: 3b c5                        	cmpl	%ebp, %eax
  40b693: 89 35 ec db ab 03            	movl	%esi, 0x3abdbec
  40b699: 75 0e                        	jne	0x40b6a9 <.text+0xa6a9>
  40b69b: 89 2e                        	movl	%ebp, (%esi)
  40b69d: 89 6e 04                     	movl	%ebp, 0x4(%esi)
  40b6a0: 89 6e 0c                     	movl	%ebp, 0xc(%esi)
  40b6a3: 89 6e 08                     	movl	%ebp, 0x8(%esi)
  40b6a6: 5d                           	popl	%ebp
  40b6a7: 5e                           	popl	%esi
  40b6a8: c3                           	retl
  40b6a9: 89 70 04                     	movl	%esi, 0x4(%eax)
  40b6ac: 89 6e 04                     	movl	%ebp, 0x4(%esi)
  40b6af: 89 6e 0c                     	movl	%ebp, 0xc(%esi)
  40b6b2: 89 6e 08                     	movl	%ebp, 0x8(%esi)
  40b6b5: 89 06                        	movl	%eax, (%esi)
  40b6b7: 5d                           	popl	%ebp
  40b6b8: 5e                           	popl	%esi
  40b6b9: c3                           	retl
  40b6ba: 90                           	nop
  40b6bb: 90                           	nop
  40b6bc: 90                           	nop
  40b6bd: 90                           	nop
  40b6be: 90                           	nop
  40b6bf: 90                           	nop
  40b6c0: 56                           	pushl	%esi
  40b6c1: 8b 74 24 08                  	movl	0x8(%esp), %esi
  40b6c5: 56                           	pushl	%esi
  40b6c6: e8 e5 f8 ff ff               	calll	0x40afb0 <.text+0x9fb0>
  40b6cb: 83 c4 04                     	addl	$0x4, %esp
  40b6ce: 85 c0                        	testl	%eax, %eax
  40b6d0: 74 07                        	je	0x40b6d9 <.text+0xa6d9>
  40b6d2: c7 46 10 70 b5 40 00         	movl	$0x40b570, 0x10(%esi)   # imm = 0x40B570
  40b6d9: 5e                           	popl	%esi
  40b6da: c3                           	retl
  40b6db: 90                           	nop
  40b6dc: 90                           	nop
  40b6dd: 90                           	nop
  40b6de: 90                           	nop
  40b6df: 90                           	nop
  40b6e0: 56                           	pushl	%esi
  40b6e1: 57                           	pushl	%edi
  40b6e2: 33 ff                        	xorl	%edi, %edi
  40b6e4: 83 ff 08                     	cmpl	$0x8, %edi
  40b6e7: 8b c7                        	movl	%edi, %eax
  40b6e9: 75 05                        	jne	0x40b6f0 <.text+0xa6f0>
  40b6eb: b8 07 00 00 00               	movl	$0x7, %eax
  40b6f0: 8b 04 85 c4 db ab 03         	movl	0x3abdbc4(,%eax,4), %eax
  40b6f7: 85 c0                        	testl	%eax, %eax
  40b6f9: 74 1d                        	je	0x40b718 <.text+0xa718>
  40b6fb: eb 03                        	jmp	0x40b700 <.text+0xa700>
  40b6fd: 8d 49 00                     	leal	(%ecx), %ecx
  40b700: 8b 48 10                     	movl	0x10(%eax), %ecx
  40b703: 85 c9                        	testl	%ecx, %ecx
  40b705: 8b 30                        	movl	(%eax), %esi
  40b707: 74 09                        	je	0x40b712 <.text+0xa712>
  40b709: 50                           	pushl	%eax
  40b70a: e8 61 fe ff ff               	calll	0x40b570 <.text+0xa570>
  40b70f: 83 c4 04                     	addl	$0x4, %esp
  40b712: 85 f6                        	testl	%esi, %esi
  40b714: 8b c6                        	movl	%esi, %eax
  40b716: 75 e8                        	jne	0x40b700 <.text+0xa700>
  40b718: 47                           	incl	%edi
  40b719: 83 ff 07                     	cmpl	$0x7, %edi
  40b71c: 7c c6                        	jl	0x40b6e4 <.text+0xa6e4>
  40b71e: 5f                           	popl	%edi
  40b71f: 5e                           	popl	%esi
  40b720: c3                           	retl
  40b721: 90                           	nop
  40b722: 90                           	nop
  40b723: 90                           	nop
  40b724: 90                           	nop
  40b725: 90                           	nop
  40b726: 90                           	nop
  40b727: 90                           	nop
  40b728: 90                           	nop
  40b729: 90                           	nop
  40b72a: 90                           	nop
  40b72b: 90                           	nop
  40b72c: 90                           	nop
  40b72d: 90                           	nop
  40b72e: 90                           	nop
  40b72f: 90                           	nop
  40b730: 56                           	pushl	%esi
  40b731: 57                           	pushl	%edi
  40b732: 33 ff                        	xorl	%edi, %edi
  40b734: 83 ff 08                     	cmpl	$0x8, %edi
  40b737: 8b c7                        	movl	%edi, %eax
  40b739: 75 05                        	jne	0x40b740 <.text+0xa740>
  40b73b: b8 07 00 00 00               	movl	$0x7, %eax
  40b740: 8b 04 85 c4 db ab 03         	movl	0x3abdbc4(,%eax,4), %eax
  40b747: 85 c0                        	testl	%eax, %eax
  40b749: 74 1d                        	je	0x40b768 <.text+0xa768>
  40b74b: eb 03                        	jmp	0x40b750 <.text+0xa750>
  40b74d: 8d 49 00                     	leal	(%ecx), %ecx
  40b750: 8b 48 10                     	movl	0x10(%eax), %ecx
  40b753: 85 c9                        	testl	%ecx, %ecx
  40b755: 8b 30                        	movl	(%eax), %esi
  40b757: 74 09                        	je	0x40b762 <.text+0xa762>
  40b759: 50                           	pushl	%eax
  40b75a: e8 11 fe ff ff               	calll	0x40b570 <.text+0xa570>
  40b75f: 83 c4 04                     	addl	$0x4, %esp
  40b762: 85 f6                        	testl	%esi, %esi
  40b764: 8b c6                        	movl	%esi, %eax
  40b766: 75 e8                        	jne	0x40b750 <.text+0xa750>
  40b768: 47                           	incl	%edi
  40b769: 83 ff 08                     	cmpl	$0x8, %edi
  40b76c: 7c c6                        	jl	0x40b734 <.text+0xa734>
  40b76e: 5f                           	popl	%edi
  40b76f: 5e                           	popl	%esi
  40b770: c3                           	retl
  40b771: 90                           	nop
  40b772: 90                           	nop
  40b773: 90                           	nop
  40b774: 90                           	nop
  40b775: 90                           	nop
  40b776: 90                           	nop
  40b777: 90                           	nop
  40b778: 90                           	nop
  40b779: 90                           	nop
  40b77a: 90                           	nop
  40b77b: 90                           	nop
  40b77c: 90                           	nop
  40b77d: 90                           	nop
  40b77e: 90                           	nop
  40b77f: 90                           	nop
  40b780: 53                           	pushl	%ebx
  40b781: 55                           	pushl	%ebp
  40b782: 8b 6c 24 0c                  	movl	0xc(%esp), %ebp
  40b786: 33 db                        	xorl	%ebx, %ebx
  40b788: 83 fb 08                     	cmpl	$0x8, %ebx
  40b78b: 56                           	pushl	%esi
  40b78c: 57                           	pushl	%edi
  40b78d: 75 05                        	jne	0x40b794 <.text+0xa794>
  40b78f: bb 07 00 00 00               	movl	$0x7, %ebx
  40b794: 8b 34 9d c4 db ab 03         	movl	0x3abdbc4(,%ebx,4), %esi
  40b79b: 85 f6                        	testl	%esi, %esi
  40b79d: 74 29                        	je	0x40b7c8 <.text+0xa7c8>
  40b79f: 90                           	nop
  40b7a0: 8b 46 10                     	movl	0x10(%esi), %eax
  40b7a3: 85 c0                        	testl	%eax, %eax
  40b7a5: 8b 3e                        	movl	(%esi), %edi
  40b7a7: 74 19                        	je	0x40b7c2 <.text+0xa7c2>
  40b7a9: 39 6e 30                     	cmpl	%ebp, 0x30(%esi)
  40b7ac: 75 14                        	jne	0x40b7c2 <.text+0xa7c2>
  40b7ae: 56                           	pushl	%esi
  40b7af: e8 fc f7 ff ff               	calll	0x40afb0 <.text+0x9fb0>
  40b7b4: 83 c4 04                     	addl	$0x4, %esp
  40b7b7: 85 c0                        	testl	%eax, %eax
  40b7b9: 74 07                        	je	0x40b7c2 <.text+0xa7c2>
  40b7bb: c7 46 10 70 b5 40 00         	movl	$0x40b570, 0x10(%esi)   # imm = 0x40B570
  40b7c2: 85 ff                        	testl	%edi, %edi
  40b7c4: 8b f7                        	movl	%edi, %esi
  40b7c6: 75 d8                        	jne	0x40b7a0 <.text+0xa7a0>
  40b7c8: 43                           	incl	%ebx
  40b7c9: 83 fb 08                     	cmpl	$0x8, %ebx
  40b7cc: 7c c6                        	jl	0x40b794 <.text+0xa794>
  40b7ce: 5f                           	popl	%edi
  40b7cf: 5e                           	popl	%esi
  40b7d0: 5d                           	popl	%ebp
  40b7d1: 5b                           	popl	%ebx
  40b7d2: c3                           	retl
  40b7d3: 90                           	nop
  40b7d4: 90                           	nop
  40b7d5: 90                           	nop
  40b7d6: 90                           	nop
  40b7d7: 90                           	nop
  40b7d8: 90                           	nop
  40b7d9: 90                           	nop
  40b7da: 90                           	nop
  40b7db: 90                           	nop
  40b7dc: 90                           	nop
  40b7dd: 90                           	nop
  40b7de: 90                           	nop
  40b7df: 90                           	nop
  40b7e0: 56                           	pushl	%esi
  40b7e1: 8b 74 24 08                  	movl	0x8(%esp), %esi
  40b7e5: 81 7e 10 e0 b7 40 00         	cmpl	$0x40b7e0, 0x10(%esi)   # imm = 0x40B7E0
  40b7ec: 75 19                        	jne	0x40b807 <.text+0xa807>
  40b7ee: 56                           	pushl	%esi
  40b7ef: e8 2c fc ff ff               	calll	0x40b420 <.text+0xa420>
  40b7f4: 8b 46 0c                     	movl	0xc(%esi), %eax
  40b7f7: 83 c4 04                     	addl	$0x4, %esp
  40b7fa: 85 c0                        	testl	%eax, %eax
  40b7fc: 75 5c                        	jne	0x40b85a <.text+0xa85a>
  40b7fe: c7 46 10 70 b5 40 00         	movl	$0x40b570, 0x10(%esi)   # imm = 0x40B570
  40b805: 5e                           	popl	%esi
  40b806: c3                           	retl
  40b807: 8b 46 0c                     	movl	0xc(%esi), %eax
  40b80a: 85 c0                        	testl	%eax, %eax
  40b80c: 74 4c                        	je	0x40b85a <.text+0xa85a>
  40b80e: 8b ff                        	movl	%edi, %edi
  40b810: 56                           	pushl	%esi
  40b811: e8 9a f7 ff ff               	calll	0x40afb0 <.text+0x9fb0>
  40b816: 83 c4 04                     	addl	$0x4, %esp
  40b819: 85 c0                        	testl	%eax, %eax
  40b81b: 74 0e                        	je	0x40b82b <.text+0xa82b>
  40b81d: 8b 46 0c                     	movl	0xc(%esi), %eax
  40b820: 50                           	pushl	%eax
  40b821: e8 4a fd ff ff               	calll	0x40b570 <.text+0xa570>
  40b826: 83 c4 04                     	addl	$0x4, %esp
  40b829: eb 28                        	jmp	0x40b853 <.text+0xa853>
  40b82b: 8b 4e 0c                     	movl	0xc(%esi), %ecx
  40b82e: 8b 01                        	movl	(%ecx), %eax
  40b830: 85 c0                        	testl	%eax, %eax
  40b832: 74 07                        	je	0x40b83b <.text+0xa83b>
  40b834: 89 40 04                     	movl	%eax, 0x4(%eax)
  40b837: 89 00                        	movl	%eax, (%eax)
  40b839: eb 18                        	jmp	0x40b853 <.text+0xa853>
  40b83b: 8b 51 08                     	movl	0x8(%ecx), %edx
  40b83e: 85 d2                        	testl	%edx, %edx
  40b840: 74 11                        	je	0x40b853 <.text+0xa853>
  40b842: 85 c0                        	testl	%eax, %eax
  40b844: 74 07                        	je	0x40b84d <.text+0xa84d>
  40b846: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%eax)
  40b84d: 8b 49 08                     	movl	0x8(%ecx), %ecx
  40b850: 89 41 0c                     	movl	%eax, 0xc(%ecx)
  40b853: 8b 46 0c                     	movl	0xc(%esi), %eax
  40b856: 85 c0                        	testl	%eax, %eax
  40b858: 75 b6                        	jne	0x40b810 <.text+0xa810>
  40b85a: 5e                           	popl	%esi
  40b85b: c3                           	retl
  40b85c: 90                           	nop
  40b85d: 90                           	nop
  40b85e: 90                           	nop
  40b85f: 90                           	nop
  40b860: 8b 54 24 08                  	movl	0x8(%esp), %edx
  40b864: 56                           	pushl	%esi
  40b865: 57                           	pushl	%edi
  40b866: 8b 7c 24 14                  	movl	0x14(%esp), %edi
  40b86a: e8 c1 f7 ff ff               	calll	0x40b030 <.text+0xa030>
  40b86f: 8b f0                        	movl	%eax, %esi
  40b871: 85 f6                        	testl	%esi, %esi
  40b873: 75 03                        	jne	0x40b878 <.text+0xa878>
  40b875: 5f                           	popl	%edi
  40b876: 5e                           	popl	%esi
  40b877: c3                           	retl
  40b878: 53                           	pushl	%ebx
  40b879: 8a 5c 24 10                  	movb	0x10(%esp), %bl
  40b87d: f6 c3 02                     	testb	$0x2, %bl
  40b880: 74 22                        	je	0x40b8a4 <.text+0xa8a4>
  40b882: 6a 40                        	pushl	$0x40
  40b884: e8 97 f9 ff ff               	calll	0x40b220 <.text+0xa220>
  40b889: 8b d0                        	movl	%eax, %edx
  40b88b: 83 c4 04                     	addl	$0x4, %esp
  40b88e: 85 d2                        	testl	%edx, %edx
  40b890: 0f 84 90 00 00 00            	je	0x40b926 <.text+0xa926>
  40b896: b9 10 00 00 00               	movl	$0x10, %ecx
  40b89b: 33 c0                        	xorl	%eax, %eax
  40b89d: 8b fa                        	movl	%edx, %edi
  40b89f: f3 ab                        	rep		stosl	%eax, %es:(%edi)
  40b8a1: 89 56 20                     	movl	%edx, 0x20(%esi)
  40b8a4: f6 c3 04                     	testb	$0x4, %bl
  40b8a7: 74 1e                        	je	0x40b8c7 <.text+0xa8c7>
  40b8a9: 6a 38                        	pushl	$0x38
  40b8ab: e8 70 f9 ff ff               	calll	0x40b220 <.text+0xa220>
  40b8b0: 8b d0                        	movl	%eax, %edx
  40b8b2: 83 c4 04                     	addl	$0x4, %esp
  40b8b5: 85 d2                        	testl	%edx, %edx
  40b8b7: 74 6d                        	je	0x40b926 <.text+0xa926>
  40b8b9: b9 0e 00 00 00               	movl	$0xe, %ecx
  40b8be: 33 c0                        	xorl	%eax, %eax
  40b8c0: 8b fa                        	movl	%edx, %edi
  40b8c2: f3 ab                        	rep		stosl	%eax, %es:(%edi)
  40b8c4: 89 56 28                     	movl	%edx, 0x28(%esi)
  40b8c7: f6 c3 01                     	testb	$0x1, %bl
  40b8ca: 74 1e                        	je	0x40b8ea <.text+0xa8ea>
  40b8cc: 6a 40                        	pushl	$0x40
  40b8ce: e8 4d f9 ff ff               	calll	0x40b220 <.text+0xa220>
  40b8d3: 8b d0                        	movl	%eax, %edx
  40b8d5: 83 c4 04                     	addl	$0x4, %esp
  40b8d8: 85 d2                        	testl	%edx, %edx
  40b8da: 74 4a                        	je	0x40b926 <.text+0xa926>
  40b8dc: b9 10 00 00 00               	movl	$0x10, %ecx
  40b8e1: 33 c0                        	xorl	%eax, %eax
  40b8e3: 8b fa                        	movl	%edx, %edi
  40b8e5: f3 ab                        	rep		stosl	%eax, %es:(%edi)
  40b8e7: 89 56 24                     	movl	%edx, 0x24(%esi)
  40b8ea: f6 c3 08                     	testb	$0x8, %bl
  40b8ed: 74 20                        	je	0x40b90f <.text+0xa90f>
  40b8ef: 6a 10                        	pushl	$0x10
  40b8f1: e8 2a f9 ff ff               	calll	0x40b220 <.text+0xa220>
  40b8f6: 83 c4 04                     	addl	$0x4, %esp
  40b8f9: 85 c0                        	testl	%eax, %eax
  40b8fb: 74 29                        	je	0x40b926 <.text+0xa926>
  40b8fd: 33 c9                        	xorl	%ecx, %ecx
  40b8ff: 8b d0                        	movl	%eax, %edx
  40b901: 89 0a                        	movl	%ecx, (%edx)
  40b903: 89 4a 04                     	movl	%ecx, 0x4(%edx)
  40b906: 89 4a 08                     	movl	%ecx, 0x8(%edx)
  40b909: 89 4a 0c                     	movl	%ecx, 0xc(%edx)
  40b90c: 89 46 2c                     	movl	%eax, 0x2c(%esi)
  40b90f: a1 f8 db ab 03               	movl	0x3abdbf8, %eax
  40b914: 85 c0                        	testl	%eax, %eax
  40b916: 75 05                        	jne	0x40b91d <.text+0xa91d>
  40b918: a1 08 6e 8f 03               	movl	0x38f6e08, %eax
  40b91d: 5b                           	popl	%ebx
  40b91e: 89 46 30                     	movl	%eax, 0x30(%esi)
  40b921: 5f                           	popl	%edi
  40b922: 8b c6                        	movl	%esi, %eax
  40b924: 5e                           	popl	%esi
  40b925: c3                           	retl
  40b926: 56                           	pushl	%esi
  40b927: e8 44 fc ff ff               	calll	0x40b570 <.text+0xa570>
  40b92c: 83 c4 04                     	addl	$0x4, %esp
  40b92f: 5b                           	popl	%ebx
  40b930: 5f                           	popl	%edi
  40b931: 33 c0                        	xorl	%eax, %eax
  40b933: 5e                           	popl	%esi
  40b934: c3                           	retl
  40b935: 90                           	nop
  40b936: 90                           	nop
  40b937: 90                           	nop
  40b938: 90                           	nop
  40b939: 90                           	nop
  40b93a: 90                           	nop
  40b93b: 90                           	nop
  40b93c: 90                           	nop
  40b93d: 90                           	nop
  40b93e: 90                           	nop
  40b93f: 90                           	nop
  40b940: 8b 44 24 08                  	movl	0x8(%esp), %eax
  40b944: 8b 4c 24 04                  	movl	0x4(%esp), %ecx
  40b948: 50                           	pushl	%eax
  40b949: 6a 07                        	pushl	$0x7
  40b94b: 51                           	pushl	%ecx
  40b94c: e8 0f ff ff ff               	calll	0x40b860 <.text+0xa860>
  40b951: 83 c4 0c                     	addl	$0xc, %esp
  40b954: 85 c0                        	testl	%eax, %eax
  40b956: 74 69                        	je	0x40b9c1 <.text+0xa9c1>
  40b958: 8b 4c 24 0c                  	movl	0xc(%esp), %ecx
  40b95c: 8b 51 0c                     	movl	0xc(%ecx), %edx
  40b95f: 89 10                        	movl	%edx, (%eax)
  40b961: 8b 51 0c                     	movl	0xc(%ecx), %edx
  40b964: 85 d2                        	testl	%edx, %edx
  40b966: 74 03                        	je	0x40b96b <.text+0xa96b>
  40b968: 89 42 04                     	movl	%eax, 0x4(%edx)
  40b96b: 89 41 0c                     	movl	%eax, 0xc(%ecx)
  40b96e: 8b 50 20                     	movl	0x20(%eax), %edx
  40b971: 85 d2                        	testl	%edx, %edx
  40b973: 56                           	pushl	%esi
  40b974: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%eax)
  40b97b: 89 48 08                     	movl	%ecx, 0x8(%eax)
  40b97e: 8b 71 20                     	movl	0x20(%ecx), %esi
  40b981: 74 36                        	je	0x40b9b9 <.text+0xa9b9>
  40b983: 85 f6                        	testl	%esi, %esi
  40b985: 74 32                        	je	0x40b9b9 <.text+0xa9b9>
  40b987: 53                           	pushl	%ebx
  40b988: 55                           	pushl	%ebp
  40b989: 57                           	pushl	%edi
  40b98a: 8d 7e 20                     	leal	0x20(%esi), %edi
  40b98d: 8b 2f                        	movl	(%edi), %ebp
  40b98f: 8d 5a 20                     	leal	0x20(%edx), %ebx
  40b992: 89 2b                        	movl	%ebp, (%ebx)
  40b994: 8b 6f 04                     	movl	0x4(%edi), %ebp
  40b997: 89 6b 04                     	movl	%ebp, 0x4(%ebx)
  40b99a: 8b 7f 08                     	movl	0x8(%edi), %edi
  40b99d: 89 7b 08                     	movl	%edi, 0x8(%ebx)
  40b9a0: 83 c6 14                     	addl	$0x14, %esi
  40b9a3: 8b 3e                        	movl	(%esi), %edi
  40b9a5: 83 c2 14                     	addl	$0x14, %edx
  40b9a8: 89 3a                        	movl	%edi, (%edx)
  40b9aa: 8b 7e 04                     	movl	0x4(%esi), %edi
  40b9ad: 89 7a 04                     	movl	%edi, 0x4(%edx)
  40b9b0: 8b 76 08                     	movl	0x8(%esi), %esi
  40b9b3: 5f                           	popl	%edi
  40b9b4: 5d                           	popl	%ebp
  40b9b5: 89 72 08                     	movl	%esi, 0x8(%edx)
  40b9b8: 5b                           	popl	%ebx
  40b9b9: 8b 49 30                     	movl	0x30(%ecx), %ecx
  40b9bc: 89 48 30                     	movl	%ecx, 0x30(%eax)
  40b9bf: 5e                           	popl	%esi
  40b9c0: c3                           	retl
  40b9c1: 33 c0                        	xorl	%eax, %eax
  40b9c3: c3                           	retl
  40b9c4: 90                           	nop
  40b9c5: 90                           	nop
  40b9c6: 90                           	nop
  40b9c7: 90                           	nop
  40b9c8: 90                           	nop
  40b9c9: 90                           	nop
  40b9ca: 90                           	nop
  40b9cb: 90                           	nop
  40b9cc: 90                           	nop
  40b9cd: 90                           	nop
  40b9ce: 90                           	nop
  40b9cf: 90                           	nop
  40b9d0: 83 ec 10                     	subl	$0x10, %esp
  40b9d3: 53                           	pushl	%ebx
  40b9d4: 55                           	pushl	%ebp
  40b9d5: 56                           	pushl	%esi
  40b9d6: 8b 74 24 24                  	movl	0x24(%esp), %esi
  40b9da: 8b 6e 20                     	movl	0x20(%esi), %ebp
  40b9dd: 33 db                        	xorl	%ebx, %ebx
  40b9df: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  40b9e4: 89 5c 24 0c                  	movl	%ebx, 0xc(%esp)
  40b9e8: e8 03 97 37 00               	calll	0x7850f0 <.text+0x3840f0>
  40b9ed: 3b eb                        	cmpl	%ebx, %ebp
  40b9ef: 74 3d                        	je	0x40ba2e <.text+0xaa2e>
  40b9f1: 8d 55 20                     	leal	0x20(%ebp), %edx
  40b9f4: 33 c9                        	xorl	%ecx, %ecx
  40b9f6: e8 75 96 37 00               	calll	0x785070 <.text+0x384070>
  40b9fb: 8b 45 1c                     	movl	0x1c(%ebp), %eax
  40b9fe: 3b c3                        	cmpl	%ebx, %eax
  40ba00: 74 0a                        	je	0x40ba0c <.text+0xaa0c>
  40ba02: 0f b7 d0                     	movzwl	%ax, %edx
  40ba05: 33 c9                        	xorl	%ecx, %ecx
  40ba07: e8 a4 95 37 00               	calll	0x784fb0 <.text+0x383fb0>
  40ba0c: 8b 45 14                     	movl	0x14(%ebp), %eax
  40ba0f: 3b c3                        	cmpl	%ebx, %eax
