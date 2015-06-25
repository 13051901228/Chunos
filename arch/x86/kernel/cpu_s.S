	.text

.extern current
.extern next_run
.extern switch_task
.extern x86_switch_task
.extern get_syscall_handler

.global x86_switch_page_table
x86_switch_page_table:
	push	%eax
	mov 	8(%esp), %eax
	mov 	%eax, %cr3
	pop 	%eax
	ret

.global arch_enter_critical
arch_enter_critical:
	push 	%eax
	push 	%ebx
	pushf
	pop 	%ebx
	mov	12(%esp), %eax
	mov 	%ebx, (%eax)
	pop	%ebx
	pop	%eax
	cli
	ret

.global x86_trap_syscall_handler
x86_trap_syscall_handler:
	push	%ebx
	push	%eax
	call 	get_syscall_handler
	add	$4, %esp
	pop	%ebx

	cmp	$0, %eax
	jne	do_syscall_handler

	mov	$-22, %eax
	jmp	out_syscall_handler

do_syscall_handler:
	/* push arg to the stack */
	push	%ebx
	push 	24(%ebx)		/* edi is arg5 */
	push 	32(%ebx)		/* ebp is arg4 */
	push 	28(%ebx)		/* esi is arg3 */
	push 	44(%ebx)		/* edx is arg2 */
	push 	48(%ebx)		/* ecx is arg1 */
	push 	40(%ebx) 		/* ebx is arg0 */

	sti
	call	%eax
	cli

	add 	$24, %esp
	pop	%ebx

out_syscall_handler:
	mov 	%eax, 52(%ebx)		/* store the returen value to userspace */
	ret

.global get_pt_regs
get_pt_regs:				/* this must called in c function */
	mov	%ebp, %eax
	add	$32, %eax
	mov	(%eax), %eax
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

	push	%ebx
	push	%eax
	call 	x86_switch_task
	add	$8, %esi

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
