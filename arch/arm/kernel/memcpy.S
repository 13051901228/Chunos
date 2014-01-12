	.text

/*
 * memcpy(void *target, void *source, int size)
 * r0 = target r1 = source r2 = size
 */
.global memcpy
memcpy:
	push {r3-r6}
	mov r3, r0
	mov r4, r1
	and r3, r3, #0x3
	and r4, r4, #0x3
	cmp r3, r4
	bne copy_left
	sub r2, r2, r3
	
copy_aligin:
	cmp r3, #0
	beq copy_bigger_16
	ldrb r4, [r1]
	strb r4, [r0]
	add r0, r0, #1
	add r1, r1, #1
	sub r3, r3, #1
	b copy_aligin

copy_bigger_16:
	cmp r2, #16
	blt copy_left
	ldmia r1!, {r3-r6}
	stmia r0!, {r3-r6}
	sub r2, r2, #16
	b copy_bigger_16

copy_left:
	cmp r2, #0
	beq memcpy_end
	ldrb r3, [r1]
	strb r3, [r0]
	add r0, r0, #1
	add r1, r1, #1
	sub r2, r2, #1
	b copy_left

memcpy_end:
	sub r0, r0, r2
	pop {r3-r6}
	mov pc, lr
