	.text
.global _start
_start:
	push %ebx
	push %eax
	call main
	push %eax
	call _exit
	hlt
