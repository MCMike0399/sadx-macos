
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  442750: 51                           	pushl	%ecx
  442751: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  442755: 53                           	pushl	%ebx
  442756: 8b 58 20                     	movl	0x20(%eax), %ebx
  442759: 8b 40 24                     	movl	0x24(%eax), %eax
  44275c: 55                           	pushl	%ebp
  44275d: 56                           	pushl	%esi
  44275e: 57                           	pushl	%edi
  44275f: 68 00 02 00 00               	pushl	$0x200                  # imm = 0x200
  442764: 6a 01                        	pushl	$0x1
  442766: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  44276a: e8 61 8b fc ff               	calll	0x40b2d0 <.text+0xa2d0>
  44276f: 0f b6 73 09                  	movzbl	0x9(%ebx), %esi
  442773: 69 f6 84 00 00 00            	imull	$0x84, %esi, %esi
  442779: 8b e8                        	movl	%eax, %ebp
  44277b: 81 c6 e8 54 91 00            	addl	$0x9154e8, %esi         # imm = 0x9154E8
  442781: 8d bd 9c 00 00 00            	leal	0x9c(%ebp), %edi
  442787: b9 21 00 00 00               	movl	$0x21, %ecx
  44278c: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  44278e: 8b 7c 24 18                  	movl	0x18(%esp), %edi
  442792: 53                           	pushl	%ebx
  442793: 89 2f                        	movl	%ebp, (%edi)
  442795: e8 e6 56 fd ff               	calll	0x417e80 <.text+0x16e80>
  44279a: 68 84 00 00 00               	pushl	$0x84
  44279f: 6a 01                        	pushl	$0x1
  4427a1: e8 2a 8b fc ff               	calll	0x40b2d0 <.text+0xa2d0>
  4427a6: 8b 4c 24 30                  	movl	0x30(%esp), %ecx
  4427aa: 89 45 64                     	movl	%eax, 0x64(%ebp)
  4427ad: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  4427b1: 89 0c 85 30 2e b4 03         	movl	%ecx, 0x3b42e30(,%eax,4)
  4427b8: 89 1c 85 10 2e b4 03         	movl	%ebx, 0x3b42e10(,%eax,4)
  4427bf: 89 3c 85 d0 6d b3 03         	movl	%edi, 0x3b36dd0(,%eax,4)
  4427c6: 89 2c 85 f0 cd b3 03         	movl	%ebp, 0x3b3cdf0(,%eax,4)
  4427cd: d9 85 0c 01 00 00            	flds	0x10c(%ebp)
  4427d3: d8 0d 8c d5 7d 00            	fmuls	0x7dd58c
  4427d9: 33 c0                        	xorl	%eax, %eax
  4427db: 8d 75 38                     	leal	0x38(%ebp), %esi
  4427de: 66 89 85 20 01 00 00         	movw	%ax, 0x120(%ebp)
  4427e5: d9 9d fc 01 00 00            	fstps	0x1fc(%ebp)
  4427eb: 66 89 85 22 01 00 00         	movw	%ax, 0x122(%ebp)
  4427f2: 89 b5 38 01 00 00            	movl	%esi, 0x138(%ebp)
  4427f8: 89 ad 3c 01 00 00            	movl	%ebp, 0x13c(%ebp)
  4427fe: 0f b6 43 09                  	movzbl	0x9(%ebx), %eax
  442802: 83 c4 14                     	addl	$0x14, %esp
  442805: 83 f8 07                     	cmpl	$0x7, %eax
  442808: 0f 87 2c 01 00 00            	ja	0x44293a <.text+0x4193a>
  44280e: ff 24 85 94 29 44 00         	jmpl	*0x442994(,%eax,4)
  442815: 68 8d 00 00 00               	pushl	$0x8d
  44281a: e8 01 05 fd ff               	calll	0x412d20 <.text+0x11d20>
  44281f: 83 c4 04                     	addl	$0x4, %esp
  442822: 85 c0                        	testl	%eax, %eax
  442824: 74 04                        	je	0x44282a <.text+0x4182a>
  442826: 80 4d 04 02                  	orb	$0x2, 0x4(%ebp)
  44282a: 68 8e 00 00 00               	pushl	$0x8e
  44282f: e8 ec 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  442834: 83 c4 04                     	addl	$0x4, %esp
  442837: 85 c0                        	testl	%eax, %eax
  442839: 74 04                        	je	0x44283f <.text+0x4183f>
  44283b: 80 4d 04 01                  	orb	$0x1, 0x4(%ebp)
  44283f: 68 9f 00 00 00               	pushl	$0x9f
  442844: e8 d7 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  442849: 83 c4 04                     	addl	$0x4, %esp
  44284c: 85 c0                        	testl	%eax, %eax
  44284e: 74 04                        	je	0x442854 <.text+0x41854>
  442850: 80 4d 04 04                  	orb	$0x4, 0x4(%ebp)
  442854: 68 cd 00 00 00               	pushl	$0xcd
  442859: e8 c2 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  44285e: 83 c4 04                     	addl	$0x4, %esp
  442861: 85 c0                        	testl	%eax, %eax
  442863: 74 04                        	je	0x442869 <.text+0x41869>
  442865: 80 4d 04 08                  	orb	$0x8, 0x4(%ebp)
  442869: 68 db 00 00 00               	pushl	$0xdb
  44286e: e8 ad 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  442873: 83 c4 04                     	addl	$0x4, %esp
  442876: 85 c0                        	testl	%eax, %eax
  442878: 0f 84 bc 00 00 00            	je	0x44293a <.text+0x4193a>
  44287e: 80 4d 04 10                  	orb	$0x10, 0x4(%ebp)
  442882: e9 b3 00 00 00               	jmp	0x44293a <.text+0x4193a>
  442887: 68 19 01 00 00               	pushl	$0x119                  # imm = 0x119
  44288c: e8 8f 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  442891: 83 c4 04                     	addl	$0x4, %esp
  442894: 85 c0                        	testl	%eax, %eax
  442896: 74 04                        	je	0x44289c <.text+0x4189c>
  442898: 80 4d 04 40                  	orb	$0x40, 0x4(%ebp)
  44289c: 68 1a 01 00 00               	pushl	$0x11a                  # imm = 0x11A
  4428a1: e8 7a 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  4428a6: 83 c4 04                     	addl	$0x4, %esp
  4428a9: 85 c0                        	testl	%eax, %eax
  4428ab: 0f 84 89 00 00 00            	je	0x44293a <.text+0x4193a>
  4428b1: 80 4d 04 20                  	orb	$0x20, 0x4(%ebp)
  4428b5: e9 80 00 00 00               	jmp	0x44293a <.text+0x4193a>
  4428ba: 68 52 01 00 00               	pushl	$0x152                  # imm = 0x152
  4428bf: e8 5c 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  4428c4: 83 c4 04                     	addl	$0x4, %esp
  4428c7: 85 c0                        	testl	%eax, %eax
  4428c9: 74 04                        	je	0x4428cf <.text+0x418cf>
  4428cb: 80 4d 05 01                  	orb	$0x1, 0x5(%ebp)
  4428cf: 68 5e 01 00 00               	pushl	$0x15e                  # imm = 0x15E
  4428d4: e8 47 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  4428d9: 83 c4 04                     	addl	$0x4, %esp
  4428dc: 85 c0                        	testl	%eax, %eax
  4428de: 74 5a                        	je	0x44293a <.text+0x4193a>
  4428e0: 80 4d 04 80                  	orb	$-0x80, 0x4(%ebp)
  4428e4: eb 54                        	jmp	0x44293a <.text+0x4193a>
  4428e6: 68 89 01 00 00               	pushl	$0x189                  # imm = 0x189
  4428eb: e8 30 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  4428f0: 83 c4 04                     	addl	$0x4, %esp
  4428f3: 85 c0                        	testl	%eax, %eax
  4428f5: 74 04                        	je	0x4428fb <.text+0x418fb>
  4428f7: 80 4d 05 04                  	orb	$0x4, 0x5(%ebp)
  4428fb: 68 8a 01 00 00               	pushl	$0x18a                  # imm = 0x18A
  442900: e8 1b 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  442905: 83 c4 04                     	addl	$0x4, %esp
  442908: 85 c0                        	testl	%eax, %eax
  44290a: 74 04                        	je	0x442910 <.text+0x41910>
  44290c: 80 4d 05 02                  	orb	$0x2, 0x5(%ebp)
  442910: 68 d0 01 00 00               	pushl	$0x1d0                  # imm = 0x1D0
  442915: e8 06 04 fd ff               	calll	0x412d20 <.text+0x11d20>
  44291a: 83 c4 04                     	addl	$0x4, %esp
  44291d: 85 c0                        	testl	%eax, %eax
  44291f: 74 04                        	je	0x442925 <.text+0x41925>
  442921: 80 4d 05 10                  	orb	$0x10, 0x5(%ebp)
  442925: 68 d1 01 00 00               	pushl	$0x1d1                  # imm = 0x1D1
  44292a: e8 f1 03 fd ff               	calll	0x412d20 <.text+0x11d20>
  44292f: 83 c4 04                     	addl	$0x4, %esp
  442932: 85 c0                        	testl	%eax, %eax
  442934: 74 04                        	je	0x44293a <.text+0x4193a>
  442936: 80 4d 05 08                  	orb	$0x8, 0x5(%ebp)
  44293a: 33 c0                        	xorl	%eax, %eax
  44293c: 89 06                        	movl	%eax, (%esi)
  44293e: 89 45 40                     	movl	%eax, 0x40(%ebp)
  442941: 89 45 3c                     	movl	%eax, 0x3c(%ebp)
  442944: be 00 00 80 3f               	movl	$0x3f800000, %esi       # imm = 0x3F800000
  442949: 89 77 3c                     	movl	%esi, 0x3c(%edi)
  44294c: 8a 43 02                     	movb	0x2(%ebx), %al
  44294f: 84 c0                        	testb	%al, %al
  442951: 75 04                        	jne	0x442957 <.text+0x41957>
  442953: c6 43 02 01                  	movb	$0x1, 0x2(%ebx)
  442957: 6a 20                        	pushl	$0x20
  442959: 6a 0f                        	pushl	$0xf
  44295b: e8 70 89 fc ff               	calll	0x40b2d0 <.text+0xa2d0>
  442960: 68 60 2b 4a 00               	pushl	$0x4a2b60               # imm = 0x4A2B60
  442965: 89 45 78                     	movl	%eax, 0x78(%ebp)
  442968: 66 c7 45 12 00 00            	movw	$0x0, 0x12(%ebp)
  44296e: 6a 02                        	pushl	$0x2
  442970: 6a 02                        	pushl	$0x2
  442972: 89 73 34                     	movl	%esi, 0x34(%ebx)
  442975: 89 73 30                     	movl	%esi, 0x30(%ebx)
  442978: 89 73 2c                     	movl	%esi, 0x2c(%ebx)
  44297b: e8 e0 8e fc ff               	calll	0x40b860 <.text+0xa860>
  442980: db 44 24 2c                  	fildl	0x2c(%esp)
  442984: 8b 50 20                     	movl	0x20(%eax), %edx
  442987: 83 c4 14                     	addl	$0x14, %esp
  44298a: 5f                           	popl	%edi
  44298b: d9 5a 34                     	fstps	0x34(%edx)
  44298e: 5e                           	popl	%esi
  44298f: 5d                           	popl	%ebp
  442990: 5b                           	popl	%ebx
  442991: 59                           	popl	%ecx
  442992: c3                           	retl
  442993: 90                           	nop
  442994: 15 28 44 00 3a               	adcl	$0x3a004428, %eax       # imm = 0x3A004428
  442999: 29 44 00 54                  	subl	%eax, 0x54(%eax,%eax)
  44299d: 28 44 00 87                  	subb	%al, -0x79(%eax,%eax)
  4429a1: 28 44 00 3a                  	subb	%al, 0x3a(%eax,%eax)
  4429a5: 29 44 00 ba                  	subl	%eax, -0x46(%eax,%eax)
  4429a9: 28 44 00 e6                  	subb	%al, -0x1a(%eax,%eax)
  4429ad: 28 44 00 10                  	subb	%al, 0x10(%eax,%eax)
  4429b1: 29 44 00 90                  	subl	%eax, -0x70(%eax,%eax)
