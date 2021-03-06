#include "include/uart.h"
#include <os/types.h>
#include <os/kernel.h>
#include <os/init.h>
#include <os/irq.h>
#include <os/io.h>

/* boot uart device use usif 1 */
static unsigned long usif1_base;

#define USIF_CLC		0x0
#define USIF_CLC_CNT		0x4
#define USIF_CLC_STAT		0x8
#define USIF_ID			0xC
#define USIF_FIFO_ID		0x10
#define USIF_SRB_MSCONF_ID	0x14
#define USIF_SRB_ERRCONF_ID	0x18
#define USIF_SWCID		0x1C
#define USIF_DPLUS_CTRL		0x2C
#define USIF_FIFO_CFG		0x30
#define USIF_FIFO_CTRL		0x34
#define USIF_MRPS_CTRL		0x38
#define USIF_RPS_STAT		0x3C
#define USIF_TPS_CTRL		0x40
#define USIF_FIFO_STAT		0x44
#define USIF_TXD_SB		0x48
#define USIF_DPLUS_STAT		0x4C
#define USIF_RIS		0x80
#define USIF_IMSC		0x84
#define USIF_MIS		0x88
#define USIF_ISR		0x90
#define USIF_DMAE		0x94
#define USIF_ICR		0x98
#define USIF_MSS_SET		0x100
#define USIF_MSS_CLR		0x104
#define USIF_MSS_STAT		0x108
#define USIF_MSS_CTRL		0x10C
#define USIF_MODE_CFG		0x110
#define USIF_PRTC_CFG		0x114
#define USIF_PRTC_STAT		0x118
#define USIF_FRM_CTRL		0x11C
#define USIF_FRM_STAT		0x120
#define USIF_CRC_CFG		0x124
#define USIF_CRCPOLY_CFG	0x128
#define USIF_CRC_CTRL		0x12C
#define USIF_CS_CFG		0x130
#define USIF_FDIV_CFG		0x140
#define USIF_BC_CFG		0x144
#define USIF_ICTMO_CFG		0x148
#define USIF_ICTM_CFG		0x14C
#define USIF_ECTM_CFG		0x150
#define USIF_CD_TIM0_CFG	0x154
#define USIF_CD_TIM1_CFG	0x158
#define USIF_CD_TIM2_CFG	0x15C
#define USIF_CD_TIM3_CFG	0x160
#define USIF_CD_TIM4_CFG	0x164
#define USIF_CD_TIM5_CFG	0x168
#define USIF_CD_TIM6_CFG	0x16C
#define USIF_CD_TIM7_CFG	0x170
#define USIF_TXD		0x40000
#define USIF_RXD		0x80000

static inline void early_xgold_usif_putc(char s)
{
	while ((ioread32(USIF_FIFO_STAT + usif1_base) & 0xff0000 ) >> 0x10);

	iowrite32(USIF_FIFO_CTRL + usif1_base, 1);
	iowrite32(USIF_TXD + usif1_base, s);
}

void xgold_uart_puts(char *str)
{
	int i;
	unsigned long flag;

	enter_critical(&flag);

	for (i = 0; *str; i++) {
		if (*str == '\n')
			early_xgold_usif_putc('\r');
		early_xgold_usif_putc(*str);
		str++;
	}

	exit_critical(&flag);
}

int __init_text sofia_init_uart_usif(unsigned long base, u32 baud)
{
	usif1_base = iomap_to_addr(0xe1100000, base);
	if (!usif1_base)
		return -ENOMEM;

	iowrite32(usif1_base + USIF_CLC, 0xaa6);
	iowrite32(usif1_base + USIF_CLC_CNT, 0x01);
	iowrite32(usif1_base + USIF_MODE_CFG, 0x10806);
	iowrite32(usif1_base + USIF_PRTC_CFG, 0x8);
	iowrite32(usif1_base + USIF_IMSC, 0x0);
	iowrite32(usif1_base + USIF_ICR, 0xffffffff);
	iowrite32(usif1_base + USIF_BC_CFG, 0x5);
	iowrite32(usif1_base + USIF_FDIV_CFG, 0x1d00037);
	iowrite32(usif1_base + USIF_FIFO_CFG, 0x2200);
	iowrite32(usif1_base + USIF_CLC, 0xaa5);

#if 0
	iowrite32(0x0, usif1_base + USIF_DMAE);
	iowrite32(0x00000001, usif1_base + USIF_CLC_CNT);
	iowrite32(0x0, usif1_base + USIF_CS_CFG);
	iowrite32(0x0, usif1_base + USIF_CRC_CFG);
	iowrite32(0x0, usif1_base + USIF_CRCPOLY_CFG);
	iowrite32(0x0000003a, usif1_base + USIF_ICTMO_CFG);
	iowrite32(0x0, usif1_base + USIF_TPS_CTRL);
	iowrite32(0x11, usif1_base + USIF_FIFO_CTRL);

	/* enable rx interrupt */
	/* iowrite32(0xa, usif1_base + USIF_IMSC); */
#endif
	register_early_printk(xgold_uart_puts);

	return 0;
}

void __init_text sofia_deinit_uart_usif(void)
{
	iounmap(usif1_base);
	unregister_early_printk();
}
