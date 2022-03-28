#ifndef DLOG_H
#define DLOG_H

#define ENABLE_DLOG 1

#if ENABLE_DLOG

#if 1
#include "vbox.h"
#define dlog_putc vbox_log_putc
#endif

static inline void dlog_endline(void)
{
	dlog_putc('\n');
}

static inline void dlog_print(const char *s)
{
	char c;
	while (c = *s++) {
		dlog_putc(c);
	}
}

static inline void dlog_printu(unsigned int num, int base)
{
	char buf[4];
	int i = 0;

	do {
		int digit = num % base;

		if (digit < 10) {
			buf[i] = '0' + digit;
		} else {
			buf[i] = 'a' + (digit - 10);
		}

		i++;
		num /= base;
	} while (num > 0);

	while (i--) {
		dlog_putc(buf[i]);
	}
}

static inline void dlog_printx(unsigned int num)
{
	dlog_printu(num, 16);
}

static inline void dlog_printd(int num, int base)
{
	unsigned int unum;

	// TODO
	if (num < 0) {
		dlog_putc('-');
		unum = -num;
	} else {
		unum = num;
	}

	dlog_printu(unum, base);
}

static inline void dlog_puts(const char *s)
{
	dlog_print(s);
	dlog_endline();
}

#else

#define dlog_putc(c)
#define dlog_endline()
#define dlog_print(s)
#define dlog_printu(n)
#define dlog_printx(n)
#define dlog_printd(n,b)
#define dlog_puts(s)

#endif

#endif
