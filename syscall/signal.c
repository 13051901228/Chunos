/*
 * syscall/signal.c
 *
 * Created By Le Min at 2014/09/01
 *
 */

#include <sys/sig.h>
#include <os/kernel.h>
#include <os/syscall.h>
#include <os/sched.h>

extern int arch_do_signal(pt_regs *, unsigned long, void *, void *);

static inline int os_do_signal(pt_regs *regs, unsigned long user_sp,
			       sighandler_t handler, void *arg)
{
	return arch_do_signal(regs, user_sp, (void *)handler, arg);
}

unsigned long get_sigreturn_addr(void)
{
	return (PROCESS_USER_BASE + PROCESS_SIGRETURN_OFFSET);
}

int signal_handler(pt_regs *regs, unsigned long user_sp)
{
	struct signal_struct *signal = &current->signal;
	int ret = 0;
	struct signal_action *action = NULL;
	struct signal_entry *entry = NULL;

	/* if the task is arealy in signal hander return 0 */
	if (signal->in_signal)
		return 0;

	if (signal->signal_pending) {
		action = signal->pending;
		if (!action) {
			signal->signal_pending = 0;
			signal->plast = NULL;
			return 0;
		}

		entry = &signal->signal_table[action->action];
		if (!entry->handler)
			return 0;

		os_do_signal(regs, user_sp, entry->handler, action->arg);
		signal->in_signal = 1;
		return 1;
	}

	return ret;
}

void copy_sigreturn_code(struct task_struct *task)
{
	struct list_head *list = &task->mm_struct.elf_image_list;
	extern char sigreturn_start[];
	extern char sigreturn_end[];
	unsigned long load_base;

	list = list_next(list);
	load_base = page_to_va(list_entry(list, struct page, plist));
	load_base += PROCESS_SIGRETURN_OFFSET;

	memcpy(load_base, sigreturn_start, sigreturn_end - sigreturn_start);
}

static sighandler_t sys_signal(int signum, sighandler_t handler)
{
	sighandler_t ret;
	struct signal_entry *entry;
	struct signal_struct *signal = &current->signal;

	if (signum >= MAX_SIGNAL)
		return NULL;

	entry = &signal->signal_table[signum];
	ret = entry->handler;

	/* update the flag */
	if (handler == SIG_DFL) {
		entry->flag |= SIG_HANDLER_DFL;
		entry->flag &= (~SIG_HANDLER_IGN);
	} else if (handler == SIG_IGN) {
		entry->flag |= SIG_HANDLER_IGN;
		entry->flag &= (~SIG_HANDLER_DFL);
	} else {
		entry->handler = handler;
		entry->flag &= (~SIG_HANDLER_DFL);
		entry->flag &= (~SIG_HANDLER_IGN);
	}

	return ret;
}
DEFINE_SYSCALL(signal, __NR_signal, (void *)sys_signal);

/* add sigaction, this is just temp implemention */
int sigaction(int signum, const struct sigaction *act,
		struct sigaction *old)
{
	sighandler_t ret;

	ret = sys_signal(signum, act->_u.sa_handler);
	old->_u.sa_handler = ret;

	return 0;
}
DEFINE_SYSCALL(sigaction, __NR_rt_sigaction, (void *)sigaction);
DEFINE_SYSCALL(rt_sigaction, __NR_rt_sigaction, (void *)sigaction);

static void sys_sigreturn(void)
{
	struct signal_struct *signal = &current->signal;
	struct signal_action *now;

	printk("in sigreteturn\n");

	/* what should do here TBC */
	lock_kernel();
	now = signal->pending;
	signal->in_signal = 0;
	signal->pending = signal->pending->next;
	signal->signal_pending -= 1;
	kfree(now);
	unlock_kernel();
}
DEFINE_SYSCALL(sigreturn, __NR_sigreturn, (void *)sys_sigreturn);

static int do_default_signal_handler(
		struct task_struct *task, int sig)
{
	/* TBC */
	switch (sig) {
		case SIGABRT:
		case SIGKILL:
		case SIGQUIT:
		case SIGSTOP:
			if (current == task)
				task_kill_self(task);
			else
				kill_task(task);
			break;

		default:
			break;
	}

	return 0;
}

static int sys_kill(pid_t pid, int sig)
{
	struct task_struct *task;
	struct signal_struct *signal;
	struct signal_action *action;
	struct signal_entry *entry;

	if ((pid < 0) || (sig >= MAX_SIGNAL))
		return -EINVAL;

	/* if pid == curretn pid, the signal is send
	 * to itself, else find the task with the pid
	 */
	task = pid_get_task(pid);
	if (!task) {
		kernel_error("kill: pid %d task is not exist\n", pid);
		return -ESRCH;
	}

	signal = &task->signal;
	entry = &signal->signal_table[sig];
	if ((entry->flag) & SIG_HANDLER_IGN)
		return 0;

	if (entry->flag & SIG_HANDLER_DFL)
		return do_default_signal_handler(task, sig);

	action = kmalloc(sizeof(struct signal_action), GFP_KERNEL);
	if (!action)
		return -ENOMEM;

	action->action = sig;
	action->next = NULL;
	action->arg = NULL;

	/* update the pending signal list */
	lock_kernel();
	if (signal->plast)
		signal->plast->next = action;
	if (!signal->pending)
		signal->pending = action;
	signal->plast = action;
	signal->signal_pending++;
	unlock_kernel();

	return 0;
}
DEFINE_SYSCALL(kill, __NR_kill, (void *)sys_kill);

int init_signal_struct(struct task_struct *task)
{
	int i;

	struct signal_struct *signal = &task->signal;

	signal->pending = NULL;
	signal->plast = NULL;
	signal->signal_pending = 0;
	signal->in_signal = 0;
	memset((char *)signal->signal_table,
		0, sizeof(struct signal_entry) * MAX_SIGNAL);

	/* here register some defalut signal handler TBC */
	for (i = 0; i < MAX_SIGNAL; i++)
		signal->signal_table[i].flag |= SIG_HANDLER_DFL;

	return 0;
}
