/*
 * syscall/uname.c
 *
 * Create by Le Min at 08/23/2014
 */

#include <os/kernel.h>
#include <errno.h>
#include <os/printk.h>
#include <os/syscall.h>
#include <os/syscall_nr.h>
#include <sys/utsname.h>

static int sys_uname(struct utsname *uname)
{
	strcpy(uname->sysname, "chunos");
	strcpy(uname->nodename, "lemin-laptop");
	strcpy(uname->release, "chunos-dev");
	strcpy(uname->version, "v0.1");
	strcpy(uname->machine, "s3c2440-tq2440");
	strcpy(uname->domainname, "lemin");

	return 0;
}
DEFINE_SYSCALL(uname, __NR_uname, sys_uname);

