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

struct task_struct;

/*
 * SECTION_RO : text data and bss of the elf file
 * SECTION_STACK : stack section
 * SECTION_MMAP : mmap section
 * SECTION_META : used to store the argv and envp and other
 */
typedef enum _task_mm_section_t {
	TASK_MM_SECTION_ELF,
	TASK_MM_SECTION_STACK,
	TASK_MM_SECTION_MMAP,
	TASK_MM_SECTION_META,
	TASK_MM_SECTION_MAX
} task_mm_section_t;

#define TASK_MM_SECTION_FLAG_RO		(0x00000001)
#define TASK_MM_SECTION_FLAG_STACK	(0x00000002)
#define TASK_MM_SECTION_FLAG_MMAP	(0x00000004)
#define TASK_MM_SECTION_FLAG_META	(0x00000008)

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
	struct task_page_table page_table;
	struct task_mm_section mm_section[TASK_MM_SECTION_MAX];
};

void task_mm_release_task_memory(struct mm_struct *mm);

int task_mm_load_elf_image(struct mm_struct *mm);

int task_mm_copy_task_memory(struct task_struct *new,
		struct task_struct *parent);

int task_mm_setup_argenv(struct mm_struct *mm,
		char *name, char **argv, char **envp);

int task_mm_copy_sigreturn(struct mm_struct *mm,
		char *start, int size);

int init_mm_struct(struct task_struct *task);

unsigned long task_mm_mmap(struct mm_struct *mm,
		unsigned long start, size_t len,
		int flags, int fd, offset_t off);

int task_mm_munmap(struct mm_struct *mm,
		unsigned long start,
		size_t length, int flags, int sync);

struct page *
pgt_unmap_mmap_page(struct task_page_table *table,
			unsigned long base);

int pgt_map_mmap_page(struct task_page_table *table,
		struct page *page, unsigned long user_addr);

int pgt_check_mmap_addr(struct task_page_table *table,
		unsigned long start, int nr);

#endif
