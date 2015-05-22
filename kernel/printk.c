#include <os/types.h>
#include <os/varlist.h>
#include <os/string.h>
#include <os/printk.h>
#include <os/mirros.h>
#include <os/spin_lock.h>
#include <os/tty.h>

#define PRINTF_DEC		0X0001
#define PRINTF_HEX		0x0002
#define PRINTF_OCT		0x0004
#define PRINTF_BIN		0x0008
#define PRINTF_MASK		(0x0f)
#define PRINTF_UNSIGNED		0X0010
#define PRINTF_SIGNED		0x0020

#define LOG_BUFFER_SIZE		(8192)

extern void uart_puts(char *buf);

struct log_buffer {
	spin_lock_t buffer_lock;
	int head;
	int tail;
	int total;
	int handle_head;
	char buf[LOG_BUFFER_SIZE];
};

static struct log_buffer log_buffer;

int log_buffer_init(void)
{
	spin_lock_init(&log_buffer.buffer_lock);
	log_buffer.head = 0;
	log_buffer.tail = 0;
	log_buffer.total = 0;
	log_buffer.handle_head = 0;

	return 0;
}

int numbric(char *buf,unsigned int num, int flag)
{
	int len = 0;

	switch (flag & PRINTF_MASK) {
		case PRINTF_DEC:
			if (flag &PRINTF_SIGNED)
				len = itoa(buf, (int)num);
			else
				len = uitoa(buf, num);
			break;
		case PRINTF_HEX:
			len = hextoa(buf, num);
			break;
		case PRINTF_OCT:
			len = octtoa(buf, num);
			break;
		case PRINTF_BIN:
			len = bintoa(buf, num);
			break;
		default:
			break;
	}

	return len;
}

/*
 *for this function if the size over than 1024
 *this will cause an overfolow error,we will fix
 *this bug later
 */
int vsprintf(char *buf, const char *fmt, va_list arg)
{
	char *str;
	int len;
	char *tmp;
	s32 number;
	u32 unumber;
	int flag = 0;

	if (buf == NULL)
		return -1;

	for (str = buf; *fmt; fmt++) {

		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}
		
		fmt++;
		switch (*fmt) {
			case 'd':
				flag |= PRINTF_DEC | PRINTF_SIGNED;
				break;
			case 'x':
				flag |= PRINTF_HEX | PRINTF_UNSIGNED;
				break;
			case 'u':
				flag |= PRINTF_DEC | PRINTF_UNSIGNED;
				break;
			case 's':
				len = strlen(tmp = va_arg(arg, char *));
				strncpy(str, tmp, len);
				str += len;
				continue;
				break;
			case 'c':
				*str = (char)(va_arg(arg, char));
				str++;
				continue;
				break;
			case 'o':
				flag |= PRINTF_DEC | PRINTF_SIGNED;
				break;
			case '%':
				*str = '%';
				str++;
				continue;
				break;
			default:
				*str = '%';
				*(str+1) = *fmt;
				str += 2;
				continue;
				break;
		}

		if(flag & PRINTF_UNSIGNED){
			unumber = va_arg(arg, u32);
			len = numbric(str, unumber, flag);
		} else {

			number = va_arg(arg, s32);
			len = numbric(str, number, flag);
		}
		str+=len;
		flag = 0;
	}

	*str = 0;

	return str-buf;
}

static int update_log_buffer(char *buf, int printed)
{
	struct log_buffer *lb = &log_buffer;
	int len1, len2;

	if ((lb->tail + printed) > (LOG_BUFFER_SIZE - 1)) {
		len1 = LOG_BUFFER_SIZE - lb->tail;
		len2 = printed - len1;
	} else {
		len1 = printed;
		len2 = 0;
	}

	if (len1)
		memcpy(lb->buf + lb->tail, buf, len1);

	if (len2)
		memcpy(lb->buf, buf + len1, len2);
	
	lb->total += printed;

	if (len2)
		lb->head = lb->tail = len1;
	else {
		lb->tail += len2;
		if (lb->total > LOG_BUFFER_SIZE)
			lb->head = lb->tail;
	}

	return 0;
}

static inline void early_printk(char *str)
{
	uart_puts(str);
}

int level_printk(const char *fmt,...)
{
	char ch;
	int level = LOG_LEVEL;
	va_list arg;
	int printed;
	ch = *fmt;
	char buf[1024];

	if (is_digit(ch)) {
		ch = ch-'0';
		if(ch > level)
			return 0;
		fmt++;
	}
	

	va_start(arg, fmt);
	printed = vsprintf(buf, fmt, arg);
	va_end(arg);

	spin_lock_irqsave(&log_buffer.buffer_lock);
	update_log_buffer(buf, printed);
	spin_unlock_irqstore(&log_buffer.buffer_lock);

	if (!tty_flush_log(buf, printed)) {
		early_printk(buf);
	}

	return printed;
}
