/*
 *
 * kernel/task_mm.c
 *
 * Create by Le Min at 2014/10/31
 *
 */

#include <os/mm.h>
#include <os/kernel.h>
#include <os/slab.h>
#include <os/list.h>
#include <os/task_mm.h>
#include <os/pgt.h>
#include <os/file.h>

static size_t init_max_alloc_size[TASK_MM_SECTION_MAX] = {
	SIZE_1M, SIZE_NK(16), 0
};

static void free_task_section_memory(struct task_mm_section *section)
{
	int i;
	
	for (i = 0; i < TASK_MM_SECTION_MAX; i++) {
		free_pages_on_list(&section->alloc_mem);
		section++;
	}
}

static void release_task_memory(struct mm_struct *mm)
{
	free_task_page_table(&mm->page_table);
	free_task_section_memory(mm->mm_section);
}

static void init_task_mm_section(struct mm_struct *mm, int i)
{
	struct task_mm_section *section;
	
	section = &mm->mm_section[i];
	switch (i) {
	case TASK_MM_SECTION_RO:
		section->section_size = elf_memory_size(mm->elf_file);
		section->flag = 0;
		break;

	case TASK_MM_SECTION_STACK:
		section->section_size = SIZE_NM(1);
		section->flag = 0;
		break;

	case TASK_MM_SECTION_MMAP:
		section->section_size = SIZE_NM(512);
		section->flag = 1;
		break;

	default:
		return;
	}

	/* 
	 * if the task is started by exec means there
	 * shold be some page for this task
	 * so do not clear the alloc_pages and 
	 * alloc_mem
	 */
	section->mapped_size = 0;
}

static void copy_mm_section_info(struct mm_struct *c,
		struct mm_struct *p, int i)
{
	struct task_mm_section *csection;
	struct task_mm_section *psection;

	csection = &c->mm_section[i];
	psection = &p->mm_section[i];

	/* task is started by fork, clear the attr */
	csection->section_size = psection->section_size;
	csection->mapped_size = 0;
	csection->alloc_pages = 0;
	csection->flag = psection->flag;
	init_list(&csection->alloc_mem);
}

static int alloc_user_stack(struct mm_struct *mm)
{
	return 0;
}

int load_elf_image(struct task_struct *task)
{
	struct page *page;
	struct mm_struct *mm = &task->mm_struct;
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_RO];
	size_t load_size;
	int page_nr, i;
	struct pgt_map_info map_info;
	unsigned long offset = 0;

	/* 
	 * here we load the all size, later will consider
	 * to load some of it
	 */
	load_size = section->section_size;
	page_nr = page_nr(load_size);
	page_nr -= section->alloc_pages;

	/*
	 * if the section already has memory use it then
	 * alloc left memory for task
	 */
	for (i = 0; i < page_nr; i++) {
		page = request_pages(1, GFP_USER);
		if (!page)
			goto out;

		add_page_to_list_tail(page, &section->alloc_mem);
	}

	list = list_next(&section->alloc_mem);

	while (load_size > 0) {
		list = pgt_map_elf_buffer(mm, &map_info, list);
		if (!map_info.size && load_size)
			goto out;

		load_elf_data(mm->elf_file, map_info->vir_base,
				map_info->size, offset);

		load_size -= map_info->size;
		offset += map_info->size;
	}

	return 0;

out:
	return -ENOMEM;
}

int init_mm_struct(struct task_struct *task)
{
	struct mm_struct *mm = &task->mm_struct;
	struct mm_struct *mmp = &task->parent->mm_struct;
	int i;

	if (task_is_kernel(task))
		return 0;

	/*
	 * if parent is user space we can dup the fd TBD
	 */
	mm->file = kernel_open(task->name, O_RDONLY);
	if (!mm->file)
		return -ENOENT;

	/*
	 * get the elf file's informaiton
	 */
	mm->elf_file = dup_elf_info(mmp->elf_file);
	if (!mm->elf_file) {
		mm->elf_file = get_elf_info(mm->file);
		if (!mm->elf_file)
			goto err_elf_file;
	}

	/* 
	 * if new task's parent is started by exec  we need get
	 * the information from the elf file, else copy
	 * the information from the parent process.
	 */
	for (i = 0; i < TASK_MM_SECTION_MAX; i++) {
		if (task->flag & PROCESS_FLAG_USER_EXEC) {
			init_task_mm_section(mm, i);
		} else if (task->flag & PROCESS_FLAG_KERN_EXEC) {
			init_task_mm_section(mm, i);
			alloc_user_stack(mm);
		} else {
			copy_mm_section_info(mm, mmp, i);
		}
	}

	if (init_task_page_table(&mm->page_table, mm->mm_section))
		goto err;

	return 0;

err:
	release_elf_file(mm->elf_file);

err_elf_file:
	kernel_close(mm->file);

	return -ENOMEM;
}

static int map_new_task_page(struct mm_struct *mm, struct page *page,
		unsigned user_addr, int section_index)
{
	struct task_mm_section *section = &mm->mm_section[section_index];
	
	add_page_to_list_tail(page, &section->alloc_mem);

	if (map_task_address(&mm->page_table,
		page_to_va(page), map_address, section_index))
		return -ENOMEM;

	section->mapped_size += PAGE_SIZE;
	section->alloc_pages++;
	page->user_page_attr.map_address = user_addr;

	return 0;
}

int copy_process_memory(struct task_struct *new, struct task_struct *parent)
{
	struct mm_struct *pmm = &parent->mm_struct;
	struct mm_struct *nmm = &new->mm_struct;
	struct list_head *p;
	struct list_head *list;
	struct page *page;
	struct page *new_page;
	struct mm_section *nsection;
	struct user_page_attr *attr;
	int i;
	
	/* 
	 * as the pdir is created can copy the memory
	 * from the code list from parent to new
	 */
	if (!new || !parent)
		return -EINVAL;

	for (i = 0; i < TASK_MM_SECTION_MAX; i++) {
		nsection = &nmm->mm_section[i];
		p = &pmm->mm_section[i].alloc_mem;

		/* list for all allocated memory of this section */
		list_for_each(p, list) {
			page = list_entry(list, struct page, plist);
			attr = &page->user_page_attr;

			new_page = request_pages(1, GFP_USER);
			if (!new_page)
				goto err;

			/* map user address to task memory space */
			if (map_new_task_page(nmm, page,
					attr->map_address, i))
				goto err;

			/*
			 * if copy user page we can directory use father's
			 * user address to incase the memory of system is
			 * so large
			 */
			memcpy((void *)page_to_va(new_page),
					(void *)attr->map_address, PAGE_SIZE);
		}
	}

	return 0;

err:
	free_task_section_memory(nmm);
	return -ENOMEM;
}

#define PROCESS_META_DATA_BASE	0

static int mm_copy_argv_envp(unsigned long *base,
		unsigned long *stack_base,
		char *name, char **argv, char **envp)
{
	char *argv_base = (char *)(PROCESS_META_DATA_BASE);
	int i, length, argv_sum = 1, env_sum = 0;
	unsigned long load_base;
	unsigned long *table_base;
	int stack_type =  0;
	int aligin = sizeof(unsigned long);

	for (i = 0; ; i++) {
		if (argv[i])
			argv_sum++;
		else
			break;
	}

	for (i = 0; ; i++) {
		if (envp[i])
			env_sum++;
		else
			break;
	}
	
	load_base = base;
#define STACK_TYPE_UP_TO_DOWN		1
	if (stack_type = STACK_TYPE_UP_TO_DOWN) {
		table_base = (unsigned long *)(stack_base + PAGE_SIZE);
		//table_base = table_base - (argv + env_sum + 1);
	} else {
		/* TBD */
		table_base += (argv_sum + env_sum + 1);
	}

	/* copy name */
	length = strlen(name);
	strcpy((char *)load_base, name);
	*table_base = (unsigned long)argv_base;
	argv_base += length;
	load_base += length;
	if (stack_base == STACK_TYPE_UP_TO_DOWN)
		table_base++;
	else
		table_base--;
	argv_base = (char *)baligin((unsigned long)argv_base, aligin);

	/* copy argv */
	for (i = 0; i < argv_sum; i++) {
		length = strlen(argv[i]);
		strcpy((char *)load_base, argv[i]);
		*table_base = (unsigned long)argv_base;
		argv_base += length;				/* argv[i] address in userspace */
		load_base += length;
		if (stack_base == STACK_TYPE_UP_TO_DOWN)
			table_base++;
		else
			table_base--;
		argv_base = (char *)baligin((u32)argv_base, aligin);
		load_base = baligin(load_base, aligin);
	}

	/* copy envp */
	for (i = 0; i < env_sum; i++) {
		length = strlen(envp[i]);
		strcpy((char *)load_base, argv[i]);
		*table_base = (unsigned long)argv_base;
		argv_base += length;	/* argv[i] address in userspace */
		load_base += length;
		if (stack_base == STACK_TYPE_UP_TO_DOWN)
			table_base++;
		else
			table_base--;
		argv_base = (char *)baligin((u32)argv_base, aligin);
		load_base = baligin(load_base, aligin);
	}

	return (argv_sum + env_sum + 1) * sizeof(unsigned long);
}

int task_mm_setup_argv_envp(struct mm_struct *mm,
		char *name, char **argv, char **envp)
{
	struct page *page;
	unsigned long stack_base;
	int ret;

	page = request_pages(1, GFP_KERNEL);
	if (!page)
		return -ENOMEM;
#define PROCESS_STACK_BASE		0
	//stack_base = (unsigned long)task_ua_to_va(PROCESS_STACK_BASE);
	ret = mm_copy_argv_envp(page_to_va(page), 
			stack_base, name, argv, envp);

	if (map_new_task_page(mm, page, 
		PROCESS_META_DATA_BASE, TASK_MM_SECTION_RO))
		return -ENOMEM;

	return ret;
}
