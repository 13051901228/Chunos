#ifndef _SCHED_H_
#define _SCHED_H_

#include <os/task.h>

#define TASK_MAX_PRIO		64
#define MAX_PRIO_SHIFT		4
#define TASK_DEFAULT_PRIO	20

extern struct task_struct *current;
extern struct task_struct *idle;
extern struct task_struct *next_run;

void set_task_state(struct task_struct *task, state_t state);

state_t get_task_state(struct task_struct *task);

int add_new_task(struct task_struct *task);

void sched(void);

pid_t get_new_pid(struct task_struct *task);

int suspend_task_timeout(struct task_struct *task, int timeout);

int wakeup_task(struct task_struct *task);

int sched_remove_task(struct task_struct *task);

pid_t get_task_pid(struct task_struct *task);

int release_pid(struct task_struct *task);

struct task_struct *pid_get_task(pid_t pid);

#define suspend_task(task)	suspend_task_timeout(task, -1);
#define suspend()		suspend_task(current);

#endif
