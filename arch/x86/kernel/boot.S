	.text

.macro va_to_pa, reg, va
	lea $va, \reg
	sub %ebp, \reg
.endm

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

	jmp set_boot_pageing		# setup pageing for kernel space

					# setup gdt
					# enable paging
loop:
	jmp main

set_boot_pageing:
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
	ret

	.align 8
gdt_table:
	.quad 0x0			# first entry is NULL
	.quad 0x0			# kernel mode gdt for cs ds es
	.quad 0x0			# user mode gdt for cs ds es
	.quad 0x0			# tss for kernel all task will share one tss
