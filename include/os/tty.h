/*
 * include/os/tty.h
 *
 * Create by Le Min at 07/26/2014
 *
 */

#ifndef _TTY_H_
#define _TTY_H_

#include <os/device.h>
#include <os/spin_lock.h>

#define TTY_MAJOR	4

typedef u32		tcflag_t;
typedef unsigned char 	cc_t;

#define NCCS	19
struct termios {
	tcflag_t c_iflag;		/* input mode flags */
  	tcflag_t c_oflag;		/* output mode flags */
  	tcflag_t c_cflag;		/* control mode flags */
 	tcflag_t c_lflag;		/* local mode flags */
  	cc_t c_line;			/* line discipline */
  	cc_t c_cc[NCCS];		/* control characters */
};

struct tty;

struct tty_operations {
	int (*open)(struct tty *tty);
	size_t (*put_chars)(struct tty *tty, char *buf, size_t size);
	size_t (*get_chars)(struct tty *tty, char *buf, size_t size);
	int (*set_baud)(struct tty *tty, u32 baud);
};

struct tty {
	struct device *dev;
	u32 baud;
	struct termios termios;
	struct tty_operations *tty_ops;
	bool is_console;
};

struct tty *allocate_tty(void);
int register_tty(struct tty *tty);
int tty_flush_log(char *buf, int printed);

#endif
