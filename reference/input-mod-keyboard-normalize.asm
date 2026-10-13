
_reference/sadx/updated-pc-reference/unpacked/input/sadx-input-mod/sadx-input-mod.dll:	file format coff-i386

Disassembly of section .text:

10001000 <.text>:
10004e59: 0f b7 f0                     	movzwl	%ax, %esi
10004e5c: bb 01 80 ff ff               	movl	$0xffff8001, %ebx       # imm = 0xFFFF8001
10004e61: 66 3b de                     	cmpw	%si, %bx
10004e64: 8b ce                        	movl	%esi, %ecx
10004e66: 8b d3                        	movl	%ebx, %edx
10004e68: 0f 4c d1                     	cmovll	%ecx, %edx
10004e6b: c1 e8 10                     	shrl	$0x10, %eax
10004e6e: 0f bf f2                     	movswl	%dx, %esi
10004e71: 66 3b d8                     	cmpw	%ax, %bx
10004e74: 8b c8                        	movl	%eax, %ecx
10004e76: 8b d3                        	movl	%ebx, %edx
10004e78: 0f 4c d1                     	cmovll	%ecx, %edx
10004e7b: 0f bf fa                     	movswl	%dx, %edi
10004e7e: 66 0f 6e d6                  	movd	%esi, %xmm2
10004e82: 0f 5b d2                     	cvtdq2ps	%xmm2, %xmm2
10004e85: 66 0f 6e c7                  	movd	%edi, %xmm0
10004e89: 0f 5b c0                     	cvtdq2ps	%xmm0, %xmm0
10004e8c: 0f 28 c8                     	movaps	%xmm0, %xmm1
10004e8f: f3 0f 59 c8                  	mulss	%xmm0, %xmm1
10004e93: 0f 28 c2                     	movaps	%xmm2, %xmm0
10004e96: f3 0f 59 c2                  	mulss	%xmm2, %xmm0
10004e9a: f3 0f 58 c8                  	addss	%xmm0, %xmm1
10004e9e: 0f 5a c1                     	cvtps2pd	%xmm1, %xmm0
10004ea1: 0f 57 c9                     	xorps	%xmm1, %xmm1
10004ea4: 66 0f 2e c8                  	ucomisd	%xmm0, %xmm1
10004ea8: 77 06                        	ja	0x10004eb0 <.text+0x3eb0>
10004eaa: f2 0f 51 c0                  	sqrtsd	%xmm0, %xmm0
10004eae: eb 05                        	jmp	0x10004eb5 <.text+0x3eb5>
10004eb0: e8 66 5a 00 00               	calll	0x1000a91b <OnExit+0x26eb>
10004eb5: 0f 57 d2                     	xorps	%xmm2, %xmm2
10004eb8: f2 0f 5a d0                  	cvtsd2ss	%xmm0, %xmm2
10004ebc: f3 0f 10 05 6c bb 00 10      	movss	0x1000bb6c, %xmm0
10004ec4: 0f 2f c2                     	comiss	%xmm2, %xmm0
10004ec7: 0f 57 c0                     	xorps	%xmm0, %xmm0
10004eca: 76 05                        	jbe	0x10004ed1 <.text+0x3ed1>
10004ecc: 0f 57 c9                     	xorps	%xmm1, %xmm1
10004ecf: eb 16                        	jmp	0x10004ee7 <.text+0x3ee7>
10004ed1: 66 0f 6e ce                  	movd	%esi, %xmm1
10004ed5: 66 0f 6e c7                  	movd	%edi, %xmm0
10004ed9: 0f 5b c9                     	cvtdq2ps	%xmm1, %xmm1
10004edc: 0f 5b c0                     	cvtdq2ps	%xmm0, %xmm0
10004edf: f3 0f 5e ca                  	divss	%xmm2, %xmm1
10004ee3: f3 0f 5e c2                  	divss	%xmm2, %xmm0
10004ee7: f3 0f 5e 15 b0 bb 00 10      	divss	0x1000bbb0, %xmm2
10004eef: 8b cb                        	movl	%ebx, %ecx
10004ef1: 0f b7 15 38 e9 00 10         	movzwl	0x1000e938, %edx
10004ef8: 66 3b da                     	cmpw	%dx, %bx
10004efb: f3 0f 59 05 a0 bb 00 10      	mulss	0x1000bba0, %xmm0
10004f03: f3 0f 5d 15 7c bb 00 10      	minss	0x1000bb7c, %xmm2
10004f0b: f3 0f 59 0d a0 bb 00 10      	mulss	0x1000bba0, %xmm1
10004f13: f3 0f 2c c0                  	cvttss2si	%xmm0, %eax
10004f17: f3 0f 11 54 24 14            	movss	%xmm2, 0x14(%esp)
10004f1d: 89 44 24 10                  	movl	%eax, 0x10(%esp)
10004f21: 66 a3 0a e9 00 10            	movw	%ax, 0x1000e90a
10004f27: 8b c2                        	movl	%edx, %eax
10004f29: 0f 4c c8                     	cmovll	%eax, %ecx
10004f2c: f3 0f 11 15 e8 e8 00 10      	movss	%xmm2, 0x1000e8e8
10004f34: 0f bf c1                     	movswl	%cx, %eax
10004f37: 0f b7 0d 3a e9 00 10         	movzwl	0x1000e93a, %ecx
10004f3e: 66 3b d9                     	cmpw	%cx, %bx
10004f41: f3 0f 2c f9                  	cvttss2si	%xmm1, %edi
10004f45: 66 0f 6e d0                  	movd	%eax, %xmm2
10004f49: 8b c1                        	movl	%ecx, %eax
10004f4b: 0f 4c d8                     	cmovll	%eax, %ebx
10004f4e: 66 89 3d 08 e9 00 10         	movw	%di, 0x1000e908
10004f55: 0f bf f3                     	movswl	%bx, %esi
10004f58: 0f 5b d2                     	cvtdq2ps	%xmm2, %xmm2
10004f5b: 66 0f 6e c6                  	movd	%esi, %xmm0
10004f5f: 0f 5b c0                     	cvtdq2ps	%xmm0, %xmm0
10004f62: f3 0f 11 54 24 0c            	movss	%xmm2, 0xc(%esp)
10004f68: 0f 28 c8                     	movaps	%xmm0, %xmm1
10004f6b: f3 0f 59 c8                  	mulss	%xmm0, %xmm1
10004f6f: 0f 28 c2                     	movaps	%xmm2, %xmm0
10004f72: f3 0f 59 c2                  	mulss	%xmm2, %xmm0
10004f76: f3 0f 58 c8                  	addss	%xmm0, %xmm1
10004f7a: 0f 5a c1                     	cvtps2pd	%xmm1, %xmm0
10004f7d: 0f 57 c9                     	xorps	%xmm1, %xmm1
10004f80: 66 0f 2e c8                  	ucomisd	%xmm0, %xmm1
10004f84: 77 06                        	ja	0x10004f8c <.text+0x3f8c>
10004f86: f2 0f 51 c0                  	sqrtsd	%xmm0, %xmm0
10004f8a: eb 05                        	jmp	0x10004f91 <.text+0x3f91>
10004f8c: e8 8a 59 00 00               	calll	0x1000a91b <OnExit+0x26eb>
10004f91: 0f 57 c9                     	xorps	%xmm1, %xmm1
10004f94: f2 0f 5a c8                  	cvtsd2ss	%xmm0, %xmm1
10004f98: f3 0f 10 05 6c bb 00 10      	movss	0x1000bb6c, %xmm0
10004fa0: 0f 2f c1                     	comiss	%xmm1, %xmm0
10004fa3: 76 08                        	jbe	0x10004fad <.text+0x3fad>
10004fa5: 0f 57 c0                     	xorps	%xmm0, %xmm0
10004fa8: 0f 57 d2                     	xorps	%xmm2, %xmm2
10004fab: eb 15                        	jmp	0x10004fc2 <.text+0x3fc2>
10004fad: f3 0f 10 44 24 0c            	movss	0xc(%esp), %xmm0
10004fb3: 66 0f 6e d6                  	movd	%esi, %xmm2
10004fb7: 0f 5b d2                     	cvtdq2ps	%xmm2, %xmm2
10004fba: f3 0f 5e c1                  	divss	%xmm1, %xmm0
10004fbe: f3 0f 5e d1                  	divss	%xmm1, %xmm2
10004fc2: 80 3d 26 e9 00 10 00         	cmpb	$0x0, 0x1000e926
10004fc9: f3 0f 5e 0d b0 bb 00 10      	divss	0x1000bbb0, %xmm1
10004fd1: f3 0f 59 05 a0 bb 00 10      	mulss	0x1000bba0, %xmm0
10004fd9: f3 0f 59 15 a0 bb 00 10      	mulss	0x1000bba0, %xmm2
10004fe1: f3 0f 5d 0d 7c bb 00 10      	minss	0x1000bb7c, %xmm1
10004fe9: f3 0f 2c c8                  	cvttss2si	%xmm0, %ecx
10004fed: f3 0f 2c f2                  	cvttss2si	%xmm2, %esi
10004ff1: 66 89 0d 0c e9 00 10         	movw	%cx, 0x1000e90c
10004ff8: f3 0f 11 0d 44 e7 00 10      	movss	%xmm1, 0x1000e744
10005000: 66 89 35 0e e9 00 10         	movw	%si, 0x1000e90e
10005007: 74 60                        	je	0x10005069 <.text+0x4069>
