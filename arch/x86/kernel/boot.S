	.text

#include "include/x86_config.h"

#define PDE_G		(1 << 8)	/* global page */
#define PDE_PS		(1 << 7)	/* page size */
#define PDE_D		(1 << 6)	/* dirty */
#define PDE_A		(1 << 5)	/* accessed */
#define PDE_PCD		(1 << 4)	/* cached 0: enable 1: disable */
#define PDE_PWT		(1 << 3)	/* cache type 0: write-back 1: write-through */
#define PDE_US		(1 << 2)	/* user/supvisor (0 is super) */
#define PDE_RW		(1 << 1)	/* read write */
#define PDE_P		(1 << 0)	/* present */

.macro va_to_pa, reg
	sub %ebp, \reg
.endm

.global _start
_start:
	cli				# disable interrupt

	mov %cr0, %ebx			# disable cache
	btr $30, %ebx
	mov %ebx, %cr0

	lgdt gdt_table			# setup new gdt for kernel

	mov $0x08, %ax
	mov %ax, %cs

	mov $0x10, %ax			# load ds
	mov %ax, %ds

	mov $0x18, %ax			# load ss
	mov %ax, %ss

	mov $0x20, %ax			# load es
	mov %ax, %es

	mov $0x28, %ax			# load fs
	mov %ax, %fs

	mov $0x30, %ax			# load gs
	mov %ax, %gs

	mov %cr0, %ebx			# ensure the code is in protect mode
	bts $0, %eax
	mov %eax, %cr0

	jmp %cs:reset			# load new cs and goto reset
	
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
	sub %eax, %ebp			# ebp is the pv offset before the gdt is loaded

	lea pv_offset, %ecx		# store the pv offset value to the memroy
	sub %ebp, %ecx
	mov %ebp, (%ecx)

	mov %eax, %ecx
	sub $0xa000, %ecx
	lea kernel_phy_start, %edx	# store the physic start address of the kernel
	sub %ebp, %edx
	mov %ecx, (%edx)

	lea soc_desc, %ecx
	va_to_pa %ecx			# fill the soc information
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

	lea kernel_virtual_start, %esi
	va_to_pa %esi			# store the kernel_virtual base address
	lea _start, %edx
	sub $0xa000, %edx
	mov %edx, (%esi)

clear_bss:
	lea bss_start, %eax
	lea bss_end, %ecx
	va_to_pa %eax
	va_to_pa %ecx
	mov $0, %edx
1:
	cmp %eax, %ecx
	jbe set_boot_pde
	mov %edx, %eax
	add $4, %eax
	jmp 1b

set_boot_pde:
	mov $KERNEL_PDE_BASE, %eax
	lea kernel_phy_start, %ebx
	va_to_pa %eax				# eax : pde table base address
	va_to_pa %ebx
	mov (%ebx), %ebx			# ebx : the physic base address of the kernel
	lea kernel_virtual_start, %ecx
	va_to_pa %ecx				# ecx : the virtual base address of the kernel
	mov (%ecx), %ecx
	mov $12, %edx				# edx : size to maped

	and $(0xffc00000), %ebx			# physic address needed 4m aligin
	and $(0xffc00000), %ecx			# virtual address need to be 4m aligin
	shr $(22), %ecx
	shl $(2), %ecx
	add %ecx, %eax
2:
	cmp $0, %edx
	jbe copy_gdt

	mov %ebx, %esi
	and $0xffc00000, %esi
	or $(PDE_PS | PDE_RW | PDE_P), %esi	# 4M PDE, r/w write-back cache enable

	mov %esi, (%eax)
	add $4, (%eax)
	sub $4, (%edx)
	add $0x400000, %ebx
	jmp 2b

copy_gdt:
	lea gdt_table, %eax
	lea gdt_table_end, %ecx
	sub %eax, %ecx
	shl $2, %ecx
	lea gdt_table, %esi
	mov $SYSTEM_GDT_BASE, %edi
	va_to_pa %esi
	va_to_pa %edi
	rep movsd

set_kernel_stack:
	mov $KERNEL_STACK_BASE, %esp		# setup stack for kernel

set_pde_base:
	mov $KERNEL_PDE_BASE, %eax
	va_to_pa %eax				# set paging base address
	mov %eax, %cr3				

pse_enable:					# if use 4M PDE need to enable pse
	mov %cr4, %eax
	bts $4, %eax
	mov %eax, %cr4

enable_cache:
	mov %cr0, %eax
	bts $30, %eax
	mov %eax, %cr0

enable_paging:
	mov %cr0, %eax				# enable paging
	bts $31, %eax
	mov %eax, %cr0

goto_main:
	lea __main, %eax
	mov (%eax), %eax
	jmp %eax

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
