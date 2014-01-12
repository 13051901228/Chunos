#ifndef _KCONFIG_H
#define _KCONFIG_H

/* virtual address kernel maped to */
#define KERNEL_BASE_ADDRESS		0x80000000

/*
 * we give kernel 40K to store the important information
 * such mmu page table and kernel stack.
 * so the kernel load address must be 0xa000 aligin.
 * 1K = 0x400 4k = 0x1000
 */
#define KERNEL_LOAD_ADDRESS_MASK	0x000fffff
#define KERNEL_LOAD_ADDRESS_ALIGIN	0x0a000

#define SVC_STACK_OFFSET		0x0a000
#define IRQ_STACK_OFFSET		0x09000
#define FIQ_STACK_OFFSET		0x08000
#define DATA_ABORT_STACK_OFFSET		0x07000
#define PREFETCH_ABORT_STACK_OFFSET	0X06000

/* MMU table size */
#define PAGE_TABLE_SIZE			(4096 * 4)

/* kernel stack size is 4k */
#define COMMAND_LINE_SIZE		(1024 *2)

#endif
