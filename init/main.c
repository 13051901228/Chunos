/*
 * init/main.c
 *
 * Created by Le Min(lemin9538@163.com)
 */

#include <os/types.h>
#include <os/mm.h>
#include <os/init.h>
#include <os/string.h>
#include <os/mirros.h>
#include <os/slab.h>
#include <os/printk.h>
#include <os/panic.h>
#include <os/task.h>
#include <os/sched.h>
#include <os/irq.h>
#include <os/soc.h>

extern int mm_init(void);
extern int slab_init(void);
extern int sched_init(void);
extern int timer_tick_init(void);
extern int build_idle_task(void);
extern int init_task();
extern unsigned long mount_ramdisk(void);
extern int syscall_init(void);
extern int system_killer(void *arg);
extern int irq_init(void);
extern int soc_late_init(void);
extern int soc_early_init(void);
extern int arch_init(void);
extern int log_buffer_init(void);
extern int mmu_init(void);

extern unsigned long init_call_start;
extern unsigned long init_call_end;

int cpu_idle(void)
{
	kernel_info("in idle\n");
	return 0;
}

int hello(void)
{
	printk("hello world\n");
}

int main(void)
{
	int i = 0;
	init_call *fn;
	int size;

	/*
	 * this function can let us use printk before 
	 * kernel mmu page table build
	 */
	disable_irqs();
	arch_early_init();
	log_buffer_init();
	console_early_init();
	soc_early_init();

	/*
	 * after kernel mmu talbe build,we need init the
	 * console again to use the uart0,or we can implement
	 * a more stronger printk system if needed.
	 */
	mmu_init();
	mm_init();
	console_late_init();
	slab_init();

	arch_init();
	soc_clock_init();
	irq_init();
	timer_tick_init();
	soc_late_init();
	syscall_init();
	sched_init();

	size = (init_call_end - init_call_start) / sizeof(unsigned long);
	fn = (init_call *)init_call_start;
	for (i = 0; i < size; i++ ) {
		(*fn)();
		fn++;
	}

	build_idle_task();

	/* mount root filesystem */
	mount_init();

	/* now we can enable irq */
	enable_irqs();
	__asm("int $20");

	kthread_run("system_killer", system_killer, NULL);
	//kthread_run("hello", hello, NULL);

	//init_task();

	for (;;) {
		cpu_idle();
		sched();
	}

	return 0;
}
