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

static int x86_build_pde_entry(unsigned long pde_address,
			unsigned long pa, int flag)
{
	if (!(flag & PDE_ATTR_PDE) && !(flag & PDE_ATTR_PTE))
		return -EINVAL;

	/* used as pde */
	if (flag & PDE_ATTR_PDE) {
		pa |= (PDE_PS | PDE_RW | PDE_P);
	} else {

	}

	*(unsigned long *)pde_address = pa;

	return 0;
}

struct mmu_ops x86_mmu_ops = {
	.build_pde_entry = x86_build_pde_entry,
};

struct mmu x86_mmu = {
	.tlb_size		= SIZE_4K,
	.kernel_tlb_base	= KERNEL_PDE_BASE,
	.mmu_ops		= &x86_mmu_ops,
};
