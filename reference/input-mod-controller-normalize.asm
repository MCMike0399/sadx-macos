
_reference/sadx/updated-pc-reference/unpacked/input/sadx-input-mod/sadx-input-mod.dll:	file format coff-i386

Disassembly of section .text:

10001000 <.text>:
10004400: 55                           	pushl	%ebp
10004401: 8b ec                        	movl	%esp, %ebp
10004403: 83 e4 f8                     	andl	$-0x8, %esp
10004406: 83 ec 14                     	subl	$0x14, %esp
10004409: 53                           	pushl	%ebx
1000440a: 56                           	pushl	%esi
1000440b: 8b f2                        	movl	%edx, %esi
1000440d: 89 4c 24 08                  	movl	%ecx, 0x8(%esp)
10004411: 0f bf 4d 08                  	movswl	0x8(%ebp), %ecx
10004415: 57                           	pushl	%edi
10004416: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
1000441a: 0f b7 06                     	movzwl	(%esi), %eax
1000441d: 8d 7e 02                     	leal	0x2(%esi), %edi
10004420: 89 44 24 18                  	movl	%eax, 0x18(%esp)
10004424: 98                           	cwtl
10004425: 99                           	cltd
10004426: 8b d8                        	movl	%eax, %ebx
10004428: 89 7c 24 14                  	movl	%edi, 0x14(%esp)
1000442c: 33 da                        	xorl	%edx, %ebx
1000442e: 2b da                        	subl	%edx, %ebx
10004430: 3b d9                        	cmpl	%ecx, %ebx
10004432: 7d 20                        	jge	0x10004454 <.text+0x3454>
10004434: 0f bf 07                     	movswl	(%edi), %eax
10004437: 99                           	cltd
10004438: 33 c2                        	xorl	%edx, %eax
1000443a: 2b c2                        	subl	%edx, %eax
1000443c: 3b c1                        	cmpl	%ecx, %eax
1000443e: 7d 18                        	jge	0x10004458 <.text+0x3458>
10004440: 8b 7c 24 0c                  	movl	0xc(%esp), %edi
10004444: 0f 57 c0                     	xorps	%xmm0, %xmm0
10004447: c7 07 00 00 00 00            	movl	$0x0, (%edi)
1000444d: 5f                           	popl	%edi
1000444e: 5e                           	popl	%esi
1000444f: 5b                           	popl	%ebx
10004450: 8b e5                        	movl	%ebp, %esp
10004452: 5d                           	popl	%ebp
10004453: c3                           	retl
10004454: 89 7c 24 14                  	movl	%edi, 0x14(%esp)
10004458: 8b 74 24 18                  	movl	0x18(%esp), %esi
1000445c: ba 01 80 ff ff               	movl	$0xffff8001, %edx       # imm = 0xFFFF8001
10004461: 66 3b d6                     	cmpw	%si, %dx
10004464: 0f b7 c6                     	movzwl	%si, %eax
10004467: 8b ca                        	movl	%edx, %ecx
10004469: 0f 4c c8                     	cmovll	%eax, %ecx
1000446c: 0f bf f1                     	movswl	%cx, %esi
1000446f: 0f b7 0f                     	movzwl	(%edi), %ecx
10004472: 66 3b d1                     	cmpw	%cx, %dx
10004475: 8b c1                        	movl	%ecx, %eax
10004477: 0f 4c d0                     	cmovll	%eax, %edx
1000447a: 66 0f 6e d6                  	movd	%esi, %xmm2
1000447e: 0f bf fa                     	movswl	%dx, %edi
10004481: 0f 5b d2                     	cvtdq2ps	%xmm2, %xmm2
10004484: 66 0f 6e c7                  	movd	%edi, %xmm0
10004488: 0f 5b c0                     	cvtdq2ps	%xmm0, %xmm0
1000448b: 0f 28 c8                     	movaps	%xmm0, %xmm1
1000448e: f3 0f 59 c8                  	mulss	%xmm0, %xmm1
10004492: 0f 28 c2                     	movaps	%xmm2, %xmm0
10004495: f3 0f 59 c2                  	mulss	%xmm2, %xmm0
10004499: f3 0f 58 c8                  	addss	%xmm0, %xmm1
1000449d: 0f 5a c1                     	cvtps2pd	%xmm1, %xmm0
100044a0: 0f 57 c9                     	xorps	%xmm1, %xmm1
100044a3: 66 0f 2e c8                  	ucomisd	%xmm0, %xmm1
100044a7: 77 06                        	ja	0x100044af <.text+0x34af>
100044a9: f2 0f 51 c0                  	sqrtsd	%xmm0, %xmm0
100044ad: eb 05                        	jmp	0x100044b4 <.text+0x34b4>
100044af: e8 67 64 00 00               	calll	0x1000a91b <OnExit+0x26eb>
100044b4: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
100044b8: 0f 57 c9                     	xorps	%xmm1, %xmm1
100044bb: f2 0f 5a c8                  	cvtsd2ss	%xmm0, %xmm1
100044bf: 66 0f 6e e1                  	movd	%ecx, %xmm4
100044c3: 0f 57 d2                     	xorps	%xmm2, %xmm2
100044c6: 0f 5b e4                     	cvtdq2ps	%xmm4, %xmm4
100044c9: f3 0f 11 4c 24 18            	movss	%xmm1, 0x18(%esp)
100044cf: 0f 2f e1                     	comiss	%xmm1, %xmm4
100044d2: 76 05                        	jbe	0x100044d9 <.text+0x34d9>
100044d4: 0f 57 db                     	xorps	%xmm3, %xmm3
100044d7: eb 0b                        	jmp	0x100044e4 <.text+0x34e4>
100044d9: 66 0f 6e de                  	movd	%esi, %xmm3
100044dd: 0f 5b db                     	cvtdq2ps	%xmm3, %xmm3
100044e0: f3 0f 5e d9                  	divss	%xmm1, %xmm3
100044e4: 77 0b                        	ja	0x100044f1 <.text+0x34f1>
100044e6: 66 0f 6e d7                  	movd	%edi, %xmm2
100044ea: 0f 5b d2                     	cvtdq2ps	%xmm2, %xmm2
100044ed: f3 0f 5e d1                  	divss	%xmm1, %xmm2
100044f1: f3 0f 10 05 b0 bb 00 10      	movss	0x1000bbb0, %xmm0
100044f9: 8d 44 24 18                  	leal	0x18(%esp), %eax
100044fd: 0f 2f c1                     	comiss	%xmm1, %xmm0
10004500: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
10004504: c7 44 24 1c 00 fe ff 46      	movl	$0x46fffe00, 0x1c(%esp) # imm = 0x46FFFE00
1000450c: be 81 ff ff ff               	movl	$0xffffff81, %esi       # imm = 0xFFFFFF81
10004511: 0f 46 c2                     	cmovbel	%edx, %eax
10004514: f3 0f 10 08                  	movss	(%eax), %xmm1
10004518: b8 ff 7f 00 00               	movl	$0x7fff, %eax           # imm = 0x7FFF
1000451d: 2b c1                        	subl	%ecx, %eax
1000451f: f3 0f 5c cc                  	subss	%xmm4, %xmm1
10004523: 80 7d 0c 00                  	cmpb	$0x0, 0xc(%ebp)
10004527: f3 0f 10 25 a4 bb 00 10      	movss	0x1000bba4, %xmm4
1000452f: 66 0f 6e c0                  	movd	%eax, %xmm0
10004533: 0f 5b c0                     	cvtdq2ps	%xmm0, %xmm0
10004536: f3 0f 5e c8                  	divss	%xmm0, %xmm1
1000453a: 75 12                        	jne	0x1000454e <.text+0x354e>
1000453c: 3b d9                        	cmpl	%ecx, %ebx
1000453e: 7d 0e                        	jge	0x1000454e <.text+0x354e>
10004540: 8b 7c 24 0c                  	movl	0xc(%esp), %edi
10004544: 33 c0                        	xorl	%eax, %eax
10004546: 66 89 07                     	movw	%ax, (%edi)
10004549: 8d 58 7f                     	leal	0x7f(%eax), %ebx
1000454c: eb 3e                        	jmp	0x1000458c <.text+0x358c>
1000454e: 8b 7c 24 0c                  	movl	0xc(%esp), %edi
10004552: 0f 28 c1                     	movaps	%xmm1, %xmm0
10004555: f3 0f 59 c3                  	mulss	%xmm3, %xmm0
10004559: 8b ce                        	movl	%esi, %ecx
1000455b: bb 7f 00 00 00               	movl	$0x7f, %ebx
10004560: f3 0f 59 c4                  	mulss	%xmm4, %xmm0
10004564: f3 0f 2c d0                  	cvttss2si	%xmm0, %edx
10004568: 66 3b f2                     	cmpw	%dx, %si
1000456b: 0f b7 c2                     	movzwl	%dx, %eax
1000456e: 0f 4c c8                     	cmovll	%eax, %ecx
10004571: 0f b7 c1                     	movzwl	%cx, %eax
10004574: 66 3b d8                     	cmpw	%ax, %bx
10004577: 8b c8                        	movl	%eax, %ecx
10004579: 0f 4c cb                     	cmovll	%ebx, %ecx
1000457c: 80 7d 0c 00                  	cmpb	$0x0, 0xc(%ebp)
10004580: 0f b7 c1                     	movzwl	%cx, %eax
10004583: 66 89 07                     	movw	%ax, (%edi)
10004586: 75 24                        	jne	0x100045ac <.text+0x35ac>
10004588: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
1000458c: 8b 44 24 14                  	movl	0x14(%esp), %eax
10004590: 0f bf 00                     	movswl	(%eax), %eax
10004593: 99                           	cltd
10004594: 33 c2                        	xorl	%edx, %eax
10004596: 2b c2                        	subl	%edx, %eax
10004598: 3b c1                        	cmpl	%ecx, %eax
1000459a: 7d 10                        	jge	0x100045ac <.text+0x35ac>
1000459c: 33 c0                        	xorl	%eax, %eax
1000459e: 0f 28 c1                     	movaps	%xmm1, %xmm0
100045a1: 66 89 47 02                  	movw	%ax, 0x2(%edi)
100045a5: 5f                           	popl	%edi
100045a6: 5e                           	popl	%esi
100045a7: 5b                           	popl	%ebx
100045a8: 8b e5                        	movl	%ebp, %esp
100045aa: 5d                           	popl	%ebp
100045ab: c3                           	retl
100045ac: 0f 28 c1                     	movaps	%xmm1, %xmm0
100045af: f3 0f 59 c2                  	mulss	%xmm2, %xmm0
100045b3: f3 0f 59 c4                  	mulss	%xmm4, %xmm0
100045b7: f3 0f 2c c8                  	cvttss2si	%xmm0, %ecx
100045bb: 0f 28 c1                     	movaps	%xmm1, %xmm0
100045be: 66 3b f1                     	cmpw	%cx, %si
100045c1: 0f b7 c1                     	movzwl	%cx, %eax
100045c4: 0f 4c f0                     	cmovll	%eax, %esi
100045c7: 0f b7 c6                     	movzwl	%si, %eax
100045ca: 66 3b d8                     	cmpw	%ax, %bx
100045cd: 8b c8                        	movl	%eax, %ecx
100045cf: 0f 4c cb                     	cmovll	%ebx, %ecx
100045d2: 0f b7 c1                     	movzwl	%cx, %eax
100045d5: 66 89 47 02                  	movw	%ax, 0x2(%edi)
100045d9: 5f                           	popl	%edi
100045da: 5e                           	popl	%esi
100045db: 5b                           	popl	%ebx
100045dc: 8b e5                        	movl	%ebp, %esp
100045de: 5d                           	popl	%ebp
100045df: c3                           	retl
