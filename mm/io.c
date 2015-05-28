#include <os/mm.h>
#include <os/types.h>
#include <os/printk.h>
#include <os/errno.h>
#include <os/io.h>

/* TBC */
unsigned long iomap_to_addr(unsigned long io_phy, unsigned long vir)
{
	return build_kernel_pde_entry(vir, io_phy,
			PDE_ALIGN_SIZE, PDE_ATTR_IO_MEMORY);
}

unsigned long iomap(unsigned long io_phy)
{
	return 0;
}

int iounmap(unsigned long vir)
{
	return 0;
}
