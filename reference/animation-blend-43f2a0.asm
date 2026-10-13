
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43f2a0: 81 ec e8 01 00 00            	subl	$0x1e8, %esp            # imm = 0x1E8
  43f2a6: 53                           	pushl	%ebx
  43f2a7: 55                           	pushl	%ebp
  43f2a8: 8b ac 24 f4 01 00 00         	movl	0x1f4(%esp), %ebp
  43f2af: 66 8b 45 04                  	movw	0x4(%ebp), %ax
  43f2b3: 8b 55 28                     	movl	0x28(%ebp), %edx
  43f2b6: 56                           	pushl	%esi
  43f2b7: 66 8b 75 06                  	movw	0x6(%ebp), %si
  43f2bb: 57                           	pushl	%edi
  43f2bc: 0f b7 f8                     	movzwl	%ax, %edi
  43f2bf: 8b 45 20                     	movl	0x20(%ebp), %eax
  43f2c2: c1 e7 04                     	shll	$0x4, %edi
  43f2c5: 8b 0c 07                     	movl	(%edi,%eax), %ecx
  43f2c8: 8b 01                        	movl	(%ecx), %eax
  43f2ca: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43f2ce: 51                           	pushl	%ecx
  43f2cf: 50                           	pushl	%eax
  43f2d0: 89 7c 24 3c                  	movl	%edi, 0x3c(%esp)
  43f2d4: 89 02                        	movl	%eax, (%edx)
  43f2d6: e8 e5 ac ff ff               	calll	0x439fc0 <.text+0x38fc0>
  43f2db: 8b 45 20                     	movl	0x20(%ebp), %eax
  43f2de: 8b 14 38                     	movl	(%eax,%edi), %edx
  43f2e1: 8b 4a 04                     	movl	0x4(%edx), %ecx
  43f2e4: 0f b7 d6                     	movzwl	%si, %edx
  43f2e7: 0f b6 71 08                  	movzbl	0x8(%ecx), %esi
  43f2eb: 8b 09                        	movl	(%ecx), %ecx
  43f2ed: c1 e2 04                     	shll	$0x4, %edx
  43f2f0: 8b 14 02                     	movl	(%edx,%eax), %edx
  43f2f3: 8b 52 04                     	movl	0x4(%edx), %edx
  43f2f6: 0f b6 5a 08                  	movzbl	0x8(%edx), %ebx
  43f2fa: 89 54 24 38                  	movl	%edx, 0x38(%esp)
  43f2fe: 8b 12                        	movl	(%edx), %edx
  43f300: 89 4c 24 24                  	movl	%ecx, 0x24(%esp)
  43f304: 8b 4d 28                     	movl	0x28(%ebp), %ecx
  43f307: 83 e6 04                     	andl	$0x4, %esi
  43f30a: 83 e3 04                     	andl	$0x4, %ebx
  43f30d: 89 54 24 20                  	movl	%edx, 0x20(%esp)
  43f311: 8b 51 04                     	movl	0x4(%ecx), %edx
  43f314: 8b 3a                        	movl	(%edx), %edi
  43f316: 8b 54 24 3c                  	movl	0x3c(%esp), %edx
  43f31a: 83 ce 08                     	orl	$0x8, %esi
  43f31d: 83 cb 08                     	orl	$0x8, %ebx
  43f320: 33 c9                        	xorl	%ecx, %ecx
  43f322: c1 ee 02                     	shrl	$0x2, %esi
  43f325: c1 eb 02                     	shrl	$0x2, %ebx
  43f328: 83 c4 08                     	addl	$0x8, %esp
  43f32b: 38 4c 10 04                  	cmpb	%cl, 0x4(%eax,%edx)
  43f32f: 89 74 24 28                  	movl	%esi, 0x28(%esp)
  43f333: 89 5c 24 2c                  	movl	%ebx, 0x2c(%esp)
  43f337: 89 7c 24 20                  	movl	%edi, 0x20(%esp)
  43f33b: 89 4c 24 38                  	movl	%ecx, 0x38(%esp)
  43f33f: 0f 86 51 06 00 00            	jbe	0x43f996 <.text+0x3e996>
  43f345: eb 09                        	jmp	0x43f350 <.text+0x3e350>
  43f347: 8d a4 24 00 00 00 00         	leal	(%esp), %esp
  43f34e: 8b ff                        	movl	%edi, %edi
  43f350: 8b 44 8c 3c                  	movl	0x3c(%esp,%ecx,4), %eax
  43f354: 8b 4c 24 1c                  	movl	0x1c(%esp), %ecx
  43f358: 89 44 24 24                  	movl	%eax, 0x24(%esp)
  43f35c: 83 39 00                     	cmpl	$0x0, (%ecx)
  43f35f: 75 0d                        	jne	0x43f36e <.text+0x3e36e>
  43f361: 8b 54 24 18                  	movl	0x18(%esp), %edx
  43f365: 83 3a 00                     	cmpl	$0x0, (%edx)
  43f368: 0f 84 88 01 00 00            	je	0x43f4f6 <.text+0x3e4f6>
  43f36e: 6a 20                        	pushl	$0x20
  43f370: e8 ab be fc ff               	calll	0x40b220 <.text+0xa220>
  43f375: 8b c8                        	movl	%eax, %ecx
  43f377: 83 c4 04                     	addl	$0x4, %esp
  43f37a: 85 c9                        	testl	%ecx, %ecx
  43f37c: 0f 84 74 01 00 00            	je	0x43f4f6 <.text+0x3e4f6>
  43f382: 8b 44 24 18                  	movl	0x18(%esp), %eax
  43f386: 89 0f                        	movl	%ecx, (%edi)
  43f388: c7 01 00 00 00 00            	movl	$0x0, (%ecx)
  43f38e: 8b 18                        	movl	(%eax), %ebx
  43f390: 85 db                        	testl	%ebx, %ebx
  43f392: 0f 84 27 01 00 00            	je	0x43f4bf <.text+0x3e4bf>
  43f398: db 03                        	fildl	(%ebx)
  43f39a: 8b d0                        	movl	%eax, %edx
  43f39c: 8b 7a 08                     	movl	0x8(%edx), %edi
  43f39f: 8b 03                        	movl	(%ebx), %eax
  43f3a1: 33 f6                        	xorl	%esi, %esi
  43f3a3: 33 d2                        	xorl	%edx, %edx
  43f3a5: 85 c0                        	testl	%eax, %eax
  43f3a7: 7d 06                        	jge	0x43f3af <.text+0x3e3af>
  43f3a9: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f3af: d8 5d 10                     	fcomps	0x10(%ebp)
  43f3b2: df e0                        	fnstsw	%ax
  43f3b4: f6 c4 05                     	testb	$0x5, %ah
  43f3b7: 7a 37                        	jp	0x43f3f0 <.text+0x3e3f0>
  43f3b9: 8d 77 ff                     	leal	-0x1(%edi), %esi
  43f3bc: 89 74 24 14                  	movl	%esi, 0x14(%esp)
  43f3c0: 8b fb                        	movl	%ebx, %edi
  43f3c2: eb 04                        	jmp	0x43f3c8 <.text+0x3e3c8>
  43f3c4: 8b 74 24 14                  	movl	0x14(%esp), %esi
  43f3c8: 3b d6                        	cmpl	%esi, %edx
  43f3ca: 73 22                        	jae	0x43f3ee <.text+0x3e3ee>
  43f3cc: 8b 47 10                     	movl	0x10(%edi), %eax
  43f3cf: db 47 10                     	fildl	0x10(%edi)
  43f3d2: 83 c7 10                     	addl	$0x10, %edi
  43f3d5: 8b f2                        	movl	%edx, %esi
  43f3d7: 42                           	incl	%edx
  43f3d8: 85 c0                        	testl	%eax, %eax
  43f3da: 7d 06                        	jge	0x43f3e2 <.text+0x3e3e2>
  43f3dc: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f3e2: d8 5d 10                     	fcomps	0x10(%ebp)
  43f3e5: df e0                        	fnstsw	%ax
  43f3e7: f6 c4 05                     	testb	$0x5, %ah
  43f3ea: 7b d8                        	jnp	0x43f3c4 <.text+0x3e3c4>
  43f3ec: eb 02                        	jmp	0x43f3f0 <.text+0x3e3f0>
  43f3ee: 33 d2                        	xorl	%edx, %edx
  43f3f0: 8b c6                        	movl	%esi, %eax
  43f3f2: c1 e0 04                     	shll	$0x4, %eax
  43f3f5: 8b 7c 18 04                  	movl	0x4(%eax,%ebx), %edi
  43f3f9: 03 c3                        	addl	%ebx, %eax
  43f3fb: 3b d6                        	cmpl	%esi, %edx
  43f3fd: 89 79 04                     	movl	%edi, 0x4(%ecx)
  43f400: 8b 78 08                     	movl	0x8(%eax), %edi
  43f403: 89 79 08                     	movl	%edi, 0x8(%ecx)
  43f406: 8b 78 0c                     	movl	0xc(%eax), %edi
  43f409: 89 79 0c                     	movl	%edi, 0xc(%ecx)
  43f40c: 74 75                        	je	0x43f483 <.text+0x3e483>
  43f40e: 8b fa                        	movl	%edx, %edi
  43f410: c1 e7 04                     	shll	$0x4, %edi
  43f413: 03 fb                        	addl	%ebx, %edi
  43f415: 8b 18                        	movl	(%eax), %ebx
  43f417: 89 7c 24 14                  	movl	%edi, 0x14(%esp)
  43f41b: 8b 3f                        	movl	(%edi), %edi
  43f41d: 2b fb                        	subl	%ebx, %edi
  43f41f: 3b d6                        	cmpl	%esi, %edx
  43f421: 73 07                        	jae	0x43f42a <.text+0x3e42a>
  43f423: 8b 54 24 30                  	movl	0x30(%esp), %edx
  43f427: 03 7a 04                     	addl	0x4(%edx), %edi
  43f42a: 85 db                        	testl	%ebx, %ebx
  43f42c: 89 5c 24 10                  	movl	%ebx, 0x10(%esp)
  43f430: db 44 24 10                  	fildl	0x10(%esp)
  43f434: 7d 06                        	jge	0x43f43c <.text+0x3e43c>
  43f436: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f43c: 85 ff                        	testl	%edi, %edi
  43f43e: d8 6d 10                     	fsubrs	0x10(%ebp)
  43f441: 89 7c 24 10                  	movl	%edi, 0x10(%esp)
  43f445: db 44 24 10                  	fildl	0x10(%esp)
  43f449: 7d 06                        	jge	0x43f451 <.text+0x3e451>
  43f44b: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f451: de f9                        	fdivrp	%st, %st(1)
  43f453: 8b 54 24 14                  	movl	0x14(%esp), %edx
  43f457: d9 42 04                     	flds	0x4(%edx)
  43f45a: d8 60 04                     	fsubs	0x4(%eax)
  43f45d: d8 c9                        	fmul	%st(1), %st
  43f45f: d8 41 04                     	fadds	0x4(%ecx)
  43f462: d9 59 04                     	fstps	0x4(%ecx)
  43f465: d9 42 08                     	flds	0x8(%edx)
  43f468: d8 60 08                     	fsubs	0x8(%eax)
  43f46b: d8 c9                        	fmul	%st(1), %st
  43f46d: d8 41 08                     	fadds	0x8(%ecx)
  43f470: d9 59 08                     	fstps	0x8(%ecx)
  43f473: d9 42 0c                     	flds	0xc(%edx)
  43f476: d8 60 0c                     	fsubs	0xc(%eax)
  43f479: d8 c9                        	fmul	%st(1), %st
  43f47b: d8 41 0c                     	fadds	0xc(%ecx)
  43f47e: d9 59 0c                     	fstps	0xc(%ecx)
  43f481: dd d8                        	fstp	%st(0)
  43f483: 8b 7c 24 20                  	movl	0x20(%esp), %edi
  43f487: 8b 74 24 28                  	movl	0x28(%esp), %esi
  43f48b: 8b 54 24 24                  	movl	0x24(%esp), %edx
  43f48f: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43f493: c7 41 10 01 00 00 00         	movl	$0x1, 0x10(%ecx)
  43f49a: 8b 00                        	movl	(%eax), %eax
  43f49c: 85 c0                        	testl	%eax, %eax
  43f49e: 74 37                        	je	0x43f4d7 <.text+0x3e4d7>
  43f4a0: 8b 50 04                     	movl	0x4(%eax), %edx
  43f4a3: 8b 5c 24 2c                  	movl	0x2c(%esp), %ebx
  43f4a7: 89 51 14                     	movl	%edx, 0x14(%ecx)
  43f4aa: 8b 50 08                     	movl	0x8(%eax), %edx
  43f4ad: 89 51 18                     	movl	%edx, 0x18(%ecx)
  43f4b0: 8b 40 0c                     	movl	0xc(%eax), %eax
  43f4b3: 89 41 1c                     	movl	%eax, 0x1c(%ecx)
  43f4b6: c7 47 08 02 00 00 00         	movl	$0x2, 0x8(%edi)
  43f4bd: eb 44                        	jmp	0x43f503 <.text+0x3e503>
  43f4bf: 8b 54 24 24                  	movl	0x24(%esp), %edx
  43f4c3: 8b 42 08                     	movl	0x8(%edx), %eax
  43f4c6: 89 41 04                     	movl	%eax, 0x4(%ecx)
  43f4c9: 8b 42 0c                     	movl	0xc(%edx), %eax
  43f4cc: 89 41 08                     	movl	%eax, 0x8(%ecx)
  43f4cf: 8b 42 10                     	movl	0x10(%edx), %eax
  43f4d2: 89 41 0c                     	movl	%eax, 0xc(%ecx)
  43f4d5: eb b8                        	jmp	0x43f48f <.text+0x3e48f>
  43f4d7: 8b 42 08                     	movl	0x8(%edx), %eax
  43f4da: 8b 5c 24 2c                  	movl	0x2c(%esp), %ebx
  43f4de: 89 41 14                     	movl	%eax, 0x14(%ecx)
  43f4e1: 8b 42 0c                     	movl	0xc(%edx), %eax
  43f4e4: 89 41 18                     	movl	%eax, 0x18(%ecx)
  43f4e7: 8b 52 10                     	movl	0x10(%edx), %edx
  43f4ea: 89 51 1c                     	movl	%edx, 0x1c(%ecx)
  43f4ed: c7 47 08 02 00 00 00         	movl	$0x2, 0x8(%edi)
  43f4f4: eb 0d                        	jmp	0x43f503 <.text+0x3e503>
  43f4f6: c7 07 00 00 00 00            	movl	$0x0, (%edi)
  43f4fc: c7 47 08 00 00 00 00         	movl	$0x0, 0x8(%edi)
  43f503: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43f507: 8b 48 04                     	movl	0x4(%eax), %ecx
  43f50a: 85 c9                        	testl	%ecx, %ecx
  43f50c: 75 0f                        	jne	0x43f51d <.text+0x3e51d>
  43f50e: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  43f512: 8b 41 04                     	movl	0x4(%ecx), %eax
  43f515: 85 c0                        	testl	%eax, %eax
  43f517: 0f 84 1f 02 00 00            	je	0x43f73c <.text+0x3e73c>
  43f51d: 6a 20                        	pushl	$0x20
  43f51f: e8 fc bc fc ff               	calll	0x40b220 <.text+0xa220>
  43f524: 8b f0                        	movl	%eax, %esi
  43f526: 83 c4 04                     	addl	$0x4, %esp
  43f529: 85 f6                        	testl	%esi, %esi
  43f52b: 0f 84 03 02 00 00            	je	0x43f734 <.text+0x3e734>
  43f531: 8b 44 24 18                  	movl	0x18(%esp), %eax
  43f535: 89 77 04                     	movl	%esi, 0x4(%edi)
  43f538: c7 06 00 00 00 00            	movl	$0x0, (%esi)
  43f53e: 8b 58 04                     	movl	0x4(%eax), %ebx
  43f541: 85 db                        	testl	%ebx, %ebx
  43f543: 0f 84 59 01 00 00            	je	0x43f6a2 <.text+0x3e6a2>
  43f549: 8b 78 0c                     	movl	0xc(%eax), %edi
  43f54c: db 03                        	fildl	(%ebx)
  43f54e: 8b 03                        	movl	(%ebx), %eax
  43f550: 33 d2                        	xorl	%edx, %edx
  43f552: 33 c9                        	xorl	%ecx, %ecx
  43f554: 85 c0                        	testl	%eax, %eax
  43f556: 7d 06                        	jge	0x43f55e <.text+0x3e55e>
  43f558: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f55e: d8 5d 10                     	fcomps	0x10(%ebp)
  43f561: df e0                        	fnstsw	%ax
  43f563: f6 c4 05                     	testb	$0x5, %ah
  43f566: 7a 37                        	jp	0x43f59f <.text+0x3e59f>
  43f568: 8d 57 ff                     	leal	-0x1(%edi), %edx
  43f56b: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  43f56f: 8b fb                        	movl	%ebx, %edi
  43f571: eb 04                        	jmp	0x43f577 <.text+0x3e577>
  43f573: 8b 54 24 14                  	movl	0x14(%esp), %edx
  43f577: 3b ca                        	cmpl	%edx, %ecx
  43f579: 73 22                        	jae	0x43f59d <.text+0x3e59d>
  43f57b: 8b 47 10                     	movl	0x10(%edi), %eax
  43f57e: db 47 10                     	fildl	0x10(%edi)
  43f581: 83 c7 10                     	addl	$0x10, %edi
  43f584: 8b d1                        	movl	%ecx, %edx
  43f586: 41                           	incl	%ecx
  43f587: 85 c0                        	testl	%eax, %eax
  43f589: 7d 06                        	jge	0x43f591 <.text+0x3e591>
  43f58b: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f591: d8 5d 10                     	fcomps	0x10(%ebp)
  43f594: df e0                        	fnstsw	%ax
  43f596: f6 c4 05                     	testb	$0x5, %ah
  43f599: 7b d8                        	jnp	0x43f573 <.text+0x3e573>
  43f59b: eb 02                        	jmp	0x43f59f <.text+0x3e59f>
  43f59d: 33 c9                        	xorl	%ecx, %ecx
  43f59f: 8b c2                        	movl	%edx, %eax
  43f5a1: c1 e0 04                     	shll	$0x4, %eax
  43f5a4: 3b ca                        	cmpl	%edx, %ecx
  43f5a6: 8d 3c 18                     	leal	(%eax,%ebx), %edi
  43f5a9: 8b 47 04                     	movl	0x4(%edi), %eax
  43f5ac: 89 46 04                     	movl	%eax, 0x4(%esi)
  43f5af: 8b 47 08                     	movl	0x8(%edi), %eax
  43f5b2: 89 46 08                     	movl	%eax, 0x8(%esi)
  43f5b5: 8b 47 0c                     	movl	0xc(%edi), %eax
  43f5b8: 89 46 0c                     	movl	%eax, 0xc(%esi)
  43f5bb: 0f 84 b3 00 00 00            	je	0x43f674 <.text+0x3e674>
  43f5c1: 8b c1                        	movl	%ecx, %eax
  43f5c3: c1 e0 04                     	shll	$0x4, %eax
  43f5c6: 03 c3                        	addl	%ebx, %eax
  43f5c8: 8b 1f                        	movl	(%edi), %ebx
  43f5ca: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  43f5ce: 8b 00                        	movl	(%eax), %eax
  43f5d0: 2b c3                        	subl	%ebx, %eax
  43f5d2: 3b ca                        	cmpl	%edx, %ecx
  43f5d4: 73 07                        	jae	0x43f5dd <.text+0x3e5dd>
  43f5d6: 8b 4c 24 30                  	movl	0x30(%esp), %ecx
  43f5da: 03 41 04                     	addl	0x4(%ecx), %eax
  43f5dd: 85 db                        	testl	%ebx, %ebx
  43f5df: 89 5c 24 14                  	movl	%ebx, 0x14(%esp)
  43f5e3: db 44 24 14                  	fildl	0x14(%esp)
  43f5e7: 7d 06                        	jge	0x43f5ef <.text+0x3e5ef>
  43f5e9: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f5ef: 85 c0                        	testl	%eax, %eax
  43f5f1: d8 6d 10                     	fsubrs	0x10(%ebp)
  43f5f4: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  43f5f8: db 44 24 14                  	fildl	0x14(%esp)
  43f5fc: 7d 06                        	jge	0x43f604 <.text+0x3e604>
  43f5fe: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f604: de f9                        	fdivrp	%st, %st(1)
  43f606: 8b 5c 24 10                  	movl	0x10(%esp), %ebx
  43f60a: 8b 53 04                     	movl	0x4(%ebx), %edx
  43f60d: 8b 47 04                     	movl	0x4(%edi), %eax
  43f610: 52                           	pushl	%edx
  43f611: 50                           	pushl	%eax
  43f612: d9 5c 24 1c                  	fstps	0x1c(%esp)
  43f616: e8 85 8d ff ff               	calll	0x4383a0 <.text+0x373a0>
  43f61b: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  43f61f: db 44 24 18                  	fildl	0x18(%esp)
  43f623: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  43f627: e8 14 4d 20 00               	calll	0x644340 <.text+0x243340>
  43f62c: 01 46 04                     	addl	%eax, 0x4(%esi)
  43f62f: 8b 4b 08                     	movl	0x8(%ebx), %ecx
  43f632: 8b 57 08                     	movl	0x8(%edi), %edx
  43f635: 51                           	pushl	%ecx
  43f636: 52                           	pushl	%edx
  43f637: e8 64 8d ff ff               	calll	0x4383a0 <.text+0x373a0>
  43f63c: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  43f640: db 44 24 20                  	fildl	0x20(%esp)
  43f644: d8 4c 24 24                  	fmuls	0x24(%esp)
  43f648: e8 f3 4c 20 00               	calll	0x644340 <.text+0x243340>
  43f64d: 01 46 08                     	addl	%eax, 0x8(%esi)
  43f650: 8b 43 0c                     	movl	0xc(%ebx), %eax
  43f653: 8b 4f 0c                     	movl	0xc(%edi), %ecx
  43f656: 50                           	pushl	%eax
  43f657: 51                           	pushl	%ecx
  43f658: e8 43 8d ff ff               	calll	0x4383a0 <.text+0x373a0>
  43f65d: 89 44 24 28                  	movl	%eax, 0x28(%esp)
  43f661: db 44 24 28                  	fildl	0x28(%esp)
  43f665: 83 c4 18                     	addl	$0x18, %esp
  43f668: d8 4c 24 14                  	fmuls	0x14(%esp)
  43f66c: e8 cf 4c 20 00               	calll	0x644340 <.text+0x243340>
  43f671: 01 46 0c                     	addl	%eax, 0xc(%esi)
  43f674: 8b 7c 24 20                  	movl	0x20(%esp), %edi
  43f678: 8b 4c 24 24                  	movl	0x24(%esp), %ecx
  43f67c: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43f680: c7 46 10 01 00 00 00         	movl	$0x1, 0x10(%esi)
  43f687: 8b 40 04                     	movl	0x4(%eax), %eax
  43f68a: 85 c0                        	testl	%eax, %eax
  43f68c: 74 2c                        	je	0x43f6ba <.text+0x3e6ba>
  43f68e: 8b 48 04                     	movl	0x4(%eax), %ecx
  43f691: 89 4e 14                     	movl	%ecx, 0x14(%esi)
  43f694: 8b 50 08                     	movl	0x8(%eax), %edx
  43f697: 89 56 18                     	movl	%edx, 0x18(%esi)
  43f69a: 8b 40 0c                     	movl	0xc(%eax), %eax
  43f69d: 89 46 1c                     	movl	%eax, 0x1c(%esi)
  43f6a0: eb 2a                        	jmp	0x43f6cc <.text+0x3e6cc>
  43f6a2: 8b 4c 24 24                  	movl	0x24(%esp), %ecx
  43f6a6: 8b 51 14                     	movl	0x14(%ecx), %edx
  43f6a9: 89 56 04                     	movl	%edx, 0x4(%esi)
  43f6ac: 8b 41 18                     	movl	0x18(%ecx), %eax
  43f6af: 89 46 08                     	movl	%eax, 0x8(%esi)
  43f6b2: 8b 51 1c                     	movl	0x1c(%ecx), %edx
  43f6b5: 89 56 0c                     	movl	%edx, 0xc(%esi)
  43f6b8: eb c2                        	jmp	0x43f67c <.text+0x3e67c>
  43f6ba: 8b 51 14                     	movl	0x14(%ecx), %edx
  43f6bd: 89 56 14                     	movl	%edx, 0x14(%esi)
  43f6c0: 8b 41 18                     	movl	0x18(%ecx), %eax
  43f6c3: 89 46 18                     	movl	%eax, 0x18(%esi)
  43f6c6: 8b 49 1c                     	movl	0x1c(%ecx), %ecx
  43f6c9: 89 4e 1c                     	movl	%ecx, 0x1c(%esi)
  43f6cc: 0f bf 56 08                  	movswl	0x8(%esi), %edx
  43f6d0: 0f bf 4e 0c                  	movswl	0xc(%esi), %ecx
  43f6d4: 0f bf 46 04                  	movswl	0x4(%esi), %eax
  43f6d8: 89 56 08                     	movl	%edx, 0x8(%esi)
  43f6db: 0f bf 56 18                  	movswl	0x18(%esi), %edx
  43f6df: 89 4e 0c                     	movl	%ecx, 0xc(%esi)
  43f6e2: 0f bf 4e 14                  	movswl	0x14(%esi), %ecx
  43f6e6: 89 56 18                     	movl	%edx, 0x18(%esi)
  43f6e9: 0f bf 56 1c                  	movswl	0x1c(%esi), %edx
  43f6ed: 51                           	pushl	%ecx
  43f6ee: 50                           	pushl	%eax
  43f6ef: 89 46 04                     	movl	%eax, 0x4(%esi)
  43f6f2: 89 4e 14                     	movl	%ecx, 0x14(%esi)
  43f6f5: 89 56 1c                     	movl	%edx, 0x1c(%esi)
  43f6f8: e8 d3 8c ff ff               	calll	0x4383d0 <.text+0x373d0>
  43f6fd: 8b 4e 08                     	movl	0x8(%esi), %ecx
  43f700: 89 46 14                     	movl	%eax, 0x14(%esi)
  43f703: 8b 46 18                     	movl	0x18(%esi), %eax
  43f706: 50                           	pushl	%eax
  43f707: 51                           	pushl	%ecx
  43f708: e8 c3 8c ff ff               	calll	0x4383d0 <.text+0x373d0>
  43f70d: 8b 56 1c                     	movl	0x1c(%esi), %edx
  43f710: 89 46 18                     	movl	%eax, 0x18(%esi)
  43f713: 8b 46 0c                     	movl	0xc(%esi), %eax
  43f716: 52                           	pushl	%edx
  43f717: 50                           	pushl	%eax
  43f718: e8 b3 8c ff ff               	calll	0x4383d0 <.text+0x373d0>
  43f71d: 8b 5c 24 44                  	movl	0x44(%esp), %ebx
  43f721: 89 46 1c                     	movl	%eax, 0x1c(%esi)
  43f724: 8b 74 24 40                  	movl	0x40(%esp), %esi
  43f728: 83 c4 18                     	addl	$0x18, %esp
  43f72b: c7 47 0c 02 00 00 00         	movl	$0x2, 0xc(%edi)
  43f732: eb 10                        	jmp	0x43f744 <.text+0x3e744>
  43f734: 8b 74 24 28                  	movl	0x28(%esp), %esi
  43f738: 8b 5c 24 2c                  	movl	0x2c(%esp), %ebx
  43f73c: 33 c0                        	xorl	%eax, %eax
  43f73e: 89 47 04                     	movl	%eax, 0x4(%edi)
  43f741: 89 47 0c                     	movl	%eax, 0xc(%edi)
  43f744: 66 83 fb 03                  	cmpw	$0x3, %bx
  43f748: 74 0a                        	je	0x43f754 <.text+0x3e754>
  43f74a: 66 83 fe 03                  	cmpw	$0x3, %si
  43f74e: 0f 85 e7 01 00 00            	jne	0x43f93b <.text+0x3e93b>
  43f754: 66 83 fe 03                  	cmpw	$0x3, %si
  43f758: 8b 4f 0c                     	movl	0xc(%edi), %ecx
  43f75b: 8b 57 08                     	movl	0x8(%edi), %edx
  43f75e: 89 4f 10                     	movl	%ecx, 0x10(%edi)
  43f761: 89 57 0c                     	movl	%edx, 0xc(%edi)
  43f764: 75 0b                        	jne	0x43f771 <.text+0x3e771>
  43f766: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43f76a: 8b 48 08                     	movl	0x8(%eax), %ecx
  43f76d: 85 c9                        	testl	%ecx, %ecx
  43f76f: 75 19                        	jne	0x43f78a <.text+0x3e78a>
  43f771: 66 83 fb 03                  	cmpw	$0x3, %bx
  43f775: 0f 85 b2 01 00 00            	jne	0x43f92d <.text+0x3e92d>
  43f77b: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  43f77f: 8b 41 08                     	movl	0x8(%ecx), %eax
  43f782: 85 c0                        	testl	%eax, %eax
  43f784: 0f 84 a3 01 00 00            	je	0x43f92d <.text+0x3e92d>
  43f78a: 6a 20                        	pushl	$0x20
  43f78c: e8 8f ba fc ff               	calll	0x40b220 <.text+0xa220>
  43f791: 8b c8                        	movl	%eax, %ecx
  43f793: 83 c4 04                     	addl	$0x4, %esp
  43f796: 85 c9                        	testl	%ecx, %ecx
  43f798: 0f 84 8f 01 00 00            	je	0x43f92d <.text+0x3e92d>
  43f79e: 66 83 fb 03                  	cmpw	$0x3, %bx
  43f7a2: 89 4f 08                     	movl	%ecx, 0x8(%edi)
  43f7a5: c7 01 00 00 00 00            	movl	$0x0, (%ecx)
  43f7ab: 0f 85 49 01 00 00            	jne	0x43f8fa <.text+0x3e8fa>
  43f7b1: 8b 54 24 18                  	movl	0x18(%esp), %edx
  43f7b5: 8b 7a 08                     	movl	0x8(%edx), %edi
  43f7b8: 85 ff                        	testl	%edi, %edi
  43f7ba: 0f 84 2e 01 00 00            	je	0x43f8ee <.text+0x3e8ee>
  43f7c0: db 07                        	fildl	(%edi)
  43f7c2: 8b c2                        	movl	%edx, %eax
  43f7c4: 8b 58 14                     	movl	0x14(%eax), %ebx
  43f7c7: 8b 07                        	movl	(%edi), %eax
  43f7c9: 33 f6                        	xorl	%esi, %esi
  43f7cb: 33 d2                        	xorl	%edx, %edx
  43f7cd: 85 c0                        	testl	%eax, %eax
  43f7cf: 7d 06                        	jge	0x43f7d7 <.text+0x3e7d7>
  43f7d1: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f7d7: d8 5d 10                     	fcomps	0x10(%ebp)
  43f7da: df e0                        	fnstsw	%ax
  43f7dc: f6 c4 05                     	testb	$0x5, %ah
  43f7df: 7a 37                        	jp	0x43f818 <.text+0x3e818>
  43f7e1: 8d 73 ff                     	leal	-0x1(%ebx), %esi
  43f7e4: 89 74 24 14                  	movl	%esi, 0x14(%esp)
  43f7e8: 8b df                        	movl	%edi, %ebx
  43f7ea: eb 04                        	jmp	0x43f7f0 <.text+0x3e7f0>
  43f7ec: 8b 74 24 14                  	movl	0x14(%esp), %esi
  43f7f0: 3b d6                        	cmpl	%esi, %edx
  43f7f2: 73 22                        	jae	0x43f816 <.text+0x3e816>
  43f7f4: 8b 43 10                     	movl	0x10(%ebx), %eax
  43f7f7: db 43 10                     	fildl	0x10(%ebx)
  43f7fa: 83 c3 10                     	addl	$0x10, %ebx
  43f7fd: 8b f2                        	movl	%edx, %esi
  43f7ff: 42                           	incl	%edx
  43f800: 85 c0                        	testl	%eax, %eax
  43f802: 7d 06                        	jge	0x43f80a <.text+0x3e80a>
  43f804: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f80a: d8 5d 10                     	fcomps	0x10(%ebp)
  43f80d: df e0                        	fnstsw	%ax
  43f80f: f6 c4 05                     	testb	$0x5, %ah
  43f812: 7b d8                        	jnp	0x43f7ec <.text+0x3e7ec>
  43f814: eb 02                        	jmp	0x43f818 <.text+0x3e818>
  43f816: 33 d2                        	xorl	%edx, %edx
  43f818: 8b c6                        	movl	%esi, %eax
  43f81a: c1 e0 04                     	shll	$0x4, %eax
  43f81d: 8b 5c 38 04                  	movl	0x4(%eax,%edi), %ebx
  43f821: 03 c7                        	addl	%edi, %eax
  43f823: 3b d6                        	cmpl	%esi, %edx
  43f825: 89 59 04                     	movl	%ebx, 0x4(%ecx)
  43f828: 8b 58 08                     	movl	0x8(%eax), %ebx
  43f82b: 89 59 08                     	movl	%ebx, 0x8(%ecx)
  43f82e: 8b 58 0c                     	movl	0xc(%eax), %ebx
  43f831: 89 59 0c                     	movl	%ebx, 0xc(%ecx)
  43f834: 74 75                        	je	0x43f8ab <.text+0x3e8ab>
  43f836: 8b da                        	movl	%edx, %ebx
  43f838: c1 e3 04                     	shll	$0x4, %ebx
  43f83b: 03 fb                        	addl	%ebx, %edi
  43f83d: 8b 18                        	movl	(%eax), %ebx
  43f83f: 89 7c 24 10                  	movl	%edi, 0x10(%esp)
  43f843: 8b 3f                        	movl	(%edi), %edi
  43f845: 2b fb                        	subl	%ebx, %edi
  43f847: 3b d6                        	cmpl	%esi, %edx
  43f849: 73 07                        	jae	0x43f852 <.text+0x3e852>
  43f84b: 8b 54 24 30                  	movl	0x30(%esp), %edx
  43f84f: 03 7a 04                     	addl	0x4(%edx), %edi
  43f852: 85 db                        	testl	%ebx, %ebx
  43f854: 89 5c 24 14                  	movl	%ebx, 0x14(%esp)
  43f858: db 44 24 14                  	fildl	0x14(%esp)
  43f85c: 7d 06                        	jge	0x43f864 <.text+0x3e864>
  43f85e: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f864: 85 ff                        	testl	%edi, %edi
  43f866: d8 6d 10                     	fsubrs	0x10(%ebp)
  43f869: 89 7c 24 14                  	movl	%edi, 0x14(%esp)
  43f86d: db 44 24 14                  	fildl	0x14(%esp)
  43f871: 7d 06                        	jge	0x43f879 <.text+0x3e879>
  43f873: d8 05 7c cb 7d 00            	fadds	0x7dcb7c
  43f879: de f9                        	fdivrp	%st, %st(1)
  43f87b: 8b 54 24 10                  	movl	0x10(%esp), %edx
  43f87f: d9 42 04                     	flds	0x4(%edx)
  43f882: d8 60 04                     	fsubs	0x4(%eax)
  43f885: d8 c9                        	fmul	%st(1), %st
  43f887: d8 41 04                     	fadds	0x4(%ecx)
  43f88a: d9 59 04                     	fstps	0x4(%ecx)
  43f88d: d9 42 08                     	flds	0x8(%edx)
  43f890: d8 60 08                     	fsubs	0x8(%eax)
  43f893: d8 c9                        	fmul	%st(1), %st
  43f895: d8 41 08                     	fadds	0x8(%ecx)
  43f898: d9 59 08                     	fstps	0x8(%ecx)
  43f89b: d9 42 0c                     	flds	0xc(%edx)
  43f89e: d8 60 0c                     	fsubs	0xc(%eax)
  43f8a1: d8 c9                        	fmul	%st(1), %st
  43f8a3: d8 41 0c                     	fadds	0xc(%ecx)
  43f8a6: d9 59 0c                     	fstps	0xc(%ecx)
  43f8a9: dd d8                        	fstp	%st(0)
  43f8ab: 8b 7c 24 20                  	movl	0x20(%esp), %edi
  43f8af: 8b 74 24 28                  	movl	0x28(%esp), %esi
  43f8b3: 8b 5c 24 2c                  	movl	0x2c(%esp), %ebx
  43f8b7: 8b 54 24 24                  	movl	0x24(%esp), %edx
  43f8bb: 66 83 fe 03                  	cmpw	$0x3, %si
  43f8bf: c7 41 10 01 00 00 00         	movl	$0x1, 0x10(%ecx)
  43f8c6: 75 4a                        	jne	0x43f912 <.text+0x3e912>
  43f8c8: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43f8cc: 8b 40 08                     	movl	0x8(%eax), %eax
  43f8cf: 85 c0                        	testl	%eax, %eax
  43f8d1: 74 3f                        	je	0x43f912 <.text+0x3e912>
  43f8d3: 8b 50 04                     	movl	0x4(%eax), %edx
  43f8d6: 89 51 14                     	movl	%edx, 0x14(%ecx)
  43f8d9: 8b 50 08                     	movl	0x8(%eax), %edx
  43f8dc: 89 51 18                     	movl	%edx, 0x18(%ecx)
  43f8df: 8b 40 0c                     	movl	0xc(%eax), %eax
  43f8e2: 89 41 1c                     	movl	%eax, 0x1c(%ecx)
  43f8e5: c7 47 14 02 00 00 00         	movl	$0x2, 0x14(%edi)
  43f8ec: eb 47                        	jmp	0x43f935 <.text+0x3e935>
  43f8ee: 8b 7c 24 20                  	movl	0x20(%esp), %edi
  43f8f2: 8b 74 24 28                  	movl	0x28(%esp), %esi
  43f8f6: 8b 5c 24 2c                  	movl	0x2c(%esp), %ebx
  43f8fa: 8b 54 24 24                  	movl	0x24(%esp), %edx
  43f8fe: 8b 42 20                     	movl	0x20(%edx), %eax
  43f901: 89 41 04                     	movl	%eax, 0x4(%ecx)
  43f904: 8b 42 24                     	movl	0x24(%edx), %eax
  43f907: 89 41 08                     	movl	%eax, 0x8(%ecx)
  43f90a: 8b 42 28                     	movl	0x28(%edx), %eax
  43f90d: 89 41 0c                     	movl	%eax, 0xc(%ecx)
  43f910: eb a9                        	jmp	0x43f8bb <.text+0x3e8bb>
  43f912: 8b 42 20                     	movl	0x20(%edx), %eax
  43f915: 89 41 14                     	movl	%eax, 0x14(%ecx)
  43f918: 8b 42 24                     	movl	0x24(%edx), %eax
  43f91b: 89 41 18                     	movl	%eax, 0x18(%ecx)
  43f91e: 8b 52 28                     	movl	0x28(%edx), %edx
  43f921: 89 51 1c                     	movl	%edx, 0x1c(%ecx)
  43f924: c7 47 14 02 00 00 00         	movl	$0x2, 0x14(%edi)
  43f92b: eb 08                        	jmp	0x43f935 <.text+0x3e935>
  43f92d: 33 c0                        	xorl	%eax, %eax
  43f92f: 89 47 08                     	movl	%eax, 0x8(%edi)
  43f932: 89 47 14                     	movl	%eax, 0x14(%edi)
  43f935: 66 83 fe 03                  	cmpw	$0x3, %si
  43f939: 74 0b                        	je	0x43f946 <.text+0x3e946>
  43f93b: 66 83 fb 03                  	cmpw	$0x3, %bx
  43f93f: 74 05                        	je	0x43f946 <.text+0x3e946>
  43f941: 83 c7 10                     	addl	$0x10, %edi
  43f944: eb 03                        	jmp	0x43f949 <.text+0x3e949>
  43f946: 83 c7 18                     	addl	$0x18, %edi
  43f949: 66 83 fe 03                  	cmpw	$0x3, %si
  43f94d: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  43f951: 89 7c 24 20                  	movl	%edi, 0x20(%esp)
  43f955: 75 05                        	jne	0x43f95c <.text+0x3e95c>
  43f957: 83 c0 18                     	addl	$0x18, %eax
  43f95a: eb 03                        	jmp	0x43f95f <.text+0x3e95f>
  43f95c: 83 c0 10                     	addl	$0x10, %eax
  43f95f: 66 83 fb 03                  	cmpw	$0x3, %bx
  43f963: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  43f967: 8b 44 24 18                  	movl	0x18(%esp), %eax
  43f96b: 75 05                        	jne	0x43f972 <.text+0x3e972>
  43f96d: 83 c0 18                     	addl	$0x18, %eax
  43f970: eb 03                        	jmp	0x43f975 <.text+0x3e975>
  43f972: 83 c0 10                     	addl	$0x10, %eax
  43f975: 8b 54 24 34                  	movl	0x34(%esp), %edx
  43f979: 8b 4c 24 38                  	movl	0x38(%esp), %ecx
  43f97d: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  43f981: 8b 45 20                     	movl	0x20(%ebp), %eax
  43f984: 0f b6 44 02 04               	movzbl	0x4(%edx,%eax), %eax
  43f989: 41                           	incl	%ecx
  43f98a: 3b c8                        	cmpl	%eax, %ecx
  43f98c: 89 4c 24 38                  	movl	%ecx, 0x38(%esp)
  43f990: 0f 82 ba f9 ff ff            	jb	0x43f350 <.text+0x3e350>
  43f996: b8 03 00 00 00               	movl	$0x3, %eax
  43f99b: 66 3b f0                     	cmpw	%ax, %si
  43f99e: 74 26                        	je	0x43f9c6 <.text+0x3e9c6>
  43f9a0: 66 3b d8                     	cmpw	%ax, %bx
  43f9a3: 74 21                        	je	0x43f9c6 <.text+0x3e9c6>
  43f9a5: 8b 4d 28                     	movl	0x28(%ebp), %ecx
  43f9a8: 8b 51 04                     	movl	0x4(%ecx), %edx
  43f9ab: 5f                           	popl	%edi
  43f9ac: 66 89 42 08                  	movw	%ax, 0x8(%edx)
  43f9b0: 8b 45 28                     	movl	0x28(%ebp), %eax
  43f9b3: 8b 48 04                     	movl	0x4(%eax), %ecx
  43f9b6: 5e                           	popl	%esi
  43f9b7: 5d                           	popl	%ebp
  43f9b8: 66 c7 41 0a 02 00            	movw	$0x2, 0xa(%ecx)
  43f9be: 5b                           	popl	%ebx
  43f9bf: 81 c4 e8 01 00 00            	addl	$0x1e8, %esp            # imm = 0x1E8
  43f9c5: c3                           	retl
  43f9c6: 8b 55 28                     	movl	0x28(%ebp), %edx
  43f9c9: 8b 4a 04                     	movl	0x4(%edx), %ecx
  43f9cc: 5f                           	popl	%edi
  43f9cd: 66 c7 41 08 07 00            	movw	$0x7, 0x8(%ecx)
  43f9d3: 8b 55 28                     	movl	0x28(%ebp), %edx
  43f9d6: 8b 4a 04                     	movl	0x4(%edx), %ecx
  43f9d9: 5e                           	popl	%esi
  43f9da: 5d                           	popl	%ebp
  43f9db: 66 89 41 0a                  	movw	%ax, 0xa(%ecx)
  43f9df: 5b                           	popl	%ebx
  43f9e0: 81 c4 e8 01 00 00            	addl	$0x1e8, %esp            # imm = 0x1E8
  43f9e6: c3                           	retl
  43f9e7: 90                           	nop
  43f9e8: 90                           	nop
  43f9e9: 90                           	nop
  43f9ea: 90                           	nop
  43f9eb: 90                           	nop
  43f9ec: 90                           	nop
  43f9ed: 90                           	nop
  43f9ee: 90                           	nop
  43f9ef: 90                           	nop
