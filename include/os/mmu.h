#ifndef _MMU_H
#define _MMU_H

#include <asm/asm_mmu.h>
#include <os/mm.h>
#include <os/types.h>

#define PAGE_MAP_SIZE	(PAGE_SIZE/sizeof(u32) * PAGE_SIZE)

/*
 *memory usage
 */
#define TLB_ATTR_KERNEL_MEMORY		0x00000001
#define TLB_ATTR_DMA_MEMORY		0x00000002
#define TLB_ATTR_IO_MEMORY		0x00000004
#define TLB_ATTR_USER_MEMORY		0x00000008
#define TLB_ATTR_MEMORY_MASK		0x0000000f

static inline void flush_mmu_tlb(void)
{
	arch_flush_mmu_tlb();
}

static inline void flush_cache(void)
{
	arch_flush_cache();
}

int build_tlb_table_entry(unsigned long vstart,
			  unsigned long pstart,
			  size_t size, u32 flag);

int build_page_table_entry(unsigned long base,
			   unsigned long vstart,
			   size_t size, u32 flag);
void clear_tlb_entry(unsigned long va, size_t size);

#endif
