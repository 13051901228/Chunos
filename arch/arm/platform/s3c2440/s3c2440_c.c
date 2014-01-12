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

int s3c2440_system_timer_init(int hz)
{
	timer_base = request_io_mem(TIMER_BASE);
	if (timer_base == NULL) {
		kernel_error("timer tick init failed\n");
		return -ENOMEM;
	}
	
	iowrite32(249 << 8, timer_base + TCFG0);
	iowrite32(0 << 16, timer_base+TCFG1);
	iowrite16(64536, timer_base +TCNTB4);
	iowrite32(1<<21, timer_base + TCON);
	iowrite32(bit(20) | bit(22), timer_base + TCON);

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
	iowrite32(0xffffffff,irq_base + INTMSK);
	iowrite32(0xffffffff,irq_base + INTSUBMSK);
	iowrite32(0, irq_base + INTMOD);

	return 0;
}

int s3c2440_disable_irq(int nr)
{
	u32 int_msk = ioread32(irq_base + INTMSK);
	u32 int_submsk = ioread32(irq_base + INTSUBMSK);

	/* get the intmsk and int mode */
	int_msk |= bit(nr);

	/* get the value of submsk */
	switch (nr) {
		case WDT_AC97:
			int_submsk |= (bit(AC_97) | bit(WDT));
			break;
		case UART0:
			int_submsk |= (bit(RXD0) | bit(TXD0) | bit(ERR0));
			break;
		case UART1:
			int_submsk |= ((bit(RXD1)) | (bit(TXD1)) | (bit(ERR1)));
			break;
		case UART2:
			int_submsk |= (bit(RXD2) | bit(TXD2) | bit(ERR2));
			break;
		case ADC:
			int_submsk |= ((bit(ADC_S)) | (bit(TC)));
			break;
		case CAM:
			int_submsk |= ((bit(CAM_P)) | (bit(CAM_C)));
			break;
		default:
			break;
	}

	iowrite32(int_msk, irq_base + INTMSK);
	iowrite32(int_submsk, irq_base + INTSUBMSK);

	return 0;
}

int s3c2440_enable_irq(int nr, u32 flags)
{
	int int_msk = ioread32(irq_base + INTMSK);
	int int_submsk = ioread32(irq_base + INTSUBMSK);

	/* get the intmsk and int mode */
	int_msk &= ~bit(nr);

	/* get the value of submsk */
	switch (nr) {
		case WDT_AC97:
			int_submsk &= ((~bit(AC_97)) & (~bit(WDT)));
			break;
		case UART0:
			int_submsk &= ((~bit(RXD0)) & (~bit(TXD0)) & (~bit(ERR0)));
			break;
		case UART1:
			int_submsk &= ((~bit(RXD1)) & (~bit(TXD1)) & (~bit(ERR1)));
			break;
		case UART2:
			int_submsk &= ( (~bit(RXD2)) & (~bit(TXD2)) & (~bit(ERR2)));
			break;
		case ADC:
			int_submsk &= ((~bit(ADC_S)) & (~bit(TC)));
			break;
		case CAM:
			int_submsk &= ((~bit(CAM_P)) & (~bit(CAM_C)));
			break;
		default:
			break;
	}

	iowrite32(int_msk, irq_base + INTMSK);
	iowrite32(int_submsk, irq_base + INTSUBMSK);

	return 0;
}

int s3c2440_clean_irq_pending(int nr)
{
	iowrite32(ioread32(irq_base + SUBSRCPND), irq_base + SUBSRCPND);
	iowrite32(ioread32(irq_base + SRCPND), irq_base + SRCPND);
	iowrite32(ioread32(irq_base + INTPND), irq_base + INTPND);

	return 0;
}

int s3c2440_uart0_init(int baud)
{
	u32 val = ((int)(PCLK /16. / baud + 0.5) - 1);

	iowrite32(0x1, uart_base + UFCON0);
	iowrite32(0x0, uart_base + UMCON0);
	iowrite32(0x03, uart_base + ULCON0);
	iowrite32(0x345, uart_base + UCON0);
	iowrite32(val, uart_base + UBRDIV1);
	iowrite32(val, uart_base + UBRDIV0);

	return 0;
}

void platform_uart0_send_byte(u16 ch)
{
	if (ch == '\n') {
//		while(!(ioread32(uart_base + UTRSTAT0) & 0x02));
		iowrite8('\r',uart_base + UTXH0);
	}

//	while(!(ioread32(uart_base + UTRSTAT0) & 0x02));
	iowrite8(ch, uart_base + UTXH0);
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

int soc_console_late_init(int baud)
{
	/*
	 * in this function we can init other module if needed
	 * but first we need get clk_base and uart_base again
	 * since the mmu tlb has changed,and the io memory mapped
	 * in boot stage can not be used any longer;in this function
	 * we can not call printk.and clk_base and uart_base must get
	 * correct value.
	 */
	uart_base = request_io_mem(UART_BASE);

	return 0;
}

int uart_puts(char *buf)
{
	int size;

	size = strlen(buf);

	while (*buf)
		platform_uart0_send_byte(*buf++);
	
	return size;
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
