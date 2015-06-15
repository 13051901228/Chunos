	.text

.extern current
.extern next_run
.extern switch_task

.global arch_switch_page_table
arch_switch_page_table:
	push %eax
	mov 8(%esp), %eax
	mov %eax, %cr3
	pop %eax
	ret

.global arch_enter_critical
arch_enter_critical:
	push %eax
	push %ebx
	pushf
	pop 	%ebx
	mov	12(%esp), %eax
	mov 	%ebx, (%eax)
	pop	%ebx
	pop	%eax
	cli
	ret

.global arch_exit_critical
arch_exit_critical:
	push	%eax
	mov	8(%esp), %eax
	mov 	(%eax), %eax
	and 	$0x200, %eax
	cmp 	$0x200, %eax
	je 	__ret_enable_irq
	pop	%eax
	ret

__ret_enable_irq:
	pop %eax
	sti
	ret


.global arch_switch_task_sw
arch_switch_task_sw:
	push 	%eax
	add 	$4, %esp
	pop 	%eax
	mov 	%eax, -12(%esp)
	mov 	-8(%esp), %eax
	pushf
	push 	%cs
	sub 	$4, %esp
	push 	$0
	pusha
	push 	$0
	push 	%ss
	push 	%es
	push 	%ds
	push 	%fs
	push 	%gs

	lea 	current, %ecx
	lea 	next_run, %edx
	mov 	(%ecx), %eax
	mov 	(%edx), %ebx
	mov 	%esp, (%eax)	/* store the stack base to current process */

	push %ebx
	push %eax
	call 	switch_task

	lea 	current, %ecx
	lea 	next_run, %edx
	mov 	(%ecx), %eax
	mov 	(%edx), %ebx

	mov 	%ebx, (%ecx)	/* current = next_run */
	mov 	(%ebx), %esp	/* stack base of next run task */
	
	pop 	%gs
	pop 	%fs
	pop 	%ds
	pop 	%es

	add 	$8, %esp
	popa

	add 	$4, %esp
	iret
