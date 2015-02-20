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
#include <os/memory_map.h>

static size_t init_max_alloc_size[TASK_MM_SECTION_MAX] = {
	SIZE_1M, SIZE_NK(16), 0
};

static void free_sections_memory(struct task_mm_section *section)
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
	free_sections_memory(mm->mm_section);
}

static void init_task_mm_section(struct mm_struct *mm, int i)
{
	struct task_mm_section *section;

	section = &mm->mm_section[i];
	section->flag = i;

	switch (i) {
	case TASK_MM_SECTION_RO:
		section->section_size =
			elf_memory_size(mm->elf_file);
		break;

	case TASK_MM_SECTION_STACK:
		section->section_size = SIZE_NM(1);
		break;

	case TASK_MM_SECTION_MMAP:
		section->section_size = SIZE_NM(512);
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
	section->flag |= i;
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

static int section_get_memory(int count,
		struct task_mm_section *section)
{
	struct page *page;
	int i;
	int flag;

	if (count <= 0)
		return -EINVAL;

#define GFP_MMAP	0
	if (section->flag & TASK_MM_SECTION_MMAP)
		flag = GFP_MMAP;
	else
		flag = GFP_USER;

	for (i = 0; i < count; i++) {
		page = request_pages(1, flag);
		if (!page)
			return -ENOMEM;

		add_page_to_list_tail(page, &section->alloc_mem);
		section->alloc_pages++;
		section->mapped_size += PAGE_SIZE;
	}

	return 0;
}

#define MEM_TYPE_GROW_UP	1
#define MEM_TYPE_GROW_DOWN	0

int load_elf_image(struct task_struct *task)
{
	struct page *page;
	struct mm_struct *mm = &task->mm_struct;
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_RO];
	size_t load_size;
	int page_nr, ret, i;

	/* 
	 * here we load the all size, later will consider
	 * to load some of it
	 */
	load_size = section->section_size;
	page_nr = page_nr(load_size);
	page_nr -= section->alloc_pages;

	/*
	 * if the section already has memory use it then
	 * alloc left memory for text and data of the task
	 */
	ret = section_get_memory(page_nr, section);
	if (ret == -ENOMEM)
		return -ENOMEM;
	
	ret = pgt_map_task_memory(mm->page_table,
			&section->alloc_mem,
			section->base_addr, MEM_TYPE_GROW_UP);
	if (ret)
		return ret;

	ret = load_elf_data(mm->elf_file,
			section->base_addr, load_size);
	if (ret)
		return ret;

	return 0;
}

static int copy_section_memory(struct task_mm_section *new,
		struct task_mm_section *parent, int type)
{
	int nr = 0;
	int count = new->alloc_pages;
	unsigned long pbase = parent->base_addr;
	unsigned long nbase = KERNEL_TEMP_BUFFER_BASE;
	struct list_head *list = list_next(&new->alloc_mem);

	if (new->alloc_pages != parent->alloc_pages)
		return -EINVAL;

	while (count) {
		list = pgt_map_temp_memory(list,
				&count, &nr, type);
		if (!list)
			return -EFAULT;

		memcpy((char *)nbase, (char *)pbase,
				nr << PAGE_SHIFT);
		if (type)
			pbase += (nr << PAGE_SHIFT);
		else
			pbase -= (nr >> PAGE_SHIFT);
		count -= nr;
	}

	return 0;
}

static int copy_task_elf(struct mm_struct *new,
		struct mm_struct *p)
{
	struct task_mm_section *nsection =
		&new->mm_section[TASK_MM_SECTION_RO];
	struct task_mm_section *psection =
		&p->mm_section[TASK_MM_SECTION_RO];
	int ret;

	/*
	 * map memory to new task's space
	 */
	ret = pgt_map_task_memory(&new->page_table,
			&nsection->alloc_mem,
			&nsection->base_addr,
			MEM_TYPE_GROW_UP);
	if (ret)
		return ret;

	/*
	 * copy memory form parent's section
	 */
	ret = copy_section_memory(nsection,
			psection, MEM_TYPE_GROW_UP);
	if (ret) {
		kernel_error("Memory is not correct \
				for section\n");
		return ret;
	}

	return 0;
}

static int copy_task_stack(struct mm_struct *new,
		struct mm_struct *p)
{
	struct task_mm_section *nsection =
		&new->mm_section[TASK_MM_SECTION_STACK];
	struct task_mm_section *psection =
		&p->mm_section[TASK_MM_SECTION_STACK];
	int ret;

	/*
	 * map memory to new task's space
	 */
	ret = pgt_map_task_memory(&new->page_table,
			&nsection->alloc_mem,
			&nsection->base_addr,
			MEM_TYPE_GROW_DOWN);
	if (ret)
		return ret;

	/*
	 * copy memory form parent's section
	 */
	ret = copy_section_memory(nsection,
			psection, MEM_TYPE_GROW_DOWN);
	if (ret) {
		kernel_error("Memory is not correct \
				for section\n");
		return ret;
	}

	return 0;
}

static int copy_task_mmap(struct mm_struct *new,
		struct mm_struct *parent)
{
	struct task_mm_section *n =
		&new->mm_section[TASK_MM_SECTION_MMAP];
	struct task_mm_section *p =
		&parent->mm_section[TASK_MM_SECTION_MMAP];
	struct list_head *nl, *list, *pl, *temp;
	struct page *np, *pp;
	int ret;

	if (n->alloc_pages != p->alloc_pages)
		return -EINVAL;

	nl = &n->alloc_mem;
	pl = &p->alloc_mem;

	list_for_each(pl, list) {
		nl = list_next(nl);
		np = list_to_page(nl);
		pp = list_to_page(pl);
		
		ret = pgt_map_task_page(&new->page_table,
				page_to_pa(np), page_to_va(pp));
		if (ret)
			return ret;

		temp = pgt_map_temp_memory(list, NULL,
				NULL, MEM_TYPE_GROW_UP);
		if (!temp)
			return -EFAULT;

		memcpy((char *)KERNEL_TEMP_BUFFER_BASE,
			(char *)page_to_va(pp), PAGE_SIZE);
	}

	return 0;
}

int copy_task_memory(struct task_struct *new,
		struct task_struct *parent)
{
	struct mm_struct *pmm = &parent->mm_struct;
	struct mm_struct *nmm = &new->mm_struct;
	struct task_mm_section *nsection;
	struct task_mm_section *psection;
	int i, ret = 0;
	int nr_page;
	
	/* 
	 * as the pdir is created can copy the memory
	 * from the code list from parent to new
	 */
	if (!new || !parent)
		return -EINVAL;

	/*
	 * first alloc memory for each section
	 */
	for (i = 0; i < TASK_MM_SECTION_MAX; i++) {
		nsection = &nmm->mm_section[i];
		psection = &pmm->mm_section[i];

		/* how many pages is needed */
		nr_page = psection->alloc_pages -
			nsection->alloc_pages;

		ret = section_get_memory(nr_page, nsection);
		if (ret == -ENOMEM)
			goto err;
	}

	if (copy_task_elf(nmm, pmm))
		goto err;

	if (copy_task_stack(nmm, pmm))
		goto err;

	if (copy_task_mmap(nmm, pmm))
		goto err;

	return 0;

err:
	free_sections_memory(nmm);
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

	//if (map_new_task_page(mm, page, 
	//	PROCESS_META_DATA_BASE, TASK_MM_SECTION_RO))
	//	return -ENOMEM;

	return ret;
}

int init_mm_struct(struct task_struct *task)
{
	struct mm_struct *mm = &task->mm_struct;
	struct mm_struct *mmp = &task->parent->mm_struct;
	int i;

	if (task_is_kernel(task))
		return 0;

	/*
	 * get the elf file's informaiton
	 */
	mm->elf_file = dup_elf_info(mmp->elf_file);
	if (!mm->elf_file) {
		struct file *file;

		file = kernel_open(task->name, O_RDONLY);
		if (!file)
			return -ENOENT;

		mm->elf_file = get_elf_info(file);
		if (!mm->elf_file)
			return -ENOMEM;

		mm->elf_file->file = file;
	}

	/* 
	 * if new task's parent is started by exec  we need get
	 * the information from the elf file, else copy
	 * the information from the parent task.
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

	if (init_task_page_table(&mm->page_table))
		goto err;

	return 0;

err:
	release_elf_file(mm->elf_file);
	return -ENOMEM;
}

