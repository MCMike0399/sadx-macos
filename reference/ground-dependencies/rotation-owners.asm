
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4491e0: 83 ec 18                     	subl	$0x18, %esp
  4491e3: 57                           	pushl	%edi
  4491e4: 8b 7c 24 28                  	movl	0x28(%esp), %edi
  4491e8: 66 83 7f 0a 00               	cmpw	$0x0, 0xa(%edi)
  4491ed: 0f 85 7b 01 00 00            	jne	0x44936e <.text+0x4836e>
  4491f3: 8b 47 5c                     	movl	0x5c(%edi), %eax
  4491f6: f6 c4 40                     	testb	$0x40, %ah
  4491f9: 0f 85 6f 01 00 00            	jne	0x44936e <.text+0x4836e>
  4491ff: a1 f8 f0 b0 03               	movl	0x3b0f0f8, %eax
  449204: 8b 0d fc f0 b0 03            	movl	0x3b0f0fc, %ecx
  44920a: 8b 15 00 f1 b0 03            	movl	0x3b0f100, %edx
  449210: 89 44 24 04                  	movl	%eax, 0x4(%esp)
  449214: 53                           	pushl	%ebx
  449215: 8b 5c 24 28                  	movl	0x28(%esp), %ebx
  449219: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  44921d: 8d 43 04                     	leal	0x4(%ebx), %eax
  449220: 8b 08                        	movl	(%eax), %ecx
  449222: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  449226: 8b 50 04                     	movl	0x4(%eax), %edx
  449229: 8b 40 08                     	movl	0x8(%eax), %eax
  44922c: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  449230: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  449234: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  449238: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  44923c: e8 7f ea 33 00               	calll	0x787cc0 <.text+0x386cc0>
  449241: d8 9f c0 00 00 00            	fcomps	0xc0(%edi)
  449247: df e0                        	fnstsw	%ax
  449249: f6 c4 41                     	testb	$0x41, %ah
  44924c: 75 2e                        	jne	0x44927c <.text+0x4827c>
  44924e: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  449252: 51                           	pushl	%ecx
  449253: e8 68 f8 fc ff               	calll	0x418ac0 <.text+0x17ac0>
  449258: dd d8                        	fstp	%st(0)
  44925a: 83 c4 04                     	addl	$0x4, %esp
  44925d: 8d 54 24 14                  	leal	0x14(%esp), %edx
  449261: b9 f8 f0 b0 03               	movl	$0x3b0f0f8, %ecx        # imm = 0x3B0F0F8
  449266: e8 85 ea 33 00               	calll	0x787cf0 <.text+0x386cf0>
  44926b: dc 1d 68 01 7e 00            	fcompl	0x7e0168
  449271: df e0                        	fnstsw	%ax
  449273: f6 c4 05                     	testb	$0x5, %ah
  449276: 0f 8b f1 00 00 00            	jnp	0x44936d <.text+0x4836d>
  44927c: 56                           	pushl	%esi
  44927d: 8b 74 24 28                  	movl	0x28(%esp), %esi
  449281: 8d 54 24 0c                  	leal	0xc(%esp), %edx
  449285: 52                           	pushl	%edx
  449286: 56                           	pushl	%esi
  449287: e8 74 59 ff ff               	calll	0x43ec00 <.text+0x3dc00>
  44928c: d9 44 24 18                  	flds	0x18(%esp)
  449290: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  449296: 83 c4 08                     	addl	$0x8, %esp
  449299: df e0                        	fnstsw	%ax
  44929b: f6 c4 05                     	testb	$0x5, %ah
  44929e: 0f 8a b6 00 00 00            	jp	0x44935a <.text+0x4835a>
  4492a4: d9 44 24 10                  	flds	0x10(%esp)
  4492a8: d8 1d 64 01 7e 00            	fcomps	0x7e0164
  4492ae: df e0                        	fnstsw	%ax
  4492b0: f6 c4 41                     	testb	$0x41, %ah
  4492b3: 0f 85 a1 00 00 00            	jne	0x44935a <.text+0x4835a>
  4492b9: d9 44 24 0c                  	flds	0xc(%esp)
  4492bd: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4492c3: df e0                        	fnstsw	%ax
  4492c5: f6 c4 05                     	testb	$0x5, %ah
  4492c8: 7a 0a                        	jp	0x4492d4 <.text+0x482d4>
  4492ca: d9 44 24 0c                  	flds	0xc(%esp)
  4492ce: d9 e0                        	fchs
  4492d0: d9 5c 24 0c                  	fstps	0xc(%esp)
  4492d4: d9 44 24 14                  	flds	0x14(%esp)
  4492d8: 55                           	pushl	%ebp
  4492d9: d9 44 24 10                  	flds	0x10(%esp)
  4492dd: d9 f3                        	fpatan
  4492df: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  4492e5: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  4492eb: e8 50 b0 1f 00               	calll	0x644340 <.text+0x243340>
  4492f0: d9 44 24 18                  	flds	0x18(%esp)
  4492f4: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4492fa: 8b 6e 18                     	movl	0x18(%esi), %ebp
  4492fd: 2b e8                        	subl	%eax, %ebp
  4492ff: df e0                        	fnstsw	%ax
  449301: f6 c4 05                     	testb	$0x5, %ah
  449304: 7a 0a                        	jp	0x449310 <.text+0x48310>
  449306: d9 44 24 18                  	flds	0x18(%esp)
  44930a: d9 e0                        	fchs
  44930c: d9 5c 24 18                  	fstps	0x18(%esp)
  449310: d9 44 24 18                  	flds	0x18(%esp)
  449314: 33 c0                        	xorl	%eax, %eax
  449316: 66 8b 46 04                  	movw	0x4(%esi), %ax
  44931a: 25 00 01 00 00               	andl	$0x100, %eax            # imm = 0x100
  44931f: 66 85 c0                     	testw	%ax, %ax
  449322: 74 08                        	je	0x44932c <.text+0x4832c>
  449324: d8 0d f4 cc 7d 00            	fmuls	0x7dccf4
  44932a: eb 06                        	jmp	0x449332 <.text+0x48332>
  44932c: d8 0d 60 01 7e 00            	fmuls	0x7e0160
  449332: e8 09 b0 1f 00               	calll	0x644340 <.text+0x243340>
  449337: 8b 4b 20                     	movl	0x20(%ebx), %ecx
  44933a: 50                           	pushl	%eax
  44933b: 55                           	pushl	%ebp
  44933c: 51                           	pushl	%ecx
  44933d: e8 0e f0 fe ff               	calll	0x438350 <.text+0x37350>
  449342: 83 c4 0c                     	addl	$0xc, %esp
  449345: 5d                           	popl	%ebp
  449346: 50                           	pushl	%eax
  449347: 53                           	pushl	%ebx
  449348: 89 43 20                     	movl	%eax, 0x20(%ebx)
  44934b: e8 00 a9 ff ff               	calll	0x443c50 <.text+0x42c50>
  449350: 83 c4 08                     	addl	$0x8, %esp
  449353: 5e                           	popl	%esi
  449354: 5b                           	popl	%ebx
  449355: 5f                           	popl	%edi
  449356: 83 c4 18                     	addl	$0x18, %esp
  449359: c3                           	retl
  44935a: 8b 56 18                     	movl	0x18(%esi), %edx
  44935d: 8b c2                        	movl	%edx, %eax
  44935f: 50                           	pushl	%eax
  449360: 53                           	pushl	%ebx
  449361: 89 53 20                     	movl	%edx, 0x20(%ebx)
  449364: e8 e7 a8 ff ff               	calll	0x443c50 <.text+0x42c50>
  449369: 83 c4 08                     	addl	$0x8, %esp
  44936c: 5e                           	popl	%esi
  44936d: 5b                           	popl	%ebx
  44936e: 5f                           	popl	%edi
  44936f: 83 c4 18                     	addl	$0x18, %esp
  449372: c3                           	retl
  449373: 90                           	nop
  449374: 90                           	nop
  449375: 90                           	nop
  449376: 90                           	nop
  449377: 90                           	nop
  449378: 90                           	nop
  449379: 90                           	nop
  44937a: 90                           	nop
  44937b: 90                           	nop
  44937c: 90                           	nop
  44937d: 90                           	nop
  44937e: 90                           	nop
  44937f: 90                           	nop
  449380: 83 ec 18                     	subl	$0x18, %esp
  449383: 55                           	pushl	%ebp
  449384: 8b 6c 24 20                  	movl	0x20(%esp), %ebp
  449388: 66 83 7d 0a 00               	cmpw	$0x0, 0xa(%ebp)
  44938d: 57                           	pushl	%edi
  44938e: 8b f8                        	movl	%eax, %edi
  449390: 0f 85 53 01 00 00            	jne	0x4494e9 <.text+0x484e9>
  449396: a1 f8 f0 b0 03               	movl	0x3b0f0f8, %eax
  44939b: 8b 0d fc f0 b0 03            	movl	0x3b0f0fc, %ecx
  4493a1: 8b 15 00 f1 b0 03            	movl	0x3b0f100, %edx
  4493a7: 89 44 24 08                  	movl	%eax, 0x8(%esp)
  4493ab: 89 4c 24 0c                  	movl	%ecx, 0xc(%esp)
  4493af: 8d 46 04                     	leal	0x4(%esi), %eax
  4493b2: 8b 08                        	movl	(%eax), %ecx
  4493b4: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  4493b8: 8b 50 04                     	movl	0x4(%eax), %edx
  4493bb: 8b 40 08                     	movl	0x8(%eax), %eax
  4493be: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  4493c2: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  4493c6: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  4493ca: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  4493ce: e8 ed e8 33 00               	calll	0x787cc0 <.text+0x386cc0>
  4493d3: d8 9d c0 00 00 00            	fcomps	0xc0(%ebp)
  4493d9: df e0                        	fnstsw	%ax
  4493db: f6 c4 41                     	testb	$0x41, %ah
  4493de: 0f 85 05 01 00 00            	jne	0x4494e9 <.text+0x484e9>
  4493e4: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  4493e8: 51                           	pushl	%ecx
  4493e9: e8 d2 f6 fc ff               	calll	0x418ac0 <.text+0x17ac0>
  4493ee: dd d8                        	fstp	%st(0)
  4493f0: 83 c4 04                     	addl	$0x4, %esp
  4493f3: 8d 54 24 14                  	leal	0x14(%esp), %edx
  4493f7: b9 f8 f0 b0 03               	movl	$0x3b0f0f8, %ecx        # imm = 0x3B0F0F8
  4493fc: e8 ef e8 33 00               	calll	0x787cf0 <.text+0x386cf0>
  449401: dc 1d 68 01 7e 00            	fcompl	0x7e0168
  449407: df e0                        	fnstsw	%ax
  449409: f6 c4 05                     	testb	$0x5, %ah
  44940c: 0f 8b d7 00 00 00            	jnp	0x4494e9 <.text+0x484e9>
  449412: 8d 54 24 08                  	leal	0x8(%esp), %edx
  449416: 52                           	pushl	%edx
  449417: 57                           	pushl	%edi
  449418: e8 e3 57 ff ff               	calll	0x43ec00 <.text+0x3dc00>
  44941d: d9 44 24 14                  	flds	0x14(%esp)
  449421: d8 1d 64 01 7e 00            	fcomps	0x7e0164
  449427: 83 c4 08                     	addl	$0x8, %esp
  44942a: df e0                        	fnstsw	%ax
  44942c: f6 c4 41                     	testb	$0x41, %ah
  44942f: 0f 85 9f 00 00 00            	jne	0x4494d4 <.text+0x484d4>
  449435: d9 44 24 08                  	flds	0x8(%esp)
  449439: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  44943f: df e0                        	fnstsw	%ax
  449441: f6 c4 05                     	testb	$0x5, %ah
  449444: 7a 0a                        	jp	0x449450 <.text+0x48450>
  449446: d9 44 24 08                  	flds	0x8(%esp)
  44944a: d9 e0                        	fchs
  44944c: d9 5c 24 08                  	fstps	0x8(%esp)
  449450: d9 44 24 10                  	flds	0x10(%esp)
  449454: 53                           	pushl	%ebx
  449455: d9 44 24 0c                  	flds	0xc(%esp)
  449459: d9 f3                        	fpatan
  44945b: dc 0d d0 cc 7d 00            	fmull	0x7dccd0
  449461: dc 0d e8 cc 7d 00            	fmull	0x7dcce8
  449467: e8 d4 ae 1f 00               	calll	0x644340 <.text+0x243340>
  44946c: d9 44 24 14                  	flds	0x14(%esp)
  449470: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  449476: 8b 5f 18                     	movl	0x18(%edi), %ebx
  449479: 2b d8                        	subl	%eax, %ebx
  44947b: df e0                        	fnstsw	%ax
  44947d: f6 c4 05                     	testb	$0x5, %ah
  449480: 7a 0a                        	jp	0x44948c <.text+0x4848c>
  449482: d9 44 24 14                  	flds	0x14(%esp)
  449486: d9 e0                        	fchs
  449488: d9 5c 24 14                  	fstps	0x14(%esp)
  44948c: 33 c0                        	xorl	%eax, %eax
  44948e: 66 8b 47 04                  	movw	0x4(%edi), %ax
  449492: 25 00 01 00 00               	andl	$0x100, %eax            # imm = 0x100
  449497: 66 85 c0                     	testw	%ax, %ax
  44949a: 74 15                        	je	0x4494b1 <.text+0x484b1>
  44949c: d9 45 38                     	flds	0x38(%ebp)
  44949f: d8 b5 c0 00 00 00            	fdivs	0xc0(%ebp)
  4494a5: d8 4c 24 14                  	fmuls	0x14(%esp)
  4494a9: d8 0d 70 01 7e 00            	fmuls	0x7e0170
  4494af: eb 0a                        	jmp	0x4494bb <.text+0x484bb>
  4494b1: d9 44 24 14                  	flds	0x14(%esp)
  4494b5: d8 0d 60 cb 7d 00            	fmuls	0x7dcb60
  4494bb: e8 80 ae 1f 00               	calll	0x644340 <.text+0x243340>
  4494c0: 8b 4e 20                     	movl	0x20(%esi), %ecx
  4494c3: 50                           	pushl	%eax
  4494c4: 53                           	pushl	%ebx
  4494c5: 51                           	pushl	%ecx
  4494c6: e8 85 ee fe ff               	calll	0x438350 <.text+0x37350>
  4494cb: 83 c4 0c                     	addl	$0xc, %esp
  4494ce: 89 46 20                     	movl	%eax, 0x20(%esi)
  4494d1: 5b                           	popl	%ebx
  4494d2: eb 06                        	jmp	0x4494da <.text+0x484da>
  4494d4: 8b 57 18                     	movl	0x18(%edi), %edx
  4494d7: 89 56 20                     	movl	%edx, 0x20(%esi)
  4494da: 8b 46 20                     	movl	0x20(%esi), %eax
  4494dd: 50                           	pushl	%eax
  4494de: 56                           	pushl	%esi
  4494df: 8b c5                        	movl	%ebp, %eax
  4494e1: e8 0a a9 ff ff               	calll	0x443df0 <.text+0x42df0>
  4494e6: 83 c4 08                     	addl	$0x8, %esp
  4494e9: 5f                           	popl	%edi
  4494ea: 5d                           	popl	%ebp
  4494eb: 83 c4 18                     	addl	$0x18, %esp
  4494ee: c3                           	retl
