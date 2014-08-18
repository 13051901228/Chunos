	.text

_exit:
	push {r4, r5, r7, lr}
	ldr r4, [sp, #16]
	ldr r5, [sp, #20]
	mov r7, #0x1
	svc 0x0
	bx lr
	
