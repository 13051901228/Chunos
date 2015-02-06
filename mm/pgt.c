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
	struct list_head *list;
	int alloc_nr;
	int nr_free;
	spin_lock_t pgt_lock;
};

static struct pgt_buffer pgt_buffer;

static unsigned long alloc_new_lvl1_pgt(void)
{
	struct page *page;
	size_t size;
	int aligin;

	spin_lock_irqsave(&pgt_buffer.pgt_lock);

	if (pgt_buffer.nr_free == 0) {
		/* alloc new pgt from system */
		//size = mmu_get_pgt_size();
		//aligin = mmu_get_pgt_aligin();

		//page = get_free_page_align(page_nr(size), aligin, GFP_PGT);
		if (!page) {
			spin_unlock_irqstore(&pgt_buffer.pgt_lock);
			return 0;
		}

		pgt_buffer.alloc_nr++;
	} else {
		page = list_entry(list_next(&pgt_buffer.list),
				struct page, plist);
		//page_del_from_list(&page);
	}

	spin_unlock_irqstore(&pgt_buffer.pgt_lock);
	return (page_to_va(page));
}

static int release_lvl1_pgt(unsigned long base)
{
	struct page *page = va_to_page(base);
	
	spin_lock_irqsave(&pgt_buffer.pgt_lock);
	//add_page_to_list_tail(page, &pgt_buffer.list);
	pgt_buffer.nr_free++;
	spin_unlock_irqstore(&pgt_buffer.pgt_lock);

	return 0;
}

static int init_lvl2_pgt(struct task_page_table *table)
{
	struct page *page;

	if (table->lvl2_alloc_size) {
		table->lvl2_current_page = list_next(&table->lvl2_pgt_list);
		page = list_entry(list_next(table->lvl2_current_page), struct page, plist);
		table->lvl2_free_size = table->lvl2_alloc_size - PAGE_SIZE;
		table->lvl2_free_base = page_to_va(page);
		table->lvl2_current_free = PAGE_SIZE;
	} else {
		init_list(&table->lvl2_pgt_list);
		table->lvl2_alloc_size;
		table->lvl2_free_size;
		table->lvl2_free_base = 0;
		table->lvl2_current_free = 0;
		table->lvl2_current_page = NULL;
	}

	return 0;
}

static unsigned long alloc_new_lvl2_pgt(struct task_page_table *table)
{
	struct page *page;
	unsigned long lvl2_free_base = 0;

	if (table->lvl2_free_size) {
		/* alloc new page for lvl2 pgt */
		page = request_pages(1, GFP_PGT);
		if (!page)
			return -ENOMEM;

		//add_page_to_list_tail(page, &table->lvl2_pgt_list);
		table->lvl2_current_page = list_prve(&table->lvl2_pgt_list);

		lvl2_free_base = page_to_va(page);
		memset(lvl2_free_base, 0, PAGE_SIZE);

		table->lvl2_alloc_size += PAGE_SIZE;
		table->lvl2_free_size = PAGE_SIZE;
		table->lvl2_current_free = PAGE_SIZE;
		table->lvl2_free_base = lvl2_free_base;
	} else {
		/* fetch a new page from the lvl2_list */
		//table->lvl2_current_page = page_list(table->lvl2_current_page);
		page = list_entry(table->lvl2_current_page, struct page, plist);
		table->lvl2_free_size -= PAGE_SIZE;
		table->lvl2_current_free = PAGE_SIZE;
		table->lvl2_free_base = page_to_va(page);
		lvl2_free_base = table->lvl2_free_base;
		memset((void *)lvl2_free_base, 0 , PAGE_SIZE);
	}

	return lvl2_free_base;
}

static int pgt_map_new_lvl1_entry(struct task_page_table *table,
		unsigned long user_address)
{
	unsigned long lvl2_free_base = table->lvl2_free_base;
	struct page *page;
	int size;
	
	/* alloc new memory for lvl2 pgt */
	if (!table->lvl2_current_free) {
		lvl2_free_base = alloc_new_lvl2_pgt(table);
		if (!lvl2_free_base)
			return -ENOMEM;
	}

	//size = mmu_set_lvl1_pgt_entry(table->lvl1_pgt_base,
	//		table->lvl2_free_base, user_address);
	if (!size)
		return -EINVAL;

	table->lvl2_free_size -= size;
	table->lvl2_current_free -= size;
	table->lvl2_free_base += size;

	return lvl2_free_base;
}

int map_task_address(struct task_page_table *table,
		unsigned long va, unsigned long user_addr)
{
	unsigned long lvl1_pgt_addr = 0;
	unsigned long lvl2_pgt_addr = 0;

	//lvl1_pgt_addr = mmu_get_lvl1_addr(table->lvl1_pgt_base, user_addr);
	lvl2_pgt_addr = *(unsigned long *)lvl1_pgt_addr;

	/* the lvl2 pgt is not ready need to map new*/
	if (!lvl2_pgt_addr) {
		lvl2_pgt_addr = pgt_map_new_lvl1_entry(table, user_addr);
		if (!lvl2_pgt_addr)
			return -ENOMEM;
	}

	//return mmu_set_lvl2_pgt_entry(lvl2_pgt_addr, va, user_addr);
}

int init_task_page_table(struct task_page_table *table,
		struct task_mm_section *section)
{
	int i;
	size_t size;
	int lvl1_pgt_size = 0;
	unsigned long base = 0;
	struct section_pgt_info *info;

	if (!table || !section)
		return -EINVAL;

	/*
	 * if the page table has been alloced
	 * we reinit the lvl1 and lvl2 page table
	 */
	if (table->lvl2_alloc_size) {
		//memset((void *)table->lvl1_pgt_base, 0, mmu_get_pgt_size());
		init_lvl2_pgt(table);
	} else {
		memset((void *)table, 0, sizeof(struct task_page_table));
		base = alloc_new_lvl1_pgt();
		if (!base) {
			kernel_error("no memory for task lvl1_pgt\n");
			return -ENOMEM;
		}

		table->lvl1_pgt_base = base;
		init_lvl2_pgt(table);
	}

	return 0;
}

void free_task_page_table(struct task_page_table *pgt)
{
	/* update the pgt_buffer */
	spin_lock_irqsave(&pgt_buffer.pgt_lock);
	//add_page_to_list_tail(va_to_page(pgt->lvl1_pgt_base), &pgt_buffer.list);
	pgt_buffer.nr_free++;
	spin_unlock_irqstore(&pgt_buffer.pgt_lock);

	/* free the lvl2 page table memory */
	pgt->lvl1_pgt_base = 0;
	//free_pages_on_list(&pgt->lvl2_pgt_list);
}

int pgt_init(void)
{
	size_t size;
	int i, aligin;
	struct page *page;
	int count = PGT_ALLOC_BUFFER_MIN;

	/* this function needed called after mmu is init */
	memset(&pgt_buffer, 0, sizeof(pgt_buffer));
	//size = mmu_get_pgt_size();
	//aligin = mmu_get_pgt_aligin();

	if (aligin == PAGE_SIZE)
		count = 0;

	for (i = 0; i < 0; i++) {
		//page = get_free_page_align(page_nr(size), aligin, GFP_PGT);
		if (!page)
			break;

		//add_page_to_list_tail(page, &pgt_buffer.list);
		pgt_buffer.alloc_nr++;
		pgt_buffer.nr_free++;
	}

	//register_memory_recycle_cb("pgt_alloc", pgt_recycle_page);

	return 0;
}
