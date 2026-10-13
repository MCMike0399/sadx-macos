
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  447010: 83 ec 20                     	subl	$0x20, %esp
  447013: d9 05 f8 f0 b0 03            	flds	0x3b0f0f8
  447019: 53                           	pushl	%ebx
  44701a: 8b 5c 24 2c                  	movl	0x2c(%esp), %ebx
  44701e: 55                           	pushl	%ebp
  44701f: 8b 6c 24 34                  	movl	0x34(%esp), %ebp
  447023: d8 8d 14 01 00 00            	fmuls	0x114(%ebp)
  447029: 8d 43 10                     	leal	0x10(%ebx), %eax
  44702c: 8b 08                        	movl	(%eax), %ecx
  44702e: 8b 50 04                     	movl	0x4(%eax), %edx
  447031: 8b 40 08                     	movl	0x8(%eax), %eax
  447034: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  447038: d8 44 24 10                  	fadds	0x10(%esp)
  44703c: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  447040: 56                           	pushl	%esi
  447041: 8b 74 24 30                  	movl	0x30(%esp), %esi
  447045: d9 5c 24 14                  	fstps	0x14(%esp)
  447049: d9 05 fc f0 b0 03            	flds	0x3b0f0fc
  44704f: 8d 4c 24 38                  	leal	0x38(%esp), %ecx
  447053: d8 8d 14 01 00 00            	fmuls	0x114(%ebp)
  447059: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  44705d: 51                           	pushl	%ecx
  44705e: 8d 54 24 10                  	leal	0x10(%esp), %edx
  447062: d8 44 24 1c                  	fadds	0x1c(%esp)
  447066: 52                           	pushl	%edx
  447067: 56                           	pushl	%esi
  447068: c7 44 24 34 00 00 00 00      	movl	$0x0, 0x34(%esp)
  447070: d9 5c 24 24                  	fstps	0x24(%esp)
  447074: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  44707c: d9 05 00 f1 b0 03            	flds	0x3b0f100
  447082: d8 8d 14 01 00 00            	fmuls	0x114(%ebp)
  447088: d8 44 24 28                  	fadds	0x28(%esp)
  44708c: d9 5c 24 28                  	fstps	0x28(%esp)
  447090: e8 2b 4d ff ff               	calll	0x43bdc0 <.text+0x3adc0>
  447095: 8b c8                        	movl	%eax, %ecx
  447097: 83 c4 0c                     	addl	$0xc, %esp
  44709a: 85 c9                        	testl	%ecx, %ecx
  44709c: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  4470a0: 75 10                        	jne	0x4470b2 <.text+0x460b2>
  4470a2: d9 45 38                     	flds	0x38(%ebp)
  4470a5: d8 9d c4 00 00 00            	fcomps	0xc4(%ebp)
  4470ab: df e0                        	fnstsw	%ax
  4470ad: f6 c4 41                     	testb	$0x41, %ah
  4470b0: 74 2e                        	je	0x4470e0 <.text+0x460e0>
  4470b2: d9 45 38                     	flds	0x38(%ebp)
  4470b5: d8 9d c8 00 00 00            	fcomps	0xc8(%ebp)
  4470bb: d9 45 38                     	flds	0x38(%ebp)
  4470be: df e0                        	fnstsw	%ax
  4470c0: f6 c4 41                     	testb	$0x41, %ah
  4470c3: 75 0e                        	jne	0x4470d3 <.text+0x460d3>
  4470c5: d8 a5 c8 00 00 00            	fsubs	0xc8(%ebp)
  4470cb: d8 8d f0 00 00 00            	fmuls	0xf0(%ebp)
  4470d1: eb 16                        	jmp	0x4470e9 <.text+0x460e9>
  4470d3: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4470d9: df e0                        	fnstsw	%ax
  4470db: f6 c4 05                     	testb	$0x5, %ah
  4470de: 7a 11                        	jp	0x4470f1 <.text+0x460f1>
  4470e0: d9 85 f0 00 00 00            	flds	0xf0(%ebp)
  4470e6: d8 4d 38                     	fmuls	0x38(%ebp)
  4470e9: d8 44 24 14                  	fadds	0x14(%esp)
  4470ed: d9 5c 24 14                  	fstps	0x14(%esp)
  4470f1: 85 c9                        	testl	%ecx, %ecx
  4470f3: d9 85 f4 00 00 00            	flds	0xf4(%ebp)
  4470f9: d8 4d 3c                     	fmuls	0x3c(%ebp)
  4470fc: d8 44 24 18                  	fadds	0x18(%esp)
  447100: d9 5c 24 18                  	fstps	0x18(%esp)
  447104: d9 85 f8 00 00 00            	flds	0xf8(%ebp)
  44710a: d8 4d 40                     	fmuls	0x40(%ebp)
  44710d: d8 44 24 1c                  	fadds	0x1c(%esp)
  447111: d9 5c 24 1c                  	fstps	0x1c(%esp)
  447115: 75 4a                        	jne	0x447161 <.text+0x46161>
  447117: d9 45 38                     	flds	0x38(%ebp)
  44711a: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447120: df e0                        	fnstsw	%ax
  447122: f6 c4 41                     	testb	$0x41, %ah
  447125: 75 12                        	jne	0x447139 <.text+0x46139>
  447127: 8b 85 e0 00 00 00            	movl	0xe0(%ebp), %eax
  44712d: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  447131: 89 4b 34                     	movl	%ecx, 0x34(%ebx)
  447134: e9 4b 01 00 00               	jmp	0x447284 <.text+0x46284>
  447139: d9 45 38                     	flds	0x38(%ebp)
  44713c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447142: df e0                        	fnstsw	%ax
  447144: f6 c4 05                     	testb	$0x5, %ah
  447147: 7a 0c                        	jp	0x447155 <.text+0x46155>
  447149: d9 85 e0 00 00 00            	flds	0xe0(%ebp)
  44714f: d9 e0                        	fchs
  447151: d9 5c 24 20                  	fstps	0x20(%esp)
  447155: c7 43 34 00 00 00 00         	movl	$0x0, 0x34(%ebx)
  44715c: e9 23 01 00 00               	jmp	0x447284 <.text+0x46284>
  447161: d9 85 c4 00 00 00            	flds	0xc4(%ebp)
  447167: d8 5d 38                     	fcomps	0x38(%ebp)
  44716a: d9 44 24 38                  	flds	0x38(%esp)
  44716e: d8 8d d8 00 00 00            	fmuls	0xd8(%ebp)
  447174: df e0                        	fnstsw	%ax
  447176: f6 c4 41                     	testb	$0x41, %ah
  447179: 74 06                        	je	0x447181 <.text+0x46181>
  44717b: d8 0d 50 d5 7d 00            	fmuls	0x7dd550
  447181: 8b 4e 18                     	movl	0x18(%esi), %ecx
  447184: d9 5c 24 38                  	fstps	0x38(%esp)
  447188: 57                           	pushl	%edi
  447189: 8b 7c 24 10                  	movl	0x10(%esp), %edi
  44718d: 57                           	pushl	%edi
  44718e: 51                           	pushl	%ecx
  44718f: e8 1c 12 ff ff               	calll	0x4383b0 <.text+0x373b0>
  447194: d9 45 38                     	flds	0x38(%ebp)
  447197: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44719d: 8b c8                        	movl	%eax, %ecx
  44719f: 83 c4 08                     	addl	$0x8, %esp
  4471a2: df e0                        	fnstsw	%ax
  4471a4: f6 c4 44                     	testb	$0x44, %ah
  4471a7: 7a 23                        	jp	0x4471cc <.text+0x461cc>
  4471a9: 81 f9 00 10 00 00            	cmpl	$0x1000, %ecx           # imm = 0x1000
  4471af: 7e 1b                        	jle	0x4471cc <.text+0x461cc>
  4471b1: 57                           	pushl	%edi
  4471b2: 89 7b 20                     	movl	%edi, 0x20(%ebx)
  4471b5: 53                           	pushl	%ebx
  4471b6: 8b c5                        	movl	%ebp, %eax
  4471b8: 8b fe                        	movl	%esi, %edi
  4471ba: c7 44 24 44 00 00 00 00      	movl	$0x0, 0x44(%esp)
  4471c2: e8 29 cc ff ff               	calll	0x443df0 <.text+0x42df0>
  4471c7: e9 a3 00 00 00               	jmp	0x44726f <.text+0x4626f>
  4471cc: d9 85 c0 00 00 00            	flds	0xc0(%ebp)
  4471d2: d8 85 c4 00 00 00            	fadds	0xc4(%ebp)
  4471d8: d8 0d 98 cc 7d 00            	fmuls	0x7dcc98
  4471de: d8 5d 38                     	fcomps	0x38(%ebp)
  4471e1: df e0                        	fnstsw	%ax
  4471e3: f6 c4 41                     	testb	$0x41, %ah
  4471e6: 7a 17                        	jp	0x4471ff <.text+0x461ff>
  4471e8: 81 f9 00 10 00 00            	cmpl	$0x1000, %ecx           # imm = 0x1000
  4471ee: 7e 0f                        	jle	0x4471ff <.text+0x461ff>
  4471f0: 89 7b 20                     	movl	%edi, 0x20(%ebx)
  4471f3: 8b 95 e0 00 00 00            	movl	0xe0(%ebp), %edx
  4471f9: 89 54 24 3c                  	movl	%edx, 0x3c(%esp)
  4471fd: eb 67                        	jmp	0x447266 <.text+0x46266>
  4471ff: d9 45 38                     	flds	0x38(%ebp)
  447202: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  447208: df e0                        	fnstsw	%ax
  44720a: f6 c4 01                     	testb	$0x1, %ah
  44720d: 75 1e                        	jne	0x44722d <.text+0x4622d>
  44720f: 81 f9 00 10 00 00            	cmpl	$0x1000, %ecx           # imm = 0x1000
  447215: 7d 16                        	jge	0x44722d <.text+0x4622d>
  447217: 57                           	pushl	%edi
  447218: 89 7b 20                     	movl	%edi, 0x20(%ebx)
  44721b: 53                           	pushl	%ebx
  44721c: 8b 5c 24 3c                  	movl	0x3c(%esp), %ebx
  447220: 8b f5                        	movl	%ebp, %esi
  447222: e8 39 cc ff ff               	calll	0x443e60 <.text+0x42e60>
  447227: 8b 5c 24 40                  	movl	0x40(%esp), %ebx
  44722b: eb 42                        	jmp	0x44726f <.text+0x4626f>
  44722d: d9 45 38                     	flds	0x38(%ebp)
  447230: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  447236: df e0                        	fnstsw	%ax
  447238: f6 c4 01                     	testb	$0x1, %ah
  44723b: 75 26                        	jne	0x447263 <.text+0x46263>
  44723d: d9 45 38                     	flds	0x38(%ebp)
  447240: d8 9d c8 00 00 00            	fcomps	0xc8(%ebp)
  447246: df e0                        	fnstsw	%ax
  447248: f6 c4 41                     	testb	$0x41, %ah
  44724b: 7a 16                        	jp	0x447263 <.text+0x46263>
  44724d: 81 f9 00 20 00 00            	cmpl	$0x2000, %ecx           # imm = 0x2000
  447253: 7e 0e                        	jle	0x447263 <.text+0x46263>
  447255: d9 44 24 3c                  	flds	0x3c(%esp)
  447259: d8 0d 5c ce 7d 00            	fmuls	0x7dce5c
  44725f: d9 5c 24 3c                  	fstps	0x3c(%esp)
  447263: 89 7b 20                     	movl	%edi, 0x20(%ebx)
  447266: 57                           	pushl	%edi
  447267: 8b fd                        	movl	%ebp, %edi
  447269: 53                           	pushl	%ebx
  44726a: e8 e1 c9 ff ff               	calll	0x443c50 <.text+0x42c50>
  44726f: 8b 44 24 44                  	movl	0x44(%esp), %eax
  447273: 8b c8                        	movl	%eax, %ecx
  447275: 83 c4 08                     	addl	$0x8, %esp
  447278: 89 4c 24 24                  	movl	%ecx, 0x24(%esp)
  44727c: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  447280: 89 43 34                     	movl	%eax, 0x34(%ebx)
  447283: 5f                           	popl	%edi
  447284: d9 45 38                     	flds	0x38(%ebp)
  447287: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44728d: df e0                        	fnstsw	%ax
  44728f: f6 c4 44                     	testb	$0x44, %ah
  447292: 7a 5a                        	jp	0x4472ee <.text+0x462ee>
  447294: 85 c9                        	testl	%ecx, %ecx
  447296: d9 85 04 01 00 00            	flds	0x104(%ebp)
  44729c: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44729f: d8 4c 24 18                  	fmuls	0x18(%esp)
  4472a3: d9 5c 24 34                  	fstps	0x34(%esp)
  4472a7: d9 44 24 14                  	flds	0x14(%esp)
  4472ab: d8 44 24 20                  	fadds	0x20(%esp)
  4472af: d9 5c 24 14                  	fstps	0x14(%esp)
  4472b3: 0f 85 3d 01 00 00            	jne	0x4473f6 <.text+0x463f6>
  4472b9: d9 44 24 14                  	flds	0x14(%esp)
  4472bd: d8 5c 24 34                  	fcomps	0x34(%esp)
  4472c1: df e0                        	fnstsw	%ax
  4472c3: f6 c4 05                     	testb	$0x5, %ah
  4472c6: 0f 8a 2a 01 00 00            	jp	0x4473f6 <.text+0x463f6>
  4472cc: d9 44 24 34                  	flds	0x34(%esp)
  4472d0: d9 e0                        	fchs
  4472d2: d8 5c 24 14                  	fcomps	0x14(%esp)
  4472d6: df e0                        	fnstsw	%ax
  4472d8: f6 c4 05                     	testb	$0x5, %ah
  4472db: 0f 8a 15 01 00 00            	jp	0x4473f6 <.text+0x463f6>
  4472e1: c7 44 24 14 00 00 00 00      	movl	$0x0, 0x14(%esp)
  4472e9: e9 08 01 00 00               	jmp	0x4473f6 <.text+0x463f6>
  4472ee: d9 44 24 20                  	flds	0x20(%esp)
  4472f2: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4472f8: df e0                        	fnstsw	%ax
  4472fa: f6 c4 05                     	testb	$0x5, %ah
  4472fd: 7a 6f                        	jp	0x44736e <.text+0x4636e>
  4472ff: d9 45 38                     	flds	0x38(%ebp)
  447302: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447308: df e0                        	fnstsw	%ax
  44730a: f6 c4 41                     	testb	$0x41, %ah
  44730d: 75 2c                        	jne	0x44733b <.text+0x4633b>
  44730f: d9 44 24 14                  	flds	0x14(%esp)
  447313: d8 44 24 20                  	fadds	0x20(%esp)
  447317: d9 44 24 20                  	flds	0x20(%esp)
  44731b: d8 c9                        	fmul	%st(1), %st
  44731d: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447323: df e0                        	fnstsw	%ax
  447325: f6 c4 05                     	testb	$0x5, %ah
  447328: 0f 8a c4 00 00 00            	jp	0x4473f2 <.text+0x463f2>
  44732e: dd d8                        	fstp	%st(0)
  447330: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  447336: e9 b7 00 00 00               	jmp	0x4473f2 <.text+0x463f2>
  44733b: d9 85 04 01 00 00            	flds	0x104(%ebp)
  447341: d8 4b 3c                     	fmuls	0x3c(%ebx)
  447344: d8 4c 24 18                  	fmuls	0x18(%esp)
  447348: d8 44 24 14                  	fadds	0x14(%esp)
  44734c: d8 44 24 20                  	fadds	0x20(%esp)
  447350: d8 15 2c cb 7d 00            	fcoms	0x7dcb2c
  447356: df e0                        	fnstsw	%ax
  447358: f6 c4 01                     	testb	$0x1, %ah
  44735b: 0f 85 91 00 00 00            	jne	0x4473f2 <.text+0x463f2>
  447361: dd d8                        	fstp	%st(0)
  447363: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  447369: e9 84 00 00 00               	jmp	0x4473f2 <.text+0x463f2>
  44736e: d9 85 04 01 00 00            	flds	0x104(%ebp)
  447374: d8 4b 3c                     	fmuls	0x3c(%ebx)
  447377: d8 4c 24 18                  	fmuls	0x18(%esp)
  44737b: d9 5c 24 34                  	fstps	0x34(%esp)
  44737f: d9 45 38                     	flds	0x38(%ebp)
  447382: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447388: df e0                        	fnstsw	%ax
  44738a: f6 c4 05                     	testb	$0x5, %ah
  44738d: 7a 1b                        	jp	0x4473aa <.text+0x463aa>
  44738f: d9 44 24 14                  	flds	0x14(%esp)
  447393: d8 44 24 20                  	fadds	0x20(%esp)
  447397: d9 54 24 14                  	fsts	0x14(%esp)
  44739b: d8 4c 24 20                  	fmuls	0x20(%esp)
  44739f: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4473a5: e9 2c ff ff ff               	jmp	0x4472d6 <.text+0x462d6>
  4473aa: d9 44 24 34                  	flds	0x34(%esp)
  4473ae: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4473b4: df e0                        	fnstsw	%ax
  4473b6: f6 c4 41                     	testb	$0x41, %ah
  4473b9: 75 2f                        	jne	0x4473ea <.text+0x463ea>
  4473bb: d9 44 24 34                  	flds	0x34(%esp)
  4473bf: d8 5c 24 20                  	fcomps	0x20(%esp)
  4473c3: df e0                        	fnstsw	%ax
  4473c5: f6 c4 05                     	testb	$0x5, %ah
  4473c8: 7a 20                        	jp	0x4473ea <.text+0x463ea>
  4473ca: d9 44 24 14                  	flds	0x14(%esp)
  4473ce: d8 44 24 20                  	fadds	0x20(%esp)
  4473d2: d8 54 24 34                  	fcoms	0x34(%esp)
  4473d6: df e0                        	fnstsw	%ax
  4473d8: f6 c4 41                     	testb	$0x41, %ah
  4473db: 75 15                        	jne	0x4473f2 <.text+0x463f2>
  4473dd: d8 64 24 34                  	fsubs	0x34(%esp)
  4473e1: d9 5b 38                     	fstps	0x38(%ebx)
  4473e4: d9 44 24 34                  	flds	0x34(%esp)
  4473e8: eb 08                        	jmp	0x4473f2 <.text+0x463f2>
  4473ea: d9 44 24 14                  	flds	0x14(%esp)
  4473ee: d8 44 24 20                  	fadds	0x20(%esp)
  4473f2: d9 5c 24 14                  	fstps	0x14(%esp)
  4473f6: d9 45 40                     	flds	0x40(%ebp)
  4473f9: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4473ff: df e0                        	fnstsw	%ax
  447401: f6 c4 44                     	testb	$0x44, %ah
  447404: 7a 3c                        	jp	0x447442 <.text+0x46442>
  447406: d9 85 04 01 00 00            	flds	0x104(%ebp)
  44740c: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44740f: d8 4c 24 18                  	fmuls	0x18(%esp)
  447413: d9 44 24 1c                  	flds	0x1c(%esp)
  447417: d8 d9                        	fcomp	%st(1)
  447419: df e0                        	fnstsw	%ax
  44741b: f6 c4 05                     	testb	$0x5, %ah
  44741e: 0f 8a c1 00 00 00            	jp	0x4474e5 <.text+0x464e5>
  447424: d9 e0                        	fchs
  447426: d8 5c 24 1c                  	fcomps	0x1c(%esp)
  44742a: df e0                        	fnstsw	%ax
  44742c: f6 c4 05                     	testb	$0x5, %ah
  44742f: 0f 8a b2 00 00 00            	jp	0x4474e7 <.text+0x464e7>
  447435: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  44743d: e9 a5 00 00 00               	jmp	0x4474e7 <.text+0x464e7>
  447442: d9 44 24 18                  	flds	0x18(%esp)
  447446: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44744c: df e0                        	fnstsw	%ax
  44744e: f6 c4 05                     	testb	$0x5, %ah
  447451: 7a 0f                        	jp	0x447462 <.text+0x46462>
  447453: d9 85 00 01 00 00            	flds	0x100(%ebp)
  447459: d8 4b 3c                     	fmuls	0x3c(%ebx)
  44745c: d8 4c 24 18                  	fmuls	0x18(%esp)
  447460: eb 06                        	jmp	0x447468 <.text+0x46468>
  447462: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  447468: d9 44 24 1c                  	flds	0x1c(%esp)
  44746c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  447472: df e0                        	fnstsw	%ax
  447474: f6 c4 41                     	testb	$0x41, %ah
  447477: 75 08                        	jne	0x447481 <.text+0x46481>
  447479: d9 e0                        	fchs
  44747b: d9 5c 24 28                  	fstps	0x28(%esp)
  44747f: eb 19                        	jmp	0x44749a <.text+0x4649a>
  447481: d9 44 24 1c                  	flds	0x1c(%esp)
  447485: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44748b: df e0                        	fnstsw	%ax
  44748d: f6 c4 05                     	testb	$0x5, %ah
  447490: 7a 06                        	jp	0x447498 <.text+0x46498>
  447492: d9 5c 24 28                  	fstps	0x28(%esp)
  447496: eb 02                        	jmp	0x44749a <.text+0x4649a>
  447498: dd d8                        	fstp	%st(0)
  44749a: d9 44 24 1c                  	flds	0x1c(%esp)
  44749e: d8 44 24 28                  	fadds	0x28(%esp)
  4474a2: d9 44 24 1c                  	flds	0x1c(%esp)
  4474a6: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4474ac: df e0                        	fnstsw	%ax
  4474ae: f6 c4 44                     	testb	$0x44, %ah
  4474b1: 7b 2c                        	jnp	0x4474df <.text+0x464df>
  4474b3: d9 44 24 28                  	flds	0x28(%esp)
  4474b7: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4474bd: df e0                        	fnstsw	%ax
  4474bf: f6 c4 44                     	testb	$0x44, %ah
  4474c2: 7b 1b                        	jnp	0x4474df <.text+0x464df>
  4474c4: d9 44 24 1c                  	flds	0x1c(%esp)
  4474c8: d8 c9                        	fmul	%st(1), %st
  4474ca: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4474d0: df e0                        	fnstsw	%ax
  4474d2: f6 c4 05                     	testb	$0x5, %ah
  4474d5: 7a 08                        	jp	0x4474df <.text+0x464df>
  4474d7: dd d8                        	fstp	%st(0)
  4474d9: d9 05 2c cb 7d 00            	flds	0x7dcb2c
  4474df: d9 5c 24 1c                  	fstps	0x1c(%esp)
  4474e3: eb 02                        	jmp	0x4474e7 <.text+0x464e7>
  4474e5: dd d8                        	fstp	%st(0)
  4474e7: 8b 54 24 18                  	movl	0x18(%esp), %edx
  4474eb: 8b 44 24 14                  	movl	0x14(%esp), %eax
  4474ef: 83 c5 2c                     	addl	$0x2c, %ebp
  4474f2: 89 45 00                     	movl	%eax, (%ebp)
  4474f5: 8b ca                        	movl	%edx, %ecx
  4474f7: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  4474fb: 8b 54 24 1c                  	movl	0x1c(%esp), %edx
  4474ff: 89 4d 04                     	movl	%ecx, 0x4(%ebp)
  447502: 5e                           	popl	%esi
  447503: 89 55 08                     	movl	%edx, 0x8(%ebp)
  447506: 5d                           	popl	%ebp
  447507: 5b                           	popl	%ebx
  447508: 83 c4 20                     	addl	$0x20, %esp
  44750b: c3                           	retl
