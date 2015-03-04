/*
 *
 * mm/pgt.c
 *
 * Created by Le Min at 2.14/12/01
 *
 */

#include <os/mm.h>
#include <os/kernel.h>
#include <os/mmu.h>
#include <os/pgt.h>
#include <os/list.h>
#include <os/spin_lock.h>
#include <os/memory_map.h>
#include <os/sched.h>

#define CONFIG_PGT_PDE_BUFFER_MIN		(64)

/* 
 * this buffer used to allocated memory for page
 * table of task, since some arch's mmu table has
 * aligin limiation, eg for arm the mmu tlb base
 * needed 16K aligin create this buffer in case
 * there is no continous memory for task page table,
 *
 * list: the buffer list of the page table;
 * alloc_nr: how many page_table allocated?
 *
 */
struct pgt_buffer {
	struct list_head list;
	int alloc_nr;
	int nr_free;
	spin_lock_t pgt_lock;
};

struct pgt_map_info {
	unsigned long vir_base;
	size_t size;
};

static struct pgt_buffer pgt_buffer;

static unsigned long alloc_new_pde(void)
{
	struct page *page;
	size_t size;
	u32 align;

	spin_lock_irqsave(&pgt_buffer.pgt_lock);

	if (pgt_buffer.nr_free == 0) {
		/* alloc new pgt from system */
		size = mmu_get_pgt_size();
		align = mmu_get_pgt_align();

		page = get_free_pages_align(page_nr(size), align, GFP_PGT);
		if (!page) {
			spin_unlock_irqstore(&pgt_buffer.pgt_lock);
			return 0;
		}

		pgt_buffer.alloc_nr++;
	} else {
		page = list_to_page(list_next(&pgt_buffer.list));
		list_del(list_next(&pgt_buffer.list));
	}

	spin_unlock_irqstore(&pgt_buffer.pgt_lock);
	return (page_to_va(page));
}

static int release_pde(unsigned long base)
{
	struct page *page = va_to_page(base);
	
	spin_lock_irqsave(&pgt_buffer.pgt_lock);
	add_page_to_list_tail(page, &pgt_buffer.list);
	pgt_buffer.nr_free++;
	spin_unlock_irqstore(&pgt_buffer.pgt_lock);

	return 0;
}

static int init_pte_cache_list(struct pte_cache_list *clist)
{
	if (clist->pte_alloc_size) {
		clist->pte_current_page = &clist->pte_list;
		clist->pte_free_size = clist->pte_alloc_size;
	} else {
		init_list(&clist->pte_list);
		clist->pte_alloc_size = 0;
		clist->pte_free_size = 0;
		clist->pte_current_page = &clist->pte_list;
	}

	return 0;
}

static int init_pte(struct task_page_table *table)
{
	init_pte_cache_list(&table->task_list);
	init_pte_cache_list(&table->mmap_list);

	return 0;
}

static unsigned long alloc_new_pte(struct pte_cache_list *clist)
{
	struct page *page;
	unsigned long pte_free_base = 0;

	if (!clist->pte_free_size) {
		/* 
		 * alloc new page for pte pgt
		 */
		page = request_pages(1, GFP_PGT);
		if (!page)
			return -ENOMEM;

		add_page_to_list_tail(page, &clist->pte_list);
		pte_free_base = page_to_va(page);
		memset((char *)pte_free_base, 0, PAGE_SIZE);

		clist->pte_alloc_size += PAGE_SIZE;
	} else {
		/*
		 * fetch a new page from the pte_list
		 */
		clist->pte_current_page =
			list_next(clist->pte_current_page);
		page = list_to_page(clist->pte_current_page);
		clist->pte_free_size -= PAGE_SIZE;
		pte_free_base = page_to_va(page);
		memset((void *)pte_free_base, 0 , PAGE_SIZE);
	}

	return pte_free_base;
}

static int pgt_map_new_pde_entry(struct pte_cache_list *clist,
		unsigned long pde_entry_addr,
		unsigned long user_address)
{
	unsigned long pte_free_base;
	int ret;
	
	/* alloc new memory for pte pgt */
	pte_free_base = alloc_new_pte(clist);
	if (!pte_free_base)
		return -ENOMEM;

	ret = mmu_create_pde_entry(pde_entry_addr,
			pte_free_base, user_address);
	if (!ret)
		return -EINVAL;

	return pte_free_base;
}

static int __pgt_map_page(struct pte_cache_list *clist,
		unsigned long pde_base, struct page *page,
		unsigned long user_addr)
{
	unsigned long pde_addr = 0;
	unsigned long pte_addr = 0;

	pde_addr = mmu_get_pde_entry(pde_base, user_addr);
	pte_addr = *(unsigned long *)pde_addr;

	/*
	 * the pte pgt is not ready need to map new
	 */
	if (!pte_addr) {
		pte_addr = pgt_map_new_pde_entry(clist,
				pde_addr, user_addr);
		if (!pte_addr)
			return -ENOMEM;
	}

	mmu_create_pte_entry(pte_addr,
			page_to_pa(page), user_addr);
	page_set_map_address(page, user_addr);

	return 0;

}

static struct list_head *
__pgt_map_temp_memory(struct list_head *head,
		int *count, int *nr, int type)
{
	int max, i;
	struct page *page;
	unsigned long base = KERNEL_TEMP_BUFFER_BASE;
	struct task_page_table *table =
		&current->mm_struct.page_table;
	unsigned long pte_base = table->temp_buffer_base;
	struct list_head *list = head;

	if (count == NULL)
		max = 1;
	else
		max = *count;

	max = MIN(max, table->temp_buffer_nr);

	if (!type) {
		base = base - (max << PAGE_SHIFT);
		pte_base += (max * sizeof(unsigned long));
	}
	
	if (type) {
		for (i = 0; i < max; i++) {
			page = list_to_page(list);
			mmu_create_pte_entry(pte_base, 
					page_to_pa(page), base);
			base += PAGE_SIZE;
			pte_base += sizeof(unsigned long);
			list = list_next(list);
		}
	} else {
		for (i = 0; i < max; i++) {
			page = list_to_page(list);
			mmu_create_pte_entry(pte_base, 
					page_to_pa(page), base);
			base -= PAGE_SIZE;
			pte_base -= sizeof(unsigned long);
			list = list_next(list);
		}
	}

	if (*nr)
		*nr = max;

	return list;
}

static int
__pgt_map_task_memory(struct task_page_table *table,
		struct list_head *mem_list,
		unsigned long map_base, int type)
{
	struct list_head *list;
	struct page *page;
	unsigned long base = map_base;

	if (!map_base)
		return -EINVAL;

	if (type) {
		list_for_each(mem_list, list) {
			page = list_to_page(list);
			pgt_map_task_page(table, page, base);
			base += PAGE_SIZE;
		}
	} else {
		list_for_each(mem_list, list) {
			page = list_to_page(list);
			pgt_map_task_page(table, page, base);
			base -= PAGE_SIZE;
		}
	}

	return 0;
}

int pgt_map_task_memory(struct task_page_table *table,
		struct list_head *mem_list,
		unsigned long map_base, int type)
{
	if (type == PGT_MAP_STACK)
		return __pgt_map_task_memory(table,
				mem_list, map_base, 0);

	return __pgt_map_task_memory(table,
			mem_list, map_base, 1);
}

struct list_head *pgt_map_temp_memory(struct list_head *head,
		int *count, int *nr, int type)
{
	if (type == PGT_MAP_STACK)
		return __pgt_map_temp_memory(head,
				count, nr, MEM_TYPE_GROW_DOWN);

	return __pgt_map_temp_memory(head,
			count, nr, MEM_TYPE_GROW_UP);
}

int pgt_map_task_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr)
{
	return __pgt_map_page(&table->task_list,
			table->pde_base, page, user_addr);
}

int pgt_map_mmap_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr)
{
	return __pgt_map_page(&table->task_list,
			table->pde_base, page, user_addr);
}

int init_task_page_table(struct task_page_table *table)
{
	unsigned long base = 0;

	if (!table)
		return -EINVAL;

	/*
	 * if the page table has been alloced
	 * we reinit the pde and pte page table
	 */
	if (!table->pde_base) {
		memset((char *)table, 0,
			sizeof(struct task_page_table));

		table->pde_base = alloc_new_pde();
		if (!table->pde_base) {
			kernel_error("No memory for task PDE\n");
			return -ENOMEM;
		}

		table->temp_buffer_base =
			(unsigned long)get_free_page(GFP_PGT);
		if (!table->temp_buffer_base) {
			release_pde(base);
			return -ENOMEM;
		}

		table->temp_buffer_nr =
			(PAGE_SIZE / sizeof(unsigned long));
	}

	/* 
	 * if do memset op here, it will cause much time
	 * to be fix
	 */
	memset((void *)table->pde_base,
			0, mmu_get_pgt_size());
	init_pte(table);

	return 0;
}

void free_task_page_table(struct task_page_table *pgt)
{
	/* update the pgt_buffer */
	release_pde(pgt->pde_base);

	/* free the pte page table memory */
	pgt->pde_base = 0;
	free_pages_on_list(&pgt->task_list.pte_list);
	free_pages_on_list(&pgt->mmap_list.pte_list);
}

#define	PAGE_PER_LONG	(PAGE_SIZE / sizeof(unsigned long))
static unsigned long
pgt_get_page_mmap_base(struct page *page, int page_nr)
{
	unsigned long data, ua;
	int start, nr;
	unsigned long *addr, *temp;
	int size = PAGE_PER_LONG;
	int sum = 0, again = 0;

	data = page_get_pdata(page);
	start = data & 0xffff;
	nr = (data & 0xffff0000) >> 16;
	ua = page_to_va(page);
	addr = (unsigned long *)ua;
	ua += PAGE_SIZE;
	addr += start;
	temp = addr;

	if ((size - nr) < page_nr)
		return -EINVAL;

	while (1) {
		if (*addr == 0) {
			sum++;
			if (sum == page_nr)
				goto exit_find;
		} else {
			sum = 0;
		}

		if ((unsigned long)addr == ua) {
			again = 1;
			sum = 0;
			addr = (unsigned long *)page_to_va(page);
		}

		if (again) {
			if (addr == temp)
				break;
		}
	}

	return 0;

exit_find:
	start = addr - (unsigned long *)page_to_va(page);
	nr += page_nr;
	pdata = (nr << 16) + start;
	page_set_pdata(page, pdata);
	addr -= page_nr;

	return (unsigned long)addr;
}

static unsigned long
get_new_mmap_base(struct task_page_table *table, int page_nr)
{
	unsigned long user_base, temp;
	unsigned long pde_addr;

	temp = user_base = table->mmap_current_base;

	while (1) {
		pde_addr = mmu_get_pde_entry(table->pde_base, user_base);
		if (!pde_addr)
			goto new_mmap_base;

		user_base += ARCH_PDE_ALIGN_SIZE;
		if (user_base == PROCESS_MMAP_END)
			user_base = PROCESS_MMAP_BASE;

		if (user_base == temp)
			break;
	}

	return 0;

new_mmap_base:
	if (!pgt_map_new_pde_entry(&table->mmap_list,
				pde_addr, user_base))
		return -ENOMEM;

	table->mmap_current_base =
		user_base + PDE_ALIGN_SIZE;

	return user_base;
}

unsigned long
pgt_get_mmap_base(struct task_page_table *table, int page_nr)
{
	struct list_head *list;
	unsigned long ua = 0;
	struct page *page;
	struct list_head *head = &table->mmap_list.pte_list;

	list_for_each(head, list) {
		page = list_to_page(list);
		ua = pgt_get_page_mmap_base(page, page_nr);
		if (ua > 0)
			goto out;
	}

	ua = get_new_mmap_base(table, page_nr);

out:
	return ua;
}

int pgt_check_mmap_addr(struct task_page_table *table,
		unsigned long start, int nr)
{
	unsigned long pte_addr;
	unsigned long pde_addr;
	int i;

	pde_addr = pgt_get_pde_addr(table->pde_base, start);
	if (!pde_addr)
		return 0;

	for (i = 0; i < nr; i++) {
		pte_addr = pgt_get_pde_addr(pde_addr, start);
		if
	}

}

int pgt_init(void)
{
	size_t size;
	int i; u32 align;
	struct page *page;
	int count = CONFIG_PGT_PDE_BUFFER_MIN;

	/* this function needed called after mmu is init */
	memset((char *)&pgt_buffer, 0, sizeof(struct pgt_buffer));
	size = mmu_get_pgt_size();
	align = mmu_get_pgt_align();

	/* do not alloc buffer if align size is PAGE_SIZE */
	if (align == PAGE_SIZE)
		count = 0;

	for (i = 0; i < count; i++) {
		page = get_free_pages_align(page_nr(size),
				align, GFP_PGT);
		if (!page)
			break;

		add_page_to_list_tail(page, &pgt_buffer.list);
		pgt_buffer.alloc_nr++;
		pgt_buffer.nr_free++;
	}

	return 0;
}
