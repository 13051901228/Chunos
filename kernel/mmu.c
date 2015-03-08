#include <os/types.h>
#include <os/printk.h>
#include <os/mmu.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/bound.h>
#include <os/errno.h>
#include <config/config.h>
#include <os/panic.h>

#ifdef DEBUG_MMU
#define mmu_debug(fmt, ...) kernel_debug(fmt, ##__VA_ARGS__)
#else
#define mmu_debug(fmt, ...)
#endif

static struct mmu *mmu;

#define MMU_MAP_SECTION		0

void clear_tlb_entry(unsigned long va, size_t size)
{
	
}

/*
 * build_pde_entry
 * this function will help to build first level tlb
 * descriptions when memory maped as section.
 * only support thick page and section mode.
 */
int build_kernel_pde_entry(unsigned long vstart,
		    unsigned long pstart,
		    size_t size, u32 flag)
{
	unsigned long pa; unsigned long va;
	unsigned long pde_addr;

	if (!is_align(size, PDE_ALIGN_SIZE))
		return -EINVAL;

	pa = PDE_MIN_ALIGN(pstart);
	va = PDE_MIN_ALIGN(vstart);

	if ((pstart - pa != 0) || (vstart - va != 0))
		return -EINVAL;

	flag |= PDE_ATTR_PDE;
	pde_addr = mmu->kernel_tlb_base +
		(va / PDE_ALIGN_SIZE) * sizeof(unsigned long);

	while (size) {
		mmu->mmu_ops->build_pde_entry(pde_addr, pa, flag);
		pa += PDE_ALIGN_SIZE;
		pde_addr += sizeof(unsigned long);
		size -= PDE_ALIGN_SIZE;
	}

	arch_flush_mmu_tlb();

	return 0;
}

int mmu_create_pde_entry(unsigned long pde_entry_addr,
		unsigned long pte_base, unsigned long user_addr)
{
	return 0;
}

int mmu_create_pte_entry(unsigned long pte_entry_addr,
		unsigned long pa, unsigned long user_addr)
{
	return 0;
}

unsigned long inline mmu_pde_entry_to_pa(unsigned long pde)
{
	return 0;
}

unsigned long inline mmu_pte_entry_to_pa(unsigned long pte)
{
	return 0;
}

void inline mmu_copy_kernel_pde(unsigned long base)
{

}

void inline mmu_clear_pte_entry(unsigned long pte)
{

}

int mmu_init(void)
{
	mmu = get_soc_mmu();
	if (!mmu)
		panic("No mmu found please disable mmu function\n");

	return 0;
}
