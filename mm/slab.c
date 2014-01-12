#include <os/slab.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/kernel.h>

#ifdef	DEBUG_SLAB
#define debug(fmt, ...)	kernel_debug(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

/* 
 * each allocted memory called by kmalloc has a header named
 * size: size of this allocated memory
 * slab_list: connect to the slab free list which
 * in his size
 */
struct slab_header {
	u32 magic;
	int size;
	struct list_head slab_list;
};

#define SLAB_HEADER_SIZE	sizeof(struct slab_header)
#define SLAB_MIN		16
#define SLAB_STEP		16
#define SLAB_NODE		((PAGE_SIZE - SLAB_HEADER_SIZE) / SLAB_MIN)
#define SLAB_MAX		(SLAB_NODE * SLAB_STEP)

#define SLAB_HEADER_MAGIC	0xabcdabcd
#define SLAB_POOL_MAX_PAGE	((SLAB_MAX_SIZE * 1024 * 1024) >> PAGE_SHIFT)

#define header_to_base(header)	((void *)((unsigned long)header + SLAB_HEADER_SIZE))
#define base_to_header(base)	(struct slab_header *)((unsigned long)base - SLAB_HEADER_SIZE)

#define list_id(size)		((size >> 4) -1)

/*
 * repersent a slab memory pool in system
 * current: page that used to allocate new memory just now.
 * list: memory cache list
 */
struct slab {
	struct mutex slab_mutex;
	struct page *slab_page_current;
	struct list_head list[SLAB_NODE];
	int page_nr;
};

static struct slab slab;
static struct slab *pslab = &slab;

static inline int get_new_alloc_size(int size)
{
	return baligin(size, SLAB_STEP) + SLAB_HEADER_SIZE;
}

static inline void add_slab_to_page(struct page *pg, struct slab_header *header)
{
	if (header && pg)
		list_add(&pg->slab_list, &header->slab_list);
}

static void add_slab_to_list(struct slab_header *header)
{
	struct list_head *list;

	if (header) {
		list = &pslab->list[list_id(header->size)];
		list_add(list, &header->slab_list);
	}
}

static void init_slab_header(struct slab_header *header, int size)
{
	header->magic = SLAB_HEADER_MAGIC;
	header->size = size - SLAB_HEADER_SIZE;
	init_list(&header->slab_list);
}

void inline remove_slab_from_list(struct slab_header *header)
{
	if (header)
		list_del(&header->slab_list);
}

void inline remove_slab_from_page(struct slab_header *header)
{
	if (header)
		list_del(&header->slab_list);
}

static struct slab_header *get_slab_from_page(struct page *pg, int size)
{
	struct slab_header *header = NULL;
	size_t free_size = get_page_free_size(pg);
	unsigned long free_base = get_page_free_base(pg);

	if (free_size >= size) {
		header = (struct slab_header *)free_base;
		init_slab_header(header, size);
		update_page(pg, free_base + size, free_size - size);
		page_get(pg);
	}
	
	return header;
}

/*
 * when need to update the current free page of slab
 * we compare the size of this two pages,and biger ones
 * will be the current free page.
 */
static void update_slab_free_page(struct page *pg)
{
	struct page *current_pg = pslab->slab_page_current;
	struct page *new = NULL;
	struct page *old = NULL;
	struct slab_header *header = NULL;
	int size;
	size_t free_size;

	if (current_pg == NULL) {
		new = pg;
		old = NULL;
	} else if (pg->free_size > current_pg->free_size) {
		new = pg;
		old = current_pg;
	} else {
		new = current_pg;
		old = pg;
	}

	pslab->slab_page_current = new;

	if (!old)
		return;

	free_size = get_page_free_size(old);
	if (free_size > (SLAB_MIN + SLAB_HEADER_SIZE)) {
		size = min_aligin(free_size - SLAB_HEADER_SIZE, SLAB_STEP);
		header = get_slab_from_page(old, size + SLAB_HEADER_SIZE);
		add_slab_to_list(header);
	}
}

/* find memory from slab cache list */
static void *get_slab_in_list(int size, unsigned long flag)
{
	struct list_head *list;
	struct slab_header *header;
	struct page *pg;
	int id = list_id(size);
	
	if (!(flag & GFP_KERNEL))
		return NULL;

	list = &pslab->list[id];
	list = list_next(list);
repet:
	if (is_list_empty(list)) {
		/*
		 * if the slab has the max memeory size, we search
		 * an appropriate memeory from next id list.
		 */
		if (pslab->page_nr >= SLAB_POOL_MAX_PAGE) {
			id ++;
			if (id == SLAB_NODE)
				return NULL;
			list = &pslab->list[id];
			list = list_next(list);

			goto repet;
		}
		else
			return NULL;
	}

	/* get the slab_header to find the free memory */
	header = list_entry(list, struct slab_header, slab_list);

	/* check whether the page has been released. */
	if (!page_state(va_to_page_id((unsigned long)header))) {
		debug("page has been released 0x%x\n",
			      (u32)header + SLAB_HEADER_SIZE);
		list = list_next(list);
		remove_slab_from_list(header);
		goto repet;
	}

	/*
	 * update the page information delete slab from
	 * cache list add slab to page list
	 */
	pg = va_to_page((unsigned long)header);
	remove_slab_from_list(header);
	add_slab_to_page(pg, header);

	debug("get slab in list id %d addr 0x%x\n",
		      list_id(size), (u32)header_to_base(header));

	return header_to_base(header);
}


static void *get_slab_from_page_free(int size, unsigned long flag)
{
	struct page *pg;
	struct slab_header *header;
	size_t new_size = get_new_alloc_size(size);

	pg = pslab->slab_page_current;
	if (pg == NULL)
		return NULL;

	/* If the page has been released, we need a new page */
	if (!page_state(page_to_page_id(pg))) {
		pslab->slab_page_current = NULL;
		return NULL;
	}

	if (new_size > get_page_free_size(pg))
		return NULL;

	header = get_slab_from_page(pg, new_size);
	add_slab_to_page(pg, header);
	debug("get memory from current free page 0x%x\n",
		      (u32)header_to_base(header));

	return header_to_base(header);
}

static void *get_kernel_slab(int size, unsigned long flag)
{
	int count;
	unsigned long base,endp;
	struct page *pg;
	int leave_size = 0;
	struct slab_header *header;

	if (pslab->page_nr >= SLAB_POOL_MAX_PAGE) {
		kernel_error("No enough memory in slab pool\n");
		return NULL;
	}

	if (is_aligin(size, PAGE_SIZE)) {
		base = get_free_pages(page_nr(size), flag);
		if (!base)
			return NULL;
		pg = va_to_page(base);
		set_page_extra_size(pg, 0);
		return base;
	}

	size = get_new_alloc_size(size);
	count = page_nr(size);
	leave_size = size - (count - 1) * PAGE_SIZE;
	base = (unsigned long )get_free_pages(count, flag);
	if (!base) {
		mm_error("get memory faile at get_kernel_slab\n");
		return NULL;
	}

	endp = base + (count - 1) * PAGE_SIZE;

	/*
	 * first we need modify the information of the full page;
	 * for the header page, we override the free_base scope
	 * to record how many slice has been allocated.
	 */
	if (count > 1) {
		pg = va_to_page(base);
		set_page_count(pg, get_page_count(pg) - 1);
		set_page_extra_size(pg, leave_size);
	}

	/* now we modify the last page information */
	pg = va_to_page(endp);
	set_page_flag(pg, __GFP_SLAB);

	/*
	 * the last page was used as SLAB memory,so we need
	 * sub the conter. since the result is base so we
	 * ingrion the return value. at last we need update
	 * the slab's current free page
	 */
	pslab->page_nr += count;

	if (count <= 1) {
		header = get_slab_from_page(pg, leave_size);
		add_slab_to_page(pg, header);
		debug("get memory from new slab 0x%x\n",
			      (u32)header_to_base(header));
		update_slab_free_page(pg);
		return header_to_base(header);
	}
	else {
		unsigned long free_base = get_page_free_base(pg);
		size_t free_size = get_page_free_size(pg);
		update_page(pg, free_base + leave_size, free_size - leave_size);
		update_slab_free_page(pg);
		return (void *)base;
	}
}

static void *get_new_slab(int size, unsigned long flag)
{
	/* if ned dma or res memory, return pages to user */
	if (flag == GFP_DMA || flag == GFP_RES)
		return get_free_pages(page_nr(size), flag);

	return get_kernel_slab(size, flag);
}

static void *__kmalloc(int size, unsigned long flag)
{
	void *ret = NULL;
	size = baligin(size, SLAB_STEP);

	mutex_lock(&pslab->slab_mutex);

	if (flag == GFP_DMA ||
	    flag == GFP_RES ||
	    size >= PAGE_SIZE) {
		goto new_slab;
	}

	/* first we find slab in cache list */
	ret = get_slab_in_list(size, flag);
	if (ret)
		goto exit;
	
	/*
	 * cache list does not cotain the memory we needed
	 * get memory from current free page;
	 */
	ret = get_slab_from_page_free(size, flag);
	if (ret)
		goto exit;
	
	/* finally we need the allocater allocte a new slab */
new_slab:
	ret = get_new_slab(size, flag);

exit:
	mutex_unlock(&pslab->slab_mutex);
	return ret;
}

void *kmalloc(int size, unsigned long flag)
{
	if (size <= 0)
		return NULL;

	if ((flag != GFP_KERNEL) &&
	    (flag != GFP_DMA) &&
	    (flag != GFP_RES))
		return NULL;

	return __kmalloc(size, flag);
}

void *kzalloc(int size,unsigned long flag)
{
	char *ret;

	ret = (char *)kmalloc(size, flag);
	memset(ret, 0, size);

	return (void *)ret;
}

static void free_slice_slab(void *addr)
{
	int usage = 0;
	struct page *pg;
	struct slab_header *header;

	pg = va_to_page((unsigned long)addr);
	header = base_to_header(addr);

	if (header->magic != SLAB_HEADER_MAGIC) {
		kernel_error("free a memory which not allcated by kmalloc()\n");
		return;
	}

	remove_slab_from_page(header);
	add_slab_to_list(header);
	usage = page_put(pg);
	if (!usage)
		pslab->page_nr--;
}

static void free_page_slab(void *addr)
{
	struct page *last_page;
	struct page *pg = va_to_page((unsigned long)addr);
	int count = get_page_count(pg);
	int size = get_page_extra_size(pg);
	struct slab_header *header;

	last_page = pg + count;
	free_pages(addr);
	pslab->page_nr -= count;

	if (size) {
		header = (struct slab_header *)page_to_va(last_page);
		init_slab_header(header, size);
		free_slice_slab(header_to_base(header));
	}
}

void kfree(void *addr)
{
	struct page *pg;
	unsigned long flag;

	if (!addr)
		return;

	mutex_lock(&pslab->slab_mutex);

	pg = va_to_page((unsigned long)addr);
	flag = get_page_flag(pg);

	if ((flag & GFP_DMA) || (flag & GFP_RES))
		return free_pages(addr);

	if (flag & __GFP_SLAB)
		free_slice_slab(addr);
	else {

		if (!is_aligin((unsigned long)addr, PAGE_SIZE))
			return;

		free_page_slab(addr);
	}

	mutex_unlock(&pslab->slab_mutex);
}

int slab_init(void)
{
	int i;

	mm_info("Slab allocter init\n");

	for (i = 0; i < SLAB_NODE; i++)
		init_list(&pslab->list[i]);

	init_mutex(&pslab->slab_mutex);
	pslab->page_nr = 0;

	return 0;
}
