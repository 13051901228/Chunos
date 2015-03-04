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

static int init_task_struct(struct task_struct *task, unsigned long flag)
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
	release_kernel_stack(task);
	task_mm_release_task_memory(&task->mm_struct);

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

	task_mm_copy_task_memory(old, new);

	return 0;
}

static int set_up_task_stack(struct task_struct *task, pt_regs *regs)
{
	unsigned long stack_base;

	/*
	 * in arm, stack is grow from up to down,so we 
	 * adjust it;
	 */
	if (task->stack_origin == 0) {
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
	return 0;
}

static int task_setup_argenv(struct task_struct *task,
		char *name, char **argv, char **envp)
{
	if (task_is_kernel(task))
		return -EINVAL;

	return task_mm_setup_argenv(&task->mm_struct,
			name, argv, envp);
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

static int reinit_task(struct task_struct *task, char *name)
{
	int ret;

	if (name)
		strncpy(task->name, name, PROCESS_NAME_SIZE);

	ret = init_mm_struct(task);
	if (ret)
		return ret;

	return 0;
}

static struct task_struct *
alloc_and_init_task(char *name, unsigned long flag)
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

int do_fork(char *name, pt_regs *regs, unsigned long flag)
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
	release_task_memory(task);
	kfree(task);
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
	int err = 0;
	unsigned long flag = current->flag;

	if (current->flag & PROCESS_TYPE_KERNEL) {
		/*
		 * need to fork a new task, before exec
		 * but no call sys_fork to fork a new task
		 * becase kernel process has not allocate page
		 * table and mm_struct.
		 */
		new = alloc_and_init_task(name,
				PROCESS_FLAG_KERN_EXEC);
		if (!new) {
			debug("Can not Create new process when exec\n");
			return -ENOMEM;
		}
	} else {
		/* need reinit the mm_struct */
		new = current;
		if (reinit_task(new, name))
			goto out_err;

		new->flag |= PROCESS_FLAG_USER_EXEC;
	}

	/* 
	 * must before load_elf_image, since the memory
	 * will be overwrited by it
	 */
	task_setup_argenv(new, name, argv, envp);

	copy_sigreturn_code(new);

	/*
	 * load the elf file to memory, the original process
	 * will be coverd by new process. so if process load
	 * failed, the process will be core dumped.
	 */
	err = task_mm_load_elf_image(&new->mm_struct);
	if (err) {
		kernel_error("Failed to load elf file to memory\n");
		goto out_err;
	}

	/* flush cache and write buffer to memory */
	flush_cache();

	/* modify the regs for new process. */
	init_pt_regs(regs, NULL, NULL);

	/*
	 * fix me - whether need to do this if exec
	 * was called by user space process?
	 */
	set_up_task_stack(new, regs);
	set_task_state(new, PROCESS_STATE_PREPARE);

out_err:
	if(err && (flag & PROCESS_TYPE_KERNEL))
		release_task(new);

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


int __init_text build_idle_task(void)
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
