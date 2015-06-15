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
#include <os/pde.h>
#include <os/mirros.h>

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

static struct pgt_buffer pgt_buffer;

static struct page *alloc_new_pde(void)
{
	struct page *page;
	size_t size = PDE_TABLE_SIZE;
	u32 align = PDE_TABLE_ALIGN_SIZE;

	spin_lock_irqsave(&pgt_buffer.pgt_lock);

	if (pgt_buffer.nr_free == 0) {
		/*
		 * alloc new pgt from system
		 */
		page = get_free_pages_align(page_nr(size), align, GFP_PGT);
		if (!page) {
			spin_unlock_irqstore(&pgt_buffer.pgt_lock);
			return NULL;
		}

		pgt_buffer.alloc_nr++;
	} else {
		page = list_to_page(list_next(&pgt_buffer.list));
		list_del(list_next(&pgt_buffer.list));
		pgt_buffer.nr_free--;
	}

	spin_unlock_irqstore(&pgt_buffer.pgt_lock);
	return page;
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
		clist->pte_current_page = NULL;
	}

	return 0;
}

static int inline init_pte(struct task_page_table *table)
{
	init_pte_cache_list(&table->task_list);
	init_pte_cache_list(&table->mmap_list);

	return 0;
}

static struct page *
alloc_new_pte_page(struct pte_cache_list *clist)
{
	struct page *page;
	unsigned long pte_free_base = 0;

	if (!clist->pte_free_size) {
		/* 
		 * alloc new page for pte pgt
		 */
		page = request_pages(1, GFP_PGT);
		if (!page)
			return NULL;

		add_page_to_list_tail(page, &clist->pte_list);
		clist->pte_alloc_size += PAGE_SIZE;
	} else {
		/*
		 * fetch a new page from the pte_list
		 */
		clist->pte_current_page =
			list_next(clist->pte_current_page);
		page = list_to_page(clist->pte_current_page);
		clist->pte_free_size -= PAGE_SIZE;
	}

	pte_free_base = page_to_va(page);
	memset((char *)pte_free_base, 0, PAGE_SIZE);

	return page;
}


static inline unsigned long
pgt_get_pde_entry_addr(unsigned long base, unsigned long mbase)
{
	return base + (mbase >> PDE_ENTRY_SHIFT) * PDE_ENTRY_SIZE;
}

static inline unsigned long
pgt_get_pte_entry_addr(unsigned long base, unsigned long mbase)
{
	return base + (((mbase % PDE_MAP_SIZE) >>
			PAGE_SHIFT) * PTE_ENTRY_SIZE);
}

static inline unsigned long
pgt_get_pte_addr(unsigned long base,
		unsigned long map_address)
{
	unsigned long pde, pte;
	struct page *page;

	pde = pgt_get_pde_entry_addr(base, map_address);
	pte = mmu_pde_entry_to_pa(pde);
	if (!pte)
		return 0;

	page = pa_to_page(pte);
	pte = page_to_va(page);

	return pgt_get_pte_entry_addr(pte, map_address);
}

static inline struct page *
pgt_get_pte_page(unsigned long base, unsigned long mbase)
{
	unsigned long pde, pte;
	struct page *page;

	pde = pgt_get_pde_entry_addr(base, mbase);
	pte = mmu_pde_entry_to_pa(pde);
	if (!pte)
		return NULL;

	page = pa_to_page(pte);

	return page;
}

static struct page *
pgt_map_new_pde_entry(struct pte_cache_list *clist,
		unsigned long pde_base,
		unsigned long user_address)
{
	int i;
	struct page *page;
	unsigned long pte_free_base;
	unsigned long pde;
	unsigned long user_base;

	user_base = min_align(user_address, MAP_SIZE_PER_PAGE);
	pde = pgt_get_pde_entry_addr(pde_base, user_base);

	/* alloc new memory for pte pgt */
	page = alloc_new_pte_page(clist);
	if (!page)
		return NULL;

	pte_free_base = page_to_pa(page);

	/*
	 * PDE size is 4M so 4k size can map 1 pde entry
	 * but arm PDE size is 1M, so 4k size can map 4
	 * pde entry
	 */
	for (i = 0; i < PTE_TABLE_PER_PAGE; i++) {
		mmu_create_pde_entry(pde, pte_free_base, user_base);
		pde += PDE_ENTRY_SIZE;
		pte_free_base += PTE_TABLE_SIZE;
		user_base += PDE_MAP_SIZE;
	}

	page_set_pdata(page, user_base);
	page->pinfo = 0;

	return page;
}

static unsigned long
pgt_get_mapped_pte_addr(struct task_page_table *table,
		unsigned long map_address)
{
	unsigned long pde, pte;
	struct page *page;

	pde = pgt_get_pde_entry_addr(table->pde_base, map_address);
	pte = mmu_pde_entry_to_pa(pde);
	if (!pte) {
		page = pgt_map_new_pde_entry(&table->task_list,
				table->pde_base, map_address);
		if (!page)
			return 0;
	} else {
		page = pa_to_page(pte);
	}

	return pgt_get_pte_entry_addr(page_to_va(page), map_address);
}

static int __pgt_map_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr)
{
	unsigned long pte_addr = 0;

	pte_addr = pgt_get_mapped_pte_addr(table, user_addr);
	if (!pte_addr)
		return -ENOMEM;


	mmu_create_pte_entry(pte_addr, page_to_pa(page), user_addr);
	page_set_map_address(page, user_addr);

	return 0;

}

/* temp implement TBD */
unsigned long
pgt_map_temp_page(struct task_page_table *table, struct page *page)
{
	unsigned long pte_base =
		table->pgt_temp_buffer.tbuf_pte_base;

	mmu_create_pte_entry(pte_base, page_to_pa(page),
			KERNEL_TEMP_BUFFER_BASE);

	return KERNEL_TEMP_BUFFER_BASE;
}

int pgt_unmap_temp_page(struct task_page_table *table, unsigned long base)
{
	return 0;
}

static int
__pgt_map_task_memory(struct task_page_table *table,
		struct list_head *mem_list,
		unsigned long map_base, int type)
{
	struct page *page;
	int offset;
	unsigned long base = map_base;
	unsigned long pte_end = 0, pte = 0;
	struct list_head *list = list_next(mem_list);

	if (!map_base)
		return -EINVAL;

	if (type)
		offset = PAGE_SIZE;
	else
		offset = 0 - PAGE_SIZE;

	while (list != mem_list){
		if (pte == pte_end) {
			pte = pgt_get_mapped_pte_addr(table, map_base);
			if (!pte)
				return -ENOMEM;

			pte_end = min_align(pte + PTES_PER_PDE, PTES_PER_PDE);
		}

		page = list_to_page(list);
		mmu_create_pte_entry(pte, page_to_pa(page), base);
		page_set_map_address(page, base);
		base += offset;

		pte += sizeof(unsigned long);
		list = list_next(list);
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

int pgt_map_task_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr)
{
	return __pgt_map_page(table, page, user_addr);
}


int init_task_page_table(struct task_page_table *table)
{
	unsigned long base = 0;
	struct page *page;
	struct pgt_temp_buffer *tb = &table->pgt_temp_buffer;

	if (!table)
		return -EINVAL;

	/*
	 * if the page table has been alloced
	 * we reinit the pde and pte page table
	 */
	if (!table->pde_base) {
		memset((char *)table, 0,
			sizeof(struct task_page_table));

		page = alloc_new_pde();
		if (!page) {
			kernel_error("No memory for task PDE\n");
			return -ENOMEM;
		}

		table->pde_base = page_to_va(page);
		table->pde_base_pa = page_to_pa(page);

		/*
		 * init temp buffer
		 */
		tb->tbuf_pte_page = request_pages(1, GFP_PGT);
		if (!tb->tbuf_pte_page) {
			release_pde(base);
			return -ENOMEM;
		}

		tb->tbuf_pte_base =
			page_to_va(tb->tbuf_pte_page);
		tb->tbuf_page_nr = PTES_PER_PDE;
	}

	/* 
	 * if do memset op here, it will cause much time
	 * to be fix
	 */
	mmu_copy_kernel_pde(table->pde_base);
	init_pte(table);

	/*
	 * init temp_buffer member
	 */

	base = pgt_get_pde_entry_addr(table->pde_base,
			KERNEL_TEMP_BUFFER_BASE);

	mmu_create_pde_entry(base,
			page_to_pa(tb->tbuf_pte_page),
			KERNEL_TEMP_BUFFER_BASE);

	table->mmap_current_base = PROCESS_USER_MMAP_BASE;

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

static unsigned long
pgt_get_page_mmap_base(struct page *page, int page_nr)
{
	unsigned long ua;
	int start, nr;
	unsigned long *addr, *temp;
	int size = PTES_PER_PDE;
	int sum = 0, again = 0;

	start = page->pinfo;
	nr = start;
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
	temp = (unsigned long *)page_to_va(page);
	addr -= page_nr;
	nr = ((unsigned long)addr -
		(unsigned long)temp) / PTE_ENTRY_SIZE;
	ua = page->pdata + nr * PAGE_SIZE;

	return (unsigned long)addr;
}

static unsigned long
get_new_mmap_base(struct task_page_table *table, int page_nr)
{
	unsigned long user_base, temp;
	unsigned long pde_addr;

	temp = user_base = table->mmap_current_base;

	while (1) {
		pde_addr = pgt_get_pde_entry_addr(table->pde_base, user_base);
		if (!(*(unsigned long *)pde_addr))
			goto new_mmap_base;

		user_base += PDE_MAP_SIZE;
		if (user_base == PROCESS_USER_MMAP_END)
			user_base = PROCESS_USER_MMAP_BASE;

		if (user_base == temp)
			break;
	}

	return 0;

new_mmap_base:
	table->mmap_current_base =
		user_base + PDE_MAP_SIZE;

	return user_base;
}

unsigned long
pgt_get_mmap_base(struct task_page_table *table, int page_nr)
{
	struct list_head *list;
	unsigned long ua = 0;
	struct list_head *head = &table->mmap_list.pte_list;
	struct page *page;

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
	int i;
	unsigned long pte_addr;
	unsigned long pde_base = table->pde_base;

	/*
	 * if the pde entry of this start address is
	 * not maped, return 0 directory
	 */
	pte_addr = pgt_get_pde_entry_addr(pde_base, start);
	if (!(*(unsigned long *)pte_addr))
		return 0;

	/*
	 * check the pte of the address
	 */
	for (i = 0; i < nr; i++) {
		pte_addr = pgt_get_pte_addr(pde_base, start);
		if (*(unsigned long *)pte_addr)
			return -ENXIO;

		start += PAGE_SIZE;
	}

	return 0;
}

int pgt_map_mmap_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr)
{
	/*
	 * for mmap page need to map PAGE_SIZE pte on time
	 * for searching purpose for example on ARM need to
	 * map 4M not 1M
	 */
	unsigned long pte;
	struct page *pte_page;

	pte_page = pgt_get_pte_page(table->pde_base, user_addr);
	if (!pte_page) {
		pte_page = pgt_map_new_pde_entry(&table->mmap_list,
				table->pde_base, user_addr);
		if (!pte_page)
			return -ENOMEM;
	}

	pte = pgt_get_pte_entry_addr(page_to_va(pte_page), user_addr);

	/* update the free nr */
	pte_page->pinfo++;
	mmu_create_pte_entry(pte, page_to_pa(page), user_addr);
	page_set_map_address(page, user_addr);

	return 0;
}

struct page *
pgt_unmap_mmap_page(struct task_page_table *table,
			unsigned long base)
{
	unsigned long pte;
	struct page *page;
	struct page *pte_page;

	pte_page = pgt_get_pte_page(table->pde_base, base);
	if (!pte_page)
		return NULL;

	pte = pgt_get_pte_entry_addr(page_to_va(pte_page), base);
	page = pa_to_page(mmu_pte_entry_to_pa(pte));
	mmu_clear_pte_entry(pte);

	return page;
}

struct page *pgt_get_task_page(struct task_page_table *table,
		unsigned long base)
{
	unsigned long pte;
	struct page *page;

	/*
	 * get the virtual address of the pte entry
	 */
	pte = pgt_get_pte_addr(table->pde_base, base);
	if (!pte)
		return NULL;

	/*
	 * translate the pte entry to physical address
	 */
	pte = mmu_pte_entry_to_pa(pte);

	/*
	 * user page can only use pa_to_page can not use
	 * va_to_page
	 */
	page = pa_to_page(pte);

	return page;
}

int __init_text pgt_init(void)
{
	size_t size;
	int i; u32 align;
	struct page *page;
	int count = CONFIG_PGT_PDE_BUFFER_MIN;

	/* this function needed called after mmu is init */
	memset((char *)&pgt_buffer, 0, sizeof(struct pgt_buffer));
	size = PDE_TABLE_SIZE;
	align = PDE_TABLE_ALIGN_SIZE;
	init_list(&pgt_buffer.list);
	spin_lock_init(&pgt_buffer.pgt_lock);

	/* do not alloc buffer if align size is PAGE_SIZE */
	if (align == PAGE_SIZE)
		count = 0;

	/* Boot pde need to be care about */
	page = va_to_page(mmu_get_boot_pde());
	if (page) {
		add_page_to_list_tail(page, &pgt_buffer.list);
		pgt_buffer.alloc_nr++;
		pgt_buffer.nr_free++;
	}

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
