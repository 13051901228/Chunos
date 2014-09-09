#ifndef _SYS_SIGNAL_H
#define _SYS_SIGNAL_H

#include <os/types.h>

#define MAX_SIGNAL	64

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGSEGV		11
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGUNUSED	31

#define SIGBUS		 7
#define SIGUSR1		10
#define SIGUSR2		12
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPWR		30
#define SIGSYS		31

#define SIGCLD		SIGCHLD
#define SIGPOLL		SIGIO

#define SA_THIRTYTWO	0x02000000
#define SA_ONSTACK	0x08000000
#define SA_RESTART	0x10000000
#define SA_INTERRUPT	0x20000000 /* dummy -- ignored */
#define SA_NODEFER	0x40000000
#define SA_RESETHAND	0x80000000

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0L)	/* default signal handling */
#define SIG_IGN ((sighandler_t)1L)	/* ignore signal */
#define SIG_ERR ((sighandler_t)-1L)	/* error return from signal */

#define SIG_HANDLER_IGN	0x00000001
#define SIG_HANDLER_DFL 0x00000002

struct signal_action;

struct signal_action {
	int action;
	void *arg;
	struct signal_action *next;
};

struct signal_entry {
	sighandler_t handler;
	int flag;
};

struct signal_struct {
	struct signal_action *pending;
	struct signal_action *plast;
	struct signal_entry signal_table[MAX_SIGNAL];
	u16 signal_pending;
	u16 in_signal;
};

#endif
