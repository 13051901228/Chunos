/*
 * mm/mm.c
 *
 * Created by Le Min (lemin9538@163.com)
 *
 */

#include <os/mm.h>
#include <os/string.h>
#include <os/init.h>
#include <os/mirros.h>
#include <os/mmu.h>
#include <os/kernel.h>
#include <os/panic.h>
#include <os/soc.h>
#include "memory_map.h"

#define ZONE_MAX_SECTION	4

typedef enum __mm_zone_t {
	MM_ZONE_NORMAL = NORMAL_MEM,
	MM_ZONE_DMA = DMA_MEM,
	MM_ZONE_RES = RES_MEM,
#define MM_ZONE_MASK		0x3
#define UNKNOWN_MEM		3
	MM_ZONE_UNKNOWN = UNKNOWN_MEM,
} mm_zone_t;

#ifdef DEBUG_MM
#define mm_debug(fmt, ...)	pr_debug("[  mm:  ] ", fmt,##__VA_ARGS__)
#define mm_info(fmt, ...)	pr_info("[  mm:  ] ", fmt,##__VA_ARGS__)
#define mm_error(fmt, ...)	pr_error("[  mm:  ] ", fmt,##__VA_ARGS__)
#else
#define mm_debug(fmt, ...)
#define mm_info(fmt, ...)
#define mm_error(fmt, ...)
#endif

/*
 * mm_section represent a memory section in the platform
 * phy_start: phyical start
 * vir_start: the adress of virtual which mapped the this section
 * size: section size needed to be 4K align
 * pv_offset: section offset p->v
 * maped: whether this section need to mapped
 */
struct mm_section {
	unsigned long phy_start;
	unsigned long vir_start;
	unsigned long size;
	size_t nr_page;
	long pv_offset;
	size_t maped_size;
	u32 bm_end;
	u32 bm_current;
	size_t nr_free_pages;
	size_t free_size;
	u32 *bitmap;
	struct page *page_table;
};

/*
 * memory_section: each section of this zone
 * mr_section: section count of this zone
 * zone_mutex: mutex of this zone to prvent race
 * total_size: total size of this zone
 * free_size: free size of this zone
 * page_num: total page of this zone
 * free_pages: free pages of this zone
 * bm_start: bit map start position
 * bm_end: bit map end position
 */
struct mm_zone {
	struct mm_section memory_section[ZONE_MAX_SECTION];
	int nr_section;
	struct mutex zone_mutex;
	size_t maped_size;
	unsigned long map_base;
	size_t total_size;
	size_t free_size;
	int page_num;
	int nr_free_pages;
};

/*
 * mm bank represent the memory object of system
 * total_size: total size of used memory in this system
 * total_page: total pages
 * total_section: section size
 * zone: each zones of memory
 */
struct mm_bank {
	u32 total_size;	
	u32 total_page;
	struct mm_zone zone[MM_ZONE_UNKNOWN];
};

static struct mm_bank _mm_bank;
static struct mm_bank *mm_bank = &_mm_bank;

#define NR_MAX_SECTIONS		(MM_ZONE_UNKNOWN * ZONE_MAX_SECTION)
static struct mm_section *section_table[NR_MAX_SECTIONS + 1] = { 0 };

u32 mm_free_page(unsigned long flag)
{
	int i;
	u32 sum = 0;
	flag = flag & MM_ZONE_MASK;

	for (i = 0; i < MM_ZONE_UNKNOWN; i++) {
		if ((flag & i) == i) {
			sum += mm_bank->zone[i].nr_free_pages;
		}
	}

	return sum;
}

static void init_memory_bank(void)
{
	int i;

	memset((char *)mm_bank, 0, sizeof(struct mm_bank));
	for (i = 0; i < MM_ZONE_UNKNOWN; i++)
		init_mutex(&mm_bank->zone[i].zone_mutex);
}

static u32 insert_region_to_zone(struct memory_region *region,
				struct mm_zone *zone)
{
	struct mm_section *section;
	
	/* fix me: we must ensure nr_section < max_zone_section */
	if (zone->nr_section == ZONE_MAX_SECTION) {
		mm_debug("Bug: Zone section is bigger than %d\n",
				ZONE_MAX_SECTION);
		return 0;
	}

	section = &zone->memory_section[zone->nr_section];
	section->phy_start = region->start;
	section->size = region->size;
	section->nr_page = section->size >> PAGE_SHIFT;
	section->nr_free_pages = section->nr_page;

	zone->nr_section++;
	zone->total_size += section->size;
	zone->free_size = zone->total_size;
	zone->page_num = (zone->total_size) >> PAGE_SHIFT;
	zone->nr_free_pages = zone->page_num;

	return (section->size);
}

int get_memory_sections(void)
{
	int i, j, min;
	struct soc_memory_info *info = get_soc_memory_info();
	struct memory_region *region = info->region;
	struct memory_region rtmp;
	struct mm_zone *zone;
	size_t size;

	if (!info->region_nr)
		panic("No any memory found\n");

	/* sort the memory region array from min to max */
	for (i = 0; i < info->region_nr; i++) {
		min = i;

		for (j = i + 1; j < info->region_nr; j++) {
			if ((region[min].start) > (region[j].start))
				min = j;
		}

		if (min != i) {
			rtmp = region[i];
			region[i] = region[min];
			region[min] = rtmp;
		}
	}

	/*
	 * convert each region to mm_section and insert
	 * them to releated memory zone, io memory do not
	 * need to include in the total size
	 */
	for (i = 0; i < info->region_nr; i++) {
		region = &info->region[i]; 
		if (region->attr >= UNKNOWN_MEM)
			continue;

		zone = &mm_bank->zone[region->attr];
		size = insert_region_to_zone(region, zone);
		if (region->attr != UNKNOWN_MEM) {
			mm_bank->total_size += size;
			mm_bank->total_page =
				(mm_bank->total_size) >> PAGE_SHIFT;
		}
	}

	return 0;
}

static void alloc_dma_section(void)
{
	int i, j = 0;
	struct mm_section tmp;
	struct mm_section *pr;
	struct mm_zone *zone;
	struct memory_region rtmp;
	size_t size;

	zone = &mm_bank->zone[MM_ZONE_NORMAL];
	size = align((zone->total_size >> 4), PAGE_SIZE);

	/* choose the section which has the max size */
	pr = &zone->memory_section[0];
	for (i = 1; i < zone->nr_section; i++) {
		if(zone->memory_section[i].size > pr->size)
			pr = &zone->memory_section[i];
	}

	if (zone->nr_section > 1) {
		tmp = *pr;
		for (i = 0; i < zone->nr_section; i++) {
			if ((zone->memory_section[i].size < pr->size) &&
			    zone->memory_section[i].size >= size &&
			    zone->memory_section[i].size < tmp.size) {
				tmp = zone->memory_section[i];
				j = i;
			}
		}

		/* fix me: when the section is boot section */
		if (tmp.size != pr->size)
			pr = &zone->memory_section[j];	
	}

	/*
	 * fix me, we must ensure that the section with max size
	 * must have enough size to split memory for DMA.
	 */
	pr->size -= size;
	pr->nr_page -= size >> PAGE_SHIFT;
	pr->nr_free_pages -= size >> PAGE_SHIFT;

	/*
	 * if the size of this section 0 after split to
	 * dma need to increase the section number of
	 * this zone
	 */
	if (!pr->size)	/* || pr->size < n */
		zone->nr_section--;

	/* get a new mm_region for dma */
	rtmp.start = tmp.phy_start + tmp.size - size;
	rtmp.size = size;
	rtmp.attr = DMA_MEM;

	zone->total_size -= rtmp.size;
	zone->free_size -= rtmp.size;
	zone->page_num -= (rtmp.size) >> PAGE_SHIFT;
	zone->nr_free_pages = zone->page_num;

	mm_debug("no dma zone,auto allocated 0x%x \
			Byte size dma zone\n",rtmp.size);
	insert_region_to_zone(&rtmp, &mm_bank->zone[MM_ZONE_DMA]);
}

static int analyse_soc_memory_info(void)
{
	get_memory_sections();

	/*
	 * if user did not assign the dma zone,we must alloce
	 * it by ourself, dma size = 1 / 16 of the total size
	 * if there is a section which size is equal or a little
	 * bigger than dma size zone we will use it as dma zone
	 */
	if (!mm_bank->zone[MM_ZONE_DMA].nr_section)
		alloc_dma_section();

	return 0;
}

int do_map_memory(void)
{
	struct mm_zone *zone;
	unsigned long flag = 0;
	struct mm_section *section;
	int i, j;
	address_t already_map;
	struct soc_memory_info *info = get_soc_memory_info();

	/* skip the maped memeory in boot stage */
	already_map = info->code_end - KERNEL_VIRTUAL_BASE;
	already_map = align(already_map, PDE_ALIGN_SIZE);

	for (i = 0; i < MM_ZONE_UNKNOWN; i++) {
		flag = 0;
		switch (i) {
			case MM_ZONE_NORMAL:
			case MM_ZONE_RES:
				flag |= PDE_ATTR_KERNEL_MEMORY;
				break;
			case MM_ZONE_DMA:
				flag |= PDE_ATTR_DMA_MEMORY;
				break;
			default:
				kernel_error("Unknown memory zone\n");
				continue;
				break;
		}

		zone = &mm_bank->zone[i];
		for (j = 0; j < zone->nr_section; j++) {
			section = &zone->memory_section[j];
			if (section->vir_start == KERNEL_VIRTUAL_BASE) {
				mm_info("Skip already maped memeory 0x%x\n", already_map);
				build_kernel_pde_entry(section->vir_start + already_map,
						section->phy_start + already_map,
						section->maped_size - already_map, flag);
			} else {
				build_kernel_pde_entry(section->vir_start,
						 section->phy_start,
						 section->maped_size, flag);
			}
		}
	}

	return 0;
}

int map_zone_memory(struct mm_zone *zone, int size)
{
	struct mm_section *section;
	int i;
	size_t map_size = 0;
	size_t tmp;
	unsigned long zone_map_end;
	unsigned long map_base;

	if (size <= 0)
		return -EINVAL;

	/* map_base should be pde align */
	zone_map_end = zone->map_base + size;
	map_base = zone->map_base;
	if (map_base == zone_map_end)
		return 0;

	for (i = 0; i < zone->nr_section; i++) {
		section = &zone->memory_section[i];

		/* if the section already maped skiped this section */
		if (section->maped_size)
			continue;

		/* the case that physical start is not pde align */
		tmp = section->phy_start -
			PDE_MIN_ALIGN(section->phy_start);
		map_base += tmp;

		/* the biggest size that this section can map */
		map_size = zone_map_end - map_base;
		if (map_size > section->size)
			map_size = section->size;

		/* 
		 * pv_offset need to consider on case - when
		 * the phy_start is not ARCH_PDE_ALIGN_SIZE align
		 */
		section->vir_start = map_base;
		section->pv_offset = section->vir_start - section->phy_start;

		section->maped_size = map_size;
		zone->maped_size += map_size;

		map_base = PDE_ALIGN(map_base + map_size);
		zone->map_base = map_base;
		if (zone->map_base == zone_map_end)
			break;
	}

	return 0;
}

static void map_kernel_memory(void)
{
	int i;
	struct mm_zone *zone;
	struct mm_section *section;
	struct soc_memory_info *info;
	size_t size;

	info = get_soc_memory_info();
	zone = &mm_bank->zone[MM_ZONE_NORMAL];
	zone->map_base = KERNEL_VIRTUAL_BASE;

	/*
	 * map normal memory to virtual address, first 
	 * we need to map the address which kernel loaded
	 * kernel is loaded at normal memory, system phsyic
	 * base memory needed to be 0xn0000000.
	 */
	for (i = 0; i < zone->nr_section; i++) {
		section = &zone->memory_section[i];
		if (section->phy_start == info->kernel_physical_start) {
			if (section->size > KERNEL_VIRTUAL_SIZE)
				size = KERNEL_VIRTUAL_SIZE;
			else 
				size = section->size;

			section->vir_start = zone->map_base;
			section->pv_offset =
				section->vir_start - section->phy_start;
			section->maped_size = size;
			zone->maped_size += size;
			zone->map_base += PDE_ALIGN(size);
			break;
		}
	}

	/*
	 * then we can directy map other memeory of other zone;
	 * a question, should we map the IO memory at first?
	 * since it may take over to many virtual memory.
	 */
	map_zone_memory(&mm_bank->zone[MM_ZONE_NORMAL],
			KERNEL_VIRTUAL_SIZE - PDE_ALIGN(size));
}

static void map_dma_memory(void)
{
	struct mm_zone *zone;

	zone = &mm_bank->zone[MM_ZONE_DMA];
	zone->map_base = KERNEL_DMA_BASE;
	map_zone_memory(zone, KERNEL_DMA_SIZE);
}

static void map_res_memory(void)
{
	struct mm_zone *zone;
	size_t map_size;

	/* resverd memory is maped at kernel memory */
	zone = &mm_bank->zone[MM_ZONE_NORMAL];
	mm_bank->zone[MM_ZONE_RES].map_base = zone->map_base;
	map_size = KERNEL_VIRTUAL_BASE + 
		KERNEL_VIRTUAL_SIZE - zone->map_base;

	zone = &mm_bank->zone[MM_ZONE_RES];
	map_zone_memory(zone, map_size);
}

static void map_memory(void)
{
	map_kernel_memory();
	map_dma_memory();
	map_res_memory();

	do_map_memory();
}

static unsigned long init_sections_page_table(unsigned long base)
{
	int i, j;
	struct mm_zone *zone;
	struct mm_section *section;
	struct page *tmp = (struct page *)base;

	for (i = 0; i < MM_ZONE_UNKNOWN; i++) {
		zone = &mm_bank->zone[i];
		for (j = 0; j < zone->nr_section; j++) {
			section = &zone->memory_section[j];
			section->page_table = tmp;
			tmp += section->size >> PAGE_SHIFT;
		}
	}

	memset((char *)base, 0, (unsigned long)tmp - base);

	return (unsigned long)tmp;
}

static unsigned long init_sections_bitmap(unsigned long base)
{
	int i, j;
	struct mm_zone *zone;
	struct mm_section *section;
	u32 *tmp = (u32 *)base;

	for (i = 0; i < MM_ZONE_UNKNOWN; i++) {
		zone = &mm_bank->zone[i];
		for (j = 0; j < zone->nr_section; j++) {
			section = &zone->memory_section[j];
			section->bitmap = tmp;
			section->bm_current = 0;
			section->bm_end = section->size >> PAGE_SHIFT;
			tmp += bits_to_long(section->size >> PAGE_SHIFT);
		}
	}

	memset((char *)base, 0, (unsigned long)tmp - base);

	return (unsigned long)tmp;
}

static int update_boot_section_info(struct mm_section *section,
			size_t kernel_boot_page)
{
	long pv_offset = section->pv_offset;
	size_t size = kernel_boot_page;
	unsigned long tmp = section->vir_start;
	struct page *page = section->page_table;
	int i;

	if (tmp != KERNEL_VIRTUAL_BASE)
		return -EINVAL;

	/*
	 * these memory maped at boot stage must
	 * in the same section
	 */
	for (i = 0; i < size; i++) {
		set_bit(section->bitmap, i);

		/* init page struct */
		page->phy_address = tmp - pv_offset;
		init_list(&page->plist);
		page->free_size = 0;
		page->count = 1;
		page->flag = __GFP_KERNEL;

		tmp += PAGE_SIZE;
	}

	section->bm_current = size;
	section->nr_free_pages -= kernel_boot_page;

	return 0;
}

static int init_page_table(void)
{
	int i;
	unsigned long size;
	struct mm_section *section = NULL;
	unsigned long base_addr = 0;
	struct mm_zone *zone = &mm_bank->zone[MM_ZONE_NORMAL];
	struct soc_memory_info *info = get_soc_memory_info();

	/* page table start addr */
	base_addr = align(info->code_end, sizeof(unsigned long));
	base_addr = init_sections_page_table(base_addr);

	base_addr = align(base_addr, sizeof(u32));
	base_addr = init_sections_bitmap(base_addr);

	/*
	 * here the base addr is the end of the addr
	 * which kernel used before load other module
	 * and start to using kmalloc and get free
	 * page
	 */
	for (i = 0; i < zone->nr_section; i++) {
		section = &zone->memory_section[i];
		if (KERNEL_VIRTUAL_BASE == section->vir_start) {
			break;
		}
	}

	if (section == NULL)
		panic("Some error happed in memory\n");

	/* update the normal zone's information */
	base_addr = align(base_addr, PAGE_SIZE);
	zone->free_size = zone->free_size -
		(base_addr - KERNEL_VIRTUAL_BASE);
	zone->nr_free_pages = zone->free_size >> PAGE_SHIFT;

	/* update the boot section information */
	size = base_addr - info->kernel_virtual_start;
	size = size >> PAGE_SHIFT;
	update_boot_section_info(section, size);

	return 0;
}

static void clear_user_tlb(void)
{
	clear_tlb_entry(0x0, 2048);
}

static void update_section_table(void)
{
	int i, j, k = 0;
	struct mm_zone *zone;

	for (i = 0; i < MM_ZONE_UNKNOWN; i++) {
		zone = &mm_bank->zone[i];
		for (j = 0; j < zone->nr_section; j++) {
			section_table[k] = &zone->memory_section[j];
			k++;
		}
	}
}

int mm_init(void)
{
	mm_info("Init memory management\n");

	/*
	 * before map kernel memory, need to analyse the soc
	 * memory information, and do some extra work. map_memory
	 * will create all page tables for kernel. after init page
	 * table can used all kernel memory.
	 */
	init_memory_bank();
	analyse_soc_memory_info();
	map_memory();
	init_page_table();
	update_section_table();

	/* do not use printk before late_console_init */
	clear_user_tlb();
	return 0;
}

static inline struct mm_section *va_get_section(unsigned long va)
{
	int i = 0;
	struct mm_section *section;

	while (1) {
		section = section_table[i];
		if (!section)
			break;

		if ((va >= section->vir_start) &&
			(va < (section->vir_start + section->size)))
			break;
		i++;
	}

	return section;
}

static inline struct mm_section *pa_get_section(unsigned long pa)
{
	int i = 0;
	struct mm_section *section;

	while (1) {
		section = section_table[i];
		if(!section)
			break;

		if ((pa >= section->phy_start) &&
			(pa < (section->phy_start + section->size)))
			break;
		i++;
	}

	return section;
}

static inline struct mm_section *page_get_section(struct page *page)
{
	return pa_get_section(page->phy_address);
}

#define section_page_id(va, base)	((va - base) >> PAGE_SHIFT)

#define section_va_to_page(section, va)	\
	(&section->page_table[section_page_id((va), section->vir_start)])

#define section_page_to_va(section, page) \
	(section->vir_start + ((page - section->page_table) << PAGE_SHIFT))

#define section_pa_to_page(section, pa)	\
	(&section->page_table[section_page_id((pa), section->phy_start)])

struct page *va_to_page(unsigned long va)
{
	struct mm_section *section = va_get_section(va);

	return section ? section_va_to_page(section, va) : 0;
}

unsigned long page_to_va(struct page *page)
{
	struct mm_section *section = page_get_section(page);

	return section ? section_page_to_va(section, page) : 0;
}

unsigned long page_to_pa(struct page *page)
{
	return (page->phy_address);
}

unsigned long pa_to_va(unsigned long pa)
{
	struct mm_section *section = pa_get_section(pa);

	return section ? (pa + section->pv_offset) : 0;
}

struct page *pa_to_page(unsigned long pa)
{
	struct mm_section *section = pa_get_section(pa);

	return section ? section_pa_to_page(section, pa) : 0;
}

unsigned long va_to_pa(unsigned long va)
{
	return page_to_pa(va_to_page(va));
}

static void init_pages(struct mm_section *section,
		int index, int count, unsigned long flag)
{
	int i;
	struct page *pg = &section->page_table[index];
	long pv_offset = section->pv_offset;
	unsigned long va;

	va = section->vir_start + (index >> PAGE_SHIFT);

	for (i = index; i < index + count; i++) {
		pg->phy_address = va - pv_offset;
		init_list(&pg->plist);

		if (i == index) {
			pg->count = count;
			flag |= __GFP_PAGE_HEADER;
		} else {
			pg->count = 1;
		}

		/*
		 * page table must 4K aligin, so when this page
		 * is used as a page table for process, we need
		 * initilize its free_base scope.
		 */
		pg->free_size = PAGE_SIZE;
		pg->flag = flag;
		va += PAGE_SIZE;
	}
}

static void update_memory_bitmap(u32 *bitmap,
		u32 index, int count, int update)
{
	int i;

	for (i = index; i < index + count; i++) {
		if (update)
			set_bit(bitmap, i);
		else
			clear_bit(bitmap, i);
	}
}

static unsigned long get_free_pages_from_section(struct mm_section *section,
			size_t count, int flag, int vp)
{
	long index;
	unsigned long ret;

	index = bitmap_find_free_base(section->bitmap,
				section->bm_current, 0,
				section->nr_page,count);
	if (index < 0) {
		mm_error("Can not find %d continous pages\n", count);
		return 0;
	}

	/* 
	 * need check whether the memory is areadly maped
	 * or not [TBD]
	 */
	/* if (flag & __GFP_USER) */

	/*
	 * set the releated bit in bitmap
	 * in case of other process read these bit
	 * when get free pages
	 */
	update_memory_bitmap(section->bitmap, index, count, 1);

	/* update the zone information */
	section->bm_current = index + count;
	if (section->bm_current >= section->bm_end)
		section->bm_current = 0;

	section->free_size -= PAGE_SIZE * count;
	section->nr_free_pages -= count;

	init_pages(section, index, count, flag);

	if (vp)
		ret = section->vir_start + (index << PAGE_SHIFT);
	else
		ret = (unsigned long)(section->page_table + index);

	return ret;
}

static unsigned long __get_free_pages(int count, u32 flag, int vp)
{
	struct mm_zone *zone;
	int id;
	struct mm_section *section = NULL;
	unsigned long ret = 0;

	/*
	 * get the zone id from the flag
	 */
	id = (flag & GFP_ZONE_ID_MASK) >> 1;
	if (id >= MM_ZONE_UNKNOWN)
		return 0;

	zone = &mm_bank->zone[id];
	mutex_lock(&zone->zone_mutex);

	if (zone->nr_free_pages < count)
		goto out;

	/* find a section which have count pages */
	for (id = 0; id < zone->nr_section; id++) {
		if (zone->memory_section[id].nr_free_pages < count)
			continue;

		section = &zone->memory_section[id];
		break;
	}

	if (!section)
		goto out;

	ret = get_free_pages_from_section(section, count, flag, vp);
	if (!ret)
		goto out;

	zone->free_size -= (count << PAGE_SHIFT);
	zone->nr_free_pages -= count;

out:
	mutex_unlock(&zone->zone_mutex);
	return ret;
}

struct page *request_pages(int count, int flag)
{
	if (count < 0)
		return NULL;

	return (struct page *)__get_free_pages(count, flag, 0);
}

void *get_free_pages(int count, int flag)
{
	if (count <= 0)
		return NULL;

	return (void *)__get_free_pages(count, flag, 1);
}

void update_bm_current(struct mm_section *section)
{
	int count = 0;
	int start = section->bm_current - 1;

	if (0 == section->bm_current)
		return;

	while (!read_bit(section->bitmap, start)) {
		count ++;
		start--;
	}

	section->bm_current -= count;
	mm_debug("update bm_current count:%d current %d\n",
			count, section->bm_current);
}

static void __free_pages(struct mm_section *section, struct page *page)
{
	int index = 0, count = 0, i;

	index = page - section->page_table;
	count = page->count;

	update_memory_bitmap(section->bitmap, index, count, 0);

	for (i = 0; i < count; i++) {
		page->flag = 0;
		page++;
	}

	update_bm_current(section);
}

static void _free_pages(struct mm_section *section, struct page *page)
{
	struct page *pg = page;
	struct mm_zone *zone;
	int id = 0;

	if (!section || !page)
		return;

	if (!(pg->flag & __GFP_PAGE_HEADER))
		return;

	id = ((pg->flag) & GFP_ZONE_ID_MASK) >> 1;
	zone = &mm_bank->zone[id];

	mutex_lock(&zone->zone_mutex);

	zone->nr_free_pages += page->count;
	zone->free_size += page->count * PAGE_SIZE;
	__free_pages(section, page);

	mutex_unlock(&zone->zone_mutex);
}


void free_pages(void *addr)
{
	struct page *pg;
	struct mm_section *section;

	if (!is_aligin((unsigned long)addr, PAGE_SIZE)) {
		mm_error("address not a page address\n");
		return;
	}

	section = va_get_section((unsigned long)addr);
	pg = va_to_page((unsigned long)addr);

	_free_pages(section, pg);
}

void release_page(struct page *page)
{
	struct mm_section *section;

	if (!page)
		return;

	section = page_get_section(page);
	_free_pages(section, page);
}

static struct page *find_align_page(struct mm_section *section,
		unsigned long align, int nr, int flag)
{
	int i, id;
	struct page *page = NULL;
	unsigned long base = section->vir_start;

	for ( ; base < section->vir_start + section->size; base += align) {
		id = (base - section->vir_start) >> PAGE_SHIFT;

		for (i = 0; i < nr; i++) {
			if (read_bit(section->bitmap, id + i)) {
				id = -1;
				break;
			}
		}

		if (id == -1)
			continue;
		else
			goto out;
	}

	id = -1;

out:
	if (id != -1) {
		update_memory_bitmap(section->bitmap, id, nr, 1);
		init_pages(section, id, nr, flag);
		page = section->page_table + id;
	}

	return page;
}

static struct page *find_match_page(struct mm_section *section,
		unsigned long align, int nr, int flag)
{
	int id;
	struct page *page = NULL;
	unsigned long base = section->vir_start;

	for ( ; base < section->vir_start + section->size; base += align) {
		id = (base - section->vir_start) >> PAGE_SHIFT;
		id += align;
		if (!read_bit(section->bitmap, id))
			goto out;
	}

	id = -1;
out:
	if (id != -1) {
		update_memory_bitmap(section->bitmap, id, nr, 1);
		init_pages(section, id, nr, flag);
		page = section->page_table + id;
	}

	return page;
}


/*
 * this function can be merged with get_free_page_addr_match
 * TBC
 */
static struct page *__request_special_pages(int nr, u32 align, int flag, int match)
{
	int i = 0, id;
	struct mm_section *section;
	struct mm_zone *zone;
	struct page *page = NULL;

	if (!is_align(align, PAGE_SIZE))
		return NULL;

	id = (flag & GFP_ZONE_ID_MASK) >> 1;
	if (id >= MM_ZONE_UNKNOWN)
		return NULL;

	if (match)
		align = (align & 0x000fffff) >> PAGE_SHIFT;

	zone = &mm_bank->zone[i];
	mutex_lock(&zone->zone_mutex);

	for (i = 0; i < zone->nr_section; i ++) {
		section = &zone->memory_section[i];
		if (match)
			page = find_match_page(section, align, nr, flag);
		else
			page = find_align_page(section, align, nr, flag);
	}

	if (page) {
		zone->free_size = zone->free_size - nr * PAGE_SIZE;
		zone->nr_free_pages = zone->nr_free_pages - nr;
	}

	mutex_unlock(&zone->zone_mutex);

	return page;
}

struct page *get_free_pages_align(int nr, u32 align, int flag)
{
	return __request_special_pages(nr, align, flag, 0);
}

struct page *get_free_page_match(unsigned long match, int flag)
{
	return __request_special_pages(1, match, flag, 1);
}

void inline add_page_to_list_tail(struct page *page,
		struct list_head *head)
{
	list_add_tail(head, &page->plist);
}

void inline del_page_from_list(struct page *page)
{
	list_del(&page->plist);
}

void free_pages_on_list(struct list_head *head)
{
	struct list_head *list;
	struct page *page;

	list_for_each(head, list) {
		page = list_entry(list, struct page, plist);
		release_page(page);
	}
}

inline struct page *list_to_page(struct list_head *list)
{
	return list_entry(list, struct page, plist);
}

void copy_page_va(unsigned long target, unsigned long source)
{
	memcpy((char *)target, (char *)source, PAGE_SIZE);
}

void copy_page_pa(unsigned long target, unsigned long source)
{
	copy_page_va(pa_to_va(target), pa_to_va(source));
}
