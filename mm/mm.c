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
#include <os/memory_map.h>

#define ZONE_MAX_SECTION	4

typedef enum __mm_zone_t {
	MM_ZONE_KERNEL = NORMAL_MEM,
	MM_ZONE_DMA = DMA_MEM,
	MM_ZONE_RES = RES_MEM,
	MM_ZONE_USER,
	MM_ZONE_UNKNOWN,
} mm_zone_t;

#ifdef CONFIG_DEBUG_MM
#define mm_debug(fmt, ...)	pr_debug("[  MM:  ]", fmt, ##__VA_ARGS__)
#else
#define mm_debug(fmt, ...)
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
	size_t size;
	size_t nr_page;
	long pv_offset;
	size_t maped_size;
	u32 bm_end;
	u32 bm_current;
	size_t nr_free_pages;
	u32 *bitmap;
	int section_table_id;
	struct page *page_table;
};

/*
 * memory_section: each section of this zone
 * mr_section: section count of this zone
 * zone_mutex: mutex of this zone to prvent race
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
	int page_num;
	int nr_free_pages;
};

/*
 * mm bank represent the memory object of system
 * total_page: total pages
 * zone: each zones of memory
 */
struct mm_bank {
	u32 total_size;	
	u32 total_page;
	struct mm_zone zone[MM_ZONE_UNKNOWN];
};

static struct mm_bank _mm_bank;
static struct mm_bank *mm_bank = &_mm_bank;

#define NR_MAX_SECTIONS		(MM_ZONE_USER * ZONE_MAX_SECTION)
static struct mm_section *
section_table[NR_MAX_SECTIONS + 1] = { 0 };

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
	size_t size;
	unsigned long end_addr;
	unsigned long base_addr;

	end_addr = min_align(region->start + region->size, PAGE_SIZE);
	base_addr = align(region->start, PAGE_SIZE);
	size = end_addr - base_addr;

	if (size < PAGE_SIZE)
		return 0;
	
	if (zone->nr_section == ZONE_MAX_SECTION) {
		mm_debug("Bug: Zone section is bigger than %d\n",
				ZONE_MAX_SECTION);
		return 0;
	}

	section = &zone->memory_section[zone->nr_section];
	section->phy_start = base_addr;
	section->nr_page = size >> PAGE_SHIFT;
	section->nr_free_pages = section->nr_page;
	section->size = size;

	zone->nr_section++;
	zone->page_num += section->nr_page;
	zone->nr_free_pages = zone->page_num;

	return size;
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

	/*
	 * sort the memory region array from min to max
	 */
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
		if (region->attr != UNKNOWN_MEM)
			mm_bank->total_size += size;
	}

	mm_bank->total_page =
		mm_bank->total_size >> PAGE_SHIFT;

	return 0;
}

static void alloc_dma_section(void)
{
	struct mm_section *pr;
	struct mm_zone *zone;
	struct memory_region rtmp;
	size_t size;

	zone = &mm_bank->zone[MM_ZONE_KERNEL];
	size = zone->page_num >> 4;

	if ((size << PAGE_SHIFT) > KERNEL_DMA_SIZE)
		size = KERNEL_DMA_SIZE >> PAGE_SHIFT;

	/*
	 * choose the section which has the max size
	 */
	pr = &zone->memory_section[zone->nr_section - 1];

	/*
	 * get a new mm_region for dma
	 */
	rtmp.start = pr->phy_start +
		((pr->nr_page - size) << PAGE_SHIFT);
	rtmp.attr = DMA_MEM;
	rtmp.size = size << PAGE_SHIFT;

	/*
	 * fix me, we must ensure that the section with
	 * max size must have enough size to split
	 * memory for DMA.
	 */
	pr->nr_page -= size;
	pr->nr_free_pages -= size;
	pr->size -= size << PAGE_SHIFT;

	
	zone->page_num -= size;
	zone->nr_free_pages = zone->page_num;

	mm_debug("No dma zone,auto allocated 0x%x \
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

static int map_boot_section(struct mm_section *section,
		size_t mapped, unsigned long flag)
{
	unsigned long vstart;
	unsigned long pstart;
	size_t msize;

	vstart = KERNEL_VIRTUAL_BASE + mapped;
	msize = section->maped_size - mapped;
	pstart = section->phy_start +
		(vstart - section->vir_start);

	if (msize > 0)
		build_kernel_pde_entry(vstart,
				pstart, msize, flag);

	return 0;
}

static int do_map_kernel_memory(void)
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

	for (i = 0; i < MM_ZONE_USER; i++) {
		flag = 0;
		switch (i) {
			case MM_ZONE_KERNEL:
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
			unsigned long start, end;

			section = &zone->memory_section[j];
			if (!section->maped_size)
				continue;

			start = section->vir_start;
			end = section->vir_start + section->size;

			if ((KERNEL_VIRTUAL_BASE >= start) &&
					(KERNEL_VIRTUAL_BASE < end)) {
				kernel_info("Skip Boot Memeory 0x%x\n",
						already_map);
				map_boot_section(section, already_map, flag);
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
	int i;
	struct mm_section *section;
	unsigned long zone_map_end;
	unsigned long map_base;
	size_t tmp, map_size = 0;

	if (size <= 0)
		return -EINVAL;

	/*
	 * map_base should be pde align
	 */
	zone_map_end = zone->map_base + size;
	map_base = zone->map_base;
	if (map_base == zone_map_end)
		return 0;

	for (i = 0; i < zone->nr_section; i++) {
		section = &zone->memory_section[i];

		/*
		 * if the section already maped skiped this section
		 */
		if (section->maped_size)
			continue;

		/*
		 * the case that physical start is not pde align
		 */
		tmp = section->phy_start -
			PDE_MIN_ALIGN(section->phy_start);
		map_base += tmp;

		/*
		 * the biggest size that this section can map
		 */
		map_size = zone_map_end - map_base;
		if (map_size > section->size)
			map_size = section->size;

		/* 
		 * pv_offset need to consider on case - when
		 * the phy_start is not ARCH_PDE_ALIGN_SIZE align
		 */
		section->vir_start = map_base;
		section->pv_offset =
			section->vir_start - section->phy_start;

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
	zone = &mm_bank->zone[MM_ZONE_KERNEL];
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
	map_zone_memory(&mm_bank->zone[MM_ZONE_KERNEL],
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
	zone = &mm_bank->zone[MM_ZONE_KERNEL];
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

	do_map_kernel_memory();
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
			tmp += section->nr_page;
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
			section->bm_end = section->nr_page;
			tmp += bits_to_long(section->nr_page);
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
		page->map_address = tmp;
		init_list(&page->plist);
		page->count = 1;
		page->flag = __GFP_KERNEL;

		tmp += PAGE_SIZE;
		page++;
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
	struct mm_zone *zone = &mm_bank->zone[MM_ZONE_KERNEL];
	struct soc_memory_info *info = get_soc_memory_info();

	/*
	 * page table start addr
	 */
	base_addr = align(info->code_end, sizeof(unsigned long));
	base_addr = init_sections_page_table(base_addr);

	base_addr = align(base_addr, sizeof(unsigned long));
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

	/*
	 * update the normal zone's information
	 */
	base_addr = align(base_addr, PAGE_SIZE);
	size = base_addr - KERNEL_VIRTUAL_BASE;
	size = size >> PAGE_SHIFT;
	zone->nr_free_pages -= size;

	/*
	 * update the boot section information
	 */
	update_boot_section_info(section, size);

	return 0;
}

static void update_section_table(void)
{
	int i, j, k = 0;
	struct mm_zone *zone;
	struct mm_section *section;

	for (i = 0; i < MM_ZONE_UNKNOWN; i++) {
		zone = &mm_bank->zone[i];
		for (j = 0; j < ZONE_MAX_SECTION; j++) {
			section = &zone->memory_section[j];
			if (section->size == 0)
				continue;
			section->section_table_id = k;
			section_table[k++] =
				&zone->memory_section[j];
		}
	}
}

static void init_user_zone(void)
{
	/*
	 * if the system memory > KERNEL_MEMORY
	 * size, the left memory will put into
	 * user zone
	 */
	int i, j = 0;
	struct mm_zone *zone, *uzone;
	struct mm_section *section, *usection;
	int nr;

	zone = &mm_bank->zone[MM_ZONE_KERNEL];
	uzone = &mm_bank->zone[MM_ZONE_USER];
	nr = zone->nr_section;

	for (i = 0; i < nr; i++) {
		section = &zone->memory_section[i];
		if (section->maped_size <
			(section->nr_page << PAGE_SHIFT)) {
			/*
			 * allocated a new section for user zone
			 * phy will echo vir and pv_offset is 0
			 */
			usection = &uzone->memory_section[j];
			usection->phy_start = section->phy_start +
				section->maped_size;
			usection->vir_start = 0;
			usection->pv_offset = 0;
			usection->nr_page = section->nr_page -
				(section->maped_size >> PAGE_SHIFT);
			usection->size = usection->nr_page << PAGE_SHIFT;
			usection->maped_size = 0;
			usection->bm_end = usection->nr_page;
			usection->bm_current = 0;
			usection->nr_free_pages = usection->nr_page;
			usection->bitmap = section->bitmap +
				bits_to_long(section->maped_size >> PAGE_SHIFT);
			usection->page_table = section->page_table +
				(section->maped_size >> PAGE_SHIFT);

			section->nr_page -= usection->nr_page;
			section->nr_free_pages -=
				usection->nr_page;
			section->size -= usection->size;
			section->bm_end = section->nr_page;

			uzone->nr_section++;
			uzone->page_num += usection->nr_page;
			uzone->nr_free_pages += usection->nr_free_pages;

			if (section->maped_size == 0) {
				zone->nr_section--;
				memset((char *)section, 0,
					sizeof(struct mm_section));
			}

			zone->page_num -= usection->nr_page;
			zone->nr_free_pages -= usection->nr_page;

			j++;
		}
	}
}

int mm_init(void)
{
	kernel_info("Memory management init...\n");

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
	init_user_zone();
	update_section_table();

	return 0;
}

static inline struct mm_section *va_get_section(unsigned long va)
{
	int i = 0;
	struct mm_section *section = NULL;

	while (1) {
		section = section_table[i++];
		if (!section)
			break;

		if ((va >= section->vir_start) &&
			(va < (section->vir_start + section->size)))
			break;
	}

	return section;
}

static inline struct mm_section *pa_get_section(unsigned long pa)
{
	int i = 0;
	struct mm_section *section = NULL;

	while (1) {
		section = section_table[i++];
		if(!section)
			break;

		if ((pa >= section->phy_start) &&
			(pa < (section->phy_start + section->size)))
			break;
	}

	return section;
}

static inline struct mm_section *page_get_section(struct page *page)
{
	return section_table[page->phy_address & 0xff];
}

#define section_page_id(va, base)	(((va) - (base)) >> PAGE_SHIFT)

#define section_va_to_page(section, va)	\
	(&section->page_table[section_page_id((va), section->vir_start)])

#define section_pa_to_page(section, pa)	\
	(&section->page_table[section_page_id((pa), section->phy_start)])

struct page *va_to_page(unsigned long va)
{
	/* only can used to translate kernel memory */

	struct mm_section *section = va_get_section(va);

	return section ? section_va_to_page(section, va) : 0;
}

unsigned long page_to_va(struct page *page)
{
	return (page->map_address & 0xfffff000);
}

unsigned long page_to_pa(struct page *page)
{
	return (page->phy_address & 0xffffff00);
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
	int i, page_size, nr = count;
	unsigned long pa;
	unsigned long va;
	unsigned long flag_temp = flag;
	struct page *pg;

	va = section->vir_start + (index << PAGE_SHIFT);
	pa = section->phy_start + (index << PAGE_SHIFT);
	pg = &section->page_table[index];

	page_size = PAGE_SIZE;
	if (flag & __GFP_USER) {
		va = 0;
		page_size = 0;
	}

	flag_temp |= __GFP_PAGE_HEADER;

	for (i = 0; i < count; i++) {
		/*
		 * section_table_id is appended to the
		 * physical address, is the page is used
		 * to the user space the va is not used.
		 */
		memset((char *)pg, 0, sizeof(struct page));
		pg->phy_address = pa |
			section->section_table_id;
		pg->map_address = va + (i * page_size);

		init_list(&pg->plist);
		pg->count = nr;
		pg->flag |= flag_temp;

		/*
		 * page table must 4K aligin, so when this page
		 * is used as a page table for process, we need
		 * initilize its free_base scope.
		 */
		pa += PAGE_SIZE;
		pg++;
		nr = 0;
		flag_temp = flag;
	}
}

static void update_memory_bitmap(u32 *bitmap,
		u32 index, int count, int update)
{
	int i;

	if (update) {
		for (i = index; i < index + count; i++)
			set_bit(bitmap, i);
	} else {
		for (i = index; i < index + count; i++)
			clear_bit(bitmap, i);
	}
}

static unsigned long
get_free_pages_from_section(struct mm_section *section,
			size_t count, int flag, int vp)
{
	long index;
	unsigned long ret;

	index = bitmap_find_free_base(section->bitmap,
				section->bm_current, 0,
				section->nr_page, count);
	if (index < 0) {
		kernel_error("Can not find %d continous pages\n", count);
		return 0;
	}

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

	section->nr_free_pages -= count;

	init_pages(section, index, count, flag);

	if (vp)
		ret = section->vir_start + (index << PAGE_SHIFT);
	else
		ret = (unsigned long)(section->page_table + index);

	return ret;
}

static unsigned long
__get_free_pages_from_zone(struct mm_zone *zone,
		int count, u32 flag, int vp)
{
	struct mm_section *section = NULL;
	unsigned long ret = 0;
	int id = 0;

	mutex_lock(&zone->zone_mutex);

	if (zone->nr_free_pages < count)
		goto out;

	/* find a section which have count pages */
	for (id = 0; id < ZONE_MAX_SECTION; id++)  {
		section = &zone->memory_section[id];
		if (!section->size)
			continue;

		if (section->nr_free_pages < count)
			continue;

		ret = get_free_pages_from_section(section,
				count, flag, vp);
		if (ret)
			break;
	}

	if (!ret)
		goto out;

	zone->nr_free_pages -= count;

out:
	mutex_unlock(&zone->zone_mutex);
	return ret;
}

static inline int get_zone_id(u32 flag)
{
	int i = 0;

	flag = flag & GFP_ZONE_ID_MASK;
	while (flag != 1) {
		flag = flag >> 1;
		i++;
	}

	return i;
}

static unsigned long __get_free_pages(int count, u32 flag, int vp)
{
	struct mm_zone *zone;
	int id;
	unsigned long ret = 0;

	/*
	 * get the zone id from the flag
	 */
	id = get_zone_id(flag);
	if (id >= MM_ZONE_UNKNOWN)
		return 0;

	while (1) {
		zone = &mm_bank->zone[id];
		ret = __get_free_pages_from_zone(zone,
				count, flag ,vp);
		if (ret)
			break;

		if (id == MM_ZONE_USER)
			id = MM_ZONE_KERNEL - 1;

		if (id++ == MM_ZONE_RES)
			break;
	}

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
	/*
	 * this function can only used to get the
	 * memory for kernel
	 */
	if ((count <= 0) || (flag & __GFP_USER))
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
	mm_debug("Update bm_current count:%d current %d\n",
			count, section->bm_current);
}

static void __free_pages(struct mm_section *section, struct page *page)
{
	int index = 0, count = 0, i;

	index = page - section->page_table;
	count = page->count;

	update_memory_bitmap(section->bitmap, index, count, 0);

#if 0
	for (i = 0; i < count; i++) {
		memset((char *)page, 0, sizeof(struct page));
		page++;
	}
#endif

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

	id = get_zone_id(pg->flag);
	zone = &mm_bank->zone[id];

	mutex_lock(&zone->zone_mutex);

	zone->nr_free_pages += page->count;
	__free_pages(section, page);

	mutex_unlock(&zone->zone_mutex);
}


void free_pages(void *addr)
{
	struct page *pg;
	struct mm_section *section;

	/*
	 * can only used to free kernel memory which
	 * geted by get_free_pages
	 */
	if (!is_align((unsigned long)addr, PAGE_SIZE)) {
		kernel_error("Address not a page address\n");
		return;
	}

	section = va_get_section((unsigned long)addr);
	pg = section_va_to_page(section, (unsigned long)addr);

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
	unsigned long base;
	size_t size = section->size;

	base = section->vir_start + (section->bm_current << PAGE_SHIFT);
	base = min_align(base, align);

	for ( ; base < section->vir_start + size; base += align) {
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
	size_t size = section->nr_page << PAGE_SHIFT;

	for ( ; base < section->vir_start + size; base += align) {
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

static struct page *__request_special_pages(int nr,
		u32 align, int flag, int match)
{
	int i = 0, id;
	struct mm_section *section;
	struct mm_zone *zone;
	struct page *page = NULL;

	if (!is_align(align, PAGE_SIZE))
		return NULL;

	id = get_zone_id(flag);
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

		if (page)
			break;
	}

	if (page)
		zone->nr_free_pages =
			zone->nr_free_pages - nr;

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

void inline
page_set_map_address(struct page *page,
		    unsigned long addr)
{
	page->map_address = addr;
}

unsigned long
page_get_map_address(struct page *page)
{
	return page->map_address;
}

void inline
page_set_pdata(struct page *page, unsigned long data)
{
	page->pdata = data;
}

unsigned long inline
page_get_pdata(struct page *page)
{
	return page->pdata;
}
