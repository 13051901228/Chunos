/*
 * arch/x86/kernel/lapic.c
 */

#include <os/kernel.h>

#define LAPIC_PADDR_BASE		0xfee00000

#define LAPIC_IDR			(LAPIC_PADDR_BASE + 0x20)
#define LAPIC_VER			(LAPIC_PADDR_BASE + 0x30)
#define LAPIC_TPR			(LAPIC_PADDR_BASE + 0x80)
#define LAPIC_PPR			(LAPIC_PADDR_BASE + 0xA0)
#define LAPIC_EOI			(LAPIC_PADDR_BASE + 0xB0)
#define LAPIC_LDR			(LAPIC_PADDR_BASE + 0xD0)
#define LAPIC_SVR			(LAPIC_PADDR_BASE + 0xF0)
#define LAPIC_DFR			(LAPIC_PADDR_BASE + 0xE0)
#define LAPIC_ISR			(LAPIC_PADDR_BASE + 0x100)
#define LAPIC_TMR			(LAPIC_PADDR_BASE + 0x180)
#define LAPIC_IRR			(LAPIC_PADDR_BASE + 0x200)
#define LAPIC_ESR			(LAPIC_PADDR_BASE + 0x280)
#define LAPIC_LVT_CMCI			(LAPIC_PADDR_BASE + 0x2F0)
#define LAPIC_INT_CMDL_REG		(LAPIC_PADDR_BASE + 0x300)
#define LAPIC_INT_CMDH_REG		(LAPIC_PADDR_BASE + 0x310)
#define LAPIC_INT_CMD_REG		(LAPIC_PADDR_BASE + LAPIC_INT_CMDL_REG)
#define LAPIC_LVT_TMR			(LAPIC_PADDR_BASE + 0x320)
#define LAPIC_LVT_THERMAL_MONITOR	(LAPIC_PADDR_BASE + 0x330)
#define LAPIC_LVT_PERF_COUNTER		(LAPIC_PADDR_BASE + 0x340)
#define LAPIC_LVT_LINT0			(LAPIC_PADDR_BASE + 0x350)
#define LAPIC_LVT_LINT1			(LAPIC_PADDR_BASE + 0x360)
#define LAPIC_LVT_ERROR			(LAPIC_PADDR_BASE + 0x370)
#define LAPIC_ICR			(LAPIC_PADDR_BASE + 0x380)
#define LAPIC_CCR			(LAPIC_PADDR_BASE + 0x390)
#define LAPIC_DCR			(LAPIC_PADDR_BASE + 0x3E0)
#define LAPIC_LINT0			(LAPIC_PADDR_BASE + 0x350)
#define LAPIC_LINT1			(LAPIC_PADDR_BASE + 0x360)

void inline x86_pic_eoi(unsigned int vector)
{
	iowrite32(LAPIC_EOI, 0xdeadbeef);
}

void x86_lapic_init(int cpu)
{
	int ret;

	if (cpu == 0) {
		ret = iomap_to_addr(LAPIC_PADDR_BASE, LAPIC_PADDR_BASE);
		if (ret)
			panic("can not map lapic address\n");
	}

	iowrite32(LAPIC_LVT_TMR, 0x10000);
	iowrite32(LAPIC_LVT_THERMAL_MONITOR, 0x10000);
	iowrite32(LAPIC_LVT_PERF_COUNTER, 0x10000);
	iowrite32(LAPIC_LVT_LINT0, 0x10000);
	iowrite32(LAPIC_LVT_LINT1, 0x10000);
	iowrite32(LAPIC_LVT_ERROR, 0x10000);
	iowrite32(LAPIC_LDR, 1 << (cpu + 24));
}
