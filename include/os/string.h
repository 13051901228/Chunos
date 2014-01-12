#ifndef _STRING_H
#define _STRING_H

int absolute(int num);
int itoa(char *buf, int num);
int uitoa(char *buf, unsigned int num);
int inline hextoa(char *buf, unsigned int num);
int inline octtoa(char *buf, unsigned int num);
int inline bintoa(char *buf, unsigned int num);
int strlen(char *buf);
char *strcpy(char *des, char *src);
char *strncpy(char *des, char *src, int len);
int is_digit(char ch);
int strcmp(const char *src, const char *dst);
int strncmp(const char *src, const char *dst,int n);
int memcpy(void *target, void *source, int size);
void memset(char *base, char ch, int size);

#endif
