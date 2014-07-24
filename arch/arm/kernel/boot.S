	.text
#include "include/kconfig.h"
#include "include/arm.h"
#include "include/asm_marco.h"

.macro va_to_pa, reg, reg_tmp
	ldr \reg_tmp, pv_offset
	sub \reg, \reg, \reg_tmp
.endm

.global _start	
_start:	
	b reset

.global start
start:				.word _start

/* 
 * these three var will be used in kernel
 * we need to count their value
 */
.global pv_offset
pv_offset:			.word 0

.global kernel_virtual_start
kernel_virtual_start:		.word 0

.global kernel_phy_start
kernel_phy_start:		.word 0

.global soc_desc
soc_desc:
	.space 4
	.space 4
	.space 4

	.align 5	
reset:						@ r4 = arch_version r5 = soc id r6 = board id,
	set_mode SVC_MODE | NO_INT		@ mmu off, icache disable, d cache disable
	mrc p15, 0, r0, c0, c0

	mov r4, #0				@ fake id for tq2440
	mov r5, #0
	mov r6, #0

	adr r3, soc_desc
	str r4, [r3]				@ arm version
	str r5, [r3, #4]			@ soc id
	str r6, [r3, #8]			@ board id

	bl low_level_init			@ do some low level init such as disable watch dog

	adr r0, _start
	mov r2, r0
	sub r2, r2, #KERNEL_LOAD_ADDRESS_ALIGIN
	adr r1, kernel_phy_start
	str r2, [r1]				@ store kernel physical address to kernel_phy_start

	ldr r1, =KERNEL_LOAD_ADDRESS_MASK	@ kenel must loaded at 1m aligin address, if not at least 16k
	and r0, r0, r1
	cmp r0, #KERNEL_LOAD_ADDRESS_ALIGIN
	bne boot_error

	adr r1, pv_offset			@ store physical to virtual offset to pv_offset 
	adr r0, _start				@ phsyic start address
	ldr r2, start				@ virtual start address
	mov r3, r2
	sub r3, r3, #KERNEL_LOAD_ADDRESS_ALIGIN
	adr r4, kernel_virtual_start
	str r3, [r4]
	sub r0, r2, r0
	str r0, [r1]

init_memory_banks:
	bl init_mm_bank

clear_bss:
	adr r2, bss_start
	va_to_pa r2, r3
1:
	mov r0, #0			@ clear the bss section
	ldr r1, bss_end
	va_to_pa r1, r3
2:	
	cmp r2, r1
	strcc r0, [r2], #4
	bcc 2b

	bl init_exception_stack

init_mmu:
	mrc p15,0,r0,c1,c0,0		@ disable d cache
	bic r0, r0, #0x04
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0	@ diable i cache
	bic r0, r0, #(1<<12)
	mcr p15, 0, r0, c1, c0, 0

	mov r1, #0			@ clean invalidate dcache index
3:
	mov r2, #0
4:
	mov r3, r1, lsl #26
	mov r4, r2, lsl #5
	orr r0, r3, r4
	mcr p15, 0, r0, c7, c14, 2

	add r2, r2, #1
	cmp r2, #8
	bls 4b

	add r1, r1, #1
	cmp r1, #64
	bls 3b
	
	mcr p15, 0, r0, c7, c5, 0	@ invalidate id cache

	mrc p15, 0, r0, c1, c0, 0	@ disable mmu
	bic r0, r0, #0x01
	mcr p15, 0, r0, c1, c0, 0

	mcr p15, 0, r0, c8, c7, 0	@ invalidate tlb

	ldr r5, code_end
	sub r5, r5, #KERNEL_BASE_ADDRESS
	add r5, r5, #0x100000
	sub r5, r5, #1
	ldr r6, =0xfff00000
	and r2, r5, r6			@ how many memory we need map for kernel at boot stage
	lsr r2, r2, #20

	ldr r0, =KERNEL_BASE_ADDRESS		@ kernel_base_address--->physical start. for kernel
	mov r1, r0
	va_to_pa r1, r7
	ldr r3, =(MST_WB_AP_SRW_UN | DOMAIN0)
	bl set_mmu_table
	
	ldr r0, = KERNEL_BASE_ADDRESS		@ phsysical start to phsysical when mmu is disable
	va_to_pa r0, r7
	mov r1, r0
	ldr r3, =(MST_WB_AP_SRW_UN | DOMAIN0)
	bl set_mmu_table

	ldr r0, = 0x48000000			@temp for io memory base, this can used for uart debug
	ldr r1, = 0x48000000
	mov r2, #0x200
	ldr r3, =(MST_NCNB_AP_SRW_URW | DOMAIN0)
	bl set_mmu_table

	ldr r0, =KERNEL_BASE_ADDRESS
	va_to_pa r0, r1
	mcr p15, 0, r0, c2, c0, 0

	ldr r0, domain_data			@ set mmu domain
	mcr p15, 0, r0, c3, c0, 0

	mov r0, #0				@ set process id
	mcr p15, 0, r0, c13, c0, 0

	mrc p15, 0, r0, c1, c0, 0		@ enable align fault
	orr r0, r0, #2
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0		@ enable mmu
	orr r0, r0, #1
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0		@ enable i cache
	orr r0, r0, #(1<<12)
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0		@ enable d cache
	orr r0, r0, #4
	mcr p15, 0, r0, c1, c0, 0	

	ldr pc, _start_kernel
	.align 5

_start_kernel:	.word main
domain_data:	.word (0x55555555)	@set all domain to client mode

boot_error:
	mov r0, #0
	b boot_error

	.align 5

set_mmu_table:				@r0=vaddr r1=paddr r2=size_m r3=mem_attr
	ldr r4, =KERNEL_BASE_ADDRESS
	va_to_pa r4, r7
	mov r5, #0

	lsr r0, r0, #20
	lsl r0, r0, #2			@4b for a 1M page
	add r4, r4, r0

6:
	mov r6, r1
	lsr r6, r6, #20			@fill the page table
	add r6, r6, r5
	lsl r6, r6, #20
	orr r6, r6, r3
	
	str r6, [r4], #4		@write phy address to the page table

	add r5, r5, #1
	cmp r5, r2
	bls 6b	
	mov pc, lr

.global init_exception_stack
init_exception_stack:
	ldr r1, =KERNEL_BASE_ADDRESS			@ svc stack base
	add r0, r1, #SVC_STACK_OFFSET
	mov sp, r0

	add r0, r1, #IRQ_STACK_OFFSET			@ irq stack base
	set_mode IRQ_MODE | NO_INT
	mov sp, r0

	add r0, r1, #FIQ_STACK_OFFSET			@ fiq stack base
	set_mode FIQ_MODE | NO_INT
	mov sp, r0

	add r0, r1, #DATA_ABORT_STACK_OFFSET		@ data abort stack base
	set_mode ABORT_MODE | NO_INT
	mov sp, r0

	add r0, r1, #PREFETCH_ABORT_STACK_OFFSET	@ prefetch error stack base
	set_mode UNDEF_MODE | NO_INT
	mov sp, r0

	set_mode SVC_MODE | NO_INT			@ return to SVC mode

	mov pc, lr

.global	code_start
code_start:			.word __code_start

.global code_end
code_end:			.word __code_end

.global bss_start
bss_start:			.word __bss_start

.global bss_end
bss_end:			.word __bss_end

.global init_start
init_start:			.word __init_start

.global init_end
init_end:			.word __init_end

.global	soc_platform_start
soc_platform_start:		.word __soc_platform_start

.global soc_platform_end
soc_platform_end:		.word __soc_platform_end

.global soc_board_start
soc_board_start:		.word __soc_board_start

.global soc_board_end
soc_board_end:			.word __soc_board_end

.global	init_text_start
init_text_start:		.word __init_text_start

.global init_text_end
init_text_end:			.word __init_text_end

.global init_data_start
init_data_start:		.word __init_data_start

.global init_data_end
init_data_end:			.word __init_data_end

.global syscall_table_start
syscall_table_start:		.word __syscall_table_start

.global syscall_table_end
syscall_table_end:		.word __syscall_table_end

.global init_call_start
init_call_start:		.word __init_func_start

.global init_call_end
init_call_end:			.word __init_func_end
