#ifndef _MM_H
#define _MM_H

#include <os/list.h>
#include <os/types.h>
#include <os/bound.h>
#include <os/mutex.h>
#include <os/mirros.h>
#include <os/soc.h>
#include <os/init.h>

struct platform_info;

#define NORMAL_MEM		0
#define DMA_MEM			1
#define RES_MEM			2
#define IO_MEM			3
#define MM_ZONE_MASK		0x3
#define UNKNOW_MEM		4

typedef enum __mm_zone_t {
	MM_ZONE_NORMAL = NORMAL_MEM,
	MM_ZONE_DMA = DMA_MEM,
	MM_ZONE_RES = RES_MEM,
	MM_ZONE_IO = IO_MEM,
	MM_ZONE_UNKNOW,
} mm_zone_t;

/*
 *indicate which zone memory needed to be allocted.
 */
#define __GFP_KERNEL		0x00000001
#define __GFP_DMA		0x00000002
#define __GFP_RES		0x00000004
#define __GFP_IO		0x00000008

#define GFP_ZONE_ID_MASK	0x0000000f

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
 *indicate the usage of the memory
 */
#define __GFP_PGT		0x00000010
#define __GFP_PAGE_HEADER	0x00000020
#define __GFP_SLAB		0x00000040
#define __GFP_SLAB_HEADER	0x00000080
#define __GFP_USER		0x00000100

#define GFP_KERNEL		(__GFP_KERNEL)
#define GFP_USER		(__GFP_KERNEL | __GFP_USER)
#define GFP_RES			(__GFP_RES)
#define GFP_PGT			(__GFP_KERNEL | __GFP_PGT)
#define GFP_DMA			(__GFP_DMA)
#define GFP_FULL		(__GFP_KERNEL | __GFP_FULL)
#define GFP_SLAB		(__GFP_KERNEL | __GFP_SLAB)
#define GFP_SLAB_HEADER		(__GFP_SLAB_HEADER | __GFP_KERNEL)

#define __GFP_MASK		(__GFP_PGT | __GFP_FULL |__GFP_SLICE | __GFP_SLAB_HEADER | __GFP_USER)

/*page:represent 4k in physical memory
 * virtual_address:virtual address that page maped to 
 * flag:attribute of the page
 * free_base: if page is used as kernel allocation,the free base of page
 * free_size:remain size of this page
 * usage:usage of this page,if 0 page can release.
 */
struct page {
	unsigned long phy_address;
	u32 flag;
	/*
	 * if page used as a pgt then use pgt_list
	 * if page used as slab elment then use slab_list
	 */
	union {
		struct list_head plist;
		struct list_head pgt_list;
		struct list_head slab_list;
		struct list_head slab_header_list;
	};

	unsigned long free_base;

	/*
	 *if flag PAGE_FULL the count used to indicate how many 
	 *continueous pages were alloctated,else if flag PAGE_SLAB
	 *then free_size indicate how many free size in this page.
	 */
	
	union {
		u16 free_size;
		u16 extra_size;
		u16 mmap_offset;
	};
	u8 count;
	u8 usage;
} __attribute__((packed));

#define PAGE_SIZE		4096
#define PAGE_SHIFT		12

#define page_nr(size)		(baligin(size, PAGE_SIZE) >> PAGE_SHIFT)

int page_get(struct page *pg);

void free_pages(void *addr);

int page_put(struct page *pg);

void  *get_free_pages(int count,unsigned long flag);

static inline void set_page_free_size(struct page *pg, size_t size)
{
	pg->free_size = size;
}

static inline int get_page_free_size(struct page *pg)
{
	return pg->free_size;
}

static inline void set_page_free_base(struct page *pg, unsigned long addr)
{
	pg->free_base = addr;
}

static inline unsigned long get_page_free_base(struct page *pg)
{
	return pg->free_base;
}

static inline u32 get_page_flag(struct page *pg)
{
	return pg->flag;
}

static inline void set_page_flag(struct page *pg, u32 flag)
{
	pg->flag |= flag;
}

static inline void set_page_extra_size(struct page *pg, size_t size)
{
	pg->extra_size = size;
}

static inline size_t get_page_extra_size(struct page *pg)
{
	return pg->extra_size;
}

static inline void *get_free_page(unsigned long flag)
{
	return get_free_pages(1, flag);
}

static inline int get_page_count(struct page *pg)
{
	return pg->count;
}

static inline void set_page_count(struct page *pg, int count)
{
	pg->count = count;
}

static inline int get_page_usage(struct page *pg)
{
	return pg->usage;
}

static inline void update_page(struct page *pg,
		          unsigned long free_base,
			  size_t free_size)
{
	set_page_free_size(pg, free_size);
	set_page_free_base(pg, free_base);
}

int page_state(int i);
/*
 *fix me why can not use (struct page *)(&page_table[i]);
 */
struct page *get_page(int i);
/*some macro to covert page and PA with each other*/
struct page *va_to_page(unsigned long va);
unsigned long page_id_to_va(int index);
int va_to_page_id(unsigned long va);
unsigned long pa_to_va(unsigned long pa);
int page_to_page_id(struct page *page);
u32 mm_free_page(unsigned long flag);
unsigned long page_to_va(struct page *page);
void copy_page_va(u32 target, u32 source);
void copy_page_pa(u32 target, u32 source);
struct page *pa_to_page(unsigned long pa);
unsigned long va_to_pa(unsigned long va);
unsigned long page_to_va(struct page *page);
unsigned long page_to_pa(struct page *page);
void *get_free_page_aligin(unsigned long aligin, u32 flag);

#endif
