  49ab58: a0 b5 8d b1 03               	movb	0x3b18db5, %al
  49ab5d: 83 c4 08                     	addl	$0x8, %esp
  49ab60: 84 c0                        	testb	%al, %al
  49ab62: 75 4c                        	jne	0x49abb0 <.text+0x99bb0>
  49ab64: 8b 0d 80 6e 8f 03            	movl	0x38f6e80, %ecx
  49ab6a: 8b 41 18                     	movl	0x18(%ecx), %eax
  49ab6d: 8b 49 14                     	movl	0x14(%ecx), %ecx
  49ab70: 39 48 30                     	cmpl	%ecx, 0x30(%eax)
  49ab73: 74 3b                        	je	0x49abb0 <.text+0x99bb0>
  49ab75: 89 48 30                     	movl	%ecx, 0x30(%eax)
  49ab78: c7 85 44 01 00 00 28 5e c5 03	movl	$0x3c55e28, 0x144(%ebp) # imm = 0x3C55E28
  49ab82: 8b 15 80 6e 8f 03            	movl	0x38f6e80, %edx
  49ab88: 8b 42 18                     	movl	0x18(%edx), %eax
  49ab8b: 8b 48 30                     	movl	0x30(%eax), %ecx
  49ab8e: 55                           	pushl	%ebp
  49ab8f: 56                           	pushl	%esi
  49ab90: 89 0d 38 5f c5 03            	movl	%ecx, 0x3c55f38
  49ab96: 53                           	pushl	%ebx
  49ab97: 66 89 bd 22 01 00 00         	movw	%di, 0x122(%ebp)
  49ab9e: e8 ed 4e fa ff               	calll	0x43fa90 <.text+0x3ea90>
  49aba3: 83 c4 0c                     	addl	$0xc, %esp
  49aba6: c7 85 44 01 00 00 28 5e c5 03	movl	$0x3c55e28, 0x144(%ebp) # imm = 0x3C55E28
  49abb0: 8b 15 80 6e 8f 03            	movl	0x38f6e80, %edx
  49abb6: 8b 42 18                     	movl	0x18(%edx), %eax
  49abb9: 8b 54 24 2c                  	movl	0x2c(%esp), %edx
  49abbd: 8b 48 30                     	movl	0x30(%eax), %ecx
  49abc0: 57                           	pushl	%edi
  49abc1: 6a 02                        	pushl	$0x2
  49abc3: 68 f0 cb 91 00               	pushl	$0x91cbf0               # imm = 0x91CBF0
  49abc8: 52                           	pushl	%edx
  49abc9: 89 0d 40 5d c5 03            	movl	%ecx, 0x3c55d40
  49abcf: e8 1c 1f f8 ff               	calll	0x41caf0 <.text+0x1baf0>
  49abd4: 8b 43 38                     	movl	0x38(%ebx), %eax
  49abd7: 8b 48 0c                     	movl	0xc(%eax), %ecx
  49abda: 8b 95 0c 01 00 00            	movl	0x10c(%ebp), %edx
  49abe0: 89 51 14                     	movl	%edx, 0x14(%ecx)
  49abe3: c6 03 01                     	movb	$0x1, (%ebx)
  49abe6: 57                           	pushl	%edi
  49abe7: 66 89 7d 0e                  	movw	%di, 0xe(%ebp)
  49abeb: e8 90 66 f8 ff               	calll	0x421280 <.text+0x20280>
  49abf0: a0 b5 8d b1 03               	movb	0x3b18db5, %al
