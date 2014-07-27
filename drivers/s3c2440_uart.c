/*
 * driver/s3c2440_uart.c
 *
 * Create by Le Min at 07/26/2014
 *
 */

#include <os/device.h>
#include <os/tty.h>
#include <os/kernel.h>
#include <os/io.h>

#define UART_BASE	0x50000000
#define ULCON0		0x00		//UART 0 Line contol
#define UCON0		0x04		//UART 0 Contol
#define UFCON0		0x08		//UART 0 FIFO contol
#define UMCON0		0x0c		//UART 0 Modem contol
#define UTRSTAT0	0x10		//UART 0 Tx/Rx status
#define UERSTAT0	0x14		//UART 0 Rx eo status
#define UFSTAT0		0x18		//UART 0 FIFO status
#define UMSTAT0		0x1c		//UART 0 Modem status
#define UTXH0		0x20		//UART 0 Tansmission Hold
#define URXH0		0x24		//UART 0 Receive buffe
#define UBRDIV0		0x28		//UART 0 Baud ate diviso

#define ULCON1		0x4000		//UART 1 Line contol
#define UCON1		0x4004		//UART 1 Contol
#define UFCON1		0x4008		//UART 1 FIFO contol
#define UMCON1		0x400c		//UART 1 Modem contol
#define UTRSTAT1	0x4010		//UART 1 Tx/Rx status
#define UERSTAT1	0x4014		//UART 1 Rx eo status
#define UFSTAT1		0x4018		//UART 1 FIFO status
#define UMSTAT1		0x401c		//UART 1 Modem status
#define UTXH1		0x4020		//UART 1 Tansmission Hold
#define URXH1		0x4024		//UART 1 Receive buffe
#define UBRDIV1		0x4028		//UART 1 Baud ate diviso

#define ULCON2		0x8000		//UART 2 Line contol
#define UCON2		0x8004		//UART 2 Contol
#define UFCON2		0x8008		//UART 2 FIFO contol
#define UMCON2		0x800c		//UART 2 Modem contol
#define UTRSTAT2	0x8010		//UART 2 Tx/Rx status
#define UERSTAT2	0x8014		//UART 2 Rx eo status
#define UFSTAT2		0x8018		//UART 2 FIFO status
#define UMSTAT2		0x801c		//UART 2 Modem status
#define UTXH2		0x8020		//UART 2 Tansmission Hold
#define URXH2		0x8024		//UART 2 Receive buffe
#define UBRDIV2		0x8028		//UART 2 Baud ate diviso

#define ULCON		0x00		//UART  Line contol
#define UCON		0x04		//UART  Contol
#define UFCON		0x08		//UART  FIFO contol
#define UMCON		0x0c		//UART  Modem contol
#define UTRSTAT		0x10		//UART  Tx/Rx status
#define UERSTAT		0x14		//UART  Rx eo status
#define UFSTAT		0x18		//UART  FIFO status
#define UMSTAT		0x1c		//UART  Modem status
#define UTXH		0x20		//UART  Tansmission Hold
#define URXH		0x24		//UART  Receive buffe
#define UBRDIV		0x28		//UART  Baud ate diviso

struct s3c2440_uart {
	struct tty *tty;
	void *io_base;
	int nr;
};

static struct s3c2440_uart uart[3];
extern u32 PCLK;

int s3c2440_uart_open(struct tty *tty)
{
	struct s3c2440_uart *uart = tty->dev->pdata;

	if (uart == 0) {
		iowrite32(0x1, uart->io_base + UFCON0);
		iowrite32(0x0, uart->io_base + UMCON0);
		iowrite32(0x03, uart->io_base + ULCON0);
		iowrite32(0x345, uart->io_base + UCON0);
	} else {
		/* TBC */
	}

	return 0;
}

void s3c2440_uart_put_char(struct tty *tty, char ch)
{
	struct s3c2440_uart *uart = tty->dev->pdata;

	if (ch == '\n')
		iowrite8('\r', uart->io_base + UTXH);

	iowrite8(ch, uart->io_base + UTXH);
}

char s3c2440_uart_get_char(struct tty *tty)
{
	return 0;
}

int s3c2440_uart_set_baud(struct tty *tty, u32 baud)
{
	struct s3c2440_uart *uart = tty->dev->pdata;
	void *io_base = uart->io_base;

	u32 val = ((int)(PCLK/16. / baud + 0.5) -1);

	iowrite32(val, io_base + UBRDIV);

	return 0;
}

static struct tty_operations s3c2440_uart_ops = {
	.put_char = s3c2440_uart_put_char,
	.get_char = s3c2440_uart_get_char,
	.open 	  = s3c2440_uart_open,
	.set_baud = s3c2440_uart_set_baud,
};

static int s3c2440_uart_init(void)
{
	void *io_base;
	struct tty *tty;
	int i;

	io_base = request_io_mem(0x50000000);
	if (!io_base)
		return -ENODEV;

	for (i = 0; i < 3; i++) {
		tty = allocate_tty();
		if (!tty)
			return -ENOMEM;

		tty->tty_ops = &s3c2440_uart_ops;
		uart[i].io_base = io_base + i * 0x4000;
		uart[i].tty = tty;
		uart[i].nr = i;
		tty->dev->pdata = &uart[i];

		register_tty(tty);
	}
	
	return 0;
}

device_initcall(s3c2440_uart_init);
