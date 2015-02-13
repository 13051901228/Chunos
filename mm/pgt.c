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

#define PGT_ALLOC_BUFFER_MIN		(64)

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
#if 0
		page = list_to_page(list_next(&pgt_buffer.list));
		del_page_from_list(page);
#endif
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

static int init_pte(struct task_page_table *table)
{
	struct page *page;

	if (table->pte_alloc_size) {
		table->pte_current_page = list_next(&table->pte_list);
		page = list_to_page(list_next(&table->pte_list));
		table->pte_free_size = table->pte_alloc_size - PAGE_SIZE;
		table->pte_free_base = page_to_va(page);
		table->pte_current_free = PAGE_SIZE;
	} else {
		init_list(&table->pte_list);
		table->pte_alloc_size = 0;
		table->pte_free_size = 0;
		table->pte_free_base = 0;
		table->pte_current_free = 0;
		table->pte_current_page = NULL;
	}

	return 0;
}

static unsigned long alloc_new_pte(struct task_page_table *table)
{
	struct page *page;
	unsigned long pte_free_base = 0;

	if (table->pte_free_size) {
		/* 
		 * alloc new page for pte pgt
		 */
		page = request_pages(1, GFP_PGT);
		if (!page)
			return -ENOMEM;

		add_page_to_list_tail(page, &table->pte_list);
		table->pte_current_page = list_prve(&table->pte_list);

		pte_free_base = page_to_va(page);
		memset((char *)pte_free_base, 0, PAGE_SIZE);

		table->pte_alloc_size += PAGE_SIZE;
		table->pte_free_size = PAGE_SIZE;
		table->pte_current_free = PAGE_SIZE;
		table->pte_free_base = pte_free_base;
	} else {
		/*
		 * fetch a new page from the pte_list
		 */
		table->pte_current_page = list_next(table->pte_current_page);
		page = list_to_page(table->pte_current_page);
		table->pte_free_size -= PAGE_SIZE;
		table->pte_current_free = PAGE_SIZE;
		table->pte_free_base = page_to_va(page);
		pte_free_base = table->pte_free_base;
		memset((void *)pte_free_base, 0 , PAGE_SIZE);
	}

	return pte_free_base;
}

static int pgt_map_new_pde_entry(struct task_page_table *table,
		unsigned long pde_entry_addr,
		unsigned long user_address)
{
	unsigned long pte_free_base = table->pte_free_base;
	int size;
	
	/* alloc new memory for pte pgt */
	if (!table->pte_current_free) {
		pte_free_base = alloc_new_pte(table);
		if (!pte_free_base)
			return -ENOMEM;
	}

	size = mmu_create_pde_entry(pde_entry_addr,
			table->pte_free_base, user_address);
	if (!size)
		return -EINVAL;

	table->pte_free_size -= size;
	table->pte_current_free -= size;
	table->pte_free_base += size;

	return pte_free_base;
}

int map_task_address(struct task_page_table *table,
		unsigned long va, unsigned long user_addr)
{
	unsigned long pde_addr = 0;
	unsigned long pte_addr = 0;

	pde_addr = mmu_get_pde_entry(table->pde_base, user_addr);
	pte_addr = *(unsigned long *)pde_addr;

	/*
	 * the pte pgt is not ready need to map new
	 */
	if (!pte_addr) {
		pte_addr = pgt_map_new_pde_entry(table,
				pde_addr, user_addr);
		if (!pte_addr)
			return -ENOMEM;
	}

	return mmu_create_pte_entry(pte_addr, va, user_addr);
}

int init_task_page_table(struct task_page_table *table,
		struct task_mm_section *section)
{
	unsigned long base = 0;

	if (!table || !section)
		return -EINVAL;

	/*
	 * if the page table has been alloced
	 * we reinit the pde and pte page table
	 */
	if (!table->pte_alloc_size) {
		memset((void *)table, 0, sizeof(struct task_page_table));
		base = alloc_new_pde();
		if (!base) {
			kernel_error("no memory for task pde\n");
			return -ENOMEM;
		}

		table->pde_base = base;
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
	free_pages_on_list(&pgt->pte_list);
}

int pgt_init(void)
{
	size_t size;
	int i; u32 align;
	struct page *page;
	int count = PGT_ALLOC_BUFFER_MIN;

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
