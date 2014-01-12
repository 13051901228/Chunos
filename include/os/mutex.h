#ifndef _MUTEX_H
#define _MUTEX_H

#include <os/list.h>

struct mutex {
	int count;
	struct list_head wait;	/* tasks which wait for this mutex */
	struct list_head task;	/* the owner of this mutex */
};

#define DECLARE_MUTEX(name)			\
	struct mutex name = {			\
		.count = 0,			\
		.wait = {			\
			.pre = &name.wait,	\
			.next = &name.wait,	\
		},				\
		.task =	{			\
			.pre = &name.task,	\
			.next = &name.task,	\
		}				\
	}

static void inline init_mutex(struct mutex *m)
{
	m->count = 0;
	m->wait.pre = &m->wait;
	m->wait.next = &m->wait;
	m->task.pre = &m->task;
	m->task.next = &m->task;
}

int mutex_lock_timeout(struct mutex *m, int ms);
void mutex_unlock(struct mutex *m);

#define mutex_lock(m)	mutex_lock_timeout(m, -1)

#endif
