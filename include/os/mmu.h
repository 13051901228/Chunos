#ifndef _MMU_H
#define _MMU_H

#include <os/mm.h>
#include <os/types.h>
#include <asm/cpu.h>

#define PAGE_MAP_SIZE	(PAGE_SIZE/sizeof(u32) * PAGE_SIZE)

/*
 *memory usage
 */
#define PDE_ATTR_KERNEL_MEMORY		0x00000001
#define PDE_ATTR_DMA_MEMORY		0x00000002
#define PDE_ATTR_IO_MEMORY		0x00000004
#define PDE_ATTR_USER_MEMORY		0x00000008
#define PDE_ATTR_PDE			0x00000010
#define PDE_ATTR_PTE			0x00000020
#define PDE_ATTR_MEMORY_MASK		0x0000000f

#define PDE_ALIGN_SIZE			ARCH_PDE_ALIGN_SIZE
#define PDE_ALIGN(addr)			ARCH_PDE_ALIGN(addr)
#define PDE_MIN_ALIGN(addr)		ARCH_PDE_MIN_ALIGN(addr)

struct mmu_ops {
	int (*build_pde_entry)(unsigned long pde_base,
			unsigned long pa, int flag);

	int (*build_pte_entry)(unsigned long *addr,
			unsigned long pa, int flag);

	int (*clear_pde_entry)(unsigned long *base, unsigned long va);
	int (*clear_pte_entry)(unsigned long *base, unsigned long va);
	void (*invalid_pgt)(void);
};

struct mmu {
	size_t tlb_size;
	unsigned long kernel_tlb_base;
	struct mmu_ops *mmu_ops;
};

static inline void flush_mmu_tlb(void)
{
	arch_flush_mmu_tlb();
}

static inline void flush_cache(void)
{
	arch_flush_cache();
}

int build_kernel_pde_entry(unsigned long vstart,
			  unsigned long pstart,
			  size_t size, u32 flag);

void clear_tlb_entry(unsigned long va, size_t size);

#endif
