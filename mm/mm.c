#include <os/mm.h>
#include <os/string.h>
#include <os/init.h>
#include <os/mirros.h>
#include <os/mmu.h>
#include <os/kernel.h>

#define ZONE_MAX_REGION		4

/*
 * mm_section represent a memory section in the platform
 * phy_start: phyical start
 * vir_start: the adress of virtual which mapped the this section
 * size: section size
 * pv_offset: section offset p->v
 * maped: whether this section need to mapped
 */
struct mm_section {
	unsigned long phy_start;
	unsigned long vir_start;
	unsigned long size;
	long pv_offset;
	u32 maped : 1;
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
	struct mm_section memory_section[ZONE_MAX_REGION];
	int nr_section;
	struct mutex zone_mutex;

	u32 total_size;
	u32 free_size;
	u32 page_num;
	u32 free_pages;

	u32 bm_start;
	u32 bm_end;
	u32 bm_current;
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
	u32 total_section;
	struct mm_zone zone[MM_ZONE_UNKNOW];
};

struct mm_bank mm_bank;
struct mm_bank *p_bank = &mm_bank;

struct page *_page_table;
u32 *_page_map;

static struct mm_bank *get_memory_bank(void)
{
	return p_bank;
}

static u32 *get_page_map(void)
{
	return _page_map;
}

u32 mm_free_page(unsigned long flag)
{
	struct mm_bank *bank = get_memory_bank();
	int i;
	u32 sum = 0;
	flag = flag & MM_ZONE_MASK;

	for (i = 0; i < MM_ZONE_UNKNOW; i++) {
		if ((flag & i) == i) {
			sum += bank->zone[i].free_pages;
		}
	}

	return sum;
}

static inline void init_memory_section(struct mm_section *section)
{
	section->phy_start = 0;
	section->vir_start = 0;
	section->size = 0;
	section->pv_offset = 0;
	section->maped = 0;
}

static int init_memory_bank(struct mm_bank *bank)
{
	int i,j;
	struct mm_zone *zone;

	bank->total_size = 0;
	bank->total_page = 0;

	for (i=0; i < MM_ZONE_UNKNOW; i++) {
		zone = &bank->zone[i];
		zone->nr_section = 0;
		init_mutex(&zone->zone_mutex);
		for (j = 0; j < ZONE_MAX_REGION; j++) {
			init_memory_section(&zone->memory_section[j]);
		}
	}

	return 0;
}

static u32 insert_region_to_zone(struct memory_region *region,
				struct mm_zone *zone)
{
	struct mm_section *section;
	
	/* fix me: we must ensure nr_section < max_zone_section */
	if (zone->nr_section == ZONE_MAX_REGION) {
		mm_debug("bug: may be you need increase the ZONE_MAX_SECTION\n");
		return 0;
	}

	section = &zone->memory_section[zone->nr_section];
	section->phy_start = region->start;
	section->size = region->size;
	zone->nr_section++;
	zone->total_size += section->size;
	zone->free_size = zone->total_size;
	zone->page_num = (zone->total_size) >> PAGE_SHIFT;
	zone->free_pages = zone->page_num;

	return (section->size);
}


/*
 * some potential issue eixst in this function
 * maybe we can fix it later.
 */
static int analyse_soc_memory_info(struct soc_memory_info *info, struct mm_bank *bank)
{
	int i, j;
	int min;
	struct memory_region *region = info->region;
	struct mm_section tmp;
	struct memory_region rtmp;
	struct mm_section *pr;
	struct mm_zone *zone;
	int dma_split = 0;
	u32 size;

	/* sort the memory region array from min to max */
	for (i = 0; i < info->region_nr; i++) {
		min = i;

		for (j = i+1; j < info->region_nr; j++) {
			if (region[min].start > region[j].start)
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
		zone = &bank->zone[region->attr];
		size = insert_region_to_zone(region,zone);
		if ((region->attr != IO_MEM) && (region->attr != UNKNOW_MEM))
			bank->total_size += size;
	}

	/*
	 * if user did not assign the dma zone,we must alloce
	 * it by ourself, dma size = 1 / 16 of the total size
	 * if there is a section which size is equal or a little
	 * bigger than dma size zone we will use it as dma zone
	 */
	zone = &bank->zone[MM_ZONE_DMA];
	if (zone->nr_section == 0) {
		zone = &bank->zone[MM_ZONE_NORMAL];
		size = zone->total_size;
		size = (size >> 4) ;
		size = baligin(size, SIZE_1M);

		pr = &zone->memory_section[0];
		for (i = 0; i < zone->nr_section; i++) {
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
			
			if (tmp.size != pr->size) {
				if ((tmp.size - size) <= ((zone->total_size) >> 5)) {
					pr = &zone->memory_section[j];	
					pr->size = 0;
					pr->phy_start = 0;
					pr->pv_offset = 0;
					zone->nr_section--;
					
					rtmp.start = tmp.phy_start;
					rtmp.size = tmp.size;
					rtmp.attr = DMA_MEM;

					goto insert_new_dma;
				}
			}
		}

		/*
		 * fix me, we must ensure that the section with max size
		 * must have enough size to split memory for DMA.
		 */
		rtmp.start = (pr->phy_start + pr->size) -size;
		rtmp.size = size;
		rtmp.attr = DMA_MEM;
		pr->size -= size;

insert_new_dma:
		zone->total_size -= rtmp.size;
		zone->free_size -= rtmp.size;
		zone->page_num -= (rtmp.size) >> PAGE_SHIFT;
		zone->free_pages = zone->page_num;
		dma_split = 1;

		mm_debug("no dma zone,auto allocated 0x%x \
			  Byte size dma zone\n",rtmp.size);
		insert_region_to_zone(&rtmp, &bank->zone[MM_ZONE_DMA]);
	}

	/* if dma_section is split then needed be add 1 */
	bank->total_page = (bank->total_size) >> PAGE_SHIFT;
	if (dma_split)
		bank->total_section = info->region_nr + 1;
	else
		bank->total_section = info->region_nr;

	return 0;
}

static inline u32 map_section_to_virtual(struct mm_section *section,
		                         unsigned long map_base)
{
	section->vir_start = map_base;
	section->pv_offset = map_base - section->phy_start;
	section->maped = 1;

	return section->size;
}

static u32 map_zone_to_virtual(struct mm_zone *zone, unsigned long map_base)
{
	int i;
	struct mm_section *section;
	u32 new_base = map_base;
	u32 size;

	/* if the section has been already maped,skip it */
	for (i = 0; i < zone->nr_section; i++) {
		section = &zone->memory_section[i];
		if (section->size > 0 && (!section->maped)) {
			size = map_section_to_virtual(section, new_base);
			new_base += size;
		}
	}

	return (new_base - map_base);
}

int do_map_memory(struct mm_bank *bank)
{
	struct mm_zone *zone;
	unsigned long flag = 0;
	struct mm_section *section;
	int i,j;
	struct soc_memory_info *info = get_soc_memory_info();
	address_t already_map;

	/* skip the maped memeory in boot stage */
	already_map = info->code_end - info->kernel_virtual_start;
	already_map = baligin(already_map, SIZE_1M);
	for (i = 0; i< MM_ZONE_UNKNOW; i++) {
		flag = 0;
		switch (i) {
			case MM_ZONE_NORMAL:
				flag |= TLB_ATTR_KERNEL_MEMORY;
				break;
			case MM_ZONE_DMA:
				flag |= TLB_ATTR_DMA_MEMORY;
				break;
			case MM_ZONE_IO:
				flag |= TLB_ATTR_IO_MEMORY;
				break;
			default:
				flag |= TLB_ATTR_KERNEL_MEMORY;
				break;
		}

		zone = &bank->zone[i];
		for (j = 0; j< zone->nr_section; j++) {
			section = &zone->memory_section[j];
			if (section->vir_start == info->kernel_virtual_start) {
				mm_info("skip already maped memeory 0x%x\n", already_map);
				build_tlb_table_entry(section->vir_start - already_map,
						section->phy_start - already_map,
						section->size - already_map, flag);
			}

			build_tlb_table_entry(section->vir_start,
					      section->phy_start,
					      section->size, flag);
		}
	}

	return 0;
}

static unsigned long map_memory(struct mm_bank *bank)
{
	int i;
	struct mm_zone *zone;
	struct mm_section *tmp;
	u32 size;
	struct soc_memory_info *info = get_soc_memory_info();
	address_t map_base;

	/*
	 * map normal memory to virtual address, firest 
	 * we need to map the address which kernel loaded
	 * kernel is loaded at normal memory
	 */
	map_base = info->kernel_virtual_start;
	zone = &bank->zone[MM_ZONE_NORMAL];
	for (i = 0; i < zone->nr_section; i++){
		tmp = &zone->memory_section[i];
		if (tmp->phy_start == info->kernel_physical_start) {
			size = map_section_to_virtual(tmp, map_base);
			map_base += size;
			break;
		}
	}
		
	/*
	 * then we can directy map other memeory of other zone;
	 * a question, should we map the IO memory at first?
	 * since it may take over to many virtual memory.
	 */
	for (i = 0; i < MM_ZONE_UNKNOW; i++) {
		size = map_zone_to_virtual(&bank->zone[i], map_base);
		map_base += size;
	}

	return do_map_memory(bank);
}

static int init_page_table(void)
{
	int i;
	struct mm_bank *bank = get_memory_bank();
	unsigned long tmp;
	unsigned long size;
	struct page *page;
	struct mm_zone *zone = &bank->zone[MM_ZONE_NORMAL];
	long pv_offset;
	struct soc_memory_info *info = get_soc_memory_info();
	address_t kernel_end = info->code_end;

	_page_table = (struct page *)(baligin(kernel_end, sizeof(int)));
	tmp = (unsigned long)(_page_table + bank->total_page);
	_page_map = (u32 *)(baligin(tmp, sizeof(u32)));
	tmp = (unsigned long)(_page_map + bits_to_long(bank->total_page));

	memset((char *)_page_map, 0, tmp - (unsigned long)_page_map);

	size = tmp - info->kernel_virtual_start;
	size = baligin(size,PAGE_SIZE);
	zone->free_size = zone->free_size - size;
	size = size >> PAGE_SHIFT;
	zone->free_pages = zone->free_pages -size;

	for (i = 0; i < zone->nr_section; i++) {
		if (info->kernel_virtual_start == zone->memory_section[i].vir_start) {
			pv_offset = zone->memory_section[i].pv_offset;
			break;
		}
	}

	tmp = info->kernel_virtual_start;
	for (i = 0; i < size; i++) {
		set_bit(_page_map,i);
		page = &_page_table[i];

		/* init page struct */
		page->phy_address = tmp - pv_offset;
		init_list(&page->plist);
		page->free_size = 0;
		page->free_base = 0;
		page->count = 0;
		page->usage = 1;

		tmp += PAGE_SIZE;
	}

	/* code to init bm_current bm_start bm_end */
	zone->bm_start = 0;
	zone->bm_current = size;
	i = zone->bm_end = zone->page_num;

	zone = &bank->zone[MM_ZONE_DMA];
	zone->bm_start = i;
	zone->bm_current = i;
	zone->bm_end = i + zone->page_num;
	i = i + zone->page_num;

	zone = &bank->zone[MM_ZONE_RES];
	zone->bm_start = i;
	zone->bm_current = i;
	zone->bm_end = i + zone->page_num;

	return 0;
}


static void dump_memory_info(void)
{
	struct mm_bank *bank = get_memory_bank();
	struct mm_zone *zone;
	int i;
	char *zone_str[MM_ZONE_UNKNOW] = {"normal", "dma", "res", "io"};

	mm_info("zone   size   free_size  bm_start  bm_current bm_end\n");
	for (i = 0; i < MM_ZONE_UNKNOW; i++) {
		zone = &bank->zone[i];
		mm_info("%s  0x%x  0x%x  %d  %d  %d\n",
			zone_str[i], zone->total_size,
			zone->free_size, zone->bm_start,
			zone->bm_current,zone->bm_end);	
	}
}

static void clear_user_tlb(void)
{

	clear_tlb_entry(0x0, 2048);
}

int mm_init(void)
{
	struct mm_bank *bank = get_memory_bank();
	struct soc_memory_info *info = get_soc_memory_info();

	mm_info("Init memory management\n");

	/*
	 * before map kernel memory, need to analyse the soc
	 * memory information, and do some extra work. map_memory
	 * will create all page tables for kernel. after init page
	 * table can used all kernel memory.
	 */
	init_memory_bank(bank);

	analyse_soc_memory_info(info, bank);
	map_memory(bank);
	init_page_table();

	dump_memory_info();

	/* do not use printk before late_console_init */
	clear_user_tlb();

	return 0;
}

int page_state(int n)
{
	return read_bit(_page_map, n);
}

struct page *get_page(int i)
{
	return (struct page *)(_page_table + i);
}

struct page *va_to_page(unsigned long va)
{
	struct soc_memory_info *info = get_soc_memory_info();

	return get_page((va - info->kernel_virtual_start) >> PAGE_SHIFT);
}

int va_to_page_id(unsigned long va)
{
	struct soc_memory_info *info = get_soc_memory_info();

	return ( (va - info->kernel_virtual_start) >> PAGE_SHIFT);
}

unsigned long page_id_to_va(int index)
{
	struct soc_memory_info *info = get_soc_memory_info();

	return (info->kernel_virtual_start + (index << PAGE_SHIFT));
}

int page_to_page_id(struct page *page)
{
	return  (page - _page_table);
}

unsigned long page_to_va(struct page *page)
{
	return page_id_to_va(page_to_page_id(page));
}

unsigned long page_to_pa(struct page *page)
{
	return (page->phy_address);
}

static long va_get_pv_offset(unsigned long va)
{
	int i,j;
	struct mm_zone *zone;
	struct mm_bank *bank = get_memory_bank();
	struct mm_section *s;

	for (i = 0; i< MM_ZONE_UNKNOW; i++) {
		zone = &bank->zone[i];
		for (j = 0; j < zone->nr_section; j++) {
			s = &zone->memory_section[j];
			if ((va >= s->vir_start) && 
			    (va <= (s->vir_start+s->size)))
				return (s->pv_offset);
		}
	}

	return 0;
}

static long pa_get_pv_offset(unsigned long pa)
{
	int i,j;
	struct mm_zone *zone;
	struct mm_bank *bank = get_memory_bank();
	struct mm_section *s;

	for (i = 0; i< MM_ZONE_UNKNOW; i++) {
		zone = &bank->zone[i];
		for (j = 0; j < zone->nr_section; j++) {
			s = &zone->memory_section[j];
			if((pa >= s->phy_start) && 
			(pa <= (s->phy_start+s->size)))
				return (s->pv_offset);
		}
	}

	return 0;
}

unsigned long pa_to_va(unsigned long pa)
{
	long pv_offset = pa_get_pv_offset(pa);

	return pv_offset ? (pa + pv_offset) : 0;

}

struct page *pa_to_page(unsigned long pa)
{
	return va_to_page(pa_to_va(pa));
}

unsigned long va_to_pa(unsigned long va)
{
	return page_to_pa(va_to_page(va));
}

int page_get(struct page *pg)
{
	pg->usage++;

	return pg->usage;
}

int page_put(struct page *pg)
{
	int usage;

	pg->usage--;
	usage = pg->usage;
	if (pg->usage == 0) {
		free_pages((void *)page_to_va(pg));
	}
	
	return usage;
}

static struct page *init_pages(u32 index, int count, unsigned long flag)
{
	struct page *pg;
	int i;
	unsigned long va;
	long pv_offset;

	for (i = index; i < index + count; i++) {
		pg = get_page(i);
		va = page_to_va(pg);
		pv_offset = va_get_pv_offset(va);
		pg->phy_address = va - pv_offset;
		pg->usage = 0;
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
		pg->free_base = va;
		pg->flag = flag;
		pg->usage = 0;
	}

	pg = get_page(index);

	return pg;
}

static void update_memory_bitmap(u32 index, int count, int update)
{
	int i;
	u32 *page_map = get_page_map();

	for (i = index; i < index + count; i++) {
		if (update)
			set_bit(page_map, i);
		else
			clear_bit(page_map, i);
	}
}

static u32 find_continous_pages(struct mm_zone *zone, int count)
{
	int i;
	int again = 0;
	int sum = 0;
	u32 *page_map = get_page_map();
	
	i = zone->bm_current;
	while (1) {
		if (!read_bit(page_map, i)) {
			sum++;
			if (sum == count)
				return i - count + 1;
		}
		else
			sum = 0;

		if (i == zone->bm_end) {
			again = 1;
			sum = 0;
			i = zone->bm_start - 1;
		}

		if (again) {
			if(i == zone->bm_current)
				break;
		}

		i++;
	}

	return 0;
}

static void *_get_free_pages(int count, u32 flag)
{
	struct mm_zone *zone;
	unsigned long res;
	struct page *pg;
	u32 index;
	int id;
	struct mm_bank *pbank = get_memory_bank();

	id = (flag & GFP_ZONE_ID_MASK) >> 1;
	if (id >= MM_ZONE_UNKNOW) {
		return NULL;
	}
	zone = &pbank->zone[id];

	/*
	 * before read the basic information of this 
	 * memory zone, we must get the mutex of this 
	 * zone to avoid zone information is modifyed
	 */
	mutex_lock(&zone->zone_mutex);

	if (zone->free_pages < count) {
		mm_error("No more size in zone %d\n", id);
		mutex_unlock(&zone->zone_mutex);
		return NULL;
	}

	index = find_continous_pages(zone, count);
	if (index == 0) {
		mm_error("can not find %d continous pages\n", count);
		mutex_unlock(&zone->zone_mutex);
		return NULL;
	}

	/*
	 * set the releated bit in bitmap
	 * in case of other process read these bit 
	 * when get free pages
	 */
	update_memory_bitmap(index, count, 1);

	/* update the zone information */
	zone->bm_current = index + count;
	zone->free_size = zone->free_size - (count >> PAGE_SHIFT);
	zone->free_pages = zone->free_pages - count;

	mutex_unlock(&zone->zone_mutex);

	pg = init_pages(index, count, flag);
	res = page_to_va(pg);

	return (void *)res;
}

void  *get_free_pages(int count, unsigned long flag)
{
	if (count <= 0)
		return NULL;

	return _get_free_pages(count, flag);
}

void update_bm_current(struct mm_zone *zone)
{
	u32 *page_map = get_page_map();
	int count = 0;
	int start = zone->bm_current - 1;

	if (zone->bm_start == zone->bm_current)
		return;

	while (!read_bit(page_map, start)) {
		count ++;
		start--;
	}

	mm_debug("update bm_current count is current end %d %d\n", count, zone->bm_current);

	zone->bm_current -= count;
}

static void __free_pages(struct page *pg, struct mm_zone *zone)
{
	int index = 0;
	int count = 0;
	index = page_to_page_id(pg);
	
	mutex_lock(&zone->zone_mutex);

	count = pg->count;
	update_memory_bitmap(index, count, 0);
	zone->free_size += count * PAGE_SIZE;
	zone->free_pages += count;

	update_bm_current(zone);

	mutex_unlock(&zone->zone_mutex);
}

void free_pages(void *addr)
{
	struct page *pg;
	struct mm_zone *zone;
	int id = 0;
	struct mm_bank *pbank = get_memory_bank();

	if (!is_aligin((unsigned long)addr, PAGE_SIZE)) {
		mm_error("address not a page address\n");
		return;
	}

	pg = va_to_page((unsigned long)addr);

	if (pg->flag & __GFP_PAGE_HEADER) {
		id = ((pg->flag) & GFP_ZONE_ID_MASK) >> 1;
		zone = &pbank->zone[id];
		__free_pages(pg, zone);
	} else {
		mm_error("addr to free are not page header\n");
	}
}

void *get_free_page_aligin(unsigned long aligin, u32 flag)
{
	struct mm_bank *bank = get_memory_bank();
	int id;
	int i;
	unsigned long j;
	struct page *page;
	struct mm_section *section;
	struct mm_zone *zone;
	unsigned long offset = (aligin & 0x000fffff) >> PAGE_SHIFT;

	id = (flag & GFP_ZONE_ID_MASK)>>1;
	if (id >= MM_ZONE_UNKNOW)
		return NULL;

	zone = &bank->zone[id];
	id = 0;
	mutex_lock(&zone->zone_mutex);
	for (i = 0; i < zone->nr_section; i++) {
		section = &zone->memory_section[i];
		for (j = section->vir_start; 
		     j < (section->vir_start) + (section->size);
		     j += SIZE_1M) {
			
			id = va_to_page_id(j); 
			id += offset;
			if (!page_state(id)) {
				goto out;
			}
		}
	}

	/* if have not find the page we need, set it to 0. */
	id = 0;
out:
	if (id) {
		update_memory_bitmap(id, 1, 1);

		zone->free_size = zone->free_size - PAGE_SIZE;
		zone->free_pages = zone->free_pages - 1;

		mutex_unlock(&zone->zone_mutex);

		page = init_pages(id, 1, flag);
		
		return ((void *)page_to_va(page));
	}

	mutex_unlock(&zone->zone_mutex);

	return NULL;
}

void copy_page_va(u32 target, u32 source)
{
	memcpy((char *)target, (char *)source, PAGE_SIZE);
}

void copy_page_pa(u32 target,u32 source)
{
	copy_page_va(pa_to_va(target), pa_to_va(source));
}
