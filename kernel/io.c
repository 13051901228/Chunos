#include <os/mm.h>
#include <os/types.h>
#include <os/printk.h>
#include <os/errno.h>
#include <os/io.h>

void *request_io_mem(unsigned long addr)
{
	return (void *)pa_to_va(addr);
}
