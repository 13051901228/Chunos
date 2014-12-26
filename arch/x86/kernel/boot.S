	.text

#include "include/x86_config.h"

.macro va_to_pa, reg, va
	lea \va, \reg
	sub %ebp, \reg
.endm

.macro build_pde_entry, reg_t, reg_pa
.endm

.global _start
_start:
	cli

	lgdt gdt_table			# setup new gdt for kernel

	mov $0x02, %ax			# load ds
	mov %ax, %ds

	mov $0x03, %ax			# load ss
	mov %ax, %ss

	mov $0x04, %ax			# load es
	mov %ax, %es

	mov $0x05, %ax			# load fs
	mov %ax, %fs

	mov $0x06, %ax			# load gs
	mov %ax, %gs

	mov %cr0, %ebx			# ensure the code is in protect mode
	bts $0, %eax
	mov %eax, %cr0

	jmp $0x01:reset			# load new cs and goto reset
	
	.align 4

.global pv_offset
pv_offset:		.long 0

.global kernel_virtual_start
kernel_virtual_start:	.long 0

.global kernel_phy_start
kernel_phy_start:	.long 0

.global soc_desc
soc_desc:
	.short		0
	.short		0
	.short		0	

	.align 4
reset:
	mov $0x1000a000, %eax		# temp code, bootloader need to tell the physical start address of kernel

	lea _start, %ebp
	sub %ebp, %ebp			# ebp is the pv offset before the gdt is loaded

	lea pv_offset, %ecx		# store the pv offset value to the memroy
	sub %ebp, %ecx
	mov %ebp, (%ecx)

	mov %eax, %ecx
	sub $0xa000, %ecx
	lea kernel_phy_start, %edx	# store the physic start address of the kernel
	sub %ebp, %edx
	mov %ecx, (%edx)

	va_to_pa ecx, soc_desc		# fill the soc information
	mov $0x8086, %edx
	mov $0x01, %esi
	mov $0x02, %edi
	movl %edx, (%ecx)
	movl %esi, 2(%ecx)
	movl %edi, 4(%ecx)

	mov $0xfffff, %ecx		# start address is aligin with specfic addr
	and %ecx, %eax
	cmp $0xa000, %eax
	jne loop

	va_to_pa %esi, kernel_virtual_start  # store the kernel_virtual base address
	lea _start, %edx
	sub $0xa000, %edx
	mov %edx, (%esi)

clear_bss:
	va_to_pa %eax, bss_start
	va_to_pa %ecx, bss_end
	mov $0, %edx
1:
	cmp %eax, %ecx
	jbe end_clear_bss
	mov %edx, %eax
	add $4, %eax
	jmp 1b
end_clear_bss:

set_boot_pde:
	va_to_pa %eax, $KERNEL_PDE_BASE_ADDRESS	# eax : pde table base address
	va_to_pa %ebx, $kernel_phy_start
	mov (%ebx), %ebx			# ebx : the physic base address of the kernel
	va_to_pa %ecx, $kernel_virtual_start	# ecx : the virtual base address of the kernel
	mov $12, %edx				# edx : size to maped

	and $(0x3fffff), %ebx			# physic address needed 4m aligin
	and $(0x3fffff), %ecx			# virtual address need to be 4m aligin
	lsr $(22), %ecx
	lsl $(2), %ecx
	add %ecx, %eax

	cmp %edx, $0
	jbe end_set_boot_pde
	build_pde_entry, %esi, %ebx
	mov %esi, (%eax)
	add $4, (%eax)
	sub $4, (%edx)
end_set_boot_pde:

	mov $KERNEL_STACK_BASE, %esp		# setup stack for kernel

copy_gdt:
	lea gtd_table, %eax
	lea gtd_table_end, %ecx
	sub %eax, %ecx
	shl $2, %ecx
	va_to_pa %esi, gdt_table
	va_to_pa %edi, $SYSTEM_GDT_BASE
	rep movsd

set_pde_base:
	va_to_pa %eax, $KERNEL_PDE_BASE_ADDRESS	# set paging base address
	mov %eax, %cr3				

enable_paging:
	mov %cr0, %eax				# enable paging
	bts $31, %eax
	mov %eax, %cr0

goto_main:
	va_to_pa %eax, __main			# goto main
	mov (%eax), %eax
	jmp eax

loop:
	jmp loop

__main:
	.long main

	.align 8
gdt_table:				# base 0 limit 0xfffff
	.quad 0x0			# first entry is NULL
	.quad 0x00cf9a000000ffff	# kernel cs
	.quad 0x00cf92000000ffff	# kernel ds expand-up
	.quad 0x00cf96000000ffff	# kernel ss expand-down
	.quad 0x00cf92000000ffff	# kernel es expand-up
	.quad 0x00cf92000000ffff	# kernel fs expand-up
	.quad 0x00cf92000000ffff	# kernel gs expand-up
	.quad 0x00cffa000000ffff	# user cs
	.quad 0x00cff2000000ffff	# user ds expand-up
	.quad 0x00cff6000000ffff	# user ss expand-down
	.quad 0x00cff2000000ffff	# user es expand-up
	.quad 0x00cff2000000ffff	# user fs expand-up
	.quad 0x00cff2000000ffff	# user gs expand-up
gdt_table_end:
