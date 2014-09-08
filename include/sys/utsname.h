/*
 * include/sys/utsname.h
 *
 * Created by Le Min at 23/08/2014
 */

#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#define UTSNAME_LENGTH			65
#define UTSNAME_NODENAME_LENGTH		UTSNAME_LENGTH

struct utsname {
	char sysname[UTSNAME_LENGTH];
	char nodename[UTSNAME_NODENAME_LENGTH];
	char release[UTSNAME_LENGTH];
	char version[UTSNAME_LENGTH];
	char machine[UTSNAME_LENGTH];
	char domainname[UTSNAME_LENGTH];
};

#endif
