/*
 * arch/x86/platform/sofia/sofia.c
 *
 * Created by Le Min at 2014/12/27
 */

#include "include/uart.h"
#include <os/types.h>
#include <os/kernel.h>
#include <os/init.h>
#include <os/irq.h>

int soc_console_early_init(u32 baud)
{
	sofia_init_uart_usif(baud);
	return 0;
}

int soc_console_late_init(u32 baud)
{
	return 0;
}

u32 sofia_clk_init(void)
{
	return 0;
}

static init_call_t sofia_init_action[] = {
	NULL
};

int sofia_timer_init(unsigned int hz)
{
	return 0;
}

void sofia_parse_memory_info(void)
{
}

extern struct irq_chip ioapic;
extern struct mmu x86_mmu;

static __init_data struct soc_platform sofia = {
	.platform_id		= 0x636,
	.system_clk_init	= sofia_clk_init,
	.system_timer_init	= sofia_timer_init,
	.irq_chip		= &ioapic,
	.mmu			= &x86_mmu,
	.init_action		= sofia_init_action,
	.parse_memory_info	= sofia_parse_memory_info,
};
DEFINE_SOC_PLATFORM(sofia);
