
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  40afb0: 56                           	pushl	%esi
  40afb1: 8b 74 24 08                  	movl	0x8(%esp), %esi
  40afb5: 57                           	pushl	%edi
  40afb6: bf c4 db ab 03               	movl	$0x3abdbc4, %edi        # imm = 0x3ABDBC4
  40afbb: eb 03                        	jmp	0x40afc0 <.text+0x9fc0>
  40afbd: 8d 49 00                     	leal	(%ecx), %ecx
  40afc0: 8b 17                        	movl	(%edi), %edx
  40afc2: 85 d2                        	testl	%edx, %edx
  40afc4: 74 30                        	je	0x40aff6 <.text+0x9ff6>
  40afc6: 3b d6                        	cmpl	%esi, %edx
  40afc8: 74 54                        	je	0x40b01e <.text+0xa01e>
  40afca: 8b 4a 0c                     	movl	0xc(%edx), %ecx
  40afcd: 85 c9                        	testl	%ecx, %ecx
  40afcf: 74 1f                        	je	0x40aff0 <.text+0x9ff0>
  40afd1: 3b ce                        	cmpl	%esi, %ecx
  40afd3: 74 49                        	je	0x40b01e <.text+0xa01e>
  40afd5: 8b 41 0c                     	movl	0xc(%ecx), %eax
  40afd8: 85 c0                        	testl	%eax, %eax
  40afda: 74 0e                        	je	0x40afea <.text+0x9fea>
  40afdc: 50                           	pushl	%eax
  40afdd: 56                           	pushl	%esi
  40afde: e8 8d ff ff ff               	calll	0x40af70 <.text+0x9f70>
  40afe3: 83 c4 08                     	addl	$0x8, %esp
  40afe6: 85 c0                        	testl	%eax, %eax
  40afe8: 75 34                        	jne	0x40b01e <.text+0xa01e>
  40afea: 8b 09                        	movl	(%ecx), %ecx
  40afec: 85 c9                        	testl	%ecx, %ecx
  40afee: 75 e1                        	jne	0x40afd1 <.text+0x9fd1>
  40aff0: 8b 12                        	movl	(%edx), %edx
  40aff2: 85 d2                        	testl	%edx, %edx
  40aff4: 75 d0                        	jne	0x40afc6 <.text+0x9fc6>
  40aff6: 83 c7 04                     	addl	$0x4, %edi
  40aff9: 81 ff e4 db ab 03            	cmpl	$0x3abdbe4, %edi        # imm = 0x3ABDBE4
  40afff: 7c bf                        	jl	0x40afc0 <.text+0x9fc0>
  40b001: 68 b0 cb 7d 00               	pushl	$0x7dcbb0               # imm = 0x7DCBB0
  40b006: e8 f5 5f ff ff               	calll	0x401000 <.text>
  40b00b: 56                           	pushl	%esi
  40b00c: 68 9c cb 7d 00               	pushl	$0x7dcb9c               # imm = 0x7DCB9C
  40b011: e8 6a 46 11 00               	calll	0x51f680 <.text+0x11e680>
  40b016: 83 c4 0c                     	addl	$0xc, %esp
  40b019: 5f                           	popl	%edi
  40b01a: 33 c0                        	xorl	%eax, %eax
  40b01c: 5e                           	popl	%esi
  40b01d: c3                           	retl
  40b01e: 5f                           	popl	%edi
  40b01f: b8 01 00 00 00               	movl	$0x1, %eax
  40b024: 5e                           	popl	%esi
  40b025: c3                           	retl
  40b026: 90                           	nop
  40b027: 90                           	nop
  40b028: 90                           	nop
  40b029: 90                           	nop
  40b02a: 90                           	nop
  40b02b: 90                           	nop
  40b02c: 90                           	nop
  40b02d: 90                           	nop
  40b02e: 90                           	nop
  40b02f: 90                           	nop
  40b030: a1 ec db ab 03               	movl	0x3abdbec, %eax
  40b035: 56                           	pushl	%esi
  40b036: 33 f6                        	xorl	%esi, %esi
  40b038: 3b c6                        	cmpl	%esi, %eax
  40b03a: 74 7f                        	je	0x40b0bb <.text+0xa0bb>
  40b03c: 3b d6                        	cmpl	%esi, %edx
  40b03e: 7c 7b                        	jl	0x40b0bb <.text+0xa0bb>
  40b040: 83 fa 09                     	cmpl	$0x9, %edx
  40b043: 7d 76                        	jge	0x40b0bb <.text+0xa0bb>
  40b045: 8b 08                        	movl	(%eax), %ecx
  40b047: 3b ce                        	cmpl	%esi, %ecx
  40b049: 74 03                        	je	0x40b04e <.text+0xa04e>
  40b04b: 89 71 04                     	movl	%esi, 0x4(%ecx)
  40b04e: 3b fe                        	cmpl	%esi, %edi
  40b050: 89 0d ec db ab 03            	movl	%ecx, 0x3abdbec
  40b056: 74 05                        	je	0x40b05d <.text+0xa05d>
  40b058: 89 78 10                     	movl	%edi, 0x10(%eax)
  40b05b: eb 07                        	jmp	0x40b064 <.text+0xa064>
  40b05d: c7 40 10 80 f6 51 00         	movl	$0x51f680, 0x10(%eax)   # imm = 0x51F680
  40b064: 83 fa 07                     	cmpl	$0x7, %edx
  40b067: 74 32                        	je	0x40b09b <.text+0xa09b>
  40b069: 83 fa 08                     	cmpl	$0x8, %edx
  40b06c: 75 05                        	jne	0x40b073 <.text+0xa073>
  40b06e: ba 07 00 00 00               	movl	$0x7, %edx
  40b073: 8b 0c 95 c4 db ab 03         	movl	0x3abdbc4(,%edx,4), %ecx
  40b07a: 3b ce                        	cmpl	%esi, %ecx
  40b07c: 75 0e                        	jne	0x40b08c <.text+0xa08c>
  40b07e: 89 04 95 c4 db ab 03         	movl	%eax, 0x3abdbc4(,%edx,4)
  40b085: 89 30                        	movl	%esi, (%eax)
  40b087: 89 70 04                     	movl	%esi, 0x4(%eax)
  40b08a: eb 0f                        	jmp	0x40b09b <.text+0xa09b>
  40b08c: 89 70 04                     	movl	%esi, 0x4(%eax)
  40b08f: 89 08                        	movl	%ecx, (%eax)
  40b091: 89 41 04                     	movl	%eax, 0x4(%ecx)
  40b094: 89 04 95 c4 db ab 03         	movl	%eax, 0x3abdbc4(,%edx,4)
  40b09b: 89 70 1c                     	movl	%esi, 0x1c(%eax)
  40b09e: 89 70 20                     	movl	%esi, 0x20(%eax)
  40b0a1: 89 70 24                     	movl	%esi, 0x24(%eax)
  40b0a4: 89 70 28                     	movl	%esi, 0x28(%eax)
  40b0a7: 89 70 2c                     	movl	%esi, 0x2c(%eax)
  40b0aa: 89 70 14                     	movl	%esi, 0x14(%eax)
  40b0ad: 89 70 18                     	movl	%esi, 0x18(%eax)
  40b0b0: 89 70 0c                     	movl	%esi, 0xc(%eax)
  40b0b3: 89 70 08                     	movl	%esi, 0x8(%eax)
  40b0b6: 89 70 30                     	movl	%esi, 0x30(%eax)
  40b0b9: 5e                           	popl	%esi
  40b0ba: c3                           	retl
  40b0bb: 33 c0                        	xorl	%eax, %eax
  40b0bd: 5e                           	popl	%esi
  40b0be: c3                           	retl
  40b0bf: 90                           	nop
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
  40b125: 90                           	nop
  40b126: 90                           	nop
  40b127: 90                           	nop
  40b128: 90                           	nop
  40b129: 90                           	nop
  40b12a: 90                           	nop
  40b12b: 90                           	nop
  40b12c: 90                           	nop
  40b12d: 90                           	nop
  40b12e: 90                           	nop
  40b12f: 90                           	nop
  40b130: 8b 44 24 04                  	movl	0x4(%esp), %eax
  40b134: 56                           	pushl	%esi
  40b135: 8b 70 0c                     	movl	0xc(%eax), %esi
  40b138: 85 f6                        	testl	%esi, %esi
  40b13a: 74 2a                        	je	0x40b166 <.text+0xa166>
  40b13c: 57                           	pushl	%edi
  40b13d: 8d 49 00                     	leal	(%ecx), %ecx
  40b140: 8b 46 14                     	movl	0x14(%esi), %eax
  40b143: 85 c0                        	testl	%eax, %eax
  40b145: 8b 3e                        	movl	(%esi), %edi
  40b147: 74 06                        	je	0x40b14f <.text+0xa14f>
  40b149: 56                           	pushl	%esi
  40b14a: ff d0                        	calll	*%eax
  40b14c: 83 c4 04                     	addl	$0x4, %esp
  40b14f: 8b 46 0c                     	movl	0xc(%esi), %eax
  40b152: 85 c0                        	testl	%eax, %eax
  40b154: 74 09                        	je	0x40b15f <.text+0xa15f>
  40b156: 56                           	pushl	%esi
  40b157: e8 d4 ff ff ff               	calll	0x40b130 <.text+0xa130>
  40b15c: 83 c4 04                     	addl	$0x4, %esp
  40b15f: 85 ff                        	testl	%edi, %edi
  40b161: 8b f7                        	movl	%edi, %esi
  40b163: 75 db                        	jne	0x40b140 <.text+0xa140>
  40b165: 5f                           	popl	%edi
  40b166: 5e                           	popl	%esi
  40b167: c3                           	retl
  40b168: 90                           	nop
  40b169: 90                           	nop
  40b16a: 90                           	nop
  40b16b: 90                           	nop
  40b16c: 90                           	nop
  40b16d: 90                           	nop
  40b16e: 90                           	nop
  40b16f: 90                           	nop
  40b170: 56                           	pushl	%esi
  40b171: 6a 08                        	pushl	$0x8
  40b173: e8 48 ff ff ff               	calll	0x40b0c0 <.text+0xa0c0>
  40b178: 83 c4 04                     	addl	$0x4, %esp
  40b17b: 33 f6                        	xorl	%esi, %esi
  40b17d: 8d 49 00                     	leal	(%ecx), %ecx
  40b180: 56                           	pushl	%esi
  40b181: e8 3a ff ff ff               	calll	0x40b0c0 <.text+0xa0c0>
  40b186: 83 c4 04                     	addl	$0x4, %esp
  40b189: 46                           	incl	%esi
  40b18a: 83 fe 07                     	cmpl	$0x7, %esi
  40b18d: 7c f1                        	jl	0x40b180 <.text+0xa180>
  40b18f: 5e                           	popl	%esi
  40b190: c3                           	retl
  40b191: 90                           	nop
  40b192: 90                           	nop
  40b193: 90                           	nop
  40b194: 90                           	nop
  40b195: 90                           	nop
  40b196: 90                           	nop
  40b197: 90                           	nop
  40b198: 90                           	nop
  40b199: 90                           	nop
  40b19a: 90                           	nop
  40b19b: 90                           	nop
  40b19c: 90                           	nop
  40b19d: 90                           	nop
  40b19e: 90                           	nop
  40b19f: 90                           	nop
  40b1a0: e8 cb 4a 24 00               	calll	0x64fc70 <.text+0x24ec70>
  40b1a5: 8b 15 f0 db ab 03            	movl	0x3abdbf0, %edx
  40b1ab: 85 d2                        	testl	%edx, %edx
  40b1ad: 75 1f                        	jne	0x40b1ce <.text+0xa1ce>
  40b1af: 52                           	pushl	%edx
  40b1b0: 52                           	pushl	%edx
  40b1b1: 52                           	pushl	%edx
  40b1b2: ff 15 78 b0 7d 00            	calll	*0x7db078
  40b1b8: 68 20 00 00 01               	pushl	$0x1000020              # imm = 0x1000020
  40b1bd: 6a 08                        	pushl	$0x8
  40b1bf: 50                           	pushl	%eax
  40b1c0: ff 15 74 b0 7d 00            	calll	*0x7db074
  40b1c6: 8b d0                        	movl	%eax, %edx
  40b1c8: 89 15 f0 db ab 03            	movl	%edx, 0x3abdbf0
  40b1ce: 56                           	pushl	%esi
  40b1cf: 8d b2 00 00 00 01            	leal	0x1000000(%edx), %esi
  40b1d5: 57                           	pushl	%edi
  40b1d6: b9 00 00 40 00               	movl	$0x400000, %ecx         # imm = 0x400000
  40b1db: 83 c8 ff                     	orl	$-0x1, %eax
  40b1de: 8b fa                        	movl	%edx, %edi
  40b1e0: f3 ab                        	rep		stosl	%eax, %es:(%edi)
  40b1e2: 89 32                        	movl	%esi, (%edx)
  40b1e4: c7 42 04 00 00 00 00         	movl	$0x0, 0x4(%edx)
  40b1eb: 89 72 08                     	movl	%esi, 0x8(%edx)
  40b1ee: c7 42 0c 00 00 00 00         	movl	$0x0, 0xc(%edx)
  40b1f5: 5f                           	popl	%edi
  40b1f6: c7 06 00 00 00 00            	movl	$0x0, (%esi)
  40b1fc: 89 56 04                     	movl	%edx, 0x4(%esi)
  40b1ff: c7 46 08 00 00 00 00         	movl	$0x0, 0x8(%esi)
  40b206: 89 56 0c                     	movl	%edx, 0xc(%esi)
  40b209: 89 15 e8 db ab 03            	movl	%edx, 0x3abdbe8
  40b20f: 5e                           	popl	%esi
  40b210: c3                           	retl
  40b211: 90                           	nop
  40b212: 90                           	nop
  40b213: 90                           	nop
  40b214: 90                           	nop
  40b215: 90                           	nop
  40b216: 90                           	nop
  40b217: 90                           	nop
  40b218: 90                           	nop
  40b219: 90                           	nop
  40b21a: 90                           	nop
  40b21b: 90                           	nop
  40b21c: 90                           	nop
  40b21d: 90                           	nop
  40b21e: 90                           	nop
  40b21f: 90                           	nop
  40b220: 8b 4c 24 04                  	movl	0x4(%esp), %ecx
  40b224: 85 c9                        	testl	%ecx, %ecx
  40b226: 74 2d                        	je	0x40b255 <.text+0xa255>
  40b228: a1 e8 db ab 03               	movl	0x3abdbe8, %eax
  40b22d: 85 c0                        	testl	%eax, %eax
  40b22f: 74 24                        	je	0x40b255 <.text+0xa255>
  40b231: 8b 50 08                     	movl	0x8(%eax), %edx
  40b234: 85 d2                        	testl	%edx, %edx
  40b236: 74 1d                        	je	0x40b255 <.text+0xa255>
  40b238: 8d 51 3f                     	leal	0x3f(%ecx), %edx
  40b23b: 83 e2 e0                     	andl	$-0x20, %edx
  40b23e: 8b ff                        	movl	%edi, %edi
  40b240: 8b 48 08                     	movl	0x8(%eax), %ecx
  40b243: 85 c9                        	testl	%ecx, %ecx
  40b245: 74 0e                        	je	0x40b255 <.text+0xa255>
  40b247: 2b c8                        	subl	%eax, %ecx
  40b249: 3b ca                        	cmpl	%edx, %ecx
  40b24b: 74 0b                        	je	0x40b258 <.text+0xa258>
  40b24d: 77 34                        	ja	0x40b283 <.text+0xa283>
  40b24f: 8b 00                        	movl	(%eax), %eax
  40b251: 85 c0                        	testl	%eax, %eax
  40b253: 75 eb                        	jne	0x40b240 <.text+0xa240>
  40b255: 33 c0                        	xorl	%eax, %eax
  40b257: c3                           	retl
  40b258: 8b 08                        	movl	(%eax), %ecx
  40b25a: 85 c9                        	testl	%ecx, %ecx
  40b25c: 8b 50 04                     	movl	0x4(%eax), %edx
  40b25f: 74 03                        	je	0x40b264 <.text+0xa264>
  40b261: 89 51 04                     	movl	%edx, 0x4(%ecx)
  40b264: 85 d2                        	testl	%edx, %edx
  40b266: 74 04                        	je	0x40b26c <.text+0xa26c>
  40b268: 89 0a                        	movl	%ecx, (%edx)
  40b26a: eb 06                        	jmp	0x40b272 <.text+0xa272>
  40b26c: 89 0d e8 db ab 03            	movl	%ecx, 0x3abdbe8
  40b272: c7 00 00 00 00 00            	movl	$0x0, (%eax)
  40b278: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%eax)
  40b27f: 83 c0 20                     	addl	$0x20, %eax
  40b282: c3                           	retl
  40b283: 8d 0c 10                     	leal	(%eax,%edx), %ecx
  40b286: 56                           	pushl	%esi
  40b287: 8b 30                        	movl	(%eax), %esi
  40b289: 85 f6                        	testl	%esi, %esi
  40b28b: 57                           	pushl	%edi
  40b28c: 8b 78 04                     	movl	0x4(%eax), %edi
  40b28f: 89 31                        	movl	%esi, (%ecx)
  40b291: 89 79 04                     	movl	%edi, 0x4(%ecx)
  40b294: 8b 50 08                     	movl	0x8(%eax), %edx
  40b297: 89 51 08                     	movl	%edx, 0x8(%ecx)
  40b29a: 89 41 0c                     	movl	%eax, 0xc(%ecx)
  40b29d: 8b 50 08                     	movl	0x8(%eax), %edx
  40b2a0: 89 4a 0c                     	movl	%ecx, 0xc(%edx)
  40b2a3: 89 48 08                     	movl	%ecx, 0x8(%eax)
  40b2a6: 74 03                        	je	0x40b2ab <.text+0xa2ab>
  40b2a8: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  40b2ab: 85 ff                        	testl	%edi, %edi
  40b2ad: 74 04                        	je	0x40b2b3 <.text+0xa2b3>
  40b2af: 89 0f                        	movl	%ecx, (%edi)
  40b2b1: eb 06                        	jmp	0x40b2b9 <.text+0xa2b9>
  40b2b3: 89 0d e8 db ab 03            	movl	%ecx, 0x3abdbe8
  40b2b9: 5f                           	popl	%edi
  40b2ba: c7 00 00 00 00 00            	movl	$0x0, (%eax)
  40b2c0: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%eax)
  40b2c7: 83 c0 20                     	addl	$0x20, %eax
  40b2ca: 5e                           	popl	%esi
  40b2cb: c3                           	retl
  40b2cc: 90                           	nop
  40b2cd: 90                           	nop
  40b2ce: 90                           	nop
  40b2cf: 90                           	nop
  40b2d0: 56                           	pushl	%esi
  40b2d1: 8b 74 24 0c                  	movl	0xc(%esp), %esi
  40b2d5: 83 c6 03                     	addl	$0x3, %esi
  40b2d8: 83 e6 fc                     	andl	$-0x4, %esi
  40b2db: 0f af 74 24 08               	imull	0x8(%esp), %esi
  40b2e0: 56                           	pushl	%esi
  40b2e1: e8 3a ff ff ff               	calll	0x40b220 <.text+0xa220>
  40b2e6: 8b d0                        	movl	%eax, %edx
  40b2e8: 83 c4 04                     	addl	$0x4, %esp
  40b2eb: 85 d2                        	testl	%edx, %edx
  40b2ed: 74 11                        	je	0x40b300 <.text+0xa300>
  40b2ef: c1 ee 02                     	shrl	$0x2, %esi
  40b2f2: 8b ce                        	movl	%esi, %ecx
  40b2f4: 85 c9                        	testl	%ecx, %ecx
  40b2f6: 74 08                        	je	0x40b300 <.text+0xa300>
  40b2f8: 57                           	pushl	%edi
  40b2f9: 33 c0                        	xorl	%eax, %eax
  40b2fb: 8b fa                        	movl	%edx, %edi
  40b2fd: f3 ab                        	rep		stosl	%eax, %es:(%edi)
  40b2ff: 5f                           	popl	%edi
  40b300: 8b c2                        	movl	%edx, %eax
  40b302: 5e                           	popl	%esi
  40b303: c3                           	retl
  40b304: 90                           	nop
  40b305: 90                           	nop
  40b306: 90                           	nop
  40b307: 90                           	nop
  40b308: 90                           	nop
  40b309: 90                           	nop
  40b30a: 90                           	nop
  40b30b: 90                           	nop
  40b30c: 90                           	nop
  40b30d: 90                           	nop
  40b30e: 90                           	nop
  40b30f: 90                           	nop
  40b310: 8b 44 24 04                  	movl	0x4(%esp), %eax
  40b314: 85 c0                        	testl	%eax, %eax
  40b316: 0f 84 01 01 00 00            	je	0x40b41d <.text+0xa41d>
  40b31c: 8d 48 e0                     	leal	-0x20(%eax), %ecx
  40b31f: 83 39 00                     	cmpl	$0x0, (%ecx)
  40b322: 74 0e                        	je	0x40b332 <.text+0xa332>
  40b324: 8b 41 04                     	movl	0x4(%ecx), %eax
  40b327: 85 c0                        	testl	%eax, %eax
  40b329: 74 07                        	je	0x40b332 <.text+0xa332>
  40b32b: eb 03                        	jmp	0x40b330 <.text+0xa330>
  40b32d: 8d 49 00                     	leal	(%ecx), %ecx
  40b330: eb fe                        	jmp	0x40b330 <.text+0xa330>
  40b332: 8b 51 0c                     	movl	0xc(%ecx), %edx
  40b335: 85 d2                        	testl	%edx, %edx
  40b337: 8b 41 08                     	movl	0x8(%ecx), %eax
  40b33a: 56                           	pushl	%esi
  40b33b: 57                           	pushl	%edi
  40b33c: 74 70                        	je	0x40b3ae <.text+0xa3ae>
  40b33e: 8b 32                        	movl	(%edx), %esi
  40b340: 85 f6                        	testl	%esi, %esi
  40b342: 75 07                        	jne	0x40b34b <.text+0xa34b>
  40b344: 8b 7a 04                     	movl	0x4(%edx), %edi
  40b347: 85 ff                        	testl	%edi, %edi
  40b349: 74 4f                        	je	0x40b39a <.text+0xa39a>
  40b34b: 83 38 00                     	cmpl	$0x0, (%eax)
  40b34e: 75 07                        	jne	0x40b357 <.text+0xa357>
  40b350: 8b 78 04                     	movl	0x4(%eax), %edi
  40b353: 85 ff                        	testl	%edi, %edi
  40b355: 74 43                        	je	0x40b39a <.text+0xa39a>
  40b357: 8b 78 08                     	movl	0x8(%eax), %edi
  40b35a: 85 ff                        	testl	%edi, %edi
  40b35c: 74 3c                        	je	0x40b39a <.text+0xa39a>
  40b35e: 89 50 0c                     	movl	%edx, 0xc(%eax)
  40b361: 89 42 08                     	movl	%eax, 0x8(%edx)
  40b364: 8b 08                        	movl	(%eax), %ecx
  40b366: 85 c9                        	testl	%ecx, %ecx
  40b368: 8b 70 04                     	movl	0x4(%eax), %esi
  40b36b: 74 03                        	je	0x40b370 <.text+0xa370>
  40b36d: 89 71 04                     	movl	%esi, 0x4(%ecx)
  40b370: 85 f6                        	testl	%esi, %esi
  40b372: 74 11                        	je	0x40b385 <.text+0xa385>
  40b374: 89 0e                        	movl	%ecx, (%esi)
  40b376: 8b 48 08                     	movl	0x8(%eax), %ecx
  40b379: 89 51 0c                     	movl	%edx, 0xc(%ecx)
  40b37c: 8b 40 08                     	movl	0x8(%eax), %eax
  40b37f: 5f                           	popl	%edi
  40b380: 89 42 08                     	movl	%eax, 0x8(%edx)
  40b383: 5e                           	popl	%esi
  40b384: c3                           	retl
  40b385: 89 0d e8 db ab 03            	movl	%ecx, 0x3abdbe8
  40b38b: 8b 48 08                     	movl	0x8(%eax), %ecx
  40b38e: 89 51 0c                     	movl	%edx, 0xc(%ecx)
  40b391: 8b 40 08                     	movl	0x8(%eax), %eax
  40b394: 5f                           	popl	%edi
  40b395: 89 42 08                     	movl	%eax, 0x8(%edx)
  40b398: 5e                           	popl	%esi
  40b399: c3                           	retl
  40b39a: 85 f6                        	testl	%esi, %esi
  40b39c: 75 07                        	jne	0x40b3a5 <.text+0xa3a5>
  40b39e: 8b 72 04                     	movl	0x4(%edx), %esi
  40b3a1: 85 f6                        	testl	%esi, %esi
  40b3a3: 74 09                        	je	0x40b3ae <.text+0xa3ae>
  40b3a5: 5f                           	popl	%edi
  40b3a6: 89 50 0c                     	movl	%edx, 0xc(%eax)
  40b3a9: 89 42 08                     	movl	%eax, 0x8(%edx)
  40b3ac: 5e                           	popl	%esi
  40b3ad: c3                           	retl
  40b3ae: 8b 10                        	movl	(%eax), %edx
  40b3b0: 85 d2                        	testl	%edx, %edx
  40b3b2: 75 07                        	jne	0x40b3bb <.text+0xa3bb>
  40b3b4: 8b 70 04                     	movl	0x4(%eax), %esi
  40b3b7: 85 f6                        	testl	%esi, %esi
  40b3b9: 74 45                        	je	0x40b400 <.text+0xa400>
  40b3bb: 8b 70 08                     	movl	0x8(%eax), %esi
  40b3be: 85 f6                        	testl	%esi, %esi
  40b3c0: 74 3e                        	je	0x40b400 <.text+0xa400>
  40b3c2: 85 d2                        	testl	%edx, %edx
  40b3c4: 8b 70 04                     	movl	0x4(%eax), %esi
  40b3c7: 89 11                        	movl	%edx, (%ecx)
  40b3c9: 89 71 04                     	movl	%esi, 0x4(%ecx)
  40b3cc: 74 05                        	je	0x40b3d3 <.text+0xa3d3>
  40b3ce: 8b 10                        	movl	(%eax), %edx
  40b3d0: 89 4a 04                     	movl	%ecx, 0x4(%edx)
  40b3d3: 85 f6                        	testl	%esi, %esi
  40b3d5: 74 14                        	je	0x40b3eb <.text+0xa3eb>
  40b3d7: 8b 50 04                     	movl	0x4(%eax), %edx
  40b3da: 89 0a                        	movl	%ecx, (%edx)
  40b3dc: 8b 50 08                     	movl	0x8(%eax), %edx
  40b3df: 89 51 08                     	movl	%edx, 0x8(%ecx)
  40b3e2: 8b 40 08                     	movl	0x8(%eax), %eax
  40b3e5: 5f                           	popl	%edi
  40b3e6: 89 48 0c                     	movl	%ecx, 0xc(%eax)
  40b3e9: 5e                           	popl	%esi
  40b3ea: c3                           	retl
  40b3eb: 8b 50 08                     	movl	0x8(%eax), %edx
  40b3ee: 89 51 08                     	movl	%edx, 0x8(%ecx)
  40b3f1: 8b 40 08                     	movl	0x8(%eax), %eax
  40b3f4: 5f                           	popl	%edi
  40b3f5: 89 0d e8 db ab 03            	movl	%ecx, 0x3abdbe8
  40b3fb: 89 48 0c                     	movl	%ecx, 0xc(%eax)
  40b3fe: 5e                           	popl	%esi
  40b3ff: c3                           	retl
  40b400: a1 e8 db ab 03               	movl	0x3abdbe8, %eax
  40b405: 85 c0                        	testl	%eax, %eax
  40b407: 89 0d e8 db ab 03            	movl	%ecx, 0x3abdbe8
  40b40d: 74 03                        	je	0x40b412 <.text+0xa412>
  40b40f: 89 48 04                     	movl	%ecx, 0x4(%eax)
  40b412: 5f                           	popl	%edi
  40b413: 89 01                        	movl	%eax, (%ecx)
  40b415: c7 41 04 00 00 00 00         	movl	$0x0, 0x4(%ecx)
  40b41c: 5e                           	popl	%esi
  40b41d: c3                           	retl
  40b41e: 90                           	nop
  40b41f: 90                           	nop
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
  40b454: 90                           	nop
  40b455: 90                           	nop
  40b456: 90                           	nop
  40b457: 90                           	nop
  40b458: 90                           	nop
  40b459: 90                           	nop
  40b45a: 90                           	nop
  40b45b: 90                           	nop
  40b45c: 90                           	nop
  40b45d: 90                           	nop
  40b45e: 90                           	nop
  40b45f: 90                           	nop
