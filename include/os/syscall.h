#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYSCALL_NR	1024

#include <os/syscall_nr.h>

struct syscall {
	int nr;
	unsigned long *addr;
};

#define DEFINE_SYSCALL(name, n, syscall_addr)	\
static struct syscall __syscall_##name __attribute__((section(".__syscall"))) = {	\
	.nr = n,	\
	.addr = syscall_addr	\
}

#endif
