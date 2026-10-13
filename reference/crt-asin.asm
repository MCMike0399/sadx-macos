
Sonic Adventure DX/sonic.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  645020: 83 ec 0c                     	subl	$0xc, %esp
  645023: dd 14 24                     	fstl	(%esp)
  645026: e8 ed 3b 00 00               	calll	0x648c18 <.text+0x247c18>
  64502b: e8 0d 00 00 00               	calll	0x64503d <.text+0x24403d>
  645030: 83 c4 0c                     	addl	$0xc, %esp
  645033: c3                           	retl
  645034: 8d 54 24 04                  	leal	0x4(%esp), %edx
  645038: e8 98 3b 00 00               	calll	0x648bd5 <.text+0x247bd5>
  64503d: 52                           	pushl	%edx
  64503e: 9b                           	wait
  64503f: d9 3c 24                     	fnstcw	(%esp)
  645042: 74 6d                        	je	0x6450b1 <.text+0x2440b1>
  645044: 66 81 3c 24 7f 02            	cmpw	$0x27f, (%esp)          # imm = 0x27F
  64504a: 74 05                        	je	0x645051 <.text+0x244051>
  64504c: e8 54 3b 00 00               	calll	0x648ba5 <.text+0x247ba5>
  645051: 3d 00 00 f0 3f               	cmpl	$0x3ff00000, %eax       # imm = 0x3FF00000
  645056: 73 2b                        	jae	0x645083 <.text+0x244083>
  645058: d9 e8                        	fld1
  64505a: d8 c1                        	fadd	%st(1), %st
  64505c: d9 e8                        	fld1
  64505e: d8 e2                        	fsub	%st(2), %st
  645060: de c9                        	fmulp	%st, %st(1)
  645062: d9 fa                        	fsqrt
  645064: d9 f3                        	fpatan
  645066: 83 3d a8 37 c8 03 00         	cmpl	$0x0, 0x3c837a8
  64506d: 0f 85 bb 3b 00 00            	jne	0x648c2e <.text+0x247c2e>
  645073: ba 0e 00 00 00               	movl	$0xe, %edx
  645078: 8d 0d 10 be bc 02            	leal	0x2bcbe10, %ecx
  64507e: e9 b8 3b 00 00               	jmp	0x648c3b <.text+0x247c3b>
  645083: 77 3a                        	ja	0x6450bf <.text+0x2440bf>
  645085: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  645089: 8b c8                        	movl	%eax, %ecx
  64508b: 25 ff ff 0f 00               	andl	$0xfffff, %eax          # imm = 0xFFFFF
  645090: 0b 44 24 08                  	orl	0x8(%esp), %eax
  645094: 75 29                        	jne	0x6450bf <.text+0x2440bf>
  645096: 81 e1 00 00 00 80            	andl	$0x80000000, %ecx       # imm = 0x80000000
  64509c: dd d8                        	fstp	%st(0)
  64509e: db 2d 0a c0 bc 02            	fldt	0x2bcc00a
  6450a4: 74 c0                        	je	0x645066 <.text+0x244066>
  6450a6: d9 e0                        	fchs
  6450a8: eb bc                        	jmp	0x645066 <.text+0x244066>
  6450aa: e8 0d 3b 00 00               	calll	0x648bbc <.text+0x247bbc>
  6450af: eb 1b                        	jmp	0x6450cc <.text+0x2440cc>
  6450b1: a9 ff ff 0f 00               	testl	$0xfffff, %eax          # imm = 0xFFFFF
  6450b6: 75 f2                        	jne	0x6450aa <.text+0x2440aa>
  6450b8: 83 7c 24 08 00               	cmpl	$0x0, 0x8(%esp)
  6450bd: 75 eb                        	jne	0x6450aa <.text+0x2440aa>
  6450bf: dd d8                        	fstp	%st(0)
  6450c1: db 2d 00 c0 bc 02            	fldt	0x2bcc000
  6450c7: b8 01 00 00 00               	movl	$0x1, %eax
  6450cc: 83 3d a8 37 c8 03 00         	cmpl	$0x0, 0x3c837a8
  6450d3: 0f 85 55 3b 00 00            	jne	0x648c2e <.text+0x247c2e>
  6450d9: ba 0e 00 00 00               	movl	$0xe, %edx
  6450de: 8d 0d 10 be bc 02            	leal	0x2bcbe10, %ecx
  6450e4: e8 5e 3a 00 00               	calll	0x648b47 <.text+0x247b47>
  6450e9: 5a                           	popl	%edx
  6450ea: c3                           	retl
