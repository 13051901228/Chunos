	.text

.global memset				/* from bionic OpenBSD */
memset:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp),%edi
	movzbl	16(%esp),%eax		/* unsigned char, zero extend */
	movl	20(%esp),%ecx
	pushl	%edi			/* push address of buffer */

	cld				/* set fill direction forward */

	/*
	 * if the string is too short, it's really not worth the overhead
	 * of aligning to word boundries, etc.  So we jump to a plain
	 * unaligned set.
	 */
	cmpl	$0x0f,%ecx
	jle	L1

	movb	%al,%ah			/* copy char to all bytes in word */
	movl	%eax,%edx
	sall	$16,%eax
	orl	%edx,%eax

	movl	%edi,%edx		/* compute misalignment */
	negl	%edx
	andl	$3,%edx
	movl	%ecx,%ebx
	subl	%edx,%ebx

	movl	%edx,%ecx		/* set until word aligned */
	rep
	stosb

	movl	%ebx,%ecx
	shrl	$2,%ecx			/* set by words */
	rep
	stosl

	movl	%ebx,%ecx		/* set remainder by bytes */
	andl	$3,%ecx
L1:	rep
	stosb

	popl	%eax			/* pop address of buffer */
	popl	%ebx
	popl	%edi
	ret


.global memcpy
memcpy:
	pushl	%esi
	pushl	%edi
	movl	12(%esp),%edi
	movl	16(%esp),%esi
	movl	%edi, %eax
	movl	20(%esp),%ecx
	movl	%ecx,%edx
	cmpl	%esi,%edi	/* potentially overlapping? */
	jnb	1f
	cld			/* nope, copy forwards. */
	shrl	$2,%ecx		/* copy by words */
	rep
	movsl
	movl	%edx,%ecx
	andl	$3,%ecx		/* any bytes left? */
	rep
	movsb
	popl	%edi
	popl	%esi
	ret
1:
	addl	%ecx,%edi	/* copy backwards. */
	addl	%ecx,%esi
	std
	andl	$3,%ecx		/* any fractional bytes? */
	decl	%edi
	decl	%esi
	rep
	movsb
	movl	%edx,%ecx
	shrl	$2,%ecx
	subl	$3,%esi
	subl	$3,%edi
	rep
	movsl
	popl	%edi
	popl	%esi
	cld
	ret
