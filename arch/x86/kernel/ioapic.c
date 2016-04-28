/*
 * arch/x86/kernel/ioapic.c
 *
 * Created by Le Min at 2014/12/19
 */

#include <os/kernel.h>
#include <os/slab.h>
#include <os/irq.h>
#include <os/types.h>
#include <os/io.h>

#ifdef CONFIG_X86_SOFIA

#define IOAPIC_PADDR_BASE	0xfec00000
#define IOAPIC_RTE_START_OFFSET	0x1000
#define IOAPIC_RTE_SIZE		0x10

#define IOAPIC_RTE_LOW_ADDR(line) \
	(IOAPIC_RTE_START_OFFSET + IOAPIC_RTE_SIZE * line)
#define IOAPIC_RTE_HIGH_ADDR(line) \
	(IOAPIC_RTE_START_OFFSET + IOAPIC_RTE_SIZE * line + 0x04)

#define IOAPIC_RTE_DESTFIELD	56
#define IOAPIC_RTE_MASK		16
#define IOAPIC_RTE_TRIG_MODE	15
#define IOAPIC_RTE_RIRR		14
#define IOAPIC_RTE_INTPOL	13
#define IOAPIC_RTE_DELIVS	12
#define IOAPIC_RTE_DESTMOD	11
#define IOAPIC_RTE_DELMOD	8
#define IOAPIC_RTE_INTVEC	0

#define IOAPIC_RTE_REG(irq)	\
	(IOAPIC_RTE_START_OFFSET + irq * IOAPIC_RTE_SIZE)

static void inline set_ioapic_reg(uint32_t reg, uint32_t value)
{
	iowrite32(IOAPIC_PADDR_BASE + reg, value);
}

static inline uint32_t get_ioapic_reg(uint32_t reg)
{
	return ioread32(IOAPIC_PADDR_BASE + reg);
}

#else

#define IOAPIC_PADDR_BASE		(0xfec00000)
#define IOAPIC_RTE_START_OFFSET		(0x10)
#define IOAPIC_RTE_SIZE			(0X02)

#define IOAPIC_RTE_LOW_ADDR(irq) \
	(IOAPIC_RTE_START_OFFSET + IOAPIC_RTE_SIZE * irq)
#define IOAPIC_RTE_HIGH_ADDR(irq) \
	(IOAPIC_RTE_START_OFFSET + IOAPIC_RTE_SIZE * irq + 0x01)

#define IOAPIC_RTE_DESTFIELD	56
#define IOAPIC_RTE_MASK		16
#define IOAPIC_RTE_TRIG_MODE	15
#define IOAPIC_RTE_RIRR		14
#define IOAPIC_RTE_INTPOL	13
#define IOAPIC_RTE_DELIVS	12
#define IOAPIC_RTE_DESTMOD	11
#define IOAPIC_RTE_DELMOD	8
#define IOAPIC_RTE_INTVEC	0

#define IOAPIC_RTE_REG(irq)	\
	(IOAPIC_RTE_START_OFFSET + irq * IOAPIC_RTE_SIZE)

static void inline set_ioapic_reg(uint8_t offset, uint32_t val)
{
	iowrite32(IOAPIC_PADDR_BASE, offset);
	iowrite32(IOAPIC_PADDR_BASE + 0x10, val);
}

static uint32_t inline get_ioapic_reg(uint8_t offset)
{
	iowrite32(IOAPIC_PADDR_BASE, offset);
	return ioread32(IOAPIC_PADDR_BASE + 0x10);
}
#endif

static void x86_ioapic_init(struct irq_chip *chip)
{
	int i;

	for (i = 0; i < 256; i++)
		set_ioapic_reg(IOAPIC_RTE_LOW_ADDR(i), 0x10000);
}

static void x86_ioapic_unmask_irq(unsigned int irq)
{
	uint32_t value;

	value = get_ioapic_reg(IOAPIC_RTE_LOW_ADDR(irq));
	value &= ~(1 << 16);
	set_ioapic_reg(IOAPIC_RTE_LOW_ADDR(irq), value);
}

static int x86_ioapic_get_irq_nr(void)
{
	return 0;
}

static void x86_ioapic_mask_irq(unsigned int irq)
{
	uint32_t value;
	
	value = get_ioapic_reg(IOAPIC_RTE_LOW_ADDR(irq));
	value |= 1 << 16;
	set_ioapic_reg(IOAPIC_RTE_LOW_ADDR(irq), value);
}

static void x86_ioapic_eoi(unsigned int irq)
{

}

static void x86_ioapic_set_affinity(unsigned int irq, unsigned int affinity)
{
	unsigned int value;
	unsigned int masked;

	value = get_ioapic_reg(IOAPIC_RTE_REG(irq));
	masked = value & 0x10000;
	if (!masked)
		set_ioapic_reg(IOAPIC_RTE_REG(irq), value | (1 << 16));

	value &= ~0xf00;
	value |= 0x900;
	set_ioapic_reg(IOAPIC_RTE_HIGH_ADDR(irq), ((affinity & 0xff) << 24));
	set_ioapic_reg(IOAPIC_RTE_LOW_ADDR(irq), value);
}

static void x86_ioapic_setup(unsigned int irq,
		unsigned int vector, unsigned int affinity)
{
	set_ioapic_reg(IOAPIC_RTE_HIGH_ADDR(irq), ((affinity & 0xff) << 24));
	set_ioapic_reg(IOAPIC_RTE_LOW_ADDR(irq), 0x18900 | vector);
}

struct irq_chip x86_ioapic = {
	.irq_nr		= 256,
	.unmask		= x86_ioapic_unmask_irq,
	.get_irq_nr	= x86_ioapic_get_irq_nr,
	.mask		= x86_ioapic_mask_irq,
	.eoi		= x86_ioapic_eoi,
	.init		= x86_ioapic_init,
	.setup		= x86_ioapic_setup,
	.set_affinity	= x86_ioapic_set_affinity,
};
