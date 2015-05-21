	.text

#include "include/gdt.h"
#include "include/x86_config.h"

.extern current
.extern next_run
.extern switch_task

.macro ISR_COMMON_HANDLER name
	mov 32(%esp), %ebx		/* ebx is the error code */

	mov %esp, %eax
	add $44, %eax			/* eax is pt_regs */

	mov 40(%esp), %edx
	and $0x3, %edx			/* irq from ring3 or ring0 */
	cmp $0, %edx

	jne isr_from_user_\name		/* nerver return */
	push $0
	push $KERNEL_SS

	push $KERNEL_ES			/* kernel segment selector */
	push $KERNEL_DS
	push $KERNEL_FS
	push $KERNEL_GS
	jmp do_isr_handler_\name

isr_from_user_\name:
	sub $4, %esp			/* user esp */
	mov -52(%esp), %edx
	mov %edx, (%esp)

	push $USER_SS
	push $USER_ES			/* user segment selector */
	push $USER_DS
	push $USER_FS
	push $USER_GS

do_isr_handler_\name:
	call %ecx

	lea current, %ecx
	lea next_run, %edx
	mov (%ecx), %eax
	mov (%edx), %ebx
	mov %esp, (%eax)	/* store the stack base to current process */

	call switch_task

	lea current, %ecx
	lea next_run, %edx
	mov (%ecx), %eax
	mov (%edx), %ebx

	mov %ebx, (%ecx)	/* current = next_run */
	mov (%ebx), %esp	/* stack base of next run task */

	pop %gs
	pop %fs
	pop %ds
	pop %es
	add $8, %esp
	popa
.endm

/************************************************************** */
isr_common_stub_nec:
	ISR_COMMON_HANDLER nec
	add $4, %esp
	iret
/************************************************************** */


/****************************************************************/
isr_common_stub_ec:
	ISR_COMMON_HANDLER ec
	iret
/****************************************************************/

.extern x86_trap_handler_de
.extern x86_trap_handler_de
.extern x86_trap_handler_db
.extern x86_trap_handler_nmi
.extern x86_trap_handler_bp
.extern x86_trap_handler_of
.extern x86_trap_handler_br
.extern x86_trap_handler_ud
.extern x86_trap_handler_nm
.extern x86_trap_handler_df
.extern x86_trap_handler_old_mf
.extern x86_trap_handler_ts
.extern x86_trap_handler_np
.extern x86_trap_handler_ss
.extern x86_trap_handler_gp
.extern x86_trap_handler_pf
.extern x86_trap_handler_spurious
.extern x86_trap_handler_mf
.extern x86_trap_handler_ac
.extern x86_trap_handler_nc
.extern x86_trap_handler_xf
.extern x86_trap_handler_undef
.extern x86_int_handler

.macro TRAP_NOERRCODE name vec_no
	.global x86_trap_\name
	.align 8
	x86_trap_\name:
	cli
	push $0				/* error code */
	pusha
	lea x86_trap_handler_\name, %ecx
	jmp isr_common_stub_nec
.endm

.macro TRAP_ERRCODE name vec_no
	.global x86_trap_\name
	.align 8
	x86_trap_\name:
	cli
	pusha
	lea x86_trap_handler_\name, %ecx
	jmp isr_common_stub_ec
.endm

.macro	IRQ vec_no
	.global x86_irq_\vec_no
	.align 8
	x86_irq_\vec_no:
	cli
	push \vec_no + 32
	pusha
	lea x86_int_handler, %ecx
	jmp isr_common_stub_nec
.endm

.global x86_trap_undef
.align 8
x86_trap_undef:
	cli
	pusha
	lea x86_trap_handler_undef, %ecx
	jmp isr_common_stub_nec

.extern x86_trap_syscall_handler		/* this is for syscall */
.global x86_trap_syscall
.align 8
x86_trap_syscall:
	cli
	push %eax				/* eax is the syscall number */
	pusha
	lea x86_trap_syscall_handler, %ecx
	jmp isr_common_stub_nec

.align 8
TRAP_NOERRCODE de 	0
TRAP_NOERRCODE db 	1
TRAP_NOERRCODE nmi 	2
TRAP_NOERRCODE bp 	3
TRAP_NOERRCODE of	4
TRAP_NOERRCODE br	5
TRAP_NOERRCODE ud	6
TRAP_NOERRCODE nm	7
TRAP_ERRCODE   df	8
TRAP_NOERRCODE old_mf	9
TRAP_ERRCODE   ts	10
TRAP_ERRCODE   np	11
TRAP_ERRCODE   ss	12
TRAP_ERRCODE   gp	13
TRAP_ERRCODE   pf	14
TRAP_NOERRCODE spurious	15
TRAP_NOERRCODE mf	16
TRAP_NOERRCODE ac	17
TRAP_NOERRCODE nc	18
TRAP_NOERRCODE xf	19

.align 8
IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15
IRQ 16
IRQ 17
IRQ 18
IRQ 19
IRQ 20
IRQ 21
IRQ 22
IRQ 23
IRQ 24
IRQ 25
IRQ 26
IRQ 27
IRQ 28
IRQ 29
IRQ 30
IRQ 31
IRQ 32
IRQ 33
IRQ 34
IRQ 35
IRQ 36
IRQ 37
IRQ 38
IRQ 39
IRQ 40
IRQ 41
IRQ 42
IRQ 43
IRQ 44
IRQ 45
IRQ 46
IRQ 47
IRQ 48
IRQ 49
IRQ 50
IRQ 51
IRQ 52
IRQ 53
IRQ 54
IRQ 55
IRQ 56
IRQ 57
IRQ 58
IRQ 59
IRQ 60
IRQ 61
IRQ 62
IRQ 63
IRQ 64
IRQ 65
IRQ 66
IRQ 67
IRQ 68
IRQ 69
IRQ 70
IRQ 71
IRQ 72
IRQ 73
IRQ 74
IRQ 75
IRQ 76
IRQ 77
IRQ 78
IRQ 79
IRQ 80
IRQ 81
IRQ 82
IRQ 83
IRQ 84
IRQ 85
IRQ 86
IRQ 87
IRQ 88
IRQ 89
IRQ 90
IRQ 91
IRQ 92
IRQ 93
IRQ 94
IRQ 95
IRQ 96
IRQ 97
IRQ 98
IRQ 99
IRQ 100
IRQ 101
IRQ 102
IRQ 103
IRQ 104
IRQ 105
IRQ 106
IRQ 107
IRQ 108
IRQ 109
IRQ 110
IRQ 111
IRQ 112
IRQ 113
IRQ 114
IRQ 115
IRQ 116
IRQ 117
IRQ 118
IRQ 119
IRQ 120
IRQ 121
IRQ 122
IRQ 123
IRQ 124
IRQ 125
IRQ 126
IRQ 127
IRQ 128
IRQ 129
IRQ 130
IRQ 131
IRQ 132
IRQ 133
IRQ 134
IRQ 135
IRQ 136
IRQ 137
IRQ 138
IRQ 139
IRQ 140
IRQ 141
IRQ 142
IRQ 143
IRQ 144
IRQ 145
IRQ 146
IRQ 147
IRQ 148
IRQ 149
IRQ 150
IRQ 151
IRQ 152
IRQ 153
IRQ 154
IRQ 155
IRQ 156
IRQ 157
IRQ 158
IRQ 159
IRQ 160
IRQ 161
IRQ 162
IRQ 163
IRQ 164
IRQ 165
IRQ 166
IRQ 167
IRQ 168
IRQ 169
IRQ 170
IRQ 171
IRQ 172
IRQ 173
IRQ 174
IRQ 175
IRQ 176
IRQ 177
IRQ 178
IRQ 179
IRQ 180
IRQ 181
IRQ 182
IRQ 183
IRQ 184
IRQ 185
IRQ 186
IRQ 187
IRQ 188
IRQ 189
IRQ 190
IRQ 191
IRQ 192
IRQ 193
IRQ 194
IRQ 195
IRQ 196
IRQ 197
IRQ 198
IRQ 199
IRQ 200
IRQ 201
IRQ 202
IRQ 203
IRQ 204
IRQ 205
IRQ 206
IRQ 207
IRQ 208
IRQ 209
IRQ 210
IRQ 211
IRQ 212
IRQ 213
IRQ 214
IRQ 215
IRQ 216
IRQ 217
IRQ 218
IRQ 219
IRQ 220
IRQ 221
IRQ 222
IRQ 223
