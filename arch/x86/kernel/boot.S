/*
 * arch/x86/kernel/boot.S
 *
 * Created by Le Min (lemin9538@163.com)
 */
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

.macro va2pa, reg
	sub %ebp, \reg
.endm

.global _start
_start:
	cli
	cld
	wbinvd

	/*
	 * temp code, bootloader need to tell
	 * the physical start address of kernel
	 */
	mov	$0x1000a000, %eax
	jmp	reset

	.align 4

.global gdt_table_ptr
gdt_table_ptr:	.word 512
		.long SYSTEM_GDT_BASE

	.align 4

.global idt_table_ptr
idt_table_ptr:	.word 512
		.long SYSTEM_IDT_BASE

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
disable_cache:
	mov	%cr0, %ebx
	btr	$30, %ebx
	mov	%ebx, %cr0

disable_paging:
	mov	%cr0, %ebx
	btr	$31, %ebx
	mov	%ebx, %cr0

/* ebp stores the pv_offset */
store_pv_offset:
	lea	_start, %ebp
	sub	%eax, %ebp
	lea	pv_offset, %ecx
	sub	%ebp, %ecx
	mov	%ebp, (%ecx)

check_phsyic_start_address:
	mov	%eax, %ebx
	mov	$0xfffff, %ecx
	and	%ecx, %ebx
	cmp	$0xa000, %ebx
	jne	loop

store_phy_start_address:
	lea	kernel_phy_start, %ebx
	va2pa	%ebx
	sub	$0xa000, %eax
	mov	%eax, (%ebx)

/* temp, soc information is filled by bootloader */
fill_soc_information:
	lea	soc_desc, %ecx
	va2pa	%ecx
	mov	$0x8087, %edx
	mov	$0x636, %esi
	mov	$0x1, %edi
	movl	%edx, (%ecx)
	movl	%esi, 2(%ecx)
	movl	%edi, 4(%ecx)

store_virtual_start_address:
	lea	kernel_virtual_start, %esi
	va2pa	%esi			# store the kernel_virtual base address
	lea	_start, %edx
	sub	$0xa000, %edx
	mov	%edx, (%esi)

clear_bss:
	lea	bss_start, %eax
	lea	bss_end, %ecx
	va2pa	%eax
	va2pa	%ecx
	mov	(%eax), %eax
	mov	(%ecx), %ecx
	va2pa	%eax
	va2pa	%ecx
	mov $0, %edx
1:
	cmp	%eax, %ecx
	je	clean_pde_table
	movl	%edx, (%eax)
	add	$4, %eax
	jmp	1b


clean_pde_table:
	mov	$KERNEL_PDE_BASE, %eax
	va2pa	%eax
	mov	$0, %ebx
	mov	$KERNEL_PDE_SIZE, %ecx
	add	%eax, %ecx
2:
	cmp	%eax, %ecx
	jbe	set_boot_pde_1
	mov	%ebx, (%eax)
	add	$4, %eax
	jmp	2b

/* KERNEL_BASE_ADDR maped to its physical mem */
set_boot_pde_1:
	mov	$KERNEL_PDE_BASE, %eax
	lea	kernel_phy_start, %ebx
	va2pa	%eax
	va2pa	%ebx
	mov	(%ebx), %ebx
	mov	%ebx, %edx
	lea	kernel_virtual_start, %ecx
	va2pa	%ecx
	mov	(%ecx), %ecx

	lea	code_end, %edx
	va2pa	%edx
	mov	(%edx), %edx
	sub	$KERNEL_VIRTUAL_BASE, %edx
	add	$(0x3fffff), %edx
	and	$(~0x3fffff), %edx
	shr	$(20), %edx
	mov	%edx, %edi

	and	$(0xffc00000), %ebx
	and	$(0xffc00000), %ecx
	shr	$(22), %ecx
	shl	$(2), %ecx
	add	%ecx, %eax
3:
	cmp	$0, %edx
	jbe	set_boot_pde_2

	mov	%ebx, %esi
	or	$(PDE_PS | PDE_RW | PDE_P), %esi

	mov	%esi, (%eax)
	add	$4, %eax
	sub	$4, %edx
	add	$0x400000, %ebx
	jmp	3b

set_boot_pde_2:
	mov	$KERNEL_PDE_BASE, %eax
	lea	kernel_phy_start, %ebx
	va2pa	%eax
	va2pa	%ebx
	mov	(%ebx), %ebx
	mov	%ebx, %ecx
	mov	%edi, %edx

	and	$(0xffc00000), %ebx
	and	$(0xffc00000), %ecx
	shr	$(22), %ecx
	shl	$(2), %ecx
	add	%ecx, %eax
4:
	cmp	$0, %edx
	jbe	clean_gdt_table

	mov	%ebx, %esi
	or	$(PDE_PS | PDE_RW | PDE_P), %esi

	mov	%esi, (%eax)
	add	$4, %eax
	sub	$4, %edx
	add	$0x400000, %ebx
	jmp	4b

clean_gdt_table:
	mov	$SYSTEM_GDT_BASE, %eax
	va2pa	%eax
	mov	$SYSTEM_GDT_SIZE, %ecx
	mov	$0, %ebx
	add	%eax, %ecx
5:
	cmp	%eax, %ecx
	jbe	copy_gdt
	mov	%ebx, (%eax)
	add	$4, %eax
	jmp	5b

copy_gdt:
	lea	gdt_table, %eax
	lea	gdt_table_end, %ecx
	sub	%eax, %ecx
	shr	$2, %ecx
	lea	gdt_table, %esi
	mov	$SYSTEM_GDT_BASE, %edi
	va2pa	%esi
	va2pa	%edi
	rep	movsd

set_kernel_stack:
	mov	$X86_KERNEL_STACK_BASE, %esp

set_pde_base:
	mov	$KERNEL_PDE_BASE, %eax
	va2pa	%eax
	mov	%eax, %cr3

load_idt_table:
	lea	idt_table_ptr, %eax
	va2pa	%eax
	mov	%eax, %ecx
	add	$2, %ecx
	mov	(%ecx), %ebx
	va2pa	%ebx
	mov	%ebx, (%ecx)
	lidt	(%eax)

load_new_gdt:
	lea	gdt_table_ptr, %eax
	va2pa	%eax
	mov	%eax, %ecx
	add	$2, %ecx
	mov	(%ecx), %ebx
	va2pa	%ebx
	mov	%ebx, (%ecx)
	lgdt	(%eax)

	movw	$0x10, %ax
	movw	%ax, %ds

	movw	$0x18, %ax
	movw	%ax, %ss

	movw	$0x20, %ax
	movw	%ax, %es

	movw	$0x28, %ax
	movw	%ax, %fs

	movw	$0x30, %ax
	movw	%ax, %gs

pse_enable:
	mov	%cr4, %eax
	bts	$4, %eax
	mov	%eax, %cr4

enable_cache:
	mov	%cr0, %eax
	bts	$30, %eax
	mov	%eax, %cr0

enable_paging:
	mov	%cr0, %eax
	bts	$31, %eax
	mov	%eax, %cr0

goto_main:
	ljmp	$0x8, $main

loop:
	jmp	loop

	.align 8
gdt_table:				# base 0 limit 0xfffff
	.quad	0x0			# first entry is NULL
	.quad	0x00cf9a000000ffff	# kernel cs
	.quad	0x00cf92000000ffff	# kernel ds expand-up
	.quad	0x00cf92000000ffff	# kernel ss expand-up
	.quad	0x00cf92000000ffff	# kernel es expand-up
	.quad	0x00cf92000000ffff	# kernel fs expand-up
	.quad	0x00cf92000000ffff	# kernel gs expand-up
	.quad	0x00cffa000000ffff	# user cs
	.quad	0x00cff2000000ffff	# user ds expand-up
	.quad	0x00cff6000000ffff	# user ss expand-up
	.quad	0x00cff2000000ffff	# user es expand-up
	.quad	0x00cff2000000ffff	# user fs expand-up
	.quad	0x00cff2000000ffff	# user gs expand-up
gdt_table_end:
