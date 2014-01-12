	.text
#include "include/kconfig.h"
#include <config/config.h>
#include "include/arm.h"

.macro set_mode, mode
msr cpsr_c, #\mode
.endm

.global _vector_start	
_vector_start:		@exception vector
	ldr pc, _reset
	ldr pc, _undefined_instruction
	ldr pc, _software_interrupt
	ldr pc, _prefetch_abort
	ldr pc, _data_abort
	ldr pc, _not_used
	ldr pc, _irq
	ldr pc, _fiq

_reset:			.word _start
_undefined_instruction: .word undefined_instruction
_software_interrupt:    .word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq

.global _vector_end
_vector_end:

undefined_instruction:
	stmfd sp!,{r0-r3,lr}
	ldmfd sp!,{r0-r3,pc}^				@^ equle spsr->cpsr
	
	.align 5					@when swi interrput,cpu is in svc mode
software_interrupt:
	push {lr}		@ save pc
	mrs lr, spsr		@ cpsr
	push {lr}

	sub sp, sp, #12		@ save r0-r12
	push {r0-r12}

	mrs r0, cpsr		@ get user_sp user_lr usr_spsr
	set_mode SYSTEM_MODE | NO_INT
	mov r1, sp
	mov r2, lr
	msr cpsr, r0

	str r1, [sp, #60]	@ user_sp
	str r2, [sp, #56]	@ user_lr

#ifdef SYSCALL_OLD_EABI
	ldr r0, [sp, #68]
	ldr r7, [r0, #-4]
	bic r7, r7, #0xff000000
#endif

	mov r8, sp		@ r8 is the base address of pt_regs

	mrs r1, cpsr
	bic r1, r1, #0xc0
	msr cpsr_c, r1		@enable irq

	bl _syscall_handler

	mrs r1, cpsr
	orr r1, r1, #0xc0
	msr cpsr_c, r1		@disable irq

	mov r0, sp

	set_mode SYSTEM_MODE | NO_INT
	ldr r2, [r0, #60]	@ restore user_sp
	mov sp, r2

	ldr r2, [r0, #56]	@ restore user_lr
	mov lr, r2
	set_mode SVC_MODE | NO_INT

	ldr r1, [sp, #64]
	msr spsr, r1

	pop {r0-r12}

	add sp, sp, #16
	ldmfd sp!, {pc}^

	.align 5
prefetch_abort:
	subs lr, lr, #4
	push {lr}

	mrs lr, spsr
	push {lr}

	sub sp, sp, #12
	push {r0-r12}

	and r0, lr, #MODE_MASK
	cmp r0, #SVC_MODE
	bleq svc_to_irq

	str r4, [sp, #60]
	str r3, [sp, #56]
	str r2, [sp, #52]

	mov r0, sp
	b data_abort_handler

	.align 5
data_abort:
	subs lr, lr, #8
	push {lr}

	mrs lr, spsr
	push {lr}

	sub sp, sp, #12
	push {r0-r12}

	and r0, lr, #MODE_MASK
	cmp r0, #SVC_MODE
	bleq svc_to_irq

	str r4, [sp, #60]
	str r3, [sp, #56]
	str r2, [sp, #52]

	mov r0, sp
	b data_abort_handler

	.align 5
not_used:
	mov r0, r0
	b not_used

	.align 5

irq:
	sub lr, lr, #4		@ save the context in irq stack temp.
	str lr, [sp, #-4]	@ pc

	mrs lr, spsr		@ cpsr
	str lr, [sp, #-8]

	str r0, [sp, #-12]	@ save r0 r1 r2 r3 r4
	str r1, [sp, #-16]
	str r2, [sp, #-20]
	str r3, [sp, #-24]
	str r4, [sp, #-28]

	and r0, lr, #MODE_MASK	@ if enter irq from user mode 
	cmp r0, #USER_MODE
	bleq user_to_irq

	cmp r0, #SVC_MODE
	bleq svc_to_irq		@ if enter irq from svc mode
	
	mov r1, sp		@ save sp of irq to r1 and goto svc mode
	msr cpsr, #SVC_MODE | NO_INT	@ r0= r1= r2=spsr r3=lr r4=sp

	sub sp, sp, #20			@ reserve for pc cpsr spsr lr sp
	push {r5-r12}			@ save r5-r12

	ldr r5, [r1, #-12]		@ r0
	ldr r6, [r1, #-16]		@ r1
	ldr r7, [r1, #-20]		@ r2
	ldr r8, [r1, #-24]		@ r3
	ldr r9, [r1, #-28]		@ r4
	push {r5-r9}			@ save r0-r4

	ldr r5, [r1, #-4]		@ pc
	str r5, [sp, #68]

	ldr r5, [r1, #-8]		@ cpsr
	str r5, [sp, #64]
	
	str r4, [sp, #60]		@ sp
	str r3, [sp, #56]		@ lr
	str r2, [sp, #52]		@ spsr

	set_mode IRQ_MODE | NO_INT

	bl irq_handler			@ goto irq handler

	bl os_exit_irq			@ r0 = process context base address

	ldr r1, [r0, #64]		@ load cpsr of last mode of the process
	and r1, r1, #MODE_MASK

	cmp r1, #USER_MODE		@ if last mode is user mode
	bleq exit_irq_from_user

	cmp r1, #SVC_MODE		@ if last mode is svc mode
	bleq exit_irq_from_svc

	ldr r1, [r0, #64]
	msr spsr, r1

	mov lr, r0			@ save context to lr
	ldmfd lr!, {r0-r12}		@ restore r0 - r12

	add lr, lr, #16			@ return to process
	ldmfd lr!, {pc}^

user_to_irq:
	mrs r1, cpsr		@ goto system mode to save some register
	msr cpsr, #SYSTEM_MODE | NO_INT

	mov r2, #0		@ spsr, user mode no spsr, set it to 0
	mov r3, lr		@ lr in user mode
	mov r4, sp		@ lr in user mode

	msr cpsr, r1		@return to irq mode

	mov pc, lr

svc_to_irq:
	mrs r1, cpsr		@ goto svc mode
	msr cpsr, #SVC_MODE | NO_INT

	mrs r2, spsr		@ spsr in svc mode
	mov r3, lr		@ lr in svc mode
	mov r4, sp		@ sp in svc mode

	msr cpsr, r1		@ return to irq mode

	mov pc, lr

exit_irq_from_user:		@ r0 is the address of process context
	set_mode SYSTEM_MODE | NO_INT
	
	ldr r2, [r0, #60]	@ sp
	mov sp, r2

	ldr r2, [r0, #56]	@ lr
	mov lr, r2

	set_mode SVC_MODE | NO_INT
	add sp, r0, #72		@ svc sp for user space to call syscall

	set_mode IRQ_MODE | NO_INT
	mov pc, lr

exit_irq_from_svc:		@r0 is the address of process context
	set_mode SVC_MODE | NO_INT

	add sp, r0, #72

	ldr r2, [r0, #56]
	mov lr, r2

	ldr r2, [r0, #52]
	msr spsr, r2

	set_mode IRQ_MODE | NO_INT
	mov pc, lr

	.align 5
fiq:
	sub lr, lr, #4
	stmfd sp!, {lr}
	mrs r14, spsr					@store register in frq mode
	stmfd sp!, {r0-r4, r12, r14}

	ldr r12, =0				@get the address of fiq handler
	add r12, r12, r3, lsl #2
	msr cpsr_c, #0x1f				@got to the svc mode

	stmfd sp!, {r0-r4, lr}				@store the register of svc mode 

	mov lr, pc
	ldr pc, [r12]					@got to the fiq handler of each interrupt

	ldmfd sp!, {r0-r4, lr}				@restore the register of svc mode 

	msr cpsr_c, #0x52				@got to fiq mode
	ldmfd sp!, {r0-r4, r12, r14}
	msr spsr_csxf, r14
	ldmfd sp!, {pc}^				@return 
