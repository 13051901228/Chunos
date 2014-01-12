#ifndef _IO_H
#define _IO_H

#include <os/types.h>

static inline u8 __raw_readb(const volatile void *addr)
{
	return *(const volatile u8 *)addr;
}

static inline u16 __raw_readw(const volatile void *addr)
{
	return *(const volatile u16 *)addr;
}

static inline u32 __raw_readl(const volatile void *addr)
{
	return *(const volatile u32 *)addr;
}

static void __raw_writeb(u8 b,volatile void *addr)
{
	*(volatile u8 *)addr = b;
}

static void __raw_writew(u16 w,volatile void *addr)
{
	*(volatile u16 *)addr = w;
}

static void __raw_writel(u32 l,volatile void *addr)
{
	*(volatile u32 *)addr = l;
}

#define readb	__raw_readb
#define readw	__raw_readw
#define readl	__raw_readl

#define writeb	__raw_writeb
#define writew	__raw_writew
#define writel  __raw_writel

static inline u8 inb(unsigned long addr)
{
	return readb((volatile u8 *)addr);
}

static inline u16 inw(unsigned long addr)
{
	return readw((volatile u16 *) addr);
}

static inline u32 inl(unsigned long addr)
{
	return readl((volatile u32 *)addr);
}

static inline void outb(u8 b, unsigned long addr)
{
	writeb(b,(volatile u8 *)addr);
}

static inline void outw(u16 w, unsigned long addr)
{
	writew(w,(volatile u16 *)addr);
}

static inline void outl(u32 l, unsigned long addr)
{
	writel(l,(volatile u32 *)addr);
}

/*
 *read or write on data to a fix addr,usually used to read data 
 *from fifo
 */
static inline void insb(unsigned long addr, void *buffer, int count)
{
	if (count) {
		u8 *buf = buffer;
		do {
			u8 x = inb(addr);
			*buf++ = x;
		} while (--count);
	}
}

static inline void insw(unsigned long addr, void *buffer, int count)
{
	if (count) {
		u16 *buf = buffer;
		do {
			u16 x = inw(addr);
			*buf++ = x;
		} while (--count);
	}
}

static inline void insl(unsigned long addr, void *buffer, int count)
{
	if (count) {
		u32 *buf = buffer;
		do {
			u32 x = inl(addr);
			*buf++ = x;
		} while (--count);
	}
}

static inline void outsb(unsigned long addr, const void *buffer, int count)
{
	if (count) {
		const u8 *buf = buffer;
		do {
			outb(*buf++,addr);
		} while (--count);
	}
}

static inline void outsw(unsigned long addr, const void *buffer, int count)
{
	if (count) {
		const u16 *buf = buffer;
		do {
			outw(*buf++,addr);
		} while (--count);
	}
}

static inline void outsl(unsigned long addr, const void *buffer, int count)
{
	if (count) {
		const u32 *buf = buffer;
		do {
			outl(*buf++,addr);
		} while (--count);
	}
}

#define ioread8(addr)			readb(addr)
#define ioread16(addr)			readw(addr)
#define ioread32(addr)			readl(addr)

#define iowrite8(v, addr)		writeb((v),(addr))
#define iowrite16(v, addr)		writew((v),(addr))
#define iowrite32(v, addr)		writel((v),(addr))

#define ioread8_rep(p, dst, count)	insb((unsigned long)(p), (dst), (count))
#define ioread16_rep(p, dst, count)	insw((unsigned long)(p), (dst), (count))
#define ioread32_rep(p, dst, count)	insl((unsigned long)(p), (dst), (count))

#define iowrite8_rep(p, src, count)	outsb((unsigned long)(p), (src), (count))
#define iowrite16_rep(p, src, count)	outsw((unsigned long)(p), (src), (count))
#define iowrite32_rep(p, src, count)	outsl((unsigned long)(p), (src), (count))

void *request_io_mem(unsigned long addr);

#endif
