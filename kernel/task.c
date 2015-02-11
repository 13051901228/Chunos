#include <os/types.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/bitops.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/slab.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/mmu.h>
#include <asm/asm_sched.h>
#include <os/elf.h>
#include <os/file.h>
#include <os/mm.h>
#include <os/task.h>
#include <os/syscall.h>
#include <os/irq.h>
#include <os/mmap.h>
#include <sys/sig.h>

#ifdef	DEBUG_PROCESS
#define debug(fmt, ...)	kernel_debug(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define MAX_ARGV	16
#define MAX_ENVP	16
static char *init_argv[MAX_ARGV + 1] = {NULL};
static char *init_envp[MAX_ENVP + 1] = {NULL};

extern int init_signal_struct(struct task_struct *task);
extern void copy_sigreturn_code(struct task_struct *task);

static int init_task_struct(struct task_struct *task, u32 flag)
{
	struct task_struct *parent;
	unsigned long flags;

	/* get a new pid */
	task->pid = get_new_pid(task);
	if ((task->pid) < 0)
		return -EINVAL;

	task->uid = current->uid;
	task->stack_base = 0;
	strncpy(task->name, current->name, strlen(current->name));
	task->flag = flag;

	/* add task to the child list of his parent. */
	task->parent = current;
	init_list(&task->p);
	init_list(&task->child);
	init_mutex(&task->mutex);

	/* if process is a userspace process, init the file desc */
	if (flag & PROCESS_TYPE_USER) {
		task->file_desc =
			alloc_and_init_file_desc(current->file_desc);
		if (!task->file_desc)
			return -ENOMEM;
	}

	enter_critical(&flags);
	list_add(&parent->child, &task->p);
	exit_critical(&flags);
	
	return 0;
}

static int inline release_kernel_stack(struct task_struct *task)
{
	if (task->stack_origin)
		free_pages((void *)task->stack_origin);

	task->stack_base = 0;
	task->stack_origin = 0;
	return 0;
}

static int release_task_memory(struct task_struct *task)
{
	/* TBD */
	release_kernel_stack(task);

	return 0;
}

#define mmap_start(start)	\
	(start - PROCESS_USER_MMAP_BASE)

#define mmap_pos_x(start)	\
	(mmap_start(start) / SIZE_NM(4))

#define mmap_pos_y(start)	\
	((mmap_start(start) % SIZE_NM(4)) >> PAGE_SHIFT)

int mmap(struct task_struct *task, unsigned long start,
	 unsigned long virt, int flags, int fd, offset_t off)
{
#if 0
	struct mm_struct *ms = &task->mm_struct;
	int i, j;
	struct list_head *list = &ms->mmap_page_list;
	unsigned long *page_table;
	struct page *page;

	i = mmap_pos_x(start);
	j = mmap_pos_y(start);

	do {
		list = list_next(list);
		i--;
	} while (i >= 0);

	page = list_entry(list, struct page, pgt_list);
	page_table = (unsigned long *)page->free_base;
	page_table += j;

	if (*page_table != 0)
		return -EAGAIN;

	/* flag to be done */
	build_page_table_entry((unsigned long)page_table, virt, PAGE_SIZE, 0);

	/* after map we need flus the cache and invaild the TLB */
	flush_mmu_tlb();
	flush_cache();

	/* update the mmap information, the mmap fd information
	 * is stored in the page->free_base
	 */
	page = va_to_page(virt);
	list_add_tail(&ms->mmap_mem_list, &page->plist);
	page->flag |= __GFP_USER;
	page->free_base = (virt & 0xfffff000) | (mmap_magic(fd));
	page->mmap_offset = off;
#endif
	return 0;
}

int munmap(struct task_struct *task, unsigned long start,
		size_t length, int flags, int sync)
{
#if 0
	struct mm_struct *ms = &task->mm_struct;
	int bit_start;
	int i, j;
	struct list_head *list = &ms->mmap_page_list;
	unsigned long *page_table;
	unsigned long buffer;
	struct page *page;
	struct page *tmp;
	int fd = 0xff;
	int pages = page_nr(length);
	size_t size;

	bit_start = mmap_start(start) >> PAGE_SHIFT;
	i = mmap_pos_x(start);
	j = mmap_pos_y(start);
	
	do {
		list = list_next(list);
		i--;
	} while (i >= 0);

	page = list_entry(list, struct page, pgt_list);
	page_table = (unsigned long *)page->free_base;
	page_table += j;

	for (i = 0; i < pages; i++) {
		/* next page table */
		if (j == 1024) {
			j = 0;
			list = list_next(list);
			page = list_entry(list, struct page, pgt_list);
			page_table = (unsigned long *)page->free_base;
			page_table += j;
		}

		if (*page_table == 0) {
			kernel_error("bug, memory is not maped please check\n");
			j++;
			page_table++;
			continue;
		}

		/* get the va of the mem */
		buffer = *page_table & 0xfffff000;
		buffer = pa_to_va(buffer);
		tmp = va_to_page(buffer);

		/* sync is 1: do msync else do munmap */
		if (sync) {
			fd = mmap_addr2fd(tmp->free_base);
			if (fd != 0xff) {
				size = length > PAGE_SIZE ? PAGE_SIZE : length;
				if (fmsync(fd, (char *)buffer, size, tmp->mmap_offset) != size) {
					kernel_warning("fmsync failed fd:%d offset:0x%x\n",
							fd, tmp->mmap_offset);
				}
				length -= size;
			}
		} else {
			list_del(&tmp->plist);
			if (read_bit(ms->mmap, bit_start + i))
				clear_bit(ms->mmap, bit_start + i);

			kfree((void *)buffer);
			*page_table = 0;
		}

		page_table++;
	}

	/* if is munmap flush the cache */
	if (!sync) {
		flush_mmu_tlb();
		flush_cache();
	}
#endif
	return 0;
}

unsigned long get_mmap_user_base(struct task_struct *task, int page_nr)
{
#if 0
	struct mm_struct *ms = &task->mm_struct;
	unsigned long start;
	int i, j, k;

	/* find continous pages to map, shall we add a spin_lock ?*/
	kernel_debug("ms->free_pos is %d\n", ms->free_pos);
	i = bitmap_find_free_base(ms->mmap, ms->free_pos,
			0, ms->mmap_nr, page_nr);
	if (i < 0) {
		kernel_error("no free user space for mmap\n");
		return 0;
	}

	/* calculate the free base of the user space
	 * j is the n list, k is the pos in the list*/
	j = i / 1024;
	k = i % 1024;

	start = PROCESS_USER_MMAP_BASE + (j * SIZE_NM(4)) + (k * PAGE_SIZE);

	/* mask the memory will be used */
	for (j = 0; j < page_nr; j++) {
		set_bit(ms->mmap, i);
		i++;
	}
	ms->free_pos = (i >= ms->mmap_nr ? 0 : i);
#endif
	return 0;
}

static int alloc_kernel_stack(struct task_struct *task)
{
	if(task->stack_base)
		return -EINVAL;

	task->stack_base = (unsigned long)
		get_free_pages(page_nr(KERNEL_STACK_SIZE), GFP_KERNEL);
	task->stack_origin = task->stack_base;
	if (task->stack_base == 0)
		return -ENOMEM;

	debug("task %s stack is 0x%x\n",
		     task->name, (u32)task->stack_base);

	return 0;
}

static int copy_process(struct task_struct *new)
{
	struct task_struct *old = new->parent;

	if (old) {
		copy_process_memory(old, new);
	}

	return 0;
}

static int set_up_task_stack(struct task_struct *task, pt_regs *regs)
{
	unsigned long stack_base;

	/*
	 * in arm, stack is grow from up to down,so we 
	 * adjust it;
	 */
	if (task->stack_base == NULL) {
		kernel_error("task kernel stack invailed\n");
		return -EFAULT;
	}

	task->stack_base = task->stack_origin;
	task->stack_base += KERNEL_STACK_SIZE;
	stack_base = task->stack_base;

	stack_base = arch_set_up_task_stack(stack_base, regs);
	task->stack_base = stack_base;

	return 0;
}

/*
 * this function used to do some work when
 * switch task such as exchange the page
 * table.
 */
int switch_task(struct task_struct *cur,
		struct task_struct *next)
{
#if 0
	struct list_head *list;
	struct list_head *head;
	unsigned long stack_base = PROCESS_USER_STACK_BASE;
	unsigned long task_base = PROCESS_USER_BASE;
	unsigned long mmap_base = PROCESS_USER_MMAP_BASE;
	unsigned long pa;
	struct page *page;

	if (!cur || !next)
		return -EINVAL;

	/*
	 * frist we need flush all cache for the current
	 * process, this cacue the two bugs which spend
	 * me two weeks to debug.
	 */
	flush_cache();

	/*
	 * if the task is a kernel task, we do not
	 * need to load his page table. else if next
	 * run is a userspace task, we load his page
	 * table.
	 */
	if (next->flag & PROCESS_TYPE_KERNEL)
		return 0;

	/*
	 * load the page table for stack, because stack
	 * is grow downward, sub 4m for one page.
	 */
	head = &next->mm_struct.stack_list;
	list_for_each (head, list) {
		page = list_entry(list, struct page, pgt_list);
		pa = page_to_pa(page);
		build_tlb_table_entry(stack_base - SIZE_NM(4), pa,
				      SIZE_NM(4), TLB_ATTR_USER_MEMORY);
		stack_base -= SIZE_NM(4);
	}

	/* load elf image memory page table */
	head = &next->mm_struct.elf_list;
	list_for_each (head, list) {
		page = list_entry(list, struct page, pgt_list);
		pa = page_to_pa(page);
		build_tlb_table_entry(task_base, pa, SIZE_NM(4), TLB_ATTR_USER_MEMORY);
		task_base += SIZE_NM(4);
	}

	head = &next->mm_struct.mmap_page_list;
	list_for_each (head, list) {
		page = list_entry(list, struct page, pgt_list);
		pa = page_to_pa(page);
		build_tlb_table_entry(mmap_base, pa, SIZE_NM(4), TLB_ATTR_USER_MEMORY);
		task_base += SIZE_NM(4);
	}
#endif
	return 0;
}

static int task_setup_argv_envp(struct task_struct *task,
		char *name, char **argv, char **envp)
{
	if (task_is_kernel(task))
		return -EINVAL;

	return task_mm_setup_argv_envp(&task->mm_struct, name, argv, envp);
}

static struct task_struct *allocate_task(char *name)
{
	struct task_struct *task = NULL;
	int pages = page_nr(sizeof(struct task_struct));
	int name_size = 0;

	/* 
	 * [TBD] each task allocate pages size, so need to
	 * check the size of struct task_struct, but now seems
	 * it will take one page for each task
	 */
	task = (struct task_struct *)get_free_pages(pages, GFP_KERNEL);
	if (!task)
		return NULL;

	memset((char *)task, 0, PAGE_SIZE * pages);

	/* set the name of the task usually the bin path */
	if (name) {
		name_size = strlen(name);
		name_size = MIN(name_size, PROCESS_NAME_SIZE);
		strncpy(task->name, name, name_size);
	}

	return task;
}

static inline void free_task(struct task_struct *task)
{
	if (task)
		free_pages((void *)task);
}

struct task_struct *alloc_and_init_task(char *name, int flag)
{
	struct task_struct *new;
	int ret = 0;

	new = allocate_task(name);
	if (!new)
		return NULL;

	ret = init_task_struct(new, flag);
	if (ret) {
		kernel_error("Invaild pid \n");
		goto exit;
	}

	init_sched_struct(new);
	init_signal_struct(new);

	if (init_mm_struct(new))
		goto exit;

	if (alloc_kernel_stack(new))
		goto exit;

	debug("Task: name:%s, prio:%d, pid:%d\n",
		     new->name, new->prio, new->pid);
	return new;

exit:
	free_task(new);
	return NULL;
}

int do_fork(char *name, pt_regs *regs, int flag)
{
	struct task_struct *new;

	/* get a new task_struct instance */
	new = alloc_and_init_task(name, flag);
	if (!new) {
		kernel_error("fork new task failed\n");
		return -ENOMEM;
	}

	if (task_is_user(new))
		copy_process(new);

	set_up_task_stack(new, regs);
	set_task_state(new, PROCESS_STATE_PREPARE);

	return new->pid;
}

static void inline init_pt_regs(pt_regs *regs, void *fn, void *arg)
{
	/*
	 * set up the stack of the task before his
	 * first running
	 */
	arch_init_pt_regs(regs, fn, arg);
}

int kthread_run(char *name, int (*fn)(void *arg), void *arg)
{
	u32 flag = 0;
	pt_regs regs;

	flag |= PROCESS_TYPE_KERNEL;	
	init_pt_regs(&regs, (void *)fn, arg);

	return do_fork(name, &regs, flag);
}

void release_task(struct task_struct *task)
{
	if (task) {
		release_task_memory(task);
		kfree(task);
	}
}

int load_elf_section(struct elf_section *section,
		     struct file *file,
		     struct mm_struct *mm)
{
#if 0
	struct list_head *list = &mm->elf_image_list;
	struct page *page = NULL;
	int i, j, k;
	char *base_addr;
	unsigned long base;

	/*
	 * the first 4k memory for task is 0x00001000
	 * so we can calculate the location of the section
	 * in the memeory
	 */
	if ((section->load_addr < PROCESS_USER_EXEC_BASE) ||
	    (section->size == 0))
		return -EINVAL;

	/* find the memeory location in the list */
	base = section->load_addr - PROCESS_USER_BASE;
	i = base >> PAGE_SHIFT;
	j = base % PAGE_SIZE;
	debug("load elf image page %d offset %d\n", i, j);
	for (k = 0; k <= i; k++) {
		list = list_next(list);
		if (list == NULL)
			return -ENOMEM;
	}

	/*
	 * if the section is bss, we clear the all memory
	 * to 0, else we load the data to the image.
	 */
	k = section->size;
	mm->elf_size += k;
	kernel_seek(file, section->offset, SEEK_SET);
	do {
		page = list_entry(list, struct page, plist);
		base_addr = (char *)page->free_base;
		base_addr = base_addr + j;
		i = PAGE_SIZE -j;
		j = k >= i ? PAGE_SIZE -j : k;
		if (strncmp(section->name, ".bss", 4)) {
			debug("Load Section:[%s] address:[0x%x] size[0x%x]\n",
				     section->name, base_addr, j);
			i = kernel_read(file, base_addr, j);
			if (i < 0)
				return -EIO;
		} else {
			debug("Clear BSS section to Zero: base:[0x%x] size:[0x%x]\n",
				      base_addr, j);
			memset(base_addr, 0, j);
		}

		k = k - j;
		j = 0;
		list = list_next(list);
	} while (k > 0);
#endif	
	return 0;
}

int load_elf_image(struct task_struct *task,
		   struct file *file,
		   struct elf_file *elf)
{
#if 0
	struct mm_struct *mm;
	struct elf_section *section;
	int ret = 0;

	if (!task || !file || !elf)
		return -ENOENT;

	/* clear elf size to zero */
	mm->elf_size = 0;

	/* load each section to memory */
	for ( ; section != NULL; section) {
		ret = load_elf_section(section, file, mm);
		if (ret)
			return ret;
	}
#endif
	return 0;
}

int task_kill_self(struct task_struct *task)
{
	struct task_struct *system_killer = pid_get_task(0);

	if (!system_killer)
		return -EINVAL;
	
	/*
	 * when a task try to kill himself, need to
	 * wake up system_killer process to help doing
	 * this
	 */
	kernel_debug("Kill self\n");
	set_task_state(task, PROCESS_STATE_IDLE);
	wakeup_task(system_killer);
	sched();
	return 0;
}

int kill_task(struct task_struct *task)
{
	unsigned long flags;
	int ret = 0;
	struct task_struct *child;
	struct list_head *list;
	struct mutex *mutex;

	kernel_debug("kill task\n");
	if (!task)
		return -EINVAL;
	/*
	 * set task state to idle in case it will run
	 * during kill action
	 */
	set_task_state(task, PROCESS_STATE_IDLE);

	ret = sched_remove_task(task);
	if (ret) {
		kernel_error("Can not remove task form sched\n");
		return ret;
	}

	/*
	 * remove task from parent modify the child task's parent
	 * to task's parent
	 */
	enter_critical(&flags);
	list_del(&task->p);
	list_for_each(&task->child, list) {
		child = list_entry(list, struct task_struct, p);
		child->parent = task->parent;
		list_add_tail(&task->parent->child, list);
	}

	/*
	 * deal with the mutex which task has wait and get
	 * remove the task from the mutex list which he has
	 * got, unlock all mutex which he has got.
	 */
	if (!is_list_empty(&task->wait))
		list_del(&task->wait);

	list_for_each(&task->mutex_get, list) {
		mutex = list_entry(list, struct mutex, task);
		mutex_unlock(mutex);
	}
	exit_critical(&flags);

	/* release the file which the task opened */
	release_task_file_desc(task);

	/* release the memory the task obtained and itself */
	release_task(task);

	return 0;
}

int do_exec(char __user *name,
	    char __user **argv,
	    char __user **envp,
	    pt_regs *regs)
{
	struct task_struct *new;
	struct file *file;
	int err = 0;
	struct elf_file *elf = NULL;
	int ae_size = 0;

	if (current->flag & PROCESS_TYPE_KERNEL) {
		/*
		 * need to fork a new task, before exec
		 * but no call sys_fork to fork a new task
		 * becase kernel process has not allocat page
		 * table and mm_struct.
		 */
		new = alloc_and_init_task(name, PROCESS_TYPE_USER);
		if (!new) {
			debug("can not fork new process when exec\n");
			return -ENOMEM;
		}
	} 
	else {
		/* need reinit the mm_struct */
		new = current;
		strncpy(new->name, name, PROCESS_NAME_SIZE);
	}

	/* 
	 * must befor load_elf_image, since the memory
	 * will be overwrited by it
	 */
	ae_size = task_setup_argv_envp(new, name, argv, envp);
	copy_sigreturn_code(new);

	/*
	 * load the elf file to memory, the original process
	 * will be coverd by new process. so if process load
	 * failed, the process will be core dumped.
	 */
	err = load_elf_image(new, file, elf);
	if (err) {
		kernel_error("Failed to load elf file to memory\n");
		goto release_elf_file;
	}

	/* flush cache and write buffer to memory */
	flush_cache();

	/* modify the regs for new process. */
	init_pt_regs(regs, NULL, (void *)ae_size);

	/*
	 * fix me - whether need to do this if exec
	 * was called by user space process?
	 */
	if (current->flag & PROCESS_TYPE_KERNEL) {
		set_up_task_stack(new, regs);
		set_task_state(new, PROCESS_STATE_PREPARE);
	}

release_elf_file:
	release_elf_file(elf);
	kernel_close(file);

	if(err && (current->flag & PROCESS_TYPE_KERNEL)) {
		release_task(new);
	}

	return err;
}

int kernel_exec(char *filename)
{
	pt_regs regs;

	memset((char *)&regs, 0, sizeof(pt_regs));
	init_argv[0] = NULL;
	init_argv[1] = NULL;

	return do_exec((char __user *)filename,
		       (char __user **)init_argv,
		       (char __user **)init_envp, &regs);
}


int build_idle_task(void)
{
	idle = allocate_task("idle");
	if (!idle)
		return -ENOMEM;

	idle->pid = -1;
	idle->uid = 0;
	idle->flag = 0 | PROCESS_TYPE_KERNEL;

	idle->parent = NULL;
	init_list(&idle->child);
	init_list(&idle->p);
	init_mutex(&idle->mutex);

	init_sched_struct(idle);
	idle->state = PROCESS_STATE_RUNNING;

	/* update current and next_run to idle task */
	current = idle;
	next_run = idle;

	return 0;
}
