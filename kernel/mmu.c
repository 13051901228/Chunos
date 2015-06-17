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
#include <os/pde.h>

static struct mmu *mmu;

#define MMU_MAP_SECTION		0

/*
 * build_pde_entry
 * this function will help to build first level tlb
 * descriptions when memory maped as section.
 * only support thick page and section mode.
 */
unsigned long build_kernel_pde_entry(unsigned long vstart,
		    unsigned long pstart,
		    size_t size, u32 flag)
{
	unsigned long pa; unsigned long va;
	unsigned long pde_addr, ret_va;

	if (!is_align(size, PDE_ALIGN_SIZE))
		return -EINVAL;

	pa = PDE_MIN_ALIGN(pstart);
	va = PDE_MIN_ALIGN(vstart);

	ret_va = va + (pstart - pa);

	pde_addr = mmu->boot_pde +
		(va / PDE_ALIGN_SIZE) * sizeof(unsigned long);

	while (size) {
		mmu->mmu_ops->build_pde_entry(pde_addr, pa, flag);
		//mmu->mmu_ops->invalid_tlb(va);
		pa += PDE_ALIGN_SIZE;
		pde_addr += sizeof(unsigned long);
		size -= PDE_ALIGN_SIZE;
		va += PDE_ALIGN_SIZE;
	}

	return ret_va;
}

void inline mmu_create_pde_entry(unsigned long pde_entry_addr,
			unsigned long pte_base, unsigned long va)
{
	mmu->mmu_ops->build_pte_pde_entry(pde_entry_addr,
			pte_base, 0);
}

void inline mmu_create_pte_entry(unsigned long pte_entry_addr,
			unsigned long pa, unsigned long va)
{
	mmu->mmu_ops->build_pte_entry(pte_entry_addr, pa, 0);
	mmu->mmu_ops->invalid_tlb(va);
}

unsigned long inline mmu_pde_entry_to_pa(unsigned long pde)
{
	return mmu->mmu_ops->pte_pde_to_pa(pde);
}

unsigned long inline mmu_pte_entry_to_pa(unsigned long pte)
{
	return mmu->mmu_ops->pte_to_pa(pte);
}

#define KERNEL_PDE_OFFSET (PDE_TABLE_SIZE >> 1)
#define KERNEL_PDE_SIZE	  (PDE_TABLE_SIZE - KERNEL_PDE_OFFSET)

void inline mmu_copy_kernel_pde(unsigned long base)
{
	if (base == mmu->boot_pde)
		return;

	memset((void *)base, 0, PDE_TABLE_SIZE);

	memcpy((char *)base + KERNEL_PDE_OFFSET,
		(char *)(mmu->boot_pde + KERNEL_PDE_OFFSET), KERNEL_PDE_SIZE);
}

unsigned long inline __init_text mmu_get_boot_pde(void)
{
	return mmu->boot_pde;
}

void inline mmu_clear_pte_entry(unsigned long pte)
{
	return mmu->mmu_ops->clear_pte_entry(pte);
}

void inline mmu_clear_pde_entry(unsigned long pde)
{
	return mmu->mmu_ops->clear_pte_pde_entry(pde);
}

int mmu_init(void)
{
	kernel_info("Get system MMU...\n");
	mmu = get_soc_mmu();
	if (!mmu)
		panic("No MMU found in system");

	return 0;
}
