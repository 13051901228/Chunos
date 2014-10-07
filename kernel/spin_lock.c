/*
 * kernel/spin_lock.c
 *
 * Created by Le Min
 */

#include <os/spin_lock.h>
#include <os/kernel.h>

void spin_lock_init(spin_lock_t *lock)
{
#ifdef	CONFIG_SMP
	lock->value = 0;
	lock->cpu = -1;
#endif
	lock->flags = 0;
}

void spin_lock(spin_lock_t *lock)
{
	disable_irqs();
#ifdef CONFIG_SMP
	do {
		if (!lock->value) {
			lock->value = 1;
			lock->cpu = get_cpu_id();
			break;
		}
	} while (1);
#endif
}

void spin_unlock(spin_lock_t *lock)
{
#ifdef CONFIG_SMP
	lock->value = 0;
	lock->cpu = -1;
#endif
	enable_irqs();
}

void spin_lock_irqsave(spin_lock_t *lock)
{
	enter_critical(&lock->flags);
#ifdef CONFIG_SMP
	do {
		if (!lock->value) {
			lock->value = 1;
			lock->cpu = get_cpu_id();
			break;
		}
	} while (1);
#endif
}

void spin_unlock_irqstore(spin_lock_t *lock)
{
#ifdef CONFIG_SMP
	lock->value = 0;
	lock->cpu = -1;
#endif
	exit_critical(&lock->flags);
}
