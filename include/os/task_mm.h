/*
 * include/os/task_mm.h
 *
 * Created by Le Min in 2014/11/26
 *
 */

#ifndef _TASK_MM_H_
#define _TASK_MM_H_

#include <os/elf.h>
#include <os/pgt.h>

typedef enum _task_mm_section_t {
	TASK_MM_SECTION_RO,
	TASK_MM_SECTION_STACK,
	TASK_MM_SECTION_MMAP,
	TASK_MM_SECTION_MAX,
} task_mm_section_t;

/*
 * user_base_addr: base address of this section in user space
 * section_size: section size of this section.
 * mapped_size: maped/alloc size of this section.
 * lvl_pdir_base: levle 1 page table base address
 * lvl1_pdir_entry: level 1 dir size
 * lvl1_pdir_alloc: alloced lvl2 size
 * alloc_pages: already allocated pages for this section
 * alloc_mem: list for all allocated memory for this section
 * flag: section flag
 */
struct task_mm_section {
	size_t section_size;
	size_t mapped_size;
	unsigned long base_addr;
	int alloc_pages;
	struct list_head alloc_mem;
	int flag;
};

/*
 * elf_file: the elf file this task belongs to
 * file: the file structure of the process
 * entry_point: the entry of the provess, user
 * page_table: the page_table of the process used to map memory
 * mm_section: each section of the task
 */
struct mm_struct {
	struct elf_file *elf_file;
	struct file *file;
	struct task_page_table page_table;
	struct task_mm_section mm_section[TASK_MM_SECTION_MAX];
};

#endif
