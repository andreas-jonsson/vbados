/*
 * VBMouse - printf & logging routines
 * Copyright (C) 2022 Javier S. Pedro
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DLOG_H
#define DLOG_H

#define ENABLE_DLOG 1

#if ENABLE_DLOG

#if 1
#include "vboxlog.h"
#define dlog_putc vbox_log_putc
#endif

#define dlog_endline() dlog_putc('\n')

/** Print string to log */
static void dlog_print(const char *s)
{
	char c;
	while (c = *s++) {
		dlog_putc(c);
	}
}

/** Print + newline */
static void dlog_puts(const char *s)
{
	dlog_print(s);
	dlog_endline();
}

/** Print unsigned number with base */
static void dlog_printub(unsigned int num, int base)
{
	char buf[8];
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
	} while (num > 0 && i < sizeof(buf));

	while (i--) {
		dlog_putc(buf[i]);
	}
}

/** Print signed number with base */
static void dlog_printdb(int num, int base)
{
	unsigned int unum;

	// TODO
	if (num < 0) {
		dlog_putc('-');
		unum = -num;
	} else {
		unum = num;
	}

	dlog_printub(unum, base);
}

/** Print unsigned number in base 16. */
static void dlog_printu(unsigned int num)
{
	dlog_printub(num, 10);
}

/** Print unsigned number in base 10. */
static void dlog_printx(unsigned int num)
{
	dlog_printub(num, 16);
}

/** Print signed number in base 10. */
static void dlog_printd(int num)
{
	dlog_printdb(num, 10);
}

#else

#define dlog_putc(c)
#define dlog_endline()
#define dlog_print(s)
#define dlog_puts(s)
#define dlog_printub(n,b)
#define dlog_printdb(n,b)
#define dlog_printx(n)
#define dlog_printu(n)
#define dlog_printd(n)


#endif /* ENABLE_DLOG */

#endif /* DLOG_H */
