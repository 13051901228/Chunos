/*
 * arch/x86/kernel/mmu.c
 *
 * Created by Le Min at 2015/01/26
 *
 */

#include <os/mmu.h>
#include <os/soc.h>
#include <os/kernel.h>
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

#define PTE_G		(1 << 8)	/* global page */
#define PTE_PS		(1 << 7)	/* PAT */
#define PTE_D		(1 << 6)	/* dirty */
#define PTE_A		(1 << 5)	/* accessed */
#define PTE_PCD		(1 << 4)	/* cached 0: enable 1: disable */
#define PTE_PWT		(1 << 3)	/* cache type 0: write-back 1: write-through */
#define PTE_US		(1 << 2)	/* user/supvisor (0 is super) */
#define PTE_RW		(1 << 1)	/* read write */
#define PTE_P		(1 << 0)	/* present */


static void inline
x86_build_pde_entry(unsigned long pde_address,
		unsigned long pa, int flag)
{
	pa |= (PDE_PS | PDE_RW | PDE_P);

	/*
	 * if memory type is IO or DMA, must set the
	 * cache type to uncached
	 */
	if ((flag & PDE_ATTR_DMA_MEMORY) || (flag & PDE_ATTR_IO_MEMORY))
		pa |= PDE_PCD;

	*(unsigned long *)pde_address = pa;
}

static void inline
x86_build_pte_pde_entry(unsigned long pde,
		unsigned long pa, int flag)
{
	/* use to map user PDE and use as a 4K page */

	pa |= (PDE_P | PDE_RW);
	*(unsigned long *)pde = pa;
}

static void inline
x86_build_pte_entry(unsigned long pte,
		unsigned long pa, int flag)
{
	pa |= (PTE_P | PTE_RW);
	*(unsigned long *)pte = pa;
}

static inline void
x86_clear_pte_pde_entry(unsigned long pde)
{
	*(unsigned long *)pde = 0;
}

static inline void
x86_clear_pte_entry(unsigned long pte)
{
	*(unsigned long *)pte = 0;
}

static inline unsigned long x86_pte_pde_to_pa(unsigned long pde)
{
	return (*(unsigned long *)pde) & (0xfffc0000);
}

static inline unsigned long x86_pte_to_pa(unsigned long pte)
{
	return (*(unsigned long *)pte) & (0xffffff00);
}

static inline void x86_invalid_tlb(void)
{

}

struct mmu_ops x86_mmu_ops = {
	.build_pde_entry	= x86_build_pde_entry,
	.build_pte_pde_entry	= x86_build_pte_pde_entry,
	.build_pte_entry	= x86_build_pte_entry,
	.clear_pte_entry	= x86_clear_pte_entry,
	.clear_pte_pde_entry	= x86_clear_pte_pde_entry,
	.pte_to_pa		= x86_pte_to_pa,
	.pte_pde_to_pa		= x86_pte_pde_to_pa,
	.invalid_tlb		= x86_invalid_tlb,
};

struct mmu x86_mmu = {
	.kernel_tlb_base	= KERNEL_PDE_BASE,
	.mmu_ops		= &x86_mmu_ops,
};
