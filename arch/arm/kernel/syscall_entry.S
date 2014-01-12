	.text

#include <os/syscall_nr.h>
/*
 * r0 is the stack of SVC mode arg of the syscall, 
 * r7 is the syscall nr, we need to create
 * the stack frame for the syscall. in arm ACPS
 * fp is r11, ip is r12 the prototyep of this function
 * void *_syscall_handler(pt_regs *regs)
 */
.global __default_syscall_handler
__default_syscall_handler:	.word default_syscall_handler

.global _syscall_handler
_syscall_handler:
	push {fp, lr}
	push {r8}
	add fp, sp, #4
	sub sp, sp, #24

	ldr r3, [r8, #16]	@ arg4
	str r3, [sp]

	ldr r3, [r8, #20]	@ arg5
	str r3, [sp, #4]

	ldr r3, [r8, #24]	@arg6
	str r3, [sp, #8]

	ldr r0, [r8]		@ arg0
	ldr r1, [r8, #4]	@ arg1
	ldr r2, [r8, #8]	@ arg2
	ldr r3, [r8, #12]	@ arg3

	ldr r6, = syscall_table_base
	ldr r6, [r6]
	sub r7, r7, #__NR_SYSCALL_BASE

	add r6, r6, r7, lsl #2
	ldr r6, [r6]		@ if syscall not inplemented go to out
	cmp r6, #0
	ldreq r6,  __default_syscall_handler

	mov lr, pc
	mov pc, r6		@ goto syscall handler

	sub sp, fp, #4

	pop {r8}		@ will return here, return value is r0
	str r0, [r8]		@ store return valude to process context

	pop {fp, lr}
	bx lr
