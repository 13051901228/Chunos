#ifndef _TASK_H
#define _TASK_H

#include <os/list.h>
#include <os/types.h>
#include <os/mutex.h>
#include <asm/asm_sched.h>
#include <os/bitops.h>
#include <os/mm.h>
#include <os/mirros.h>
#include <sys/sig.h>
#include <os/task_mm.h>

#define KERNEL_STACK_SIZE	(SIZE_4K)
#define PROCESS_STACK_SIZE	(4 * PAGE_SIZE)		/* 16k stack size */
#define PROCESS_IMAGE_SIZE	(256 * PAGE_SIZE)	/* 1M stack size */

#define PROCESS_NAME_SIZE	255

#define PROCESS_TYPE_KERNEL	0x00000001
#define PROCESS_TYPE_USER	0x00000002

#define PROCESS_FLAG_FORK	0x00000010
#define PROCESS_FLAG_EXEC	0x00000020

#define PROCESS_MAP_STACK	0
#define PROCESS_MAP_ELF		1
#define PROCESS_MAP_HEAP	2
#define PROCESS_MAP_MASK	0x0f

typedef enum _state_t {
	PROCESS_STATE_PREPARE,
	PROCESS_STATE_RUNNING,
	PROCESS_STATE_SLEEP,
	PROCESS_STATE_IDLE,
	PROCESS_STATE_STOP,
	PROCESS_STATE_UNKNOWN
} state_t;

/*
 * task_struct:represent a process in the kernel
 * stack_base:the base address of stack of one task
 * regs:backup the process's register value when switch
 * pid:id of the task
 * uid:the task belongs to which user
 * state:current task state of the task
 * exit_state:the exit state of the task;
 * prio_running:use to communicate to the sched array
 * for find next run task
 * task:used to connect all the task in system
 * sleep:used to connect all the sleeping task in system
 * running:used to connect all the running task in system
 * idle:used to connect all the idle task in system
 * wait:used for mutex,if a task is waitting a mutex,
 * this list_head will insert to the mutex's waitting list
 */
struct task_struct {
	unsigned long stack_base;
	unsigned long stack_origin;

	char name[PROCESS_NAME_SIZE + 1];
	u32 flag;

	pid_t pid;
	u32 uid;
	state_t state;

	struct signal_struct signal;
	struct mm_struct mm_struct;

	struct task_struct *parent;
	struct list_head child;
	struct list_head p;

	int run_time;
	int wait_time;	/* 0-wait time is expire >0:wait time -1:do not wait */
	int time_out;
	u32 run_count;
	int prio;
	int pre_prio;
	struct list_head prio_running;
	struct list_head system;
	struct list_head sleep;
	struct list_head idle;

	struct file_desc *file_desc;

	/*
	 * mutex which task wait and got, wait list only can
	 * have one mutex but get list can have many.
	 */
	struct list_head wait;
	struct list_head mutex_get;
	struct mutex mutex;

	void *message;
};

extern pt_regs *arch_get_pt_regs(void);

static inline int task_is_kernel(struct task_struct *task)
{
	return (!!(task->flag & PROCESS_TYPE_KERNEL));
}

static inline int task_is_user(struct task_struct *task)
{
	return (!!(task->flag & PROCESS_TYPE_USER));
}

int kthread_run(char *name, int (*fn)(void *arg), void *arg);
int kernel_exec(char *filename);
int kill_task(struct task_struct *task);
int task_kill_self(struct task_struct *task);

static inline pt_regs *get_pt_regs(void)
{
	return arch_get_pt_regs();
}

int mmap(struct task_struct *task, unsigned long start,
		unsigned long virt, int flags,
		int fd, offset_t offset);
int munmap(struct task_struct *task, unsigned long start, size_t length, int flags, int sync);
unsigned long get_mmap_user_base(struct task_struct *task, int page_nr);

#endif
