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
#include <os/mirros.h>

#define MEM_TYPE_GROW_UP	1
#define MEM_TYPE_GROW_DOWN	0

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
		section->base_addr = elf_get_base_addr(mm->elf_file);
		break;

	case TASK_MM_SECTION_STACK:
		section->section_size = SIZE_NM(1);
		section->base_addr = PROCESS_USER_STACK_BASE;
		break;

	case TASK_MM_SECTION_MMAP:
		section->section_size = SIZE_NM(512);
		section->base_addr = PROCESS_USER_MMAP_BASE;
		break;

	case TASK_MM_SECTION_META:
		section->section_size = SIZE_NK(16);
		section->base_addr = PROCESS_USER_META_BASE;
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

static int alloc_task_user_stack(struct mm_struct *mm)
{
	int nr_page;
	int ret = 0;

	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_STACK];

	nr_page = 4 - section->alloc_pages;

	if (nr_page > 0)
		ret = section_get_memory(nr_page, section);

	if (ret)
		return ret;

	return pgt_map_task_memory(&mm->page_table,
			&section->alloc_mem,
			section->base_addr,
			MEM_TYPE_GROW_DOWN);
}


int task_mm_load_elf_image(struct mm_struct *mm)
{
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_RO];
	size_t load_size;
	int page_nr, ret;

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

	ret = elf_load_elf_image(mm->elf_file,
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

static int copy_task_mm_section(struct mm_struct *nmm,
		struct mm_struct *pmm, int i, int type)
{
	int ret;
	struct task_mm_section *n = &nmm->mm_section[i];
	struct task_mm_section *p = &pmm->mm_section[i];

	/*
	 * map memory to new task's space
	 */
	ret = pgt_map_task_memory(&nmm->page_table,
			&n->alloc_mem,
			&n->base_addr, type);
	if (ret)
		return ret;

	/*
	 * copy memory form parent's section
	 */
	ret = copy_section_memory(n, p, type);
	if (ret) {
		kernel_error("Memory is not correct \
				for section\n");
		return ret;
	}

	return 0;
}

static inline int copy_task_elf(struct mm_struct *new,
		struct mm_struct *p)
{
	return copy_task_mm_section(new, p,
			TASK_MM_SECTION_RO,
			MEM_TYPE_GROW_UP);
}

static inline int copy_task_stack(struct mm_struct *new,
		struct mm_struct *p)
{
	return copy_task_mm_section(new, p,
			TASK_MM_SECTION_STACK,
			MEM_TYPE_GROW_UP);
}

static inline int copy_task_meta(struct mm_struct *new,
		struct mm_struct *p)
{
	return copy_task_mm_section(new, p,
			TASK_MM_SECTION_META,
			MEM_TYPE_GROW_UP);
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

	if (copy_task_meta(nmm, pmm))
		goto err;

	if (copy_task_stack(nmm, pmm))
		goto err;

	if (copy_task_mmap(nmm, pmm))
		goto err;

	if (copy_task_elf(nmm, pmm))
		goto err;

	return 0;

err:
	free_sections_memory(nmm->mm_section);
	return -ENOMEM;
}

/*
 * below marco define the meta data struct
 * now the max length of meta area is 4k
 * should be increase later
 */
#define SIGNAL_META_OFFSET	(0)
#define SIGNAL_META_SIZE	(256)

#define ARGV_META_TABLE_OFFSET	\
	(SIGNAL_META_OFFSET + SIGNAL_META_SIZE)
#define ARGV_MAX_NR		(32)
#define ARGV_META_TABLE_SIZE	(ARGV_MAX_NR * sizeof(unsigned long))

#define ENV_META_TABLE_OFFSET	\
	(ARGV_META_TABLE_OFFSET + ARGV_META_TABLE_SIZE)
#define ENV_MAX_NR		(32)
#define ENV_META_TABLE_SIZE	(ENV_MAX_NR * sizeof(unsigned long))

#define ARGENV_META_OFFSET	\
	(ENV_META_TABLE_OFFSET + ENV_META_TABLE_SIZE)

int task_mm_setup_argenv(struct mm_struct *mm,
		char *name, char **argv, char **envp)
{
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_META];
	unsigned long str_base;
	unsigned long *table_base;
	int i = 0, length;
	unsigned long limit =
		section->base_addr + section->mapped_size;

	/* assume all size will < 1 page */

	str_base = section->base_addr + ARGENV_META_OFFSET;
	
	/* Copy argv */
	table_base = (unsigned long *)
		section->base_addr + ARGV_META_TABLE_OFFSET;

	length = strlen(name);
	memcpy((char *)str_base, name, length);
	*table_base = str_base;
	table_base++;
	str_base += length;
	str_base = align(str_base, sizeof(unsigned long));
	while (argv[i] && (i < ARGV_MAX_NR - 1)) {
		length = strlen(argv[i]);
		memcpy((char *)str_base, argv[i], length);
		if ((str_base + length) >= limit)
			goto out;
		*table_base = str_base;
		table_base++;
		str_base += length;
		str_base = align(str_base, sizeof(unsigned long));
		i++;
	}

	i = 0;
	table_base = (unsigned long *)
		section->base_addr + ENV_META_TABLE_OFFSET;
	while (envp[i] && (i < ENV_MAX_NR)) {
		length = strlen(envp[i]);
		memcpy((char *)str_base, envp[i], length);
		if ((str_base + length) >= limit)
			goto out;
		*table_base = str_base;
		table_base++;
		str_base += length;
		str_base = align(str_base, sizeof(unsigned long));
		i++;
	}

out:
	return 0;
}

int task_mm_copy_sigreturn(struct mm_struct *mm,
		char *start, int size)
{
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_META];
	unsigned long load_base =
		section->base_addr + SIGNAL_META_OFFSET;

	if (!start || !size)
		return -EINVAL;

	memcpy((char *)load_base, start, size);
}

static int init_task_meta_section(struct mm_struct *mm)
{
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_META];

	if (!section->alloc_pages) {
		if (section_get_memory(1, section))
			return -ENOMEM;
	}

	if (pgt_map_task_memory(&mm->page_table,
				&section->alloc_mem,
				section->base_addr,
				MEM_TYPE_GROW_UP))
		return -EFAULT;

	return 0;
}

int init_mm_struct(struct task_struct *task)
{
	int i, ret;
	struct mm_struct *mm = &task->mm_struct;
	struct mm_struct *mmp = &task->parent->mm_struct;

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
		} else {
			copy_mm_section_info(mm, mmp, i);
		}
	}

	/*
	 * if task is a user task, need to allocate
	 * user stack for it.
	 */
	if (task_is_user(task)) {
		ret = alloc_task_user_stack(mm);
		if (ret)
			return ret;

		ret = init_task_meta_section(mm);
		if (ret)
			return ret;
	}

	ret = init_task_page_table(&mm->page_table);
	if (ret)
		return ret;

	return 0;
}

