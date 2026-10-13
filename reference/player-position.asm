
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  44cdf0: 83 ec 20                     	subl	$0x20, %esp
  44cdf3: 53                           	pushl	%ebx
  44cdf4: 55                           	pushl	%ebp
  44cdf5: 8b 6c 24 30                  	movl	0x30(%esp), %ebp
  44cdf9: 33 c0                        	xorl	%eax, %eax
  44cdfb: 56                           	pushl	%esi
  44cdfc: 8b 74 24 38                  	movl	0x38(%esp), %esi
  44ce00: 8b 4e 5c                     	movl	0x5c(%esi), %ecx
  44ce03: 57                           	pushl	%edi
  44ce04: 8b 7c 24 34                  	movl	0x34(%esp), %edi
  44ce08: 89 46 4c                     	movl	%eax, 0x4c(%esi)
  44ce0b: 89 46 48                     	movl	%eax, 0x48(%esi)
  44ce0e: 89 46 44                     	movl	%eax, 0x44(%esi)
  44ce11: 89 4e 60                     	movl	%ecx, 0x60(%esi)
  44ce14: 89 46 5c                     	movl	%eax, 0x5c(%esi)
  44ce17: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  44ce1b: 8b 47 38                     	movl	0x38(%edi), %eax
  44ce1e: 66 8b 48 04                  	movw	0x4(%eax), %cx
  44ce22: f6 c1 10                     	testb	$0x10, %cl
  44ce25: 0f 84 93 04 00 00            	je	0x44d2be <.text+0x4c2be>
  44ce2b: f6 47 04 04                  	testb	$0x4, 0x4(%edi)
  44ce2f: 0f 85 89 04 00 00            	jne	0x44d2be <.text+0x4c2be>
  44ce35: f6 c1 04                     	testb	$0x4, %cl
  44ce38: 0f 84 80 04 00 00            	je	0x44d2be <.text+0x4c2be>
  44ce3e: 0f bf 90 a2 00 00 00         	movswl	0xa2(%eax), %edx
  44ce45: 8b 98 a4 00 00 00            	movl	0xa4(%eax), %ebx
  44ce4b: 8b 5b 0c                     	movl	0xc(%ebx), %ebx
  44ce4e: 8d 14 52                     	leal	(%edx,%edx,2), %edx
  44ce51: c1 e2 04                     	shll	$0x4, %edx
  44ce54: 03 d3                        	addl	%ebx, %edx
  44ce56: f6 42 02 07                  	testb	$0x7, 0x2(%edx)
  44ce5a: 89 54 24 34                  	movl	%edx, 0x34(%esp)
  44ce5e: 0f 84 5a 04 00 00            	je	0x44d2be <.text+0x4c2be>
  44ce64: 8b 40 0c                     	movl	0xc(%eax), %eax
  44ce67: 8d 5d 04                     	leal	0x4(%ebp), %ebx
  44ce6a: 8b cb                        	movl	%ebx, %ecx
  44ce6c: 8b 11                        	movl	(%ecx), %edx
  44ce6e: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  44ce72: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  44ce76: 8b 41 04                     	movl	0x4(%ecx), %eax
  44ce79: 8b 49 08                     	movl	0x8(%ecx), %ecx
  44ce7c: 8d 54 24 24                  	leal	0x24(%esp), %edx
  44ce80: 52                           	pushl	%edx
  44ce81: 57                           	pushl	%edi
  44ce82: 89 44 24 24                  	movl	%eax, 0x24(%esp)
  44ce86: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  44ce8a: e8 41 f0 fc ff               	calll	0x41bed0 <.text+0x1aed0>
  44ce8f: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  44ce93: 8b 4c 24 30                  	movl	0x30(%esp), %ecx
  44ce97: 8b 54 24 34                  	movl	0x34(%esp), %edx
  44ce9b: 89 46 44                     	movl	%eax, 0x44(%esi)
  44ce9e: 89 4e 48                     	movl	%ecx, 0x48(%esi)
  44cea1: 83 c4 08                     	addl	$0x8, %esp
  44cea4: 8d 4c 24 24                  	leal	0x24(%esp), %ecx
  44cea8: 89 56 4c                     	movl	%edx, 0x4c(%esi)
  44ceab: e8 10 ae 33 00               	calll	0x787cc0 <.text+0x386cc0>
  44ceb0: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44ceb6: df e0                        	fnstsw	%ax
  44ceb8: f6 c4 44                     	testb	$0x44, %ah
  44cebb: 7a 1a                        	jp	0x44ced7 <.text+0x4bed7>
  44cebd: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  44cec5: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%esp)
  44cecd: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  44ced5: eb 0f                        	jmp	0x44cee6 <.text+0x4bee6>
  44ced7: 8d 44 24 24                  	leal	0x24(%esp), %eax
  44cedb: 50                           	pushl	%eax
  44cedc: e8 df bb fc ff               	calll	0x418ac0 <.text+0x17ac0>
  44cee1: dd d8                        	fstp	%st(0)
  44cee3: 83 c4 04                     	addl	$0x4, %esp
  44cee6: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  44ceea: e8 d1 ad 33 00               	calll	0x787cc0 <.text+0x386cc0>
  44ceef: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cef5: df e0                        	fnstsw	%ax
  44cef7: f6 c4 44                     	testb	$0x44, %ah
  44cefa: 7a 1a                        	jp	0x44cf16 <.text+0x4bf16>
  44cefc: c7 44 24 18 00 00 00 00      	movl	$0x0, 0x18(%esp)
  44cf04: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44cf0c: c7 44 24 20 00 00 00 00      	movl	$0x0, 0x20(%esp)
  44cf14: eb 0f                        	jmp	0x44cf25 <.text+0x4bf25>
  44cf16: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  44cf1a: 51                           	pushl	%ecx
  44cf1b: e8 a0 bb fc ff               	calll	0x418ac0 <.text+0x17ac0>
  44cf20: dd d8                        	fstp	%st(0)
  44cf22: 83 c4 04                     	addl	$0x4, %esp
  44cf25: 8d 54 24 24                  	leal	0x24(%esp), %edx
  44cf29: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  44cf2d: e8 be ad 33 00               	calll	0x787cf0 <.text+0x386cf0>
  44cf32: d9 5c 24 3c                  	fstps	0x3c(%esp)
  44cf36: d9 46 38                     	flds	0x38(%esi)
  44cf39: d8 1d 2c 01 7e 00            	fcomps	0x7e012c
  44cf3f: df e0                        	fnstsw	%ax
  44cf41: f6 c4 01                     	testb	$0x1, %ah
  44cf44: 75 17                        	jne	0x44cf5d <.text+0x4bf5d>
  44cf46: 56                           	pushl	%esi
  44cf47: 55                           	pushl	%ebp
  44cf48: c7 44 24 18 02 00 00 00      	movl	$0x2, 0x18(%esp)
  44cf50: e8 fb f5 fe ff               	calll	0x43c550 <.text+0x3b550>
  44cf55: 83 c4 08                     	addl	$0x8, %esp
  44cf58: e9 13 04 00 00               	jmp	0x44d370 <.text+0x4c370>
  44cf5d: d9 46 38                     	flds	0x38(%esi)
  44cf60: d8 9e cc 00 00 00            	fcomps	0xcc(%esi)
  44cf66: df e0                        	fnstsw	%ax
  44cf68: f6 c4 01                     	testb	$0x1, %ah
  44cf6b: 75 2b                        	jne	0x44cf98 <.text+0x4bf98>
  44cf6d: d9 44 24 3c                  	flds	0x3c(%esp)
  44cf71: d8 1d ac 01 7e 00            	fcomps	0x7e01ac
  44cf77: df e0                        	fnstsw	%ax
  44cf79: f6 c4 05                     	testb	$0x5, %ah
  44cf7c: 7a 1a                        	jp	0x44cf98 <.text+0x4bf98>
  44cf7e: c7 44 24 10 02 00 00 00      	movl	$0x2, 0x10(%esp)
  44cf86: c7 45 0c 00 00 00 00         	movl	$0x0, 0xc(%ebp)
  44cf8d: c7 03 00 00 00 00            	movl	$0x0, (%ebx)
  44cf93: e9 d8 03 00 00               	jmp	0x44d370 <.text+0x4c370>
  44cf98: d9 46 38                     	flds	0x38(%esi)
  44cf9b: d8 9e c4 00 00 00            	fcomps	0xc4(%esi)
  44cfa1: df e0                        	fnstsw	%ax
  44cfa3: f6 c4 01                     	testb	$0x1, %ah
  44cfa6: 0f 85 20 02 00 00            	jne	0x44d1cc <.text+0x4c1cc>
  44cfac: d9 44 24 3c                  	flds	0x3c(%esp)
  44cfb0: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44cfb6: df e0                        	fnstsw	%ax
  44cfb8: f6 c4 05                     	testb	$0x5, %ah
  44cfbb: 0f 8a 0b 02 00 00            	jp	0x44d1cc <.text+0x4c1cc>
  44cfc1: 8b 57 38                     	movl	0x38(%edi), %edx
  44cfc4: 0f bf 82 a2 00 00 00         	movswl	0xa2(%edx), %eax
  44cfcb: 50                           	pushl	%eax
  44cfcc: 57                           	pushl	%edi
  44cfcd: c7 44 24 18 01 00 00 00      	movl	$0x1, 0x18(%esp)
  44cfd5: e8 e6 f1 fc ff               	calll	0x41c1c0 <.text+0x1b1c0>
  44cfda: 83 c4 08                     	addl	$0x8, %esp
  44cfdd: 85 c0                        	testl	%eax, %eax
  44cfdf: 74 1d                        	je	0x44cffe <.text+0x4bffe>
  44cfe1: 8b 4f 38                     	movl	0x38(%edi), %ecx
  44cfe4: 0f bf 91 a2 00 00 00         	movswl	0xa2(%ecx), %edx
  44cfeb: 52                           	pushl	%edx
  44cfec: 57                           	pushl	%edi
  44cfed: e8 ce f1 fc ff               	calll	0x41c1c0 <.text+0x1b1c0>
  44cff2: 83 c4 08                     	addl	$0x8, %esp
  44cff5: 83 f8 01                     	cmpl	$0x1, %eax
  44cff8: 0f 85 3e 01 00 00            	jne	0x44d13c <.text+0x4c13c>
  44cffe: 8b 54 24 34                  	movl	0x34(%esp), %edx
  44d002: d9 42 14                     	flds	0x14(%edx)
  44d005: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  44d009: d8 59 14                     	fcomps	0x14(%ecx)
  44d00c: df e0                        	fnstsw	%ax
  44d00e: f6 c4 41                     	testb	$0x41, %ah
  44d011: 0f 85 25 01 00 00            	jne	0x44d13c <.text+0x4c13c>
  44d017: 8a 41 03                     	movb	0x3(%ecx), %al
  44d01a: 8a 52 03                     	movb	0x3(%edx), %dl
  44d01d: c0 e8 02                     	shrb	$0x2, %al
  44d020: 24 03                        	andb	$0x3, %al
  44d022: 80 e2 03                     	andb	$0x3, %dl
  44d025: 3a d0                        	cmpb	%al, %dl
  44d027: 0f 87 0f 01 00 00            	ja	0x44d13c <.text+0x4c13c>
  44d02d: d9 41 14                     	flds	0x14(%ecx)
  44d030: 8b 44 24 34                  	movl	0x34(%esp), %eax
  44d034: d8 40 14                     	fadds	0x14(%eax)
  44d037: 51                           	pushl	%ecx
  44d038: d9 5c 24 40                  	fstps	0x40(%esp)
  44d03c: d9 44 24 24                  	flds	0x24(%esp)
  44d040: d8 4c 24 24                  	fmuls	0x24(%esp)
  44d044: d9 44 24 1c                  	flds	0x1c(%esp)
  44d048: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  44d04c: de c1                        	faddp	%st, %st(1)
  44d04e: d9 1c 24                     	fstps	(%esp)
  44d051: e8 9a a1 33 00               	calll	0x7871f0 <.text+0x3861f0>
  44d056: d8 74 24 3c                  	fdivs	0x3c(%esp)
  44d05a: e8 91 80 1f 00               	calll	0x6450f0 <.text+0x2440f0>
  44d05f: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  44d065: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  44d06b: e8 d0 72 1f 00               	calll	0x644340 <.text+0x243340>
  44d070: d9 44 24 2c                  	flds	0x2c(%esp)
  44d074: d9 44 24 24                  	flds	0x24(%esp)
  44d078: 89 44 24 3c                  	movl	%eax, 0x3c(%esp)
  44d07c: d9 f3                        	fpatan
  44d07e: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  44d084: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  44d08a: e8 b1 72 1f 00               	calll	0x644340 <.text+0x243340>
  44d08f: d9 44 24 20                  	flds	0x20(%esp)
  44d093: d9 44 24 18                  	flds	0x18(%esp)
  44d097: bb 00 80 00 00               	movl	$0x8000, %ebx           # imm = 0x8000
  44d09c: d9 f3                        	fpatan
  44d09e: 2b d8                        	subl	%eax, %ebx
  44d0a0: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  44d0a6: dc 0d c8 cc 7d 00            	fmull	0x7dccc8
  44d0ac: e8 8f 72 1f 00               	calll	0x644340 <.text+0x243340>
  44d0b1: 50                           	pushl	%eax
  44d0b2: 53                           	pushl	%ebx
  44d0b3: e8 e8 b2 fe ff               	calll	0x4383a0 <.text+0x373a0>
  44d0b8: 83 c4 08                     	addl	$0x8, %esp
  44d0bb: 85 c0                        	testl	%eax, %eax
  44d0bd: 7d 11                        	jge	0x44d0d0 <.text+0x4c0d0>
  44d0bf: 8b 44 24 3c                  	movl	0x3c(%esp), %eax
  44d0c3: bb 00 00 01 00               	movl	$0x10000, %ebx          # imm = 0x10000
  44d0c8: 2b d8                        	subl	%eax, %ebx
  44d0ca: 89 5c 24 3c                  	movl	%ebx, 0x3c(%esp)
  44d0ce: eb 04                        	jmp	0x44d0d4 <.text+0x4c0d4>
  44d0d0: 8b 5c 24 3c                  	movl	0x3c(%esp), %ebx
  44d0d4: e8 47 ba fc ff               	calll	0x418b20 <.text+0x17b20>
  44d0d9: 8b cb                        	movl	%ebx, %ecx
  44d0db: f7 d9                        	negl	%ecx
  44d0dd: 74 0c                        	je	0x44d0eb <.text+0x4c0eb>
  44d0df: f7 db                        	negl	%ebx
  44d0e1: 0f b7 d3                     	movzwl	%bx, %edx
  44d0e4: 33 c9                        	xorl	%ecx, %ecx
  44d0e6: e8 35 7d 33 00               	calll	0x784e20 <.text+0x383e20>
  44d0eb: 8d 5d 04                     	leal	0x4(%ebp), %ebx
  44d0ee: 8b d3                        	movl	%ebx, %edx
  44d0f0: 8b 02                        	movl	(%edx), %eax
  44d0f2: 8b 4a 04                     	movl	0x4(%edx), %ecx
  44d0f5: 8b 52 08                     	movl	0x8(%edx), %edx
  44d0f8: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  44d0fc: 8d 44 24 24                  	leal	0x24(%esp), %eax
  44d100: 89 4c 24 1c                  	movl	%ecx, 0x1c(%esp)
  44d104: 89 54 24 20                  	movl	%edx, 0x20(%esp)
  44d108: 50                           	pushl	%eax
  44d109: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
  44d10d: 33 c9                        	xorl	%ecx, %ecx
  44d10f: e8 4c 7c 33 00               	calll	0x784d60 <.text+0x383d60>
  44d114: b9 01 00 00 00               	movl	$0x1, %ecx
  44d119: e8 22 80 33 00               	calll	0x785140 <.text+0x384140>
  44d11e: 8b 47 18                     	movl	0x18(%edi), %eax
  44d121: 8b 4c 24 3c                  	movl	0x3c(%esp), %ecx
  44d125: 8b 54 24 24                  	movl	0x24(%esp), %edx
  44d129: 03 c1                        	addl	%ecx, %eax
  44d12b: 89 47 18                     	movl	%eax, 0x18(%edi)
  44d12e: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  44d132: 89 13                        	movl	%edx, (%ebx)
  44d134: 89 45 0c                     	movl	%eax, 0xc(%ebp)
  44d137: e9 34 02 00 00               	jmp	0x44d370 <.text+0x4c370>
  44d13c: 8b 4f 38                     	movl	0x38(%edi), %ecx
  44d13f: 0f bf 91 a2 00 00 00         	movswl	0xa2(%ecx), %edx
  44d146: 52                           	pushl	%edx
  44d147: 57                           	pushl	%edi
  44d148: e8 73 f0 fc ff               	calll	0x41c1c0 <.text+0x1b1c0>
  44d14d: 83 c4 08                     	addl	$0x8, %esp
  44d150: 83 f8 03                     	cmpl	$0x3, %eax
  44d153: 0f 85 17 02 00 00            	jne	0x44d370 <.text+0x4c370>
  44d159: 8b 47 38                     	movl	0x38(%edi), %eax
  44d15c: 0f bf 88 a2 00 00 00         	movswl	0xa2(%eax), %ecx
  44d163: 51                           	pushl	%ecx
  44d164: 57                           	pushl	%edi
  44d165: e8 56 f0 fc ff               	calll	0x41c1c0 <.text+0x1b1c0>
  44d16a: 83 c4 08                     	addl	$0x8, %esp
  44d16d: 83 f8 04                     	cmpl	$0x4, %eax
  44d170: 0f 85 fa 01 00 00            	jne	0x44d370 <.text+0x4c370>
  44d176: 8b 47 38                     	movl	0x38(%edi), %eax
  44d179: 66 8b 48 04                  	movw	0x4(%eax), %cx
  44d17d: f6 c1 10                     	testb	$0x10, %cl
  44d180: 74 3a                        	je	0x44d1bc <.text+0x4c1bc>
  44d182: f6 47 04 04                  	testb	$0x4, 0x4(%edi)
  44d186: 75 34                        	jne	0x44d1bc <.text+0x4c1bc>
  44d188: 0f bf 90 a2 00 00 00         	movswl	0xa2(%eax), %edx
  44d18f: 8b 80 a4 00 00 00            	movl	0xa4(%eax), %eax
  44d195: 8b 40 0c                     	movl	0xc(%eax), %eax
  44d198: 8d 14 52                     	leal	(%edx,%edx,2), %edx
  44d19b: c1 e2 04                     	shll	$0x4, %edx
  44d19e: f6 44 02 05 20               	testb	$0x20, 0x5(%edx,%eax)
  44d1a3: 75 17                        	jne	0x44d1bc <.text+0x4c1bc>
  44d1a5: f6 c1 04                     	testb	$0x4, %cl
  44d1a8: 74 12                        	je	0x44d1bc <.text+0x4c1bc>
  44d1aa: c7 45 0c 00 00 00 00         	movl	$0x0, 0xc(%ebp)
  44d1b1: c7 03 00 00 00 00            	movl	$0x0, (%ebx)
  44d1b7: e9 b4 01 00 00               	jmp	0x44d370 <.text+0x4c370>
  44d1bc: 55                           	pushl	%ebp
  44d1bd: 8b c6                        	movl	%esi, %eax
  44d1bf: e8 bc f3 fe ff               	calll	0x43c580 <.text+0x3b580>
  44d1c4: 83 c4 04                     	addl	$0x4, %esp
  44d1c7: e9 a4 01 00 00               	jmp	0x44d370 <.text+0x4c370>
  44d1cc: d9 44 24 3c                  	flds	0x3c(%esp)
  44d1d0: c7 44 24 10 01 00 00 00      	movl	$0x1, 0x10(%esp)
  44d1d8: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44d1de: df e0                        	fnstsw	%ax
  44d1e0: f6 c4 05                     	testb	$0x5, %ah
  44d1e3: 0f 8a 87 01 00 00            	jp	0x44d370 <.text+0x4c370>
  44d1e9: 57                           	pushl	%edi
  44d1ea: e8 d1 e7 fe ff               	calll	0x43b9c0 <.text+0x3a9c0>
  44d1ef: 83 c4 04                     	addl	$0x4, %esp
  44d1f2: f6 c4 04                     	testb	$0x4, %ah
  44d1f5: 74 1c                        	je	0x44d213 <.text+0x4c213>
  44d1f7: d9 44 24 24                  	flds	0x24(%esp)
  44d1fb: d8 4e 38                     	fmuls	0x38(%esi)
  44d1fe: d9 e0                        	fchs
  44d200: d9 1b                        	fstps	(%ebx)
  44d202: d9 44 24 2c                  	flds	0x2c(%esp)
  44d206: d8 4e 38                     	fmuls	0x38(%esi)
  44d209: d9 e0                        	fchs
  44d20b: d9 5d 0c                     	fstps	0xc(%ebp)
  44d20e: e9 5d 01 00 00               	jmp	0x44d370 <.text+0x4c370>
  44d213: 8b 47 38                     	movl	0x38(%edi), %eax
  44d216: 66 8b 48 04                  	movw	0x4(%eax), %cx
  44d21a: f6 c1 10                     	testb	$0x10, %cl
  44d21d: 74 9d                        	je	0x44d1bc <.text+0x4c1bc>
  44d21f: f6 47 04 04                  	testb	$0x4, 0x4(%edi)
  44d223: 75 97                        	jne	0x44d1bc <.text+0x4c1bc>
  44d225: 0f bf 90 a2 00 00 00         	movswl	0xa2(%eax), %edx
  44d22c: 8b 80 a4 00 00 00            	movl	0xa4(%eax), %eax
  44d232: 8b 40 0c                     	movl	0xc(%eax), %eax
  44d235: 8d 14 52                     	leal	(%edx,%edx,2), %edx
  44d238: c1 e2 04                     	shll	$0x4, %edx
  44d23b: f6 44 02 05 20               	testb	$0x20, 0x5(%edx,%eax)
  44d240: 0f 85 76 ff ff ff            	jne	0x44d1bc <.text+0x4c1bc>
  44d246: f6 c1 04                     	testb	$0x4, %cl
  44d249: 0f 84 6d ff ff ff            	je	0x44d1bc <.text+0x4c1bc>
  44d24f: d9 44 24 18                  	flds	0x18(%esp)
  44d253: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  44d257: d8 44 24 24                  	fadds	0x24(%esp)
  44d25b: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44d263: d9 5c 24 18                  	fstps	0x18(%esp)
  44d267: d9 44 24 20                  	flds	0x20(%esp)
  44d26b: d8 44 24 2c                  	fadds	0x2c(%esp)
  44d26f: d9 5c 24 20                  	fstps	0x20(%esp)
  44d273: e8 48 aa 33 00               	calll	0x787cc0 <.text+0x386cc0>
  44d278: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44d27e: df e0                        	fnstsw	%ax
  44d280: f6 c4 44                     	testb	$0x44, %ah
  44d283: 7b 27                        	jnp	0x44d2ac <.text+0x4c2ac>
  44d285: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  44d289: 51                           	pushl	%ecx
  44d28a: e8 31 b8 fc ff               	calll	0x418ac0 <.text+0x17ac0>
  44d28f: dd d8                        	fstp	%st(0)
  44d291: d9 44 24 1c                  	flds	0x1c(%esp)
  44d295: 83 c4 04                     	addl	$0x4, %esp
  44d298: d8 4e 38                     	fmuls	0x38(%esi)
  44d29b: d9 1b                        	fstps	(%ebx)
  44d29d: d9 44 24 20                  	flds	0x20(%esp)
  44d2a1: d8 4e 38                     	fmuls	0x38(%esi)
  44d2a4: d9 5d 0c                     	fstps	0xc(%ebp)
  44d2a7: e9 c4 00 00 00               	jmp	0x44d370 <.text+0x4c370>
  44d2ac: c7 03 00 00 00 00            	movl	$0x0, (%ebx)
  44d2b2: c7 45 0c 00 00 00 00         	movl	$0x0, 0xc(%ebp)
  44d2b9: e9 b2 00 00 00               	jmp	0x44d370 <.text+0x4c370>
  44d2be: f6 c1 01                     	testb	$0x1, %cl
  44d2c1: 0f 84 a9 00 00 00            	je	0x44d370 <.text+0x4c370>
  44d2c7: 33 db                        	xorl	%ebx, %ebx
  44d2c9: 3b c3                        	cmpl	%ebx, %eax
  44d2cb: 0f 84 9f 00 00 00            	je	0x44d370 <.text+0x4c370>
  44d2d1: 8b 80 a4 00 00 00            	movl	0xa4(%eax), %eax
  44d2d7: 3b c3                        	cmpl	%ebx, %eax
  44d2d9: 0f 84 91 00 00 00            	je	0x44d370 <.text+0x4c370>
  44d2df: 8b 88 9c 00 00 00            	movl	0x9c(%eax), %ecx
  44d2e5: 3b cb                        	cmpl	%ebx, %ecx
  44d2e7: 0f 84 83 00 00 00            	je	0x44d370 <.text+0x4c370>
  44d2ed: 66 39 58 02                  	cmpw	%bx, 0x2(%eax)
  44d2f1: 74 7d                        	je	0x44d370 <.text+0x4c370>
  44d2f3: 8b 51 20                     	movl	0x20(%ecx), %edx
  44d2f6: 3b d3                        	cmpl	%ebx, %edx
  44d2f8: 74 76                        	je	0x44d370 <.text+0x4c370>
  44d2fa: 80 7a 02 0c                  	cmpb	$0xc, 0x2(%edx)
  44d2fe: 75 70                        	jne	0x44d370 <.text+0x4c370>
  44d300: 66 8b 4f 04                  	movw	0x4(%edi), %cx
  44d304: f6 c1 01                     	testb	$0x1, %cl
  44d307: 74 67                        	je	0x44d370 <.text+0x4c370>
  44d309: d9 46 3c                     	flds	0x3c(%esi)
  44d30c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44d312: df e0                        	fnstsw	%ax
  44d314: f6 c4 41                     	testb	$0x41, %ah
  44d317: 74 57                        	je	0x44d370 <.text+0x4c370>
  44d319: d9 46 38                     	flds	0x38(%esi)
  44d31c: d8 9e c4 00 00 00            	fcomps	0xc4(%esi)
  44d322: df e0                        	fnstsw	%ax
  44d324: f6 c4 05                     	testb	$0x5, %ah
  44d327: 7a 0b                        	jp	0x44d334 <.text+0x4c334>
  44d329: 81 e1 00 01 00 00            	andl	$0x100, %ecx            # imm = 0x100
  44d32f: 66 85 c9                     	testw	%cx, %cx
  44d332: 74 3c                        	je	0x44d370 <.text+0x4c370>
  44d334: 8b 52 18                     	movl	0x18(%edx), %edx
  44d337: 52                           	pushl	%edx
  44d338: 8b 57 18                     	movl	0x18(%edi), %edx
  44d33b: b8 00 40 00 00               	movl	$0x4000, %eax           # imm = 0x4000
  44d340: 2b c2                        	subl	%edx, %eax
  44d342: 50                           	pushl	%eax
  44d343: e8 68 b0 fe ff               	calll	0x4383b0 <.text+0x373b0>
  44d348: 83 c4 08                     	addl	$0x8, %esp
  44d34b: 3d 00 30 00 00               	cmpl	$0x3000, %eax           # imm = 0x3000
  44d350: 7d 1e                        	jge	0x44d370 <.text+0x4c370>
  44d352: 3b eb                        	cmpl	%ebx, %ebp
  44d354: c7 44 24 10 03 00 00 00      	movl	$0x3, 0x10(%esp)
  44d35c: 89 5e 40                     	movl	%ebx, 0x40(%esi)
  44d35f: 89 5e 3c                     	movl	%ebx, 0x3c(%esi)
  44d362: 89 5e 38                     	movl	%ebx, 0x38(%esi)
  44d365: 74 09                        	je	0x44d370 <.text+0x4c370>
  44d367: 89 5d 0c                     	movl	%ebx, 0xc(%ebp)
  44d36a: 89 5d 08                     	movl	%ebx, 0x8(%ebp)
  44d36d: 89 5d 04                     	movl	%ebx, 0x4(%ebp)
  44d370: 8a 47 04                     	movb	0x4(%edi), %al
  44d373: 0f bf 0d ec 2d b2 03         	movswl	0x3b22dec, %ecx
  44d37a: a8 02                        	testb	$0x2, %al
  44d37c: 0f bf 05 cc 2d b2 03         	movswl	0x3b22dcc, %eax
  44d383: 0f 84 12 01 00 00            	je	0x44d49b <.text+0x4c49b>
  44d389: c1 e0 08                     	shll	$0x8, %eax
  44d38c: 0b c1                        	orl	%ecx, %eax
  44d38e: 3d 02 09 00 00               	cmpl	$0x902, %eax            # imm = 0x902
  44d393: 74 11                        	je	0x44d3a6 <.text+0x4c3a6>
  44d395: 3d 03 09 00 00               	cmpl	$0x903, %eax            # imm = 0x903
  44d39a: 74 0a                        	je	0x44d3a6 <.text+0x4c3a6>
  44d39c: 56                           	pushl	%esi
  44d39d: 55                           	pushl	%ebp
  44d39e: 57                           	pushl	%edi
  44d39f: e8 5c c1 ff ff               	calll	0x449500 <.text+0x48500>
  44d3a4: eb 08                        	jmp	0x44d3ae <.text+0x4c3ae>
  44d3a6: 56                           	pushl	%esi
  44d3a7: 55                           	pushl	%ebp
  44d3a8: 57                           	pushl	%edi
  44d3a9: e8 62 89 ff ff               	calll	0x445d10 <.text+0x44d10>
  44d3ae: 8b 4c 24 1c                  	movl	0x1c(%esp), %ecx
  44d3b2: 83 c4 0c                     	addl	$0xc, %esp
  44d3b5: 85 c9                        	testl	%ecx, %ecx
  44d3b7: 75 04                        	jne	0x44d3bd <.text+0x4c3bd>
  44d3b9: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  44d3bd: f6 47 04 01                  	testb	$0x1, 0x4(%edi)
  44d3c1: 0f 85 08 01 00 00            	jne	0x44d4cf <.text+0x4c4cf>
  44d3c7: e8 b4 48 33 00               	calll	0x781c80 <.text+0x380c80>
  44d3cc: 33 c9                        	xorl	%ecx, %ecx
  44d3ce: e8 1d a2 33 00               	calll	0x7875f0 <.text+0x3865f0>
  44d3d3: 8b 47 1c                     	movl	0x1c(%edi), %eax
  44d3d6: 85 c0                        	testl	%eax, %eax
  44d3d8: 74 0a                        	je	0x44d3e4 <.text+0x4c3e4>
  44d3da: 0f b7 d0                     	movzwl	%ax, %edx
  44d3dd: 33 c9                        	xorl	%ecx, %ecx
  44d3df: e8 cc 7b 33 00               	calll	0x784fb0 <.text+0x383fb0>
  44d3e4: 8b 47 14                     	movl	0x14(%edi), %eax
  44d3e7: 85 c0                        	testl	%eax, %eax
  44d3e9: 74 0a                        	je	0x44d3f5 <.text+0x4c3f5>
  44d3eb: 0f b7 d0                     	movzwl	%ax, %edx
  44d3ee: 33 c9                        	xorl	%ecx, %ecx
  44d3f0: e8 eb 7a 33 00               	calll	0x784ee0 <.text+0x383ee0>
  44d3f5: 8b 57 18                     	movl	0x18(%edi), %edx
  44d3f8: f7 da                        	negl	%edx
  44d3fa: 74 11                        	je	0x44d40d <.text+0x4c40d>
  44d3fc: 66 8b 47 18                  	movw	0x18(%edi), %ax
  44d400: 66 f7 d8                     	negw	%ax
  44d403: 0f b7 d0                     	movzwl	%ax, %edx
  44d406: 33 c9                        	xorl	%ecx, %ecx
  44d408: e8 13 7a 33 00               	calll	0x784e20 <.text+0x383e20>
  44d40d: 8d 4c 24 24                  	leal	0x24(%esp), %ecx
  44d411: 51                           	pushl	%ecx
  44d412: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
  44d416: 33 c9                        	xorl	%ecx, %ecx
  44d418: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  44d420: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44d428: c7 44 24 20 00 00 80 3f      	movl	$0x3f800000, 0x20(%esp) # imm = 0x3F800000
  44d430: e8 2b 79 33 00               	calll	0x784d60 <.text+0x383d60>
  44d435: 8b 54 24 24                  	movl	0x24(%esp), %edx
  44d439: 8b 44 24 28                  	movl	0x28(%esp), %eax
  44d43d: 8b 4c 24 2c                  	movl	0x2c(%esp), %ecx
  44d441: 83 c6 50                     	addl	$0x50, %esi
  44d444: 89 16                        	movl	%edx, (%esi)
  44d446: 89 46 04                     	movl	%eax, 0x4(%esi)
  44d449: 8d 54 24 24                  	leal	0x24(%esp), %edx
  44d44d: 89 4e 08                     	movl	%ecx, 0x8(%esi)
  44d450: 52                           	pushl	%edx
  44d451: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
  44d455: 33 c9                        	xorl	%ecx, %ecx
  44d457: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  44d45f: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44d467: c7 44 24 20 00 00 00 bf      	movl	$0xbf000000, 0x20(%esp) # imm = 0xBF000000
  44d46f: e8 ec 78 33 00               	calll	0x784d60 <.text+0x383d60>
  44d474: b9 01 00 00 00               	movl	$0x1, %ecx
  44d479: e8 c2 7c 33 00               	calll	0x785140 <.text+0x384140>
  44d47e: d9 44 24 28                  	flds	0x28(%esp)
  44d482: d8 47 24                     	fadds	0x24(%edi)
  44d485: 8b 44 24 10                  	movl	0x10(%esp), %eax
  44d489: d9 5f 24                     	fstps	0x24(%edi)
  44d48c: 5f                           	popl	%edi
  44d48d: 5e                           	popl	%esi
  44d48e: c7 45 08 00 00 00 00         	movl	$0x0, 0x8(%ebp)
  44d495: 5d                           	popl	%ebp
  44d496: 5b                           	popl	%ebx
  44d497: 83 c4 20                     	addl	$0x20, %esp
  44d49a: c3                           	retl
  44d49b: c1 e0 08                     	shll	$0x8, %eax
  44d49e: 0b c1                        	orl	%ecx, %eax
  44d4a0: 3d 02 09 00 00               	cmpl	$0x902, %eax            # imm = 0x902
  44d4a5: 74 11                        	je	0x44d4b8 <.text+0x4c4b8>
  44d4a7: 3d 03 09 00 00               	cmpl	$0x903, %eax            # imm = 0x903
  44d4ac: 74 0a                        	je	0x44d4b8 <.text+0x4c4b8>
  44d4ae: 56                           	pushl	%esi
  44d4af: 55                           	pushl	%ebp
  44d4b0: 57                           	pushl	%edi
  44d4b1: e8 4a c0 ff ff               	calll	0x449500 <.text+0x48500>
  44d4b6: eb 08                        	jmp	0x44d4c0 <.text+0x4c4c0>
  44d4b8: 56                           	pushl	%esi
  44d4b9: 55                           	pushl	%ebp
  44d4ba: 57                           	pushl	%edi
  44d4bb: e8 50 88 ff ff               	calll	0x445d10 <.text+0x44d10>
  44d4c0: 8b 4c 24 1c                  	movl	0x1c(%esp), %ecx
  44d4c4: 83 c4 0c                     	addl	$0xc, %esp
  44d4c7: 85 c9                        	testl	%ecx, %ecx
  44d4c9: 75 04                        	jne	0x44d4cf <.text+0x4c4cf>
  44d4cb: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  44d4cf: 8b 44 24 10                  	movl	0x10(%esp), %eax
  44d4d3: 5f                           	popl	%edi
  44d4d4: 5e                           	popl	%esi
  44d4d5: 5d                           	popl	%ebp
  44d4d6: 5b                           	popl	%ebx
  44d4d7: 83 c4 20                     	addl	$0x20, %esp
  44d4da: c3                           	retl
  44d4db: 90                           	nop
  44d4dc: 90                           	nop
  44d4dd: 90                           	nop
  44d4de: 90                           	nop
  44d4df: 90                           	nop
