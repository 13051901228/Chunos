#ifndef _SLAB_H
#define _SLAB_H

void *kmalloc(int size, unsigned long flag);
void kfree(void *addr);
void *kzalloc(int size, unsigned long flag);

#endif
