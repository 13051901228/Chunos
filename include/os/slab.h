/*
 * include/os/slab.h
 *
 * Created by Le Min (lemin9538@163.com)
 *
 */

#ifndef _SLAB_H
#define _SLAB_H

void *kmalloc(int size, int flag);
void kfree(void *addr);
void *kzalloc(int size, int flag);

#endif
