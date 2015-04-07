	.text

.extern x86_irq_handler
.global int_handler_common_stub
.align 8
int_handler_common_stub:
	int $5

.global trap_handler_common_stub
.align 8
trap_handler_common_stub:

.extern x86_trap_handler_de

.macro TRAP_NOERRCODE name vec_no
	.global x86_trap_\name
	.align 8
	x86_trap_\name:
	cli
	push $0
	.byte 0x6a
	.byte \vec_no
	push x86_trap_handler_\name
	jmp trap_handler_common_stub
.endm

.macro TRAP_ERRCODE name vec_no
	.global x86_trap_\name
	.align 8
	x86_trap_\name:
	cli
	push x86_trap_handler_\name
	jmp trap_handler_common_stub
.endm

TRAP_NOERRCODE de 0
TRAP_NOERRCODE db 1
