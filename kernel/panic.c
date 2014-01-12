#include <os/printk.h>
#include <os/types.h>

void panic(char *str)
{
	kernel_error("-----PANIC-----\n");
	kernel_error("%s\n",str);

	while(1);
}
