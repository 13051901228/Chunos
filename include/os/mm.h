#ifndef _MM_H
#define _MM_H

#include <os/list.h>
#include <os/types.h>
#include <os/bound.h>
#include <os/mutex.h>
#include <os/mirros.h>
#include <os/soc.h>
#include <os/init.h>

#define PAGE_SIZE		4096
#define PAGE_SHIFT		12

#define page_nr(size)		(align(size, PAGE_SIZE) >> PAGE_SHIFT)

#define __GFP_KERNEL		0x00000001
#define __GFP_DMA		0x00000002
#define __GFP_RES		0x00000004
#define __GFP_USER		0x00000008
#define GFP_ZONE_ID_MASK	0x000000ff

/* indicate the usage of the memory */
#define __GFP_PGT		0x00000100
#define __GFP_MMAP		0x00000200
#define __GFP_SLAB		0x00000400

#define __GFP_PAGE_HEADER	0x00010000

#define GFP_KERNEL		(__GFP_KERNEL)
#define GFP_USER		(__GFP_USER)
#define GFP_RES			(__GFP_RES)
#define GFP_PGT			(__GFP_KERNEL | __GFP_PGT)
#define GFP_DMA			(__GFP_DMA)
#define GFP_SLAB		(__GFP_KERNEL | __GFP_SLAB)
#define GFP_MMAP		(__GFP_USER | __GFP_MMAP)
#define __GFP_MASK		(0xffffffff)

#define MEM_TYPE_GROW_UP	1
#define MEM_TYPE_GROW_DOWN	0

struct page {
	/*
	 * phy_address: the phy_address of this page
	 * map_address: the address where this page maped to
	 */
	unsigned long phy_address;
	unsigned long map_address;

	/*
	 * flag: flags of this page
	 */
	unsigned long flag;

	/*
	 * plist: used to connect other page
	 */
	struct list_head plist;

	/*
	 * count: how many continuous pages has
	 * been allocated
	 *
	 * unused: to be done
	 */
	unsigned long count : 16;
	unsigned long pinfo : 16;

	/*
	 * pdata: private data of this page
	 */
	unsigned long pdata;
} __attribute__((packed));

void free_pages(void *addr);

void *get_free_pages(int count, int flag);

struct page *request_pages(int count, int flag);

struct page *va_to_page(unsigned long va);

unsigned long page_to_va(struct page *page);

struct page *pa_to_page(unsigned long pa);

unsigned long page_to_pa(struct page *page);

unsigned long va_to_pa(unsigned long va);

struct page *get_free_pages_align(int nr, u32 aligin, int flag);

struct page *get_free_page_match(unsigned long match, int flag);

void add_page_to_list_tail(struct page *page,
		struct list_head *head);

struct page *list_to_page(struct list_head *list);

void free_pages_on_list(struct list_head *head);

static inline void *get_free_page(unsigned long flag)
{
	return get_free_pages(1, flag);
}

void page_set_map_address(struct page *page, unsigned long addr);

unsigned long page_get_map_address(struct page *page);

void release_page(struct page *page);

unsigned long page_get_pdata(struct page *page);

void page_set_pdata(struct page *page, unsigned long pdata);

void inline del_page_from_list(struct page *page);

#endif
