#ifndef _S3C244O_REG_H
#define _S3C2440_REG_H

// PWM Timer
#define TIMER_BASE	0x51000000
#define TCFG0   0x00	//Time 0 configuation
#define TCFG1   0x04	//Time 1 configuation
#define TCON    0x08	//Time contol
#define TCNTB0  0x0c	//Time count buffe 0
#define TCMPB0  0x10	//Time compae buffe 0
#define TCNTO0  0x14	//Time count obsevation 0
#define TCNTB1  0x18	//Time count buffe 1
#define TCMPB1  0x1c	//Time compae buffe 1
#define TCNTO1  0x20	//Time count obsevation 1
#define TCNTB2  0x24	//Time count buffe 2
#define TCMPB2  0x28	//Time compae buffe 2
#define TCNTO2  0x2c	//Time count obsevation 2
#define TCNTB3  0x30	//Time count buffe 3
#define TCMPB3  0x34	//Time compae buffe 3
#define TCNTO3  0x38	//Time count obsevation 3
#define TCNTB4  0x3c	//Time count buffe 4
#define TCNTO4  0x40	//Time count obsevation 4

//interrupt registers
#define IRQ_BASE    0x4a000000
#define SRCPND      0x0		//Inteupt equest status
#define INTMOD      0x04	//Inteupt mode contol
#define INTMSK      0x08	//Inteupt mask contol
#define PRIORITY    0x0c	//IRQ pioity contol
#define INTPND      0x10	//Inteupt equest status
#define INTOFFSET   0x14	//Inteuot equest souce offset
#define SUBSRCPND   0x18	//Sub souce pending
#define INTSUBMSK   0x1c	//Inteupt sub mask

//clk register
#define CLK_BASE	0x4c000000
#define LOCKTIME	0X00
#define MPLLCON		0x04
#define UPLLCON		0x08
#define CLKCON		0x0c
#define CLKSLOW		0x10
#define CLKDIVN		0x14
#define CAMDIVN		0x18

//uart
// UART
#define UART_BASE	0x50000000
#define ULCON0      0x00	//UART 0 Line contol
#define UCON0       0x04	//UART 0 Contol
#define UFCON0      0x08	//UART 0 FIFO contol
#define UMCON0      0x0c	//UART 0 Modem contol
#define UTRSTAT0    0x10	//UART 0 Tx/Rx status
#define UERSTAT0    0x14	//UART 0 Rx eo status
#define UFSTAT0     0x18	//UART 0 FIFO status
#define UMSTAT0     0x1c	//UART 0 Modem status
#define UBRDIV0     0x28	//UART 0 Baud ate diviso
#define ULCON1      0x4000	//UART 1 Line contol
#define UCON1       0x4004	//UART 1 Contol
#define UFCON1      0x4008	//UART 1 FIFO contol
#define UMCON1      0x400c	//UART 1 Modem contol
#define UTRSTAT1    0x4010	//UART 1 Tx/Rx status
#define UERSTAT1    0x4014	//UART 1 Rx eo status
#define UFSTAT1     0x4018	//UART 1 FIFO status
#define UMSTAT1     0x401c	//UART 1 Modem status
#define UBRDIV1     0x4028	//UART 1 Baud ate diviso
#define ULCON2      0x8000	//UART 2 Line contol
#define UCON2       0x8004	//UART 2 Contol
#define UFCON2      0x8008	//UART 2 FIFO contol
#define UMCON2      0x800c	//UART 2 Modem contol
#define UTRSTAT2    0x8010	//UART 2 Tx/Rx status
#define UERSTAT2    0x8014	//UART 2 Rx eo status
#define UFSTAT2     0x8018	//UART 2 FIFO status
#define UMSTAT2     0x801c	//UART 2 Modem status
#define UBRDIV2     0x8028	//UART 2 Baud ate diviso
#define UTXH0   0x20	//UART 0 Tansmission Hold
#define URXH0   0x24	//UART 0 Receive buffe
#define UTXH1   0x4020	//UART 1 Tansmission Hold
#define URXH1   0x4024	//UART 1 Receive buffe
#define UTXH2   0x8020	//UART 2 Tansmission Hold
#define URXH2   0x8024	//UART 2 Receive buffe

#endif
