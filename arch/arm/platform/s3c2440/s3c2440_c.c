#include <os/io.h>
#include <os/types.h>
#include <os/string.h>
#include <os/irq.h>
#include <os/io.h>
#include <os/init.h>
#include <os/errno.h>
#include <os/printk.h>
#include "include/s3c2440_reg.h"
#include "include/s3c2440_irq.h"
#include <os/soc.h>
#include <os/soc_id.h>

extern int os_tick_handler(void *arg);

static void *irq_base = NULL;
static void *timer_base = NULL;
static void *clk_base = NULL;
static void *uart_base = NULL;

/* the clock of the system */
u32 HCLK, FCLK, PCLK, UCLK;	

/* code for s3c2440 clock management */
u32 s3c2440_clk_init(void)
{
	u32 fin = 12000000;
	u32 tmp, m, s, p;

	clk_base = request_io_mem(CLK_BASE);
	if (!clk_base)
		return -EIO;

	tmp = ioread32(clk_base + MPLLCON);
	m = (tmp >> 12) & 0xff;
	p = (tmp >> 4) & 0x3f;
	s = tmp & 0x03;
	
	FCLK = ((m + 8)*(fin / 100) * 2) / ((p + 2) * (1 << s)) * 100;

	tmp = ioread32(clk_base + CLKDIVN);
	m = (tmp >> 1) & 0x03;
	p = tmp & 0x01;
	tmp = ioread32(clk_base + CAMDIVN);
	s = tmp >> 8;

	switch (m) {
		case 0:
			HCLK = FCLK;
			break;
		case 1:
			HCLK = FCLK >> 1;
			break;
		case 2:
		{
			if (s & 2) {
				HCLK = FCLK >> 3;
			}
			else {
				HCLK = FCLK >> 2;
			}
			break;
		}
		case 3:
		{
			if (s & 0x01) {
				HCLK = FCLK / 6;
			} else {
				HCLK = FCLK / 3;
			}
			break;
		}
	}

	if (p)
		PCLK = HCLK >> 1;
	else
		PCLK = HCLK;

	tmp = ioread32(clk_base + UPLLCON);
	m = (tmp >> 12) & 0xff;
	p = (tmp >> 4) & 0x3f;
	s = tmp & 0x03;

	UCLK = ((m + 8) * fin) / ((p + 2) * (1 << s));

	return FCLK;
}

int s3c2440_system_timer_init(u32 hz)
{
	timer_base = request_io_mem(TIMER_BASE);
	if (timer_base == NULL) {
		kernel_error("timer tick init failed\n");
		return -ENOMEM;
	}
	
	iowrite32(timer_base + TCFG0, 249 << 8);
	iowrite32(timer_base+TCFG1, 0 << 16);
	iowrite16(timer_base +TCNTB4, 64536);
	iowrite32(timer_base + TCON, 1 << 21);
	iowrite32(timer_base + TCON, bit(20) | bit(22));

	return 0;
}

/* code for s3c2440 irq handler help function */
int s3c2440_get_irq_nr(void)
{
	return ioread32(irq_base + INTOFFSET);
}

int s3c2440_irq_init(struct irq_chip *chip)
{
	irq_base = request_io_mem(IRQ_BASE);
	if(irq_base == NULL){
		kernel_error("request 0x%x failed\n",IRQ_BASE);
		return -ENOMEM;
	}

	/* mask all interrupt,and set all int to irq mode */
	iowrite32(irq_base + INTMSK, 0xffffffff);
	iowrite32(irq_base + INTSUBMSK, 0xffffffff);
	iowrite32(irq_base + INTMOD, 0);

	return 0;
}

int s3c2440_do_enable_irq(int nr, int enable)
{
	u32 int_msk = ioread32(irq_base + INTMSK);
	u32 int_submsk = ioread32(irq_base + INTSUBMSK);

	if (nr < 0 || nr > 48)
		return -EINVAL;

	/* get the value of submsk */
	switch (nr) {
	case WDT_AC97:
	case UART0:
	case UART1:
	case UART2:
	case ADC:
	case CAM:
		kernel_error("please spcific the subirq\n");
		return -EINVAL;
	default:
		break;
	}

	if (enable) {
		if (nr > 31 && nr < 48)
			int_submsk &= ~bit(nr - 32);
		int_msk &= (~bit(nr));
	} else {
		if (nr > 31 && nr < 48)
			int_submsk |= bit(nr - 32);
		int_msk |= bit(nr);
	}

	iowrite32(irq_base + INTMSK, int_mask);
	iowrite32(irq_base + INTSUBMSK, int_submask);
	return 0;
}

int s3c2440_disable_irq(int nr)
{
	return s3c2440_do_enable_irq(nr, 0);
}

int s3c2440_enable_irq(int nr, u32 flags)
{
	return s3c2440_do_enable_irq(nr, 1);
}

int s3c2440_clean_irq_pending(int nr)
{
	unsigned long src_pnd = 0;
	unsigned long subsrc_pnd = 0;

	/* TBC */
	if (nr > 31) {
		switch (nr) {
		case UART0_RXD0:
		case UART0_TXD0:
		case UART0_ERR0:
			src_pnd |= bit(UART0);
			break;

		case UART1_RXD1:
		case UART1_TXD1:
		case UART1_ERR1:
			src_pnd |= bit(UART1);
			break;

		case UART2_RXD2:
		case UART2_TXD2:
		case UART2_ERR2:
			src_pnd |= bit(UART2);
			break;

		/* TBC for other subirq */
		default:
			break;
		}

		subsrc_pnd |= bit(nr - 32);
	} else {
		src_pnd |= bit(nr);
	}

	iowrite32(subsrc_pnirq_base + SUBSRCPND, subsrc_pnd);
	iowrite32(irq_base + SRCPND, src_pnd);
	iowrite32(irq_base + INTPND, src_pnd);

	return 0;
}

int s3c2440_uart0_init(int baud)
{
	u32 val = ((int)(PCLK /16. / baud + 0.5) - 1);

	iowrite32(uart_base + UFCON0, 0x1);
	iowrite32(uart_base + UMCON0, 0x0);
	iowrite32(uart_base + ULCON0, 0x03);
	iowrite32(uart_base + UCON0, 0x345);
	iowrite32(uart_base + UBRDIV0, val);

	return 0;
}

void platform_uart0_send_byte(u16 ch)
{
	if (ch == '\n') {
//		while(!(ioread32(uart_base + UTRSTAT0) & 0x02));
		iowrite8(uart_base + UTXH0, '\r');
	}

//	while(!(ioread32(uart_base + UTRSTAT0) & 0x02));
	iowrite8(uart_base + UTXH0, ch);
}

int soc_console_early_init(int baud)
{
	/*
	 * because at boot stage mmu has not enable
	 * we can use the plat mode address to write
	 * value to the address. mmu muset set correctly
	 * when booting
	 */
	uart_base = (void *)UART_BASE;
	s3c2440_uart0_init(baud);

	return 0;
}

void uart_puts(char *buf)
{
	while (*buf)
		platform_uart0_send_byte(*buf++);
}

int uart_put_char(char *buf, int size)
{
	int i;

	for (i = 0; i < size; i++)
		platform_uart0_send_byte(buf[i]);

	return size;
}

static struct irq_chip s3c2440_irq_chip = {
	.enable_irq		= s3c2440_enable_irq,
	.get_irq_nr		= s3c2440_get_irq_nr,
	.disable_irq		= s3c2440_disable_irq,
	.clean_irq_pending	= s3c2440_clean_irq_pending,
	.irq_init		= s3c2440_irq_init
};

static __init_data init_call s3c2440_init_action[] = {
	NULL,
};

static __init_data struct soc_platform s3c2440 = {
	.platform_id		= PLATFORM_S3C2440,
	.system_clk_init	= s3c2440_clk_init,
	.system_timer_init	= s3c2440_system_timer_init,
	.irq_chip		= &s3c2440_irq_chip,
	.init_action		= s3c2440_init_action,
};
DEFINE_SOC_PLATFORM(s3c2440);
