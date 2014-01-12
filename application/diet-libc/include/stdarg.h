#ifndef _STDARG_H
#define _STDARG_H

#include <sys/cdefs.h>
#include <endian.h>

__BEGIN_DECLS

#if (__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 96))

#if 0
typedef __builtin_va_list va_list;
#define va_start(v,l)	__builtin_stdarg_start((v),l)
#define va_end		__builtin_va_end
#define va_arg		__builtin_va_arg
#define __va_copy(d,s)	__builtin_va_copy((d),(s))

#else

typedef char *va_list;
#define addr(n)		(&n)
#define offset(n)	( (sizeof(n)+sizeof(int)-1) & ~(sizeof(int)-1))
#define va_start(ap,n)  ap = (char *)addr(n) + offset(n)
#define va_end(ap)	(void)0
#define va_arg(ap,type) ( *(type *)((ap += offset(type)) - offset(type)) )
#define __va_copy(d, s)	(d) = (s)

#endif

#endif

#ifndef va_end
#include <stdarg-cruft.h>
#endif

#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L
#define va_copy(d,s)	__va_copy(d,s)
#endif

__END_DECLS

#endif
