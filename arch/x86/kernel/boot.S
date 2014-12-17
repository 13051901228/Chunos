	.text

.global _start
_start:
	jmp reset
	
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

reset:					# eax is the start phsyic address of the kernel seted by bootloader
	cli
	mov $0x1000a000, %eax

	lea _start, %ebx
	sub %eax, %ebx			# the pv offset ebx is the pv offset before the gdt is loaded

	lea pv_offset, %ecx		# store the pv offset value to the memroy
	sub %ebx, %ecx
	mov %ebx, (%ecx)

	mov %eax, %ecx
	sub $0xa000, %ecx
	lea kernel_phy_start, %edx	# store the physic start address of the kernel
	sub %ebx, %edx
	mov %ecx, (%edx)

	lea soc_desc, %ecx		# fill the soc information
	sub %ebx, %ecx
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

	lea kernel_virtual_start, %esi  # store the kernel_virtual base address
	sub %ebx, %esi
	lea _start, %edx
	sub $0xa000, %edx
	mov %edx, (%esi)

	jmp setup_gdt			# ebx is the pv offset

	jmp set_kernel_pageing		# setup pageing for kernel space

loop:
	jmp loop

setup_gdt:

	.align 8
gdt_table:
	.quad 0x0			# first entry is NULL
	.quad 0x0			# kernel mode gdt for cs ds es
	.quad 0x0			# user mode gdt for cs ds es
