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
	void (*build_pde_entry)(unsigned long pde_base,
			unsigned long pa, int flag);

	void (*build_pte_pde_entry)(unsigned long pde_base,
			unsigned long pa, int flag);

	void (*build_pte_entry)(unsigned long addr,
			unsigned long pa, int flag);

	void (*clear_pte_pde_entry)(unsigned long base);

	void (*clear_pte_entry)(unsigned long base);

	unsigned long (*pte_pde_to_pa)(unsigned long pde);

	unsigned long (*pte_to_pa)(unsigned long pte);

	void (*invalid_tlb)(void);
};

struct mmu {
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

void mmu_create_pde_entry(unsigned long pde_entry_addr,
			unsigned long pte_base);

void mmu_create_pte_entry(unsigned long pte_entry_addr,
			unsigned long pa);

unsigned long inline mmu_pde_entry_to_pa(unsigned long pde);

unsigned long inline mmu_pte_entry_to_pa(unsigned long pte);

void inline mmu_copy_kernel_pde(unsigned long base);

void inline mmu_clear_pte_entry(unsigned long pte);

void inline mmu_clear_pde_entry(unsigned long pde);

#endif
