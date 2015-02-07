/*
 * mm/slab.c
 *
 * Created by Le Min (lemin9538@163.com)
 */

#include <os/slab.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/kernel.h>

#define SLAB_CACHE_SIZE	(1 * 1024 * 1024)

#ifdef	DEBUG_SLAB
#define debug(fmt, ...)	kernel_debug(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

/* 
 * each allocted memory called by kmalloc has a header named
 * size: size of this allocated memory
 * next: list to the nest slab which in same size
 */
struct slab_header;

struct slab_header {
	size_t size;
	union {
		u32 magic;
		struct slab_header *next;
	};
} __attribute__((packed));

/*
 * slab pool store the slab memory which has the
 * same size
 */
struct slab_pool {
	size_t size;
	struct slab_header *head;
	int nr;
};

struct slab {
	struct list_head plist;
	struct slab_pool *pool;
	struct mutex slab_mutex;
	int pool_nr;

	unsigned long slab_free;
	int free_size;
	size_t alloc_size;
	int alloc_pages;
};

static struct slab slab;
static struct slab *pslab = &slab;

static struct slab_pool slab_pool[] = {
	{16, 	NULL, 0},
	{32, 	NULL, 0},
	{48, 	NULL, 0},
	{64, 	NULL, 0},
	{80, 	NULL, 0},
	{96, 	NULL, 0},
	{112, 	NULL, 0},
	{128, 	NULL, 0},
	{144, 	NULL, 0},
	{160, 	NULL, 0},
	{176, 	NULL, 0},
	{192, 	NULL, 0},
	{208, 	NULL, 0},
	{224, 	NULL, 0},
	{240, 	NULL, 0},
	{256, 	NULL, 0},
	{272, 	NULL, 0},
	{288, 	NULL, 0},
	{304, 	NULL, 0},
	{320, 	NULL, 0},
	{336, 	NULL, 0},
	{352, 	NULL, 0},
	{368, 	NULL, 0},
	{384, 	NULL, 0},
	{400, 	NULL, 0},
	{416, 	NULL, 0},
	{432, 	NULL, 0},
	{448, 	NULL, 0},
	{464, 	NULL, 0},
	{480, 	NULL, 0},
	{496, 	NULL, 0},
	{512, 	NULL, 0},
	{0, 	NULL, 0}		/* size > 512 will store here */
};

#define SLAB_HEADER_SIZE	(sizeof(struct slab_header))
#define SLAB_MAGIC	(0x1234abcd)
#define header_to_base(header)	((void *)((unsigned long)header + SLAB_HEADER_SIZE))
#define base_to_header(base)	(struct slab_header *)((unsigned long)base - SLAB_HEADER_SIZE)
#define SLAB_SIZE(size)		((size) + SLAB_HEADER_SIZE)

static int get_slab_alloc_size(int size)
{
	/* this pool is aligned in 16 now*/
	return align(size, pslab->pool[0].size);
}

static inline int slab_pool_id(int size)
{
	int id = size / pslab->pool[0].size - 1;

	return id >= pslab->pool_nr ? (pslab->pool_nr - 1) : id;
}

static int alloc_new_slab_cache(size_t size)
{
	int nr = page_nr(size);

	/*
	 * when system booting up, allocate one big cache for
	 * slab allocater, if the pool is used out, then one
	 * page one time. and the max size kmalloc can get is
	 * PAGE_SIZE
	 */
	mutex_lock(&pslab->slab_mutex);
	pslab->slab_free = (unsigned long)get_free_pages(nr, GFP_SLAB);
	if (!pslab->slab_free)
		return -ENOMEM;

	page_add_to_list_tail(va_to_page(pslab->slab_free), &pslab->plist);
	pslab->free_size = nr << PAGE_SHIFT;
	pslab->alloc_size += nr << PAGE_SHIFT;
	pslab->alloc_pages += nr;
	mutex_unlock(&pslab->slab_mutex);

	return 0;
}

struct slab_header *get_slab_from_slab_pool(struct slab_pool *slab_pool)
{
	struct slab_header *header;

	header = slab_pool->head;
	slab_pool->head = header->next;
	header->magic = SLAB_MAGIC;

	return header;
}

void add_slab_to_slab_pool(struct slab_header *header,
		struct slab_pool *slab_pool)
{
	header->next = slab_pool->head;
	slab_pool->head	= header;
}

/* find memory from slab cache list */
static void *get_slab_from_pool(int size, int flag)
{
	struct slab_pool *slab_pool;
	struct slab_header *header;
	int id = slab_pool_id(size);
	
	mutex_lock(&pslab->slab_mutex);

	slab_pool = &pslab->pool[id];
	if (slab_pool->head) {
		mutex_unlock(&pslab->slab_mutex);
		return NULL;
	}

	/* get the slab_header to find the free memory */
	header = get_slab_from_slab_pool(slab_pool);
	mutex_unlock(&pslab->slab_mutex);

	return header_to_base(header);
}


static void *get_slab_from_slab_free(int size, int flag)
{
	struct slab_header *header;

	mutex_lock(&pslab->slab_mutex);

	header = (struct slab_header *)pslab->slab_free;
	if (pslab->free_size < SLAB_SIZE(size))
		return NULL;

	memset((void *)header, 0, sizeof(struct slab_header));
	header->size = size;
	header->magic = SLAB_MAGIC;

	/*
	 * if the free size < min_slab_size
	 */
	pslab->slab_free += SLAB_SIZE(size);
	pslab->free_size -= SLAB_SIZE(size);
	if (pslab->free_size < SLAB_SIZE(pslab->pool[0].size)) {
		header->size += pslab->free_size;
		pslab->slab_free = 0;
		pslab->free_size = 0;
	}

	return header_to_base(header);
}

static void *__get_new_slab(int size, int flag)
{
	int id;
	struct slab_pool *pool;
	struct slab_header *header;

	/*
	 * first need to check the free size of
	 * current slab_cache
	 */

	mutex_lock(&pslab->slab_mutex);
	if (pslab->free_size >= SLAB_SIZE(pslab->pool[0].size)) {
		id = slab_pool_id(pslab->free_size -
				SLAB_HEADER_SIZE);
		pool = &pslab->pool[id];
		header = (struct slab_header *)pslab->slab_free;
		memset((void *)header, 0, SLAB_HEADER_SIZE);
		header->size = pslab->free_size - SLAB_HEADER_SIZE;
		header->magic = SLAB_MAGIC;
		add_slab_to_slab_pool(header, pool);

		pslab->free_size = 0;
		pslab->slab_free = 0;
	}
	mutex_unlock(&pslab->slab_mutex);

	if (alloc_new_slab_cache(PAGE_SIZE))
		return NULL;

	return get_slab_from_slab_free(size, flag);
}

static void *get_new_slab(int size, int flag)
{
	/* if ned dma or res memory, return pages to user */
	if (flag == GFP_DMA || flag == GFP_RES || size >= PAGE_SIZE)
		return get_free_pages(page_nr(size), flag);

	return __get_new_slab(size, flag);
}

static void *get_big_slab(int size, int flag)
{
	struct slab_pool *slab_pool;
	struct slab_header *header;
	int id = slab_pool_id(size);

	mutex_lock(&pslab->slab_mutex);

	/*
	 * if the pool has no buffer then search
	 * in the next size pool when the free size
	 * of this pool is not enough
	 */
	while (1) {
		slab_pool = &pslab->pool[id];
		if ((!slab_pool->head)) {
			id ++;
			if (id == pslab->pool_nr)
				return NULL;
		} else {
			break;
		}
	}

	/* get the slab_header to find the free memory */
	header = (struct slab_header *)
		get_slab_from_slab_pool(slab_pool);

	mutex_unlock(&pslab->slab_mutex);

	return header_to_base(header);
}

typedef void *(*slab_alloc_func)(int size, int flag);

static void *__kmalloc(int size, int flag)
{
	int i;
	void *ret = NULL;
	slab_alloc_func func;

	static slab_alloc_func alloc_func[] = {
		get_slab_from_pool,
		get_slab_from_slab_free,
		get_new_slab,
		get_big_slab,
		NULL,
	};

	size = get_slab_alloc_size(size);

	if ((flag != GFP_KERNEL) || (size >= PAGE_SIZE))
		return get_new_slab(size, flag);

	while (1) {
		func = alloc_func[i];
		if (!func)
			break;

		ret = func(size, flag);
		if (ret)
			break;

		i ++;
	}

	return ret;
}

void *kmalloc(int size, int flag)
{
	if (size <= 0)
		return NULL;

	if ((flag != GFP_KERNEL) &&
	    (flag != GFP_DMA) &&
	    (flag != GFP_RES))
		return NULL;

	return __kmalloc(size, flag);
}

void *kzalloc(int size, int flag)
{
	char *ret;

	ret = (char *)kmalloc(size, flag);
	memset(ret, 0, size);

	return (void *)ret;
}

void kfree(void *addr)
{
	struct slab_header *header;
	struct slab_pool *slab_pool;

	if (!addr)
		return;

	debug("kfree free address is 0x%x\n", (unsigned long)addr);
	mutex_lock(&pslab->slab_mutex);
	header = base_to_header(addr);
	if (header->magic != SLAB_MAGIC) {
		free_pages(addr);
		goto out;
	}

	slab_pool = &pslab->pool[slab_pool_id(header->size)];
	add_slab_to_slab_pool(header, slab_pool);

out:
	mutex_unlock(&pslab->slab_mutex);
}


int slab_init(void)
{
	memset((char *)pslab, 0, sizeof(struct slab));
	init_list(&pslab->plist);
	pslab->pool = slab_pool;
	pslab->pool_nr = ARRAY_SIZE(slab_pool);
	init_mutex(&pslab->slab_mutex);

	return alloc_new_slab_cache(SLAB_CACHE_SIZE);
}
