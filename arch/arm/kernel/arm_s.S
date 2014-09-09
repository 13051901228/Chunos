#include "include/arm.h"

.macro set_mode, mode
msr cpsr_c, #\mode
.endm

	.text

/*
 * fix it: this function can only be called
 * in the entry of syscall
 */
.global arch_get_pt_regs
arch_get_pt_regs:
	mov r0, r8
	mov pc, lr

/*
 * code for schedule, there is only one case
 * will call this function, user_mode->syscall->
 * svc_mode->sched()->arch_switch_task_sw so we only
 * need to save the register of the svc mode, user
 * mode register is saved in the syscall interrupt
 * entry
 */
.global arch_switch_task_sw
arch_switch_task_sw:
	push {lr}		@ pc

	sub sp, sp, #16
	push {r0-r12}		@r0-r12

	mrs r0, cpsr
	str r0, [sp, #64]	@ cpsr or spsr

	mrs r0, spsr		@ spsr
	str r0, [sp, #52]

	str lr, [sp, #56]	@ lr
	sub r0, sp, #72
	str r0, [sp, #60]	@ sp

	ldr r2, = current	@ change page table.
	ldr r3, = next_run
	ldr r0, [r2]
	ldr r1, [r3]
	bl switch_task

	ldr r0, = current
	ldr r1, = next_run

	ldr r2, [r0]		@ store sp to current process.
	str sp, [r2]

	ldr r2, [r1]
	str r2, [r0]		@ current = next_run

	ldr sp, [r2]		@ load sp of next_run.

	ldr r1, [sp, #64]	@ load cpsr of next process
	and r1, r1, #MODE_MASK
	mov r0, sp

	cmp r1, #USER_MODE
	bleq sw_switch_to_user

	cmp r1, #SVC_MODE
	bleq sw_switch_to_svc

	cmp r1, #USER_MODE
	bne sw_switch_out

	sub sp, sp, #72
	mov r0, sp
	bl signal_handler
	cmp r0, #0
	addeq sp, sp, #72

sw_switch_out:
	ldr r1, [sp, #64]	@ cpsr
	msr spsr, r1

	pop {r0-r12}		@ r0-r12

	add sp, sp, #16		@ return to new process
	ldmfd sp!, {pc}^

sw_switch_to_user:
	set_mode SYSTEM_MODE | NO_INT
	
	ldr r2, [r0, #60]	@ sp_user
	mov sp, r2

	ldr r2, [r0, #56]	@ lr
	mov lr, r2

	set_mode SVC_MODE | NO_INT
	mov pc, lr

sw_switch_to_svc:
	mov r3, lr
	ldr r2, [r0, #56]	@ lr
	mov lr, r2

	ldr r2, [r0, #52]	@ spsr
	msr spsr, r2

	mov pc, r3

/* r0 = &current
 * r1 = &next_run
 * r2 = current
 * r3 = next_run
 * we are in irq mode
 */
.global os_exit_irq
os_exit_irq:			@ check wether need to swtich task
	ldr r0, = current
	ldr r1, = next_run
	ldr r2, [r0]
	ldr r3, [r1]

	cmp r2, r3
	beq out

	set_mode SVC_MODE | NO_INT
	str sp, [r2]		@ save the sp to current process
	set_mode IRQ_MODE | NO_INT

	mov r0, r2		@ goto switch_task
	mov r1, r3
	push {lr}
	bl switch_task
	pop {lr}

	ldr r0, = current	@ current = next_run
	ldr r1, = next_run
	ldr r2, [r0]

	ldr r3, [r1]		@ current = next_run
	str r3, [r0]

	ldr r0, [r3]		@ load the sp of next run process
	mov pc, lr
out:
	set_mode SVC_MODE | NO_INT
	mov r0, sp
	set_mode IRQ_MODE | NO_INT
	mov pc, lr

