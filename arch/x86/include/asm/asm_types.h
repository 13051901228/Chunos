#ifndef __X86_ASM_TYPES_H
#define __X86_ASM_TYPES_H

#define ASM_FUNCTION(name)	\
	.global name		\
	##name\:

#define ASM_FUNCTION_END()

#endif
