#ifndef _S3C2440_IRQ_H
#define _S3C2440_IRQ_H

/*
 *interrupt id
 */
#define EINT0           0
#define EINT1           1
#define EINT2           2
#define EINT3           3
#define EINT4_7         4
#define EINT8_23        5
#define CAM             6
#define BAT_FLT         7
#define TICK            8
#define WDT_AC97        9 
#define TIMER0          10
#define TIMER1          11
#define TIMER2          12
#define TIMER3          13
#define TIMER4          14
#define UART2           15
#define LCD             16
#define DMA0            17
#define DMA1            18
#define DMA2            19
#define DMA3            20
#define SDI             21
#define SPI0            22
#define UART1           23
#define NFCON           24
#define USBD            25
#define USBH            26
#define IIC             27
#define UART0           28
#define SPI1            29
#define RTC             30
#define ADC             31

//define submark of the interrupt
#define AC_97           14
#define WDT             13
#define CAM_P           12
#define CAM_C           11
#define ADC_S           10
#define TC              9
#define ERR2            8
#define TXD2            7
#define RXD2            6
#define ERR1            5       
#define TXD1            4
#define RXD1            3
#define ERR0            2
#define TXD0            1
#define RXD0            0
#define NO_SUBMASK      15

#endif
