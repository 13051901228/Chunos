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
#include <os/sched.h>

static void free_sections_memory(struct mm_struct *mm)
{
	int i;
	struct task_mm_section *section;
	
	for (i = 0; i < TASK_MM_SECTION_MAX; i++) {
		section = &mm->mm_section[i];
		free_pages_on_list(&section->alloc_mem);
		section++;
	}
}

void task_mm_release_task_memory(struct mm_struct *mm)
{
	free_task_page_table(&mm->page_table);
	free_sections_memory(mm);
}

static void init_task_mm_section(struct mm_struct *mm, int i)
{
	struct task_mm_section *section;

	section = &mm->mm_section[i];

	switch (i) {
	case TASK_MM_SECTION_RO:
		section->section_size =
			elf_memory_size(mm->elf_file);
		section->base_addr = elf_get_elf_base(mm->elf_file);
		section->flag |= TASK_MM_SECTION_FLAG_RO;
		break;

	case TASK_MM_SECTION_STACK:
		section->section_size = SIZE_NM(1);
		section->base_addr = PROCESS_USER_STACK_BASE;
		section->flag |= TASK_MM_SECTION_FLAG_STACK;
		break;

	case TASK_MM_SECTION_MMAP:
		section->section_size = SIZE_NM(512);
		section->base_addr = PROCESS_USER_MMAP_BASE;
		section->flag |= TASK_MM_SECTION_FLAG_MMAP;
		break;

	case TASK_MM_SECTION_META:
		section->section_size = SIZE_NK(16);
		section->base_addr = PROCESS_USER_META_BASE;
		section->flag |= TASK_MM_SECTION_FLAG_META;
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
	init_list(&section->alloc_mem);
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
	csection->base_addr = psection->base_addr;
	csection->mapped_size = 0;
	csection->alloc_pages = 0;
	csection->flag = psection->flag;
	init_list(&csection->alloc_mem);
}

static int section_get_memory(int count,
		struct task_mm_section *section)
{
	int i;
	int flag;
	struct page *page;
	struct list_head *list;

	if (count == 0)
		return 0;

	if (section->flag & TASK_MM_SECTION_FLAG_MMAP)
		flag = GFP_MMAP;
	else
		flag = GFP_USER;

	/*
	 * if need more page allocate page for section
	 * else will free unused page from the list
	 */
	if (count > 0) {
		for (i = 0; i < count; i++) {
			page = request_pages(1, flag);
			if (!page)
				return -ENOMEM;

			add_page_to_list_tail(page,
					&section->alloc_mem);
			section->alloc_pages++;
			section->mapped_size += PAGE_SIZE;
		}
	} else {
		while (count != 0) {
			list = &section->alloc_mem;
			list = list_prve(list);
			page = list_to_page(list);
			release_page(page);
			count++;
		}
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

	ret = section_get_memory(nr_page, section);
	if (ret)
		return ret;

	return pgt_map_task_memory(&mm->page_table,
			&section->alloc_mem,
			section->base_addr, PGT_MAP_STACK);
}


int task_mm_load_elf_image(struct mm_struct *mm)
{
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_RO];
	size_t load_size;
	int page_nr, ret;
	struct list_head *list = &section->alloc_mem;
	struct list_head *temp;
	struct page *page;
	unsigned long base, offset = 0;
	struct task_page_table *ctable =
		&current->mm_struct.page_table;

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
	
	ret = pgt_map_task_memory(&mm->page_table,
			&section->alloc_mem,
			section->base_addr, PGT_MAP_NORMAL);
	if (ret)
		return ret;

	if (mm != &current->mm_struct) {
		list_for_each(list, temp) {
			page = list_to_page(temp);
			base = pgt_map_temp_page(ctable, page);
			ret = elf_load_elf_image(mm->elf_file, base,
					MIN(load_size, PAGE_SIZE), offset);
			load_size -= ret;
			offset += ret;
			pgt_unmap_temp_page(ctable, base);
		}

		ret = 0;
	} else {
		ret = elf_load_elf_image(mm->elf_file,
				section->base_addr,
				load_size, 0);
	}

	return ret;
}

static int copy_section_memory(struct task_mm_section *new,
		struct task_mm_section *parent, int type)
{
	int count = new->alloc_pages;
	unsigned long pbase = parent->base_addr;
	unsigned long nbase;
	struct page *page;
	struct list_head *list = list_next(&new->alloc_mem);
	struct task_page_table *page_table =
		&current->mm_struct.page_table;

	if (new->alloc_pages != parent->alloc_pages)
		return -EINVAL;

	while (count) {
		page = list_to_page(list);
		nbase = pgt_map_temp_page(page_table, page);
		if (!nbase)
			return -EFAULT;

		memcpy((char *)nbase, (char *)pbase, PAGE_SIZE);
		if (type != PGT_MAP_STACK)
			pbase += PAGE_SIZE;
		else
			pbase -= PAGE_SIZE;

		count --;
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
			n->base_addr, type);
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
			PGT_MAP_NORMAL);
}

static inline int copy_task_stack(struct mm_struct *new,
		struct mm_struct *p)
{
	return copy_task_mm_section(new, p,
			TASK_MM_SECTION_STACK,
			PGT_MAP_STACK);
}

static inline int copy_task_meta(struct mm_struct *new,
		struct mm_struct *p)
{
	return copy_task_mm_section(new, p,
			TASK_MM_SECTION_META,
			PGT_MAP_NORMAL);
}

static int copy_task_mmap(struct mm_struct *new,
		struct mm_struct *parent)
{
	struct task_mm_section *n =
		&new->mm_section[TASK_MM_SECTION_MMAP];
	struct task_mm_section *p =
		&parent->mm_section[TASK_MM_SECTION_MMAP];
	struct list_head *nl, *pl;
	struct page *np, *pp;
	int ret, count;
	unsigned long base;

	if (n->alloc_pages != p->alloc_pages)
		return -EINVAL;

	nl = list_next(&n->alloc_mem);
	pl = list_next(&p->alloc_mem);
	count = n->alloc_pages;

	while (count) {
		np = list_to_page(nl);
		pp = list_to_page(pl);
		
		ret = pgt_map_mmap_page(&new->page_table,
				np, page_get_map_address(pp));
		if (ret)
			return ret;

		base = pgt_map_temp_page(&parent->page_table, np);
		if (!base)
			return -EFAULT;

		memcpy((char *)base, (char *)page_to_va(pp), PAGE_SIZE);

		nl = list_next(nl);
		pl = list_next(pl);
		count--;
		pgt_unmap_temp_page(&parent->page_table, base);
	}

	return 0;
}

int task_mm_copy_task_memory(struct task_struct *new,
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
	free_sections_memory(nmm);
	return -ENOMEM;
}


int task_mm_setup_meta_data(struct mm_struct *mm, void *callback[])
{
	int i;
	struct task_struct *task = container_of(mm,
		struct task_struct, mm_struct);
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_META];
	unsigned long base;
	struct list_head *list = list_next(&section->alloc_mem);
	struct task_page_table *ctable =
		&current->mm_struct.page_table;
	meta_data_func func;

	if (&current->mm_struct != mm) {
		base = pgt_map_temp_page(ctable, list_to_page(list));
		if (!base)
			return -ENOMEM;
	} else {
		base = section->base_addr;
	}

	for (i = 0; ; i++) {
		func = (meta_data_func)callback[i];
		if (!func)
			break;

		func(task, base);
	}

	if (&current->mm_struct != mm)
		pgt_unmap_temp_page(&current->mm_struct.page_table, base);

	return 0;
}

static int init_task_meta_section(struct mm_struct *mm)
{
	struct task_mm_section *section =
		&mm->mm_section[TASK_MM_SECTION_META];

	if (!section->alloc_pages) {
		if (section_get_memory(1, section))
			return -ENOMEM;

		/*
		 * if task is execed by user space
		 * can skip to init it again
		 */
		if (pgt_map_task_memory(&mm->page_table,
				&section->alloc_mem,
				section->base_addr,
				PGT_MAP_NORMAL))
			return -EFAULT;
	}

	return 0;
}

unsigned long task_mm_mmap(struct mm_struct *mm,
		unsigned long start, size_t len,
		int flags, int fd, offset_t off)
{
	int i, ret = 0;
	unsigned long ua, base;
	struct page *page;
	int nr = page_nr(len);
	struct task_mm_section *mmap =
		&mm->mm_section[TASK_MM_SECTION_MMAP];

	if (start) {
		ret = pgt_check_mmap_addr(&mm->page_table,
				start, nr);
		if (ret)
			return 0;

		ua = start;
	} else {
		ua = pgt_get_mmap_base(mm->page_table, nr);
		if (!ua)
			return 0;
	}

	base = ua;

	for (i = 0; i < nr; i++) {
		page = request_pages(1, GFP_MMAP);
		if (!page)
			return 0;

		add_page_to_list_tail(page, &mmap->alloc_mem);
		ret = pgt_map_mmap_page(&mm->page_table, page, ua);
		page->pinfo = fd;
		page->pdata = off + i * PAGE_SIZE;
		ua += PAGE_SIZE;
	}

	if (fd > 0) {
		ret = fmmap(fd, (char *)ua, len, off);
		if (ret != len)
			return 0;
	}

	return base;
}

int task_mm_munmap(struct mm_struct *mm,
		unsigned long start,
		size_t length, int flags, int sync)
{
	int i, nr;
	struct page *page;

	nr = page_nr(length);

	if (sync) {
		page = pgt_get_task_page(&mm->page_table, start);
		if (!page)
			return -EINVAL;

		if (page->pinfo < 0)
			return -EINVAL;

		/*
		 * write data to the file if needed
		 */
		fmsync(page->pinfo, (char *)start, length, page->pdata);
	}

	for (i = 0; i < nr; i++) {
		/*
		 * unmap page
		 */
		page = pgt_unmap_mmap_page(&mm->page_table, start);
		if (!page) {
			kernel_error("Umap address is not exist\n");
			start += PAGE_SIZE;
			continue;
		}

		/*
		 * del this page from mmap alloc list
		 */
		del_page_from_list(page);

		start += PAGE_SIZE;
	}

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
		if (task->flag & _PROCESS_FLAG_USER_EXEC) {
			init_task_mm_section(mm, i);
		} else if (task->flag & _PROCESS_FLAG_KERN_EXEC) {
			init_task_mm_section(mm, i);
		} else {
			copy_mm_section_info(mm, mmp, i);
		}
	}

	ret = init_task_page_table(&mm->page_table);
	if (ret)
		return ret;

	/*
	 * if task is a user task, need to allocate
	 * user stack for it.
	 */
	ret = alloc_task_user_stack(mm);
	if (ret)
		return ret;

	ret = init_task_meta_section(mm);
	if (ret)
		return ret;

	return 0;
}

