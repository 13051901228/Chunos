#ifndef _BITS_OPS_H
#define _BITS_OPS_H

#include <os/types.h>
#include <os/printk.h>

typedef enum bit_ops {
	BITS_CLEAR,
	BITS_SET,
	BITS_READ
} bit_ops_t;

#define bits_of(type)	(sizeof(type) * 8)
#define bits_to_long(n)  (((n & 0xffffffc0) / (sizeof(u32) * 8) ) + 1)   
#define DECLARE_BITMAP(name,n) \
	u32 name[bits_to_long(n)]

int op_bits(u32 *bit_map, int n, bit_ops_t ops);

static void inline clear_bit(u32 *bit_map, int n)
{
	op_bits(bit_map,n,BITS_CLEAR);
}

static void inline set_bit(u32 *bit_map, int n)
{
	op_bits(bit_map,n,BITS_SET);
}

static int inline read_bit(u32 *bit_map, int n)
{
	return op_bits(bit_map, n, BITS_READ);
}

void init_bitmap(u32 bitmap[], int n);

#endif
