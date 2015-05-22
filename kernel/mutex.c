#include <os/mutex.h>
#include <os/types.h>
#include <os/sched.h>
#include <os/printk.h>
#include <os/irq.h>
#include <os/list.h>
#include <os/errno.h>

int mutex_lock_timeout(struct mutex *m, int ms)
{
	struct task_struct *task = current;
	unsigned long flags;
	int timeout = 0;

	if (in_interrupt) {
		kernel_info("waring: maybe sleep \
				in interrupt\n");
	}

	if (!task)
		return 0;

	/*
	 * frist we check wether this mutex has
	 * been locked, if yes, put this task to the list of
	 * mutex wait. if not change the mutex count value to 1
	 * the exist.
	 */

	for (;;) {
		enter_critical(&flags);
		if (m->count) {
			/*
			 * if mutex has been locked, then suspend this task
			 * if task suspend timeout, del it form the list.
			 */
			list_add_tail(&m->wait, &task->wait);
			exit_critical(&flags);
			timeout = suspend_task_timeout(task, ms);
			if (timeout) {
				list_del(&task->wait);
				return -ETIMEDOUT;
			}
		}
		else {
			m->count = 1;
			list_add_tail(&task->mutex_get, &m->task);
			exit_critical(&flags);
			break;
		}
	}

	return 0;
}

void mutex_unlock(struct mutex *m)
{
	struct list_head *list;
	struct task_struct *task;
	unsigned long flags;

	/*
	 * set m->count to 0, and wake up all the task
	 * which wait for this mutex.
	 */
	if (!m->count) {
		kernel_debug("mutex has not been locked\n");
		return;
	}

	enter_critical(&flags);
	m->count = 0;

	/*
	 * wake up all the process whick wait for the mutex,
	 * if we enable interrupt now. if there some many task
	 * wait for this mutex, it may cause some task enter sleep
	 * state agin quickly since the first wake up task will obtain
	 * the mutex.
	 */
	list_for_each ((&m->wait), list) {
		task = list_entry(list,struct task_struct, wait);
		list_del(&task->wait);
		wakeup_task(task);
	}

	if (!is_list_empty(&m->task)) {
		list_del(&m->task);
	}

	exit_critical(&flags);
}

