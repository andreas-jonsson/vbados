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

#include <conio.h>

// Customizable defines
/** If 0, these routines become nops */
#define ENABLE_DLOG 0
/** 1 means target serial port, 0 means target IO port. */
#define DLOG_TARGET_SERIAL 0
/** IO port to target.
 *  VirtualBox uses 0x504, Bochs, DOSBox and descendants use 0xE9.
 *  When using DLOG_TARGET_SERIAL, use desired UART IO base port. (e.g. COM1 = 0x3F8). */
#define DLOG_TARGET_PORT 0x504

// End of customizable defines

#if ENABLE_DLOG

/** Initializes the debug log port. */
static void dlog_init();

/** Logs a single character to the debug message IO port. */
static inline void dlog_putc(char c);

#if DLOG_TARGET_SERIAL

static void dlog_init()
{
	// Comes straight from https://wiki.osdev.org/Serial_Ports#Initialization
	outp(DLOG_TARGET_PORT + 1, 0x00);    // Disable all interrupts
	outp(DLOG_TARGET_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outp(DLOG_TARGET_PORT + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
	outp(DLOG_TARGET_PORT + 1, 0x00);    //                  (hi byte)
	outp(DLOG_TARGET_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	outp(DLOG_TARGET_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outp(DLOG_TARGET_PORT + 4, 0x03);    // RTS/DSR set, IRQs disabled
}

static inline void dlog_putc(char c)
{
	while (!(inp(DLOG_TARGET_PORT + 5) & 0x20));
	outp(DLOG_TARGET_PORT, c);
}

#else /* DLOG_TARGET_SERIAL */

static void dlog_init()
{
}

static inline void dlog_putc(char c)
{
	outp(DLOG_TARGET_PORT, c);
}



#endif /* DLOG_TARGET_SERIAL */

static void dlog_endline(void)
{
	dlog_putc('\n');
}

/** Print string to log */
static void dlog_print(const char *s)
{
	char c;
	while (c = *s++) {
		dlog_putc(c);
	}
}

/** Print (far) string to log */
static void dlog_fprint(const char __far *s)
{
	char c;
	while (c = *s++) {
		dlog_putc(c);
	}
}

/** Print (far) string of fixed length to log */
static void dlog_fnprint(const char __far *s, unsigned l)
{
	while (l > 0) {
		dlog_putc(*s++);
		l--;
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

#else /* ENABLE_DLOG */

#define dlog_nop()          do { } while(0)
#define dlog_init()         dlog_nop()
#define dlog_putc(c)        dlog_nop()
#define dlog_endline()      dlog_nop()
#define dlog_print(s)       dlog_nop()
#define dlog_fprint(s)      dlog_nop()
#define dlog_fnprint(s,n)   dlog_nop()
#define dlog_puts(s)        dlog_nop()
#define dlog_printub(n,b)   dlog_nop()
#define dlog_printdb(n,b)   dlog_nop()
#define dlog_printx(n)      dlog_nop()
#define dlog_printu(n)      dlog_nop()
#define dlog_printd(n)      dlog_nop()


#endif /* ENABLE_DLOG */

#endif /* DLOG_H */
