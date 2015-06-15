/*
 * include/os/pgt.h
 *
 * Created by Le Min at 2.14/12/01
 *
 */

#ifndef _PGT_H_
#define _PGT_H_

#include <os/mm.h>
#include <os/kernel.h>
#include <os/mmu.h>
#include <os/task_mm.h>
#include <os/spin_lock.h>

#define PGT_MAP_NORMAL	0x0
#define PGT_MAP_STACK	0x1
#define PGT_MAP_MMAP	0x2

struct pte_cache_list {
	struct list_head pte_list;
	size_t pte_alloc_size;
	size_t pte_free_size;
	struct list_head *pte_current_page;
};

struct pgt_temp_buffer {
	unsigned long tbuf_pte_base;
	struct page *tbuf_pte_page;
	spin_lock_t tbuf_lock;
	int tbuf_page_nr;
};

/*
 * lvl1_pgt_base: the level 1 pgt base address
 * lvl2_pgt_list: the memory for lvl2 page table
 * lvl2_alloc_size: alloc size of lvl2 page table
 * lvl2_free_size: free size of lvl2 pgt current
 * lvl2_free_base: base address of current lvl2 pgt
 */
struct task_page_table {
	unsigned long pde_base;
	unsigned long pde_base_pa;
	struct pte_cache_list task_list;
	struct pte_cache_list mmap_list;
	unsigned long mmap_current_base;
	struct pgt_temp_buffer pgt_temp_buffer;
};

struct pgt_map_info {
	struct list_head *mlist;
	int request_nr;
	int alloc_nr;
	int type;
};

void free_task_page_table(struct task_page_table *pgt);

int init_task_page_table(struct task_page_table *table);

unsigned long pgt_map_temp_page(struct task_page_table *table,
		struct page *page);

int pgt_unmap_temp_page(struct task_page_table *table, unsigned long base);

int pgt_map_task_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr);

int pgt_map_task_memory(struct task_page_table *table,
		       struct list_head *mem_list,
		       unsigned long map_base, int type);

int pgt_map_mmap_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr);

#endif
