
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  419030: 51                           	pushl	%ecx
  419031: 57                           	pushl	%edi
  419032: 8b 7c 24 10                  	movl	0x10(%esp), %edi
  419036: 8b 47 04                     	movl	0x4(%edi), %eax
  419039: a8 20                        	testb	$0x20, %al
  41903b: 74 1e                        	je	0x41905b <.text+0x1805b>
  41903d: 8b 57 08                     	movl	0x8(%edi), %edx
  419040: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  419044: 8d 47 08                     	leal	0x8(%edi), %eax
  419047: 89 11                        	movl	%edx, (%ecx)
  419049: 8b 50 04                     	movl	0x4(%eax), %edx
  41904c: 83 c0 04                     	addl	$0x4, %eax
  41904f: 89 51 04                     	movl	%edx, 0x4(%ecx)
  419052: 8b 40 04                     	movl	0x4(%eax), %eax
  419055: 89 41 08                     	movl	%eax, 0x8(%ecx)
  419058: 5f                           	popl	%edi
  419059: 59                           	popl	%ecx
  41905a: c3                           	retl
  41905b: 84 e4                        	testb	%ah, %ah
  41905d: 0f 88 a2 00 00 00            	js	0x419105 <.text+0x18105>
  419063: 56                           	pushl	%esi
  419064: e8 17 8c 36 00               	calll	0x781c80 <.text+0x380c80>
  419069: 33 c9                        	xorl	%ecx, %ecx
  41906b: e8 80 e5 36 00               	calll	0x7875f0 <.text+0x3865f0>
  419070: 8b 74 24 10                  	movl	0x10(%esp), %esi
  419074: 8d 4e 20                     	leal	0x20(%esi), %ecx
  419077: e8 94 8b 36 00               	calll	0x781c10 <.text+0x380c10>
  41907c: 8b 47 04                     	movl	0x4(%edi), %eax
  41907f: f6 c4 02                     	testb	$0x2, %ah
  419082: 74 35                        	je	0x4190b9 <.text+0x180b9>
  419084: 8b 46 18                     	movl	0x18(%esi), %eax
  419087: 85 c0                        	testl	%eax, %eax
  419089: 74 0a                        	je	0x419095 <.text+0x18095>
  41908b: 0f b7 d0                     	movzwl	%ax, %edx
  41908e: 33 c9                        	xorl	%ecx, %ecx
  419090: e8 8b bd 36 00               	calll	0x784e20 <.text+0x383e20>
  419095: 8b 46 1c                     	movl	0x1c(%esi), %eax
  419098: 85 c0                        	testl	%eax, %eax
  41909a: 74 0a                        	je	0x4190a6 <.text+0x180a6>
  41909c: 0f b7 d0                     	movzwl	%ax, %edx
  41909f: 33 c9                        	xorl	%ecx, %ecx
  4190a1: e8 0a bf 36 00               	calll	0x784fb0 <.text+0x383fb0>
  4190a6: 8b 46 14                     	movl	0x14(%esi), %eax
  4190a9: 85 c0                        	testl	%eax, %eax
  4190ab: 74 3f                        	je	0x4190ec <.text+0x180ec>
  4190ad: 0f b7 d0                     	movzwl	%ax, %edx
  4190b0: 33 c9                        	xorl	%ecx, %ecx
  4190b2: e8 29 be 36 00               	calll	0x784ee0 <.text+0x383ee0>
  4190b7: eb 33                        	jmp	0x4190ec <.text+0x180ec>
  4190b9: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4190bc: 85 c0                        	testl	%eax, %eax
  4190be: 74 0a                        	je	0x4190ca <.text+0x180ca>
  4190c0: 0f b7 d0                     	movzwl	%ax, %edx
  4190c3: 33 c9                        	xorl	%ecx, %ecx
  4190c5: e8 e6 be 36 00               	calll	0x784fb0 <.text+0x383fb0>
  4190ca: 8b 46 14                     	movl	0x14(%esi), %eax
  4190cd: 85 c0                        	testl	%eax, %eax
  4190cf: 74 0a                        	je	0x4190db <.text+0x180db>
  4190d1: 0f b7 d0                     	movzwl	%ax, %edx
  4190d4: 33 c9                        	xorl	%ecx, %ecx
  4190d6: e8 05 be 36 00               	calll	0x784ee0 <.text+0x383ee0>
  4190db: 8b 46 18                     	movl	0x18(%esi), %eax
  4190de: 85 c0                        	testl	%eax, %eax
  4190e0: 74 0a                        	je	0x4190ec <.text+0x180ec>
  4190e2: 0f b7 d0                     	movzwl	%ax, %edx
  4190e5: 33 c9                        	xorl	%ecx, %ecx
  4190e7: e8 34 bd 36 00               	calll	0x784e20 <.text+0x383e20>
  4190ec: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  4190f0: 51                           	pushl	%ecx
  4190f1: 8d 57 08                     	leal	0x8(%edi), %edx
  4190f4: 33 c9                        	xorl	%ecx, %ecx
  4190f6: e8 75 85 36 00               	calll	0x781670 <.text+0x380670>
  4190fb: 5e                           	popl	%esi
  4190fc: 5f                           	popl	%edi
  4190fd: 83 c4 04                     	addl	$0x4, %esp
  419100: e9 db 85 36 00               	jmp	0x7816e0 <.text+0x3806e0>
  419105: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  419109: d9 40 20                     	flds	0x20(%eax)
  41910c: 8b 57 08                     	movl	0x8(%edi), %edx
  41910f: 83 c0 20                     	addl	$0x20, %eax
  419112: d9 40 04                     	flds	0x4(%eax)
  419115: 83 c0 04                     	addl	$0x4, %eax
  419118: d9 40 04                     	flds	0x4(%eax)
  41911b: 8d 47 08                     	leal	0x8(%edi), %eax
  41911e: d9 40 08                     	flds	0x8(%eax)
  419121: 8b 48 04                     	movl	0x4(%eax), %ecx
  419124: d8 c1                        	fadd	%st(1), %st
  419126: 83 c0 04                     	addl	$0x4, %eax
  419129: 8b 44 24 14                  	movl	0x14(%esp), %eax
  41912d: 83 c0 08                     	addl	$0x8, %eax
  419130: d9 18                        	fstps	(%eax)
  419132: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  419136: dd d8                        	fstp	%st(0)
  419138: 83 e8 04                     	subl	$0x4, %eax
  41913b: d9 44 24 10                  	flds	0x10(%esp)
  41913f: 89 54 24 04                  	movl	%edx, 0x4(%esp)
  419143: d8 c1                        	fadd	%st(1), %st
  419145: 5f                           	popl	%edi
  419146: d9 18                        	fstps	(%eax)
  419148: dd d8                        	fstp	%st(0)
  41914a: d9 44 24 00                  	flds	(%esp)
  41914e: d8 c1                        	fadd	%st(1), %st
  419150: d9 58 fc                     	fstps	-0x4(%eax)
  419153: dd d8                        	fstp	%st(0)
  419155: 59                           	popl	%ecx
  419156: c3                           	retl
  419157: 90                           	nop
  419158: 90                           	nop
  419159: 90                           	nop
  41915a: 90                           	nop
  41915b: 90                           	nop
  41915c: 90                           	nop
  41915d: 90                           	nop
  41915e: 90                           	nop
  41915f: 90                           	nop
  419160: 8b 47 04                     	movl	0x4(%edi), %eax
  419163: 83 ec 08                     	subl	$0x8, %esp
  419166: a8 20                        	testb	$0x20, %al
  419168: 74 1b                        	je	0x419185 <.text+0x18185>
  41916a: 8b 4f 08                     	movl	0x8(%edi), %ecx
  41916d: 8d 47 08                     	leal	0x8(%edi), %eax
  419170: 89 0b                        	movl	%ecx, (%ebx)
  419172: 8b 50 04                     	movl	0x4(%eax), %edx
  419175: 83 c0 04                     	addl	$0x4, %eax
  419178: 89 53 04                     	movl	%edx, 0x4(%ebx)
  41917b: 8b 40 04                     	movl	0x4(%eax), %eax
  41917e: 89 43 08                     	movl	%eax, 0x8(%ebx)
  419181: 83 c4 08                     	addl	$0x8, %esp
  419184: c3                           	retl
  419185: 84 e4                        	testb	%ah, %ah
  419187: 0f 88 97 00 00 00            	js	0x419224 <.text+0x18224>
  41918d: 33 c9                        	xorl	%ecx, %ecx
  41918f: e8 5c e4 36 00               	calll	0x7875f0 <.text+0x3865f0>
  419194: 8d 4e 20                     	leal	0x20(%esi), %ecx
  419197: e8 74 8a 36 00               	calll	0x781c10 <.text+0x380c10>
  41919c: 8b 47 04                     	movl	0x4(%edi), %eax
  41919f: f6 c4 02                     	testb	$0x2, %ah
  4191a2: 74 35                        	je	0x4191d9 <.text+0x181d9>
  4191a4: 8b 46 18                     	movl	0x18(%esi), %eax
  4191a7: 85 c0                        	testl	%eax, %eax
  4191a9: 74 0a                        	je	0x4191b5 <.text+0x181b5>
  4191ab: 0f b7 d0                     	movzwl	%ax, %edx
  4191ae: 33 c9                        	xorl	%ecx, %ecx
  4191b0: e8 6b bc 36 00               	calll	0x784e20 <.text+0x383e20>
  4191b5: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4191b8: 85 c0                        	testl	%eax, %eax
  4191ba: 74 0a                        	je	0x4191c6 <.text+0x181c6>
  4191bc: 0f b7 d0                     	movzwl	%ax, %edx
  4191bf: 33 c9                        	xorl	%ecx, %ecx
  4191c1: e8 ea bd 36 00               	calll	0x784fb0 <.text+0x383fb0>
  4191c6: 8b 46 14                     	movl	0x14(%esi), %eax
  4191c9: 85 c0                        	testl	%eax, %eax
  4191cb: 74 3f                        	je	0x41920c <.text+0x1820c>
  4191cd: 0f b7 d0                     	movzwl	%ax, %edx
  4191d0: 33 c9                        	xorl	%ecx, %ecx
  4191d2: e8 09 bd 36 00               	calll	0x784ee0 <.text+0x383ee0>
  4191d7: eb 33                        	jmp	0x41920c <.text+0x1820c>
  4191d9: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4191dc: 85 c0                        	testl	%eax, %eax
  4191de: 74 0a                        	je	0x4191ea <.text+0x181ea>
  4191e0: 0f b7 d0                     	movzwl	%ax, %edx
  4191e3: 33 c9                        	xorl	%ecx, %ecx
  4191e5: e8 c6 bd 36 00               	calll	0x784fb0 <.text+0x383fb0>
  4191ea: 8b 46 14                     	movl	0x14(%esi), %eax
  4191ed: 85 c0                        	testl	%eax, %eax
  4191ef: 74 0a                        	je	0x4191fb <.text+0x181fb>
  4191f1: 0f b7 d0                     	movzwl	%ax, %edx
  4191f4: 33 c9                        	xorl	%ecx, %ecx
  4191f6: e8 e5 bc 36 00               	calll	0x784ee0 <.text+0x383ee0>
  4191fb: 8b 46 18                     	movl	0x18(%esi), %eax
  4191fe: 85 c0                        	testl	%eax, %eax
  419200: 74 0a                        	je	0x41920c <.text+0x1820c>
  419202: 0f b7 d0                     	movzwl	%ax, %edx
  419205: 33 c9                        	xorl	%ecx, %ecx
  419207: e8 14 bc 36 00               	calll	0x784e20 <.text+0x383e20>
  41920c: 53                           	pushl	%ebx
  41920d: 8d 57 08                     	leal	0x8(%edi), %edx
  419210: 33 c9                        	xorl	%ecx, %ecx
  419212: e8 59 84 36 00               	calll	0x781670 <.text+0x380670>
  419217: e8 c4 84 36 00               	calll	0x7816e0 <.text+0x3806e0>
  41921c: 83 c4 08                     	addl	$0x8, %esp
  41921f: e9 5c 8a 36 00               	jmp	0x781c80 <.text+0x380c80>
  419224: d9 46 20                     	flds	0x20(%esi)
  419227: 8b 4f 08                     	movl	0x8(%edi), %ecx
  41922a: 8d 46 20                     	leal	0x20(%esi), %eax
  41922d: d9 40 04                     	flds	0x4(%eax)
  419230: 83 c0 04                     	addl	$0x4, %eax
  419233: d9 40 04                     	flds	0x4(%eax)
  419236: 8d 47 08                     	leal	0x8(%edi), %eax
  419239: d9 40 08                     	flds	0x8(%eax)
  41923c: 8b 50 04                     	movl	0x4(%eax), %edx
  41923f: d8 c1                        	fadd	%st(1), %st
  419241: 83 c0 04                     	addl	$0x4, %eax
  419244: 8d 43 08                     	leal	0x8(%ebx), %eax
  419247: 89 54 24 00                  	movl	%edx, (%esp)
  41924b: d9 18                        	fstps	(%eax)
  41924d: 83 e8 04                     	subl	$0x4, %eax
  419250: dd d8                        	fstp	%st(0)
  419252: 89 4c 24 04                  	movl	%ecx, 0x4(%esp)
  419256: d9 44 24 00                  	flds	(%esp)
  41925a: d8 c1                        	fadd	%st(1), %st
  41925c: d9 18                        	fstps	(%eax)
  41925e: dd d8                        	fstp	%st(0)
  419260: d9 44 24 04                  	flds	0x4(%esp)
  419264: d8 c1                        	fadd	%st(1), %st
  419266: d9 58 fc                     	fstps	-0x4(%eax)
  419269: dd d8                        	fstp	%st(0)
  41926b: 83 c4 08                     	addl	$0x8, %esp
  41926e: c3                           	retl
  41926f: 90                           	nop
  419270: 51                           	pushl	%ecx
  419271: 57                           	pushl	%edi
  419272: 8b 7c 24 10                  	movl	0x10(%esp), %edi
  419276: 8b 47 04                     	movl	0x4(%edi), %eax
  419279: a8 20                        	testb	$0x20, %al
  41927b: 74 18                        	je	0x419295 <.text+0x18295>
  41927d: 8b 4f 08                     	movl	0x8(%edi), %ecx
  419280: 8b 44 24 14                  	movl	0x14(%esp), %eax
  419284: 89 08                        	movl	%ecx, (%eax)
  419286: 8b 57 0c                     	movl	0xc(%edi), %edx
  419289: 89 50 04                     	movl	%edx, 0x4(%eax)
  41928c: 8b 4f 10                     	movl	0x10(%edi), %ecx
  41928f: 89 48 08                     	movl	%ecx, 0x8(%eax)
  419292: 5f                           	popl	%edi
  419293: 59                           	popl	%ecx
  419294: c3                           	retl
  419295: 84 e4                        	testb	%ah, %ah
  419297: 0f 88 a2 00 00 00            	js	0x41933f <.text+0x1833f>
  41929d: 56                           	pushl	%esi
  41929e: e8 dd 89 36 00               	calll	0x781c80 <.text+0x380c80>
  4192a3: 33 c9                        	xorl	%ecx, %ecx
  4192a5: e8 46 e3 36 00               	calll	0x7875f0 <.text+0x3865f0>
  4192aa: 8b 74 24 10                  	movl	0x10(%esp), %esi
  4192ae: 8d 4e 20                     	leal	0x20(%esi), %ecx
  4192b1: e8 5a 89 36 00               	calll	0x781c10 <.text+0x380c10>
  4192b6: 8b 47 04                     	movl	0x4(%edi), %eax
  4192b9: f6 c4 02                     	testb	$0x2, %ah
  4192bc: 74 35                        	je	0x4192f3 <.text+0x182f3>
  4192be: 8b 46 18                     	movl	0x18(%esi), %eax
  4192c1: 85 c0                        	testl	%eax, %eax
  4192c3: 74 0a                        	je	0x4192cf <.text+0x182cf>
  4192c5: 0f b7 d0                     	movzwl	%ax, %edx
  4192c8: 33 c9                        	xorl	%ecx, %ecx
  4192ca: e8 51 bb 36 00               	calll	0x784e20 <.text+0x383e20>
  4192cf: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4192d2: 85 c0                        	testl	%eax, %eax
  4192d4: 74 0a                        	je	0x4192e0 <.text+0x182e0>
  4192d6: 0f b7 d0                     	movzwl	%ax, %edx
  4192d9: 33 c9                        	xorl	%ecx, %ecx
  4192db: e8 d0 bc 36 00               	calll	0x784fb0 <.text+0x383fb0>
  4192e0: 8b 46 14                     	movl	0x14(%esi), %eax
  4192e3: 85 c0                        	testl	%eax, %eax
  4192e5: 74 3f                        	je	0x419326 <.text+0x18326>
  4192e7: 0f b7 d0                     	movzwl	%ax, %edx
  4192ea: 33 c9                        	xorl	%ecx, %ecx
  4192ec: e8 ef bb 36 00               	calll	0x784ee0 <.text+0x383ee0>
  4192f1: eb 33                        	jmp	0x419326 <.text+0x18326>
  4192f3: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4192f6: 85 c0                        	testl	%eax, %eax
  4192f8: 74 0a                        	je	0x419304 <.text+0x18304>
  4192fa: 0f b7 d0                     	movzwl	%ax, %edx
  4192fd: 33 c9                        	xorl	%ecx, %ecx
  4192ff: e8 ac bc 36 00               	calll	0x784fb0 <.text+0x383fb0>
  419304: 8b 46 14                     	movl	0x14(%esi), %eax
  419307: 85 c0                        	testl	%eax, %eax
  419309: 74 0a                        	je	0x419315 <.text+0x18315>
  41930b: 0f b7 d0                     	movzwl	%ax, %edx
  41930e: 33 c9                        	xorl	%ecx, %ecx
  419310: e8 cb bb 36 00               	calll	0x784ee0 <.text+0x383ee0>
  419315: 8b 46 18                     	movl	0x18(%esi), %eax
  419318: 85 c0                        	testl	%eax, %eax
  41931a: 74 0a                        	je	0x419326 <.text+0x18326>
  41931c: 0f b7 d0                     	movzwl	%ax, %edx
  41931f: 33 c9                        	xorl	%ecx, %ecx
  419321: e8 fa ba 36 00               	calll	0x784e20 <.text+0x383e20>
  419326: 8b 54 24 18                  	movl	0x18(%esp), %edx
  41932a: 52                           	pushl	%edx
  41932b: 8d 57 08                     	leal	0x8(%edi), %edx
  41932e: 33 c9                        	xorl	%ecx, %ecx
  419330: e8 3b 83 36 00               	calll	0x781670 <.text+0x380670>
  419335: 5e                           	popl	%esi
  419336: 5f                           	popl	%edi
  419337: 83 c4 04                     	addl	$0x4, %esp
  41933a: e9 a1 83 36 00               	jmp	0x7816e0 <.text+0x3806e0>
  41933f: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  419343: d9 40 20                     	flds	0x20(%eax)
  419346: 8b 4f 08                     	movl	0x8(%edi), %ecx
  419349: 83 c0 20                     	addl	$0x20, %eax
  41934c: d9 40 04                     	flds	0x4(%eax)
  41934f: 83 c0 04                     	addl	$0x4, %eax
  419352: d9 40 04                     	flds	0x4(%eax)
  419355: 8d 47 08                     	leal	0x8(%edi), %eax
  419358: d9 40 08                     	flds	0x8(%eax)
  41935b: 8b 50 04                     	movl	0x4(%eax), %edx
  41935e: d8 c1                        	fadd	%st(1), %st
  419360: 83 c0 04                     	addl	$0x4, %eax
  419363: 8b 44 24 14                  	movl	0x14(%esp), %eax
  419367: 83 c0 08                     	addl	$0x8, %eax
  41936a: d9 18                        	fstps	(%eax)
  41936c: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  419370: dd d8                        	fstp	%st(0)
  419372: 83 e8 04                     	subl	$0x4, %eax
  419375: d9 44 24 10                  	flds	0x10(%esp)
  419379: 89 4c 24 04                  	movl	%ecx, 0x4(%esp)
  41937d: d8 c1                        	fadd	%st(1), %st
  41937f: 5f                           	popl	%edi
  419380: d9 18                        	fstps	(%eax)
  419382: dd d8                        	fstp	%st(0)
  419384: d9 44 24 00                  	flds	(%esp)
  419388: d8 c1                        	fadd	%st(1), %st
  41938a: d9 58 fc                     	fstps	-0x4(%eax)
  41938d: dd d8                        	fstp	%st(0)
  41938f: 59                           	popl	%ecx
  419390: c3                           	retl
  419391: 90                           	nop
  419392: 90                           	nop
  419393: 90                           	nop
  419394: 90                           	nop
  419395: 90                           	nop
  419396: 90                           	nop
  419397: 90                           	nop
  419398: 90                           	nop
  419399: 90                           	nop
  41939a: 90                           	nop
  41939b: 90                           	nop
  41939c: 90                           	nop
  41939d: 90                           	nop
  41939e: 90                           	nop
  41939f: 90                           	nop
  4193a0: 57                           	pushl	%edi
  4193a1: 8b 7c 24 0c                  	movl	0xc(%esp), %edi
  4193a5: f6 47 04 20                  	testb	$0x20, 0x4(%edi)
  4193a9: 74 17                        	je	0x4193c2 <.text+0x183c2>
  4193ab: 8b 4f 08                     	movl	0x8(%edi), %ecx
  4193ae: 8b 44 24 10                  	movl	0x10(%esp), %eax
  4193b2: 89 08                        	movl	%ecx, (%eax)
  4193b4: 8b 57 0c                     	movl	0xc(%edi), %edx
  4193b7: 89 50 04                     	movl	%edx, 0x4(%eax)
  4193ba: 8b 4f 10                     	movl	0x10(%edi), %ecx
  4193bd: 89 48 08                     	movl	%ecx, 0x8(%eax)
  4193c0: 5f                           	popl	%edi
  4193c1: c3                           	retl
  4193c2: 56                           	pushl	%esi
  4193c3: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  4193c8: e8 23 bd 36 00               	calll	0x7850f0 <.text+0x3840f0>
  4193cd: 8b 74 24 0c                  	movl	0xc(%esp), %esi
  4193d1: 8d 4e 20                     	leal	0x20(%esi), %ecx
  4193d4: e8 37 88 36 00               	calll	0x781c10 <.text+0x380c10>
  4193d9: 8b 47 04                     	movl	0x4(%edi), %eax
  4193dc: 84 e4                        	testb	%ah, %ah
  4193de: 78 6d                        	js	0x41944d <.text+0x1844d>
  4193e0: f6 c4 02                     	testb	$0x2, %ah
  4193e3: 74 35                        	je	0x41941a <.text+0x1841a>
  4193e5: 8b 46 18                     	movl	0x18(%esi), %eax
  4193e8: 85 c0                        	testl	%eax, %eax
  4193ea: 74 0a                        	je	0x4193f6 <.text+0x183f6>
  4193ec: 0f b7 d0                     	movzwl	%ax, %edx
  4193ef: 33 c9                        	xorl	%ecx, %ecx
  4193f1: e8 2a ba 36 00               	calll	0x784e20 <.text+0x383e20>
  4193f6: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4193f9: 85 c0                        	testl	%eax, %eax
  4193fb: 74 0a                        	je	0x419407 <.text+0x18407>
  4193fd: 0f b7 d0                     	movzwl	%ax, %edx
  419400: 33 c9                        	xorl	%ecx, %ecx
  419402: e8 a9 bb 36 00               	calll	0x784fb0 <.text+0x383fb0>
  419407: 8b 46 14                     	movl	0x14(%esi), %eax
  41940a: 85 c0                        	testl	%eax, %eax
  41940c: 74 3f                        	je	0x41944d <.text+0x1844d>
  41940e: 0f b7 d0                     	movzwl	%ax, %edx
  419411: 33 c9                        	xorl	%ecx, %ecx
  419413: e8 c8 ba 36 00               	calll	0x784ee0 <.text+0x383ee0>
  419418: eb 33                        	jmp	0x41944d <.text+0x1844d>
  41941a: 8b 46 1c                     	movl	0x1c(%esi), %eax
  41941d: 85 c0                        	testl	%eax, %eax
  41941f: 74 0a                        	je	0x41942b <.text+0x1842b>
  419421: 0f b7 d0                     	movzwl	%ax, %edx
  419424: 33 c9                        	xorl	%ecx, %ecx
  419426: e8 85 bb 36 00               	calll	0x784fb0 <.text+0x383fb0>
  41942b: 8b 46 14                     	movl	0x14(%esi), %eax
  41942e: 85 c0                        	testl	%eax, %eax
  419430: 74 0a                        	je	0x41943c <.text+0x1843c>
  419432: 0f b7 d0                     	movzwl	%ax, %edx
  419435: 33 c9                        	xorl	%ecx, %ecx
  419437: e8 a4 ba 36 00               	calll	0x784ee0 <.text+0x383ee0>
  41943c: 8b 46 18                     	movl	0x18(%esi), %eax
  41943f: 85 c0                        	testl	%eax, %eax
  419441: 74 0a                        	je	0x41944d <.text+0x1844d>
  419443: 0f b7 d0                     	movzwl	%ax, %edx
  419446: 33 c9                        	xorl	%ecx, %ecx
  419448: e8 d3 b9 36 00               	calll	0x784e20 <.text+0x383e20>
  41944d: 8b 54 24 14                  	movl	0x14(%esp), %edx
  419451: 52                           	pushl	%edx
  419452: 8d 57 08                     	leal	0x8(%edi), %edx
  419455: 33 c9                        	xorl	%ecx, %ecx
  419457: e8 14 82 36 00               	calll	0x781670 <.text+0x380670>
  41945c: 5e                           	popl	%esi
  41945d: 5f                           	popl	%edi
  41945e: e9 7d 82 36 00               	jmp	0x7816e0 <.text+0x3806e0>
  419463: 90                           	nop
  419464: 90                           	nop
  419465: 90                           	nop
  419466: 90                           	nop
  419467: 90                           	nop
  419468: 90                           	nop
  419469: 90                           	nop
  41946a: 90                           	nop
  41946b: 90                           	nop
  41946c: 90                           	nop
  41946d: 90                           	nop
  41946e: 90                           	nop
  41946f: 90                           	nop
  419470: 83 ec 0c                     	subl	$0xc, %esp
  419473: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  419478: c7 44 24 00 00 00 00 00      	movl	$0x0, (%esp)
  419480: c7 44 24 04 00 00 80 3f      	movl	$0x3f800000, 0x4(%esp)  # imm = 0x3F800000
  419488: c7 44 24 08 00 00 00 00      	movl	$0x0, 0x8(%esp)
  419490: e8 5b bc 36 00               	calll	0x7850f0 <.text+0x3840f0>
  419495: 8d 4e 20                     	leal	0x20(%esi), %ecx
  419498: e8 73 87 36 00               	calll	0x781c10 <.text+0x380c10>
  41949d: 8b 47 04                     	movl	0x4(%edi), %eax
  4194a0: f6 c4 02                     	testb	$0x2, %ah
  4194a3: 74 70                        	je	0x419515 <.text+0x18515>
  4194a5: a8 20                        	testb	$0x20, %al
  4194a7: 75 37                        	jne	0x4194e0 <.text+0x184e0>
  4194a9: 84 e4                        	testb	%ah, %ah
  4194ab: 78 33                        	js	0x4194e0 <.text+0x184e0>
  4194ad: 8b 46 18                     	movl	0x18(%esi), %eax
  4194b0: 85 c0                        	testl	%eax, %eax
  4194b2: 74 0a                        	je	0x4194be <.text+0x184be>
  4194b4: 0f b7 d0                     	movzwl	%ax, %edx
  4194b7: 33 c9                        	xorl	%ecx, %ecx
  4194b9: e8 62 b9 36 00               	calll	0x784e20 <.text+0x383e20>
  4194be: 8b 46 1c                     	movl	0x1c(%esi), %eax
  4194c1: 85 c0                        	testl	%eax, %eax
  4194c3: 74 0a                        	je	0x4194cf <.text+0x184cf>
  4194c5: 0f b7 d0                     	movzwl	%ax, %edx
  4194c8: 33 c9                        	xorl	%ecx, %ecx
  4194ca: e8 e1 ba 36 00               	calll	0x784fb0 <.text+0x383fb0>
  4194cf: 8b 46 14                     	movl	0x14(%esi), %eax
  4194d2: 85 c0                        	testl	%eax, %eax
  4194d4: 74 0a                        	je	0x4194e0 <.text+0x184e0>
  4194d6: 0f b7 d0                     	movzwl	%ax, %edx
  4194d9: 33 c9                        	xorl	%ecx, %ecx
  4194db: e8 00 ba 36 00               	calll	0x784ee0 <.text+0x383ee0>
  4194e0: 8b 47 28                     	movl	0x28(%edi), %eax
  4194e3: 85 c0                        	testl	%eax, %eax
  4194e5: 74 0a                        	je	0x4194f1 <.text+0x184f1>
  4194e7: 0f b7 d0                     	movzwl	%ax, %edx
  4194ea: 33 c9                        	xorl	%ecx, %ecx
  4194ec: e8 2f b9 36 00               	calll	0x784e20 <.text+0x383e20>
  4194f1: 8b 47 2c                     	movl	0x2c(%edi), %eax
  4194f4: 85 c0                        	testl	%eax, %eax
  4194f6: 74 0a                        	je	0x419502 <.text+0x18502>
  4194f8: 0f b7 d0                     	movzwl	%ax, %edx
  4194fb: 33 c9                        	xorl	%ecx, %ecx
  4194fd: e8 ae ba 36 00               	calll	0x784fb0 <.text+0x383fb0>
  419502: 8b 47 24                     	movl	0x24(%edi), %eax
  419505: 85 c0                        	testl	%eax, %eax
  419507: 74 7a                        	je	0x419583 <.text+0x18583>
  419509: 0f b7 d0                     	movzwl	%ax, %edx
  41950c: 33 c9                        	xorl	%ecx, %ecx
  41950e: e8 cd b9 36 00               	calll	0x784ee0 <.text+0x383ee0>
  419513: eb 6e                        	jmp	0x419583 <.text+0x18583>
  419515: a8 20                        	testb	$0x20, %al
  419517: 75 37                        	jne	0x419550 <.text+0x18550>
  419519: 84 e4                        	testb	%ah, %ah
  41951b: 78 33                        	js	0x419550 <.text+0x18550>
  41951d: 8b 46 1c                     	movl	0x1c(%esi), %eax
  419520: 85 c0                        	testl	%eax, %eax
  419522: 74 0a                        	je	0x41952e <.text+0x1852e>
  419524: 0f b7 d0                     	movzwl	%ax, %edx
  419527: 33 c9                        	xorl	%ecx, %ecx
  419529: e8 82 ba 36 00               	calll	0x784fb0 <.text+0x383fb0>
  41952e: 8b 46 14                     	movl	0x14(%esi), %eax
  419531: 85 c0                        	testl	%eax, %eax
  419533: 74 0a                        	je	0x41953f <.text+0x1853f>
  419535: 0f b7 d0                     	movzwl	%ax, %edx
  419538: 33 c9                        	xorl	%ecx, %ecx
  41953a: e8 a1 b9 36 00               	calll	0x784ee0 <.text+0x383ee0>
  41953f: 8b 46 18                     	movl	0x18(%esi), %eax
  419542: 85 c0                        	testl	%eax, %eax
  419544: 74 0a                        	je	0x419550 <.text+0x18550>
  419546: 0f b7 d0                     	movzwl	%ax, %edx
  419549: 33 c9                        	xorl	%ecx, %ecx
  41954b: e8 d0 b8 36 00               	calll	0x784e20 <.text+0x383e20>
  419550: 8b 47 2c                     	movl	0x2c(%edi), %eax
  419553: 85 c0                        	testl	%eax, %eax
  419555: 74 0a                        	je	0x419561 <.text+0x18561>
  419557: 0f b7 d0                     	movzwl	%ax, %edx
  41955a: 33 c9                        	xorl	%ecx, %ecx
  41955c: e8 4f ba 36 00               	calll	0x784fb0 <.text+0x383fb0>
  419561: 8b 47 24                     	movl	0x24(%edi), %eax
  419564: 85 c0                        	testl	%eax, %eax
  419566: 74 0a                        	je	0x419572 <.text+0x18572>
  419568: 0f b7 d0                     	movzwl	%ax, %edx
  41956b: 33 c9                        	xorl	%ecx, %ecx
  41956d: e8 6e b9 36 00               	calll	0x784ee0 <.text+0x383ee0>
  419572: 8b 47 28                     	movl	0x28(%edi), %eax
  419575: 85 c0                        	testl	%eax, %eax
  419577: 74 0a                        	je	0x419583 <.text+0x18583>
  419579: 0f b7 d0                     	movzwl	%ax, %edx
  41957c: 33 c9                        	xorl	%ecx, %ecx
  41957e: e8 9d b8 36 00               	calll	0x784e20 <.text+0x383e20>
  419583: 8b 44 24 10                  	movl	0x10(%esp), %eax
  419587: 50                           	pushl	%eax
  419588: 8d 54 24 04                  	leal	0x4(%esp), %edx
  41958c: 33 c9                        	xorl	%ecx, %ecx
  41958e: e8 cd b7 36 00               	calll	0x784d60 <.text+0x383d60>
  419593: 83 c4 0c                     	addl	$0xc, %esp
  419596: e9 45 81 36 00               	jmp	0x7816e0 <.text+0x3806e0>
  41959b: 90                           	nop
  41959c: 90                           	nop
  41959d: 90                           	nop
  41959e: 90                           	nop
  41959f: 90                           	nop
  4195a0: 83 ec 34                     	subl	$0x34, %esp
  4195a3: 8b 4c 24 40                  	movl	0x40(%esp), %ecx
  4195a7: 8b 51 38                     	movl	0x38(%ecx), %edx
  4195aa: 8b 4c 24 44                  	movl	0x44(%esp), %ecx
  4195ae: 53                           	pushl	%ebx
  4195af: 55                           	pushl	%ebp
  4195b0: 8b 6c 24 40                  	movl	0x40(%esp), %ebp
  4195b4: 8b 45 38                     	movl	0x38(%ebp), %eax
  4195b7: 56                           	pushl	%esi
  4195b8: 57                           	pushl	%edi
  4195b9: 8b 7c 24 4c                  	movl	0x4c(%esp), %edi
  4195bd: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  4195c1: 8b 47 14                     	movl	0x14(%edi), %eax
  4195c4: 89 54 24 10                  	movl	%edx, 0x10(%esp)
  4195c8: 8b 51 14                     	movl	0x14(%ecx), %edx
  4195cb: 8d 5c 24 2c                  	leal	0x2c(%esp), %ebx
  4195cf: 8b f5                        	movl	%ebp, %esi
  4195d1: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  4195d5: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  4195d9: e8 82 fb ff ff               	calll	0x419160 <.text+0x18160>
  4195de: 8b 7c 24 54                  	movl	0x54(%esp), %edi
  4195e2: 8b 74 24 50                  	movl	0x50(%esp), %esi
  4195e6: 8d 5c 24 38                  	leal	0x38(%esp), %ebx
  4195ea: e8 71 fb ff ff               	calll	0x419160 <.text+0x18160>
  4195ef: d9 44 24 34                  	flds	0x34(%esp)
  4195f3: d8 64 24 40                  	fsubs	0x40(%esp)
  4195f7: d9 44 24 30                  	flds	0x30(%esp)
  4195fb: d8 64 24 3c                  	fsubs	0x3c(%esp)
  4195ff: d9 44 24 2c                  	flds	0x2c(%esp)
  419603: d8 64 24 38                  	fsubs	0x38(%esp)
  419607: d9 c0                        	fld	%st(0)
  419609: d8 c9                        	fmul	%st(1), %st
  41960b: d9 c2                        	fld	%st(2)
  41960d: d8 cb                        	fmul	%st(3), %st
  41960f: de c1                        	faddp	%st, %st(1)
  419611: d9 c3                        	fld	%st(3)
  419613: d8 cc                        	fmul	%st(4), %st
  419615: de c1                        	faddp	%st, %st(1)
  419617: d9 5c 24 48                  	fstps	0x48(%esp)
  41961b: dd d8                        	fstp	%st(0)
  41961d: dd d8                        	fstp	%st(0)
  41961f: dd d8                        	fstp	%st(0)
  419621: d9 44 24 14                  	flds	0x14(%esp)
  419625: d8 44 24 18                  	fadds	0x18(%esp)
  419629: d9 54 24 18                  	fsts	0x18(%esp)
  41962d: d9 c0                        	fld	%st(0)
  41962f: d8 c9                        	fmul	%st(1), %st
  419631: d9 44 24 48                  	flds	0x48(%esp)
  419635: de d9                        	fcompp
  419637: df e0                        	fnstsw	%ax
  419639: dd d8                        	fstp	%st(0)
  41963b: f6 c4 05                     	testb	$0x5, %ah
  41963e: 0f 8a 62 01 00 00            	jp	0x4197a6 <.text+0x187a6>
  419644: 8b df                        	movl	%edi, %ebx
  419646: 53                           	pushl	%ebx
  419647: 8b fe                        	movl	%esi, %edi
  419649: 8b 74 24 50                  	movl	0x50(%esp), %esi
  41964d: 57                           	pushl	%edi
  41964e: 56                           	pushl	%esi
  41964f: 55                           	pushl	%ebp
  419650: e8 1b f8 ff ff               	calll	0x418e70 <.text+0x17e70>
  419655: 56                           	pushl	%esi
  419656: 55                           	pushl	%ebp
  419657: 8b c3                        	movl	%ebx, %eax
  419659: e8 82 f7 ff ff               	calll	0x418de0 <.text+0x17de0>
  41965e: d9 44 24 44                  	flds	0x44(%esp)
  419662: d8 64 24 50                  	fsubs	0x50(%esp)
  419666: 8b 44 24 60                  	movl	0x60(%esp), %eax
  41966a: 83 c4 18                     	addl	$0x18, %esp
  41966d: 50                           	pushl	%eax
  41966e: d9 5c 24 24                  	fstps	0x24(%esp)
  419672: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%esp)
  41967a: d9 44 24 38                  	flds	0x38(%esp)
  41967e: d8 64 24 44                  	fsubs	0x44(%esp)
  419682: d9 5c 24 2c                  	fstps	0x2c(%esp)
  419686: e8 65 db 36 00               	calll	0x7871f0 <.text+0x3861f0>
  41968b: d8 6c 24 18                  	fsubrs	0x18(%esp)
  41968f: 8d 4c 24 20                  	leal	0x20(%esp), %ecx
  419693: d9 5c 24 54                  	fstps	0x54(%esp)
  419697: e8 24 e6 36 00               	calll	0x787cc0 <.text+0x386cc0>
  41969c: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4196a2: df e0                        	fnstsw	%ax
  4196a4: f6 c4 44                     	testb	$0x44, %ah
  4196a7: 7a 1a                        	jp	0x4196c3 <.text+0x186c3>
  4196a9: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  4196b1: c7 44 24 20 00 00 00 00      	movl	$0x0, 0x20(%esp)
  4196b9: c7 44 24 28 cd cc cc 3d      	movl	$0x3dcccccd, 0x28(%esp) # imm = 0x3DCCCCCD
  4196c1: eb 0f                        	jmp	0x4196d2 <.text+0x186d2>
  4196c3: 8d 4c 24 20                  	leal	0x20(%esp), %ecx
  4196c7: 51                           	pushl	%ecx
  4196c8: e8 f3 f3 ff ff               	calll	0x418ac0 <.text+0x17ac0>
  4196cd: dd d8                        	fstp	%st(0)
  4196cf: 83 c4 04                     	addl	$0x4, %esp
  4196d2: d9 44 24 54                  	flds	0x54(%esp)
  4196d6: 8b 54 24 20                  	movl	0x20(%esp), %edx
  4196da: d8 0d 44 d5 7d 00            	fmuls	0x7dd544
  4196e0: 8b 74 24 1c                  	movl	0x1c(%esp), %esi
  4196e4: d9 44 24 20                  	flds	0x20(%esp)
  4196e8: 8b 44 24 28                  	movl	0x28(%esp), %eax
  4196ec: 89 96 90 00 00 00            	movl	%edx, 0x90(%esi)
  4196f2: d8 c9                        	fmul	%st(1), %st
  4196f4: 89 86 98 00 00 00            	movl	%eax, 0x98(%esi)
  4196fa: 8b 4d 38                     	movl	0x38(%ebp), %ecx
  4196fd: 8b 91 9c 00 00 00            	movl	0x9c(%ecx), %edx
  419703: 8b 42 20                     	movl	0x20(%edx), %eax
  419706: d9 5c 24 54                  	fstps	0x54(%esp)
  41970a: d9 44 24 24                  	flds	0x24(%esp)
  41970e: 8b 4f 38                     	movl	0x38(%edi), %ecx
  419711: 0f b6 40 09                  	movzbl	0x9(%eax), %eax
  419715: d8 c9                        	fmul	%st(1), %st
  419717: 85 c0                        	testl	%eax, %eax
  419719: 8b 91 9c 00 00 00            	movl	0x9c(%ecx), %edx
  41971f: 8b 4a 20                     	movl	0x20(%edx), %ecx
  419722: d9 5c 24 50                  	fstps	0x50(%esp)
  419726: 0f b6 49 09                  	movzbl	0x9(%ecx), %ecx
  41972a: d8 4c 24 28                  	fmuls	0x28(%esp)
  41972e: d9 5c 24 14                  	fstps	0x14(%esp)
  419732: 75 05                        	jne	0x419739 <.text+0x18739>
  419734: 83 f9 02                     	cmpl	$0x2, %ecx
  419737: eb 07                        	jmp	0x419740 <.text+0x18740>
  419739: 83 f8 02                     	cmpl	$0x2, %eax
  41973c: 75 04                        	jne	0x419742 <.text+0x18742>
  41973e: 85 c9                        	testl	%ecx, %ecx
  419740: 74 64                        	je	0x4197a6 <.text+0x187a6>
  419742: 8b 54 24 10                  	movl	0x10(%esp), %edx
  419746: 8b 44 24 4c                  	movl	0x4c(%esp), %eax
  41974a: 8b cb                        	movl	%ebx, %ecx
  41974c: e8 2f f6 ff ff               	calll	0x418d80 <.text+0x17d80>
  419751: 85 c0                        	testl	%eax, %eax
  419753: 74 1e                        	je	0x419773 <.text+0x18773>
  419755: d9 44 24 54                  	flds	0x54(%esp)
  419759: d8 45 20                     	fadds	0x20(%ebp)
  41975c: d9 5d 20                     	fstps	0x20(%ebp)
  41975f: d9 44 24 50                  	flds	0x50(%esp)
  419763: d8 45 24                     	fadds	0x24(%ebp)
  419766: d9 5d 24                     	fstps	0x24(%ebp)
  419769: d9 44 24 14                  	flds	0x14(%esp)
  41976d: d8 45 28                     	fadds	0x28(%ebp)
  419770: d9 5d 28                     	fstps	0x28(%ebp)
  419773: 8b 4c 24 4c                  	movl	0x4c(%esp), %ecx
  419777: 8b d6                        	movl	%esi, %edx
  419779: 8b 74 24 10                  	movl	0x10(%esp), %esi
  41977d: 8b c3                        	movl	%ebx, %eax
  41977f: e8 fc f5 ff ff               	calll	0x418d80 <.text+0x17d80>
  419784: 85 c0                        	testl	%eax, %eax
  419786: 74 1e                        	je	0x4197a6 <.text+0x187a6>
  419788: d9 47 20                     	flds	0x20(%edi)
  41978b: d8 64 24 54                  	fsubs	0x54(%esp)
  41978f: d9 5f 20                     	fstps	0x20(%edi)
  419792: d9 47 24                     	flds	0x24(%edi)
  419795: d8 64 24 50                  	fsubs	0x50(%esp)
  419799: d9 5f 24                     	fstps	0x24(%edi)
  41979c: d9 47 28                     	flds	0x28(%edi)
  41979f: d8 64 24 14                  	fsubs	0x14(%esp)
  4197a3: d9 5f 28                     	fstps	0x28(%edi)
  4197a6: d9 44 24 48                  	flds	0x48(%esp)
  4197aa: 5f                           	popl	%edi
  4197ab: 5e                           	popl	%esi
  4197ac: 5d                           	popl	%ebp
  4197ad: 5b                           	popl	%ebx
  4197ae: 83 c4 34                     	addl	$0x34, %esp
  4197b1: c3                           	retl
  4197b2: 90                           	nop
  4197b3: 90                           	nop
  4197b4: 90                           	nop
  4197b5: 90                           	nop
  4197b6: 90                           	nop
  4197b7: 90                           	nop
  4197b8: 90                           	nop
  4197b9: 90                           	nop
  4197ba: 90                           	nop
  4197bb: 90                           	nop
  4197bc: 90                           	nop
  4197bd: 90                           	nop
  4197be: 90                           	nop
  4197bf: 90                           	nop
  4197c0: d9 44 24 04                  	flds	0x4(%esp)
  4197c4: d9 e0                        	fchs
  4197c6: d8 19                        	fcomps	(%ecx)
  4197c8: df e0                        	fnstsw	%ax
  4197ca: f6 c4 05                     	testb	$0x5, %ah
  4197cd: 7a 31                        	jp	0x419800 <.text+0x18800>
  4197cf: d9 01                        	flds	(%ecx)
  4197d1: d8 5c 24 04                  	fcomps	0x4(%esp)
  4197d5: df e0                        	fnstsw	%ax
  4197d7: f6 c4 05                     	testb	$0x5, %ah
  4197da: 7a 24                        	jp	0x419800 <.text+0x18800>
  4197dc: d9 44 24 08                  	flds	0x8(%esp)
  4197e0: d9 e0                        	fchs
  4197e2: d8 59 08                     	fcomps	0x8(%ecx)
  4197e5: df e0                        	fnstsw	%ax
  4197e7: f6 c4 05                     	testb	$0x5, %ah
  4197ea: 7a 14                        	jp	0x419800 <.text+0x18800>
  4197ec: d9 41 08                     	flds	0x8(%ecx)
  4197ef: d8 5c 24 08                  	fcomps	0x8(%esp)
  4197f3: df e0                        	fnstsw	%ax
  4197f5: f6 c4 05                     	testb	$0x5, %ah
  4197f8: 7a 06                        	jp	0x419800 <.text+0x18800>
  4197fa: b8 01 00 00 00               	movl	$0x1, %eax
  4197ff: c3                           	retl
  419800: 33 c0                        	xorl	%eax, %eax
  419802: c3                           	retl
  419803: 90                           	nop
  419804: 90                           	nop
  419805: 90                           	nop
  419806: 90                           	nop
  419807: 90                           	nop
  419808: 90                           	nop
  419809: 90                           	nop
  41980a: 90                           	nop
  41980b: 90                           	nop
  41980c: 90                           	nop
  41980d: 90                           	nop
  41980e: 90                           	nop
  41980f: 90                           	nop
  419810: d9 44 24 04                  	flds	0x4(%esp)
  419814: d8 5c 24 08                  	fcomps	0x8(%esp)
  419818: df e0                        	fnstsw	%ax
  41981a: f6 c4 41                     	testb	$0x41, %ah
  41981d: 75 67                        	jne	0x419886 <.text+0x18886>
  41981f: d9 44 24 04                  	flds	0x4(%esp)
  419823: d8 01                        	fadds	(%ecx)
  419825: d8 64 24 08                  	fsubs	0x8(%esp)
  419829: d9 c0                        	fld	%st(0)
  41982b: d9 e0                        	fchs
  41982d: d8 59 08                     	fcomps	0x8(%ecx)
  419830: df e0                        	fnstsw	%ax
  419832: f6 c4 41                     	testb	$0x41, %ah
  419835: 75 0d                        	jne	0x419844 <.text+0x18844>
  419837: d8 59 08                     	fcomps	0x8(%ecx)
  41983a: df e0                        	fnstsw	%ax
  41983c: f6 c4 05                     	testb	$0x5, %ah
  41983f: 7a 05                        	jp	0x419846 <.text+0x18846>
  419841: 33 c0                        	xorl	%eax, %eax
  419843: c3                           	retl
  419844: dd d8                        	fstp	%st(0)
  419846: d9 01                        	flds	(%ecx)
  419848: d8 64 24 04                  	fsubs	0x4(%esp)
  41984c: d8 44 24 08                  	fadds	0x8(%esp)
  419850: d9 c0                        	fld	%st(0)
  419852: d9 e0                        	fchs
  419854: d8 59 08                     	fcomps	0x8(%ecx)
  419857: df e0                        	fnstsw	%ax
  419859: f6 c4 05                     	testb	$0x5, %ah
  41985c: 7a 10                        	jp	0x41986e <.text+0x1886e>
  41985e: d8 59 08                     	fcomps	0x8(%ecx)
  419861: df e0                        	fnstsw	%ax
  419863: f6 c4 41                     	testb	$0x41, %ah
  419866: 75 08                        	jne	0x419870 <.text+0x18870>
  419868: b8 01 00 00 00               	movl	$0x1, %eax
  41986d: c3                           	retl
  41986e: dd d8                        	fstp	%st(0)
  419870: d9 41 08                     	flds	0x8(%ecx)
  419873: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  419879: df e0                        	fnstsw	%ax
  41987b: f6 c4 05                     	testb	$0x5, %ah
  41987e: 7a 50                        	jp	0x4198d0 <.text+0x188d0>
  419880: b8 02 00 00 00               	movl	$0x2, %eax
  419885: c3                           	retl
  419886: d9 44 24 08                  	flds	0x8(%esp)
  41988a: d8 41 08                     	fadds	0x8(%ecx)
  41988d: d8 64 24 04                  	fsubs	0x4(%esp)
  419891: d9 c0                        	fld	%st(0)
  419893: d9 e0                        	fchs
  419895: d8 19                        	fcomps	(%ecx)
  419897: df e0                        	fnstsw	%ax
  419899: f6 c4 41                     	testb	$0x41, %ah
  41989c: 75 0f                        	jne	0x4198ad <.text+0x188ad>
  41989e: d8 19                        	fcomps	(%ecx)
  4198a0: df e0                        	fnstsw	%ax
  4198a2: f6 c4 05                     	testb	$0x5, %ah
  4198a5: 7a 08                        	jp	0x4198af <.text+0x188af>
  4198a7: b8 02 00 00 00               	movl	$0x2, %eax
  4198ac: c3                           	retl
  4198ad: dd d8                        	fstp	%st(0)
  4198af: d9 41 08                     	flds	0x8(%ecx)
  4198b2: d8 64 24 08                  	fsubs	0x8(%esp)
  4198b6: d8 44 24 04                  	fadds	0x4(%esp)
  4198ba: d9 c0                        	fld	%st(0)
  4198bc: d9 e0                        	fchs
  4198be: d8 19                        	fcomps	(%ecx)
  4198c0: df e0                        	fnstsw	%ax
  4198c2: f6 c4 05                     	testb	$0x5, %ah
  4198c5: 7a 0f                        	jp	0x4198d6 <.text+0x188d6>
  4198c7: d8 19                        	fcomps	(%ecx)
  4198c9: df e0                        	fnstsw	%ax
  4198cb: f6 c4 41                     	testb	$0x41, %ah
  4198ce: 75 08                        	jne	0x4198d8 <.text+0x188d8>
  4198d0: b8 03 00 00 00               	movl	$0x3, %eax
  4198d5: c3                           	retl
  4198d6: dd d8                        	fstp	%st(0)
  4198d8: d9 01                        	flds	(%ecx)
  4198da: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  4198e0: df e0                        	fnstsw	%ax
  4198e2: f6 c4 05                     	testb	$0x5, %ah
  4198e5: 0f 8b 56 ff ff ff            	jnp	0x419841 <.text+0x18841>
  4198eb: b8 01 00 00 00               	movl	$0x1, %eax
  4198f0: c3                           	retl
  4198f1: 90                           	nop
  4198f2: 90                           	nop
  4198f3: 90                           	nop
  4198f4: 90                           	nop
  4198f5: 90                           	nop
  4198f6: 90                           	nop
  4198f7: 90                           	nop
  4198f8: 90                           	nop
  4198f9: 90                           	nop
  4198fa: 90                           	nop
  4198fb: 90                           	nop
  4198fc: 90                           	nop
  4198fd: 90                           	nop
  4198fe: 90                           	nop
  4198ff: 90                           	nop
  419900: 83 ec 18                     	subl	$0x18, %esp
  419903: d9 01                        	flds	(%ecx)
  419905: d8 20                        	fsubs	(%eax)
  419907: d9 5c 24 00                  	fstps	(%esp)
  41990b: d9 41 04                     	flds	0x4(%ecx)
  41990e: d8 60 04                     	fsubs	0x4(%eax)
  419911: d9 5c 24 04                  	fstps	0x4(%esp)
  419915: d9 41 08                     	flds	0x8(%ecx)
  419918: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  41991d: d8 60 08                     	fsubs	0x8(%eax)
  419920: d9 5c 24 08                  	fstps	0x8(%esp)
  419924: e8 c7 b7 36 00               	calll	0x7850f0 <.text+0x3840f0>
  419929: 85 f6                        	testl	%esi, %esi
  41992b: 74 0a                        	je	0x419937 <.text+0x18937>
  41992d: 0f b7 d6                     	movzwl	%si, %edx
  419930: 33 c9                        	xorl	%ecx, %ecx
  419932: e8 e9 b4 36 00               	calll	0x784e20 <.text+0x383e20>
  419937: 8d 44 24 0c                  	leal	0xc(%esp), %eax
  41993b: 50                           	pushl	%eax
  41993c: 8d 54 24 04                  	leal	0x4(%esp), %edx
  419940: 33 c9                        	xorl	%ecx, %ecx
  419942: e8 19 b4 36 00               	calll	0x784d60 <.text+0x383d60>
  419947: b9 01 00 00 00               	movl	$0x1, %ecx
  41994c: e8 ef b7 36 00               	calll	0x785140 <.text+0x384140>
  419951: 8b 4c 24 0c                  	movl	0xc(%esp), %ecx
  419955: 8b 54 24 10                  	movl	0x10(%esp), %edx
  419959: 8b 44 24 14                  	movl	0x14(%esp), %eax
  41995d: 89 0f                        	movl	%ecx, (%edi)
  41995f: 89 57 04                     	movl	%edx, 0x4(%edi)
  419962: 89 47 08                     	movl	%eax, 0x8(%edi)
  419965: 83 c4 18                     	addl	$0x18, %esp
  419968: c3                           	retl
  419969: 90                           	nop
  41996a: 90                           	nop
  41996b: 90                           	nop
  41996c: 90                           	nop
  41996d: 90                           	nop
  41996e: 90                           	nop
  41996f: 90                           	nop
  419970: 83 ec 0c                     	subl	$0xc, %esp
  419973: 57                           	pushl	%edi
  419974: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  419979: 8b f8                        	movl	%eax, %edi
  41997b: e8 70 b7 36 00               	calll	0x7850f0 <.text+0x3840f0>
  419980: 8b c7                        	movl	%edi, %eax
  419982: f7 d8                        	negl	%eax
  419984: 74 0c                        	je	0x419992 <.text+0x18992>
  419986: f7 df                        	negl	%edi
  419988: 0f b7 d7                     	movzwl	%di, %edx
  41998b: 33 c9                        	xorl	%ecx, %ecx
  41998d: e8 8e b4 36 00               	calll	0x784e20 <.text+0x383e20>
  419992: 8d 4c 24 04                  	leal	0x4(%esp), %ecx
  419996: 51                           	pushl	%ecx
  419997: 8b d6                        	movl	%esi, %edx
  419999: 33 c9                        	xorl	%ecx, %ecx
  41999b: e8 c0 b3 36 00               	calll	0x784d60 <.text+0x383d60>
  4199a0: e8 3b 7d 36 00               	calll	0x7816e0 <.text+0x3806e0>
  4199a5: d9 44 24 04                  	flds	0x4(%esp)
  4199a9: d8 03                        	fadds	(%ebx)
  4199ab: 5f                           	popl	%edi
  4199ac: d9 1e                        	fstps	(%esi)
  4199ae: d9 44 24 04                  	flds	0x4(%esp)
  4199b2: d8 43 04                     	fadds	0x4(%ebx)
  4199b5: d9 5e 04                     	fstps	0x4(%esi)
  4199b8: d9 44 24 08                  	flds	0x8(%esp)
  4199bc: d8 43 08                     	fadds	0x8(%ebx)
  4199bf: d9 5e 08                     	fstps	0x8(%esi)
  4199c2: 83 c4 0c                     	addl	$0xc, %esp
  4199c5: c3                           	retl
  4199c6: 90                           	nop
  4199c7: 90                           	nop
  4199c8: 90                           	nop
  4199c9: 90                           	nop
  4199ca: 90                           	nop
  4199cb: 90                           	nop
  4199cc: 90                           	nop
  4199cd: 90                           	nop
  4199ce: 90                           	nop
  4199cf: 90                           	nop
  4199d0: 81 ec 80 00 00 00            	subl	$0x80, %esp
  4199d6: 53                           	pushl	%ebx
  4199d7: 55                           	pushl	%ebp
  4199d8: 56                           	pushl	%esi
  4199d9: 8b f2                        	movl	%edx, %esi
  4199db: 8b 91 9c 00 00 00            	movl	0x9c(%ecx), %edx
  4199e1: d9 46 14                     	flds	0x14(%esi)
  4199e4: d8 84 24 9c 00 00 00         	fadds	0x9c(%esp)
  4199eb: 8b 4e 1c                     	movl	0x1c(%esi), %ecx
  4199ee: 8b ac 24 98 00 00 00         	movl	0x98(%esp), %ebp
  4199f5: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  4199f9: 8b 48 04                     	movl	0x4(%eax), %ecx
  4199fc: d9 54 24 0c                  	fsts	0xc(%esp)
  419a00: 57                           	pushl	%edi
  419a01: 8b 7a 20                     	movl	0x20(%edx), %edi
  419a04: d9 5c 24 30                  	fstps	0x30(%esp)
  419a08: d9 44 24 14                  	flds	0x14(%esp)
  419a0c: 8b 10                        	movl	(%eax), %edx
  419a0e: d8 84 24 a0 00 00 00         	fadds	0xa0(%esp)
  419a15: 89 54 24 18                  	movl	%edx, 0x18(%esp)
  419a19: 8b 50 08                     	movl	0x8(%eax), %edx
  419a1c: 8b c1                        	movl	%ecx, %eax
  419a1e: d9 54 24 38                  	fsts	0x38(%esp)
  419a22: 89 4c 24 1c                  	movl	%ecx, 0x1c(%esp)
  419a26: 8b 4c 24 10                  	movl	0x10(%esp), %ecx
  419a2a: d9 c0                        	fld	%st(0)
  419a2c: d9 e0                        	fchs
  419a2e: 89 4c 24 3c                  	movl	%ecx, 0x3c(%esp)
  419a32: d9 54 24 44                  	fsts	0x44(%esp)
  419a36: 8b c8                        	movl	%eax, %ecx
  419a38: d9 44 24 10                  	flds	0x10(%esp)
  419a3c: 89 54 24 20                  	movl	%edx, 0x20(%esp)
  419a40: d9 e0                        	fchs
  419a42: 8b d0                        	movl	%eax, %edx
  419a44: d9 54 24 48                  	fsts	0x48(%esp)
  419a48: 89 4c 24 58                  	movl	%ecx, 0x58(%esp)
  419a4c: d9 c9                        	fxch	%st(1)
  419a4e: b9 50 d6 89 03               	movl	$0x389d650, %ecx        # imm = 0x389D650
  419a53: d9 5c 24 50                  	fstps	0x50(%esp)
  419a57: 89 44 24 34                  	movl	%eax, 0x34(%esp)
  419a5b: 89 54 24 40                  	movl	%edx, 0x40(%esp)
  419a5f: d9 5c 24 54                  	fstps	0x54(%esp)
  419a63: 89 44 24 4c                  	movl	%eax, 0x4c(%esp)
  419a67: d9 5c 24 5c                  	fstps	0x5c(%esp)
  419a6b: e8 80 b6 36 00               	calll	0x7850f0 <.text+0x3840f0>
  419a70: 8b 56 28                     	movl	0x28(%esi), %edx
  419a73: 03 57 18                     	addl	0x18(%edi), %edx
  419a76: 74 12                        	je	0x419a8a <.text+0x18a8a>
  419a78: 66 8b 46 28                  	movw	0x28(%esi), %ax
  419a7c: 66 03 47 18                  	addw	0x18(%edi), %ax
  419a80: 33 c9                        	xorl	%ecx, %ecx
  419a82: 0f b7 d0                     	movzwl	%ax, %edx
  419a85: e8 96 b3 36 00               	calll	0x784e20 <.text+0x383e20>
  419a8a: 33 f6                        	xorl	%esi, %esi
  419a8c: c7 44 24 10 04 00 00 00      	movl	$0x4, 0x10(%esp)
  419a94: 8d 7c 34 60                  	leal	0x60(%esp,%esi), %edi
  419a98: 8d 5c 34 30                  	leal	0x30(%esp,%esi), %ebx
  419a9c: 57                           	pushl	%edi
  419a9d: 8b d3                        	movl	%ebx, %edx
  419a9f: 33 c9                        	xorl	%ecx, %ecx
  419aa1: e8 ca 7b 36 00               	calll	0x781670 <.text+0x380670>
  419aa6: d9 45 00                     	flds	(%ebp)
  419aa9: d8 07                        	fadds	(%edi)
  419aab: 8b 44 24 10                  	movl	0x10(%esp), %eax
  419aaf: 83 c6 0c                     	addl	$0xc, %esi
  419ab2: 48                           	decl	%eax
  419ab3: d9 1b                        	fstps	(%ebx)
  419ab5: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  419ab9: d9 44 24 1c                  	flds	0x1c(%esp)
  419abd: d9 5c 34 28                  	fstps	0x28(%esp,%esi)
  419ac1: d9 44 34 5c                  	flds	0x5c(%esp,%esi)
  419ac5: d8 45 08                     	fadds	0x8(%ebp)
  419ac8: d9 5c 34 2c                  	fstps	0x2c(%esp,%esi)
  419acc: 75 c6                        	jne	0x419a94 <.text+0x18a94>
  419ace: b9 01 00 00 00               	movl	$0x1, %ecx
  419ad3: e8 68 b6 36 00               	calll	0x785140 <.text+0x384140>
  419ad8: 32 db                        	xorb	%bl, %bl
  419ada: 8d 9b 00 00 00 00            	leal	(%ebx), %ebx
  419ae0: 0f b6 cb                     	movzbl	%bl, %ecx
  419ae3: 8d 41 01                     	leal	0x1(%ecx), %eax
  419ae6: 25 03 00 00 80               	andl	$0x80000003, %eax       # imm = 0x80000003
  419aeb: 8d 54 24 24                  	leal	0x24(%esp), %edx
  419aef: 52                           	pushl	%edx
  419af0: 79 05                        	jns	0x419af7 <.text+0x18af7>
  419af2: 48                           	decl	%eax
  419af3: 83 c8 fc                     	orl	$-0x4, %eax
  419af6: 40                           	incl	%eax
  419af7: 8d 04 40                     	leal	(%eax,%eax,2), %eax
  419afa: 8d 54 84 34                  	leal	0x34(%esp,%eax,4), %edx
  419afe: 52                           	pushl	%edx
  419aff: 8d 04 49                     	leal	(%ecx,%ecx,2), %eax
  419b02: 8d 4c 84 38                  	leal	0x38(%esp,%eax,4), %ecx
  419b06: 51                           	pushl	%ecx
  419b07: 8d 54 24 24                  	leal	0x24(%esp), %edx
  419b0b: 52                           	pushl	%edx
  419b0c: e8 8f b5 03 00               	calll	0x4550a0 <.text+0x540a0>
  419b11: d9 44 24 38                  	flds	0x38(%esp)
  419b15: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
  419b1b: 83 c4 10                     	addl	$0x10, %esp
  419b1e: df e0                        	fnstsw	%ax
  419b20: f6 c4 41                     	testb	$0x41, %ah
  419b23: 7b 17                        	jnp	0x419b3c <.text+0x18b3c>
  419b25: fe c3                        	incb	%bl
  419b27: 80 fb 04                     	cmpb	$0x4, %bl
  419b2a: 72 b4                        	jb	0x419ae0 <.text+0x18ae0>
  419b2c: 5f                           	popl	%edi
  419b2d: 5e                           	popl	%esi
  419b2e: 5d                           	popl	%ebp
  419b2f: b8 01 00 00 00               	movl	$0x1, %eax
  419b34: 5b                           	popl	%ebx
  419b35: 81 c4 80 00 00 00            	addl	$0x80, %esp
  419b3b: c3                           	retl
  419b3c: 5f                           	popl	%edi
  419b3d: 5e                           	popl	%esi
  419b3e: 5d                           	popl	%ebp
  419b3f: 33 c0                        	xorl	%eax, %eax
  419b41: 5b                           	popl	%ebx
  419b42: 81 c4 80 00 00 00            	addl	$0x80, %esp
  419b48: c3                           	retl
  419b49: 90                           	nop
  419b4a: 90                           	nop
  419b4b: 90                           	nop
  419b4c: 90                           	nop
  419b4d: 90                           	nop
  419b4e: 90                           	nop
  419b4f: 90                           	nop
  419b50: 83 ec 1c                     	subl	$0x1c, %esp
  419b53: 8b 91 9c 00 00 00            	movl	0x9c(%ecx), %edx
  419b59: 8b 4a 20                     	movl	0x20(%edx), %ecx
  419b5c: 8b 54 24 24                  	movl	0x24(%esp), %edx
  419b60: 53                           	pushl	%ebx
  419b61: 89 4c 24 10                  	movl	%ecx, 0x10(%esp)
  419b65: 8b 4a 14                     	movl	0x14(%edx), %ecx
  419b68: 8b 50 14                     	movl	0x14(%eax), %edx
  419b6b: 55                           	pushl	%ebp
  419b6c: 8b 6c 24 28                  	movl	0x28(%esp), %ebp
  419b70: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  419b74: 8b 48 1c                     	movl	0x1c(%eax), %ecx
  419b77: 89 4c 24 2c                  	movl	%ecx, 0x2c(%esp)
  419b7b: 8b 48 04                     	movl	0x4(%eax), %ecx
  419b7e: f6 c1 20                     	testb	$0x20, %cl
  419b81: 56                           	pushl	%esi
  419b82: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  419b86: 75 19                        	jne	0x419ba1 <.text+0x18ba1>
  419b88: 84 ed                        	testb	%ch, %ch
  419b8a: 78 15                        	js	0x419ba1 <.text+0x18ba1>
  419b8c: 8b 54 24 34                  	movl	0x34(%esp), %edx
  419b90: 8b 8a 9c 00 00 00            	movl	0x9c(%edx), %ecx
  419b96: 8b 51 20                     	movl	0x20(%ecx), %edx
  419b99: 8b 5a 18                     	movl	0x18(%edx), %ebx
  419b9c: 03 58 28                     	addl	0x28(%eax), %ebx
  419b9f: eb 03                        	jmp	0x419ba4 <.text+0x18ba4>
  419ba1: 8b 58 28                     	movl	0x28(%eax), %ebx
  419ba4: 8b 44 24 38                  	movl	0x38(%esp), %eax
  419ba8: f7 db                        	negl	%ebx
  419baa: 57                           	pushl	%edi
  419bab: 8d 7c 24 20                  	leal	0x20(%esp), %edi
  419baf: 8b f3                        	movl	%ebx, %esi
  419bb1: 8b cd                        	movl	%ebp, %ecx
  419bb3: e8 48 fd ff ff               	calll	0x419900 <.text+0x18900>
  419bb8: d9 44 24 18                  	flds	0x18(%esp)
  419bbc: d8 44 24 30                  	fadds	0x30(%esp)
  419bc0: 5f                           	popl	%edi
  419bc1: d9 54 24 10                  	fsts	0x10(%esp)
  419bc5: d8 4c 24 24                  	fmuls	0x24(%esp)
  419bc9: d9 5c 24 34                  	fstps	0x34(%esp)
  419bcd: d9 44 24 30                  	flds	0x30(%esp)
  419bd1: d8 44 24 2c                  	fadds	0x2c(%esp)
  419bd5: d9 54 24 30                  	fsts	0x30(%esp)
  419bd9: d8 4c 24 1c                  	fmuls	0x1c(%esp)
  419bdd: d9 54 24 0c                  	fsts	0xc(%esp)
  419be1: d8 44 24 34                  	fadds	0x34(%esp)
  419be5: d9 44 24 34                  	flds	0x34(%esp)
  419be9: d8 64 24 0c                  	fsubs	0xc(%esp)
  419bed: d8 1d 2c cb 7d 00            	fcomps	0x7dcb2c
