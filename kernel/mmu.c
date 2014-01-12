#include <os/types.h>
#include <os/printk.h>
#include <os/mmu.h>
#include <os/string.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/sched.h>
#include <asm/asm_mmu.h>
#include <os/bound.h>
#include <os/errno.h>

static inline unsigned long get_tlb_base(void)
{
	return arch_get_tlb_base();
}

static inline u32 build_tlb_des(unsigned long pa, u32 attr)
{
	return arch_build_tlb_des(pa, attr);
}

static inline u32 build_page_table_des(unsigned long pa, u32 attr)
{
	return arch_build_page_table_des(pa, attr);
}

static inline u32 get_tlb_attr(u32 flag)
{
	return arch_get_tlb_attr(flag);
}

static inline u32 get_page_table_attr(u32 flag)
{
	return arch_get_page_table_attr(flag);
}

void clear_tlb_entry(unsigned long va, size_t size)
{
	int i;
	unsigned long *tlb_base = (unsigned long *)get_tlb_base();

	if(va == 0)
		va = 1;
	/*
	 * for this type of loop, we can optimize it
	 * do it later we system is ok.
	 */
	for (i = ((va-1) >> 20); i < size; i++){
		*tlb_base = 0;
		tlb_base++;
	}

	flush_mmu_tlb();
}

/*
 * build_tlb_table_entry
 * this function will help to build first level tlb
 * descriptions when memory maped as section.
 * only support thick page and section mode.
 */
int build_tlb_table_entry(unsigned long vstart,
			  unsigned long pstart,
			  size_t size, u32 flag)
{
	unsigned long *tlb_base;
	int offset;
	unsigned long ps;
	int i;
	u32 value;
	u32 attr = 0;

	if (is_aligin(vstart, SIZE_1M) &&
	    is_aligin(vstart, SIZE_1M) &&
	    is_aligin(size, SIZE_1M)) {

		tlb_base = (unsigned long *)get_tlb_base();
		ps = pstart;
		offset = (vstart - 0) >> 20;
		tlb_base += offset;
		attr = get_tlb_attr(flag);

		for (i = 0; i < (size >> 20); i++) {
			value = build_tlb_des(ps, attr);
			*(unsigned long *)tlb_base = value;

			/*
			 * if TLB is section mode then add 1M one time
			 * else add 1K one time.
			 */
			if ((flag & TLB_ATTR_MEMORY_MASK) == TLB_ATTR_USER_MEMORY)
				ps += SIZE_1K;
			else
				ps += SIZE_1M;
			tlb_base ++;
		}

		flush_mmu_tlb();

		return 0;
	}
	return -EINVAL;
}

/*
 * vstart is the virtual address, base is the page
 * table base, base will record the tlb of vstart
 * create the entry of a 2level page table address
 */
int build_page_table_entry(unsigned long base,
			   unsigned long vstart,
			   size_t size, u32 flag)
{
	int i;
	unsigned long *addr_base = (unsigned long *)base;
	u32 value;
	u32 attr;
	unsigned long pa = va_to_pa(vstart);

	if (!is_aligin(size, PAGE_SIZE) || size > SIZE_1M) {
		kernel_error("build page_table_entry: size not aligin");
		return -EINVAL;
	}

	attr = get_page_table_attr(flag);
	for (i = 0; i < (size >> PAGE_SHIFT); i++) {
		value = build_page_table_des(pa, attr);
		*addr_base = value;

		addr_base++;
		pa += PAGE_SIZE;
	}

	return 0;
}
