/*
 * arch/x86/kernel/include/idt.h
 *
 * Created by Le Min at 2015
 */

#ifndef _GDT_H_
#define _GDT_H_

#define KERNEL_CS	(0x08)
#define KERNEL_DS	(0X10)
#define KERNEL_SS	(0x18)
#define KERNEL_ES	(0x20)
#define KERNEL_FS	(0x28)
#define KERNEL_GS	(0x30)
#define USER_CS		(0x38)
#define USER_DS		(0x40)
#define USER_SS		(0x48)
#define USER_ES		(0x50)
#define USER_FS		(0x58)
#define USER_GS		(0x60)
#define KERNEL_TSS	(0x68)

#endif
