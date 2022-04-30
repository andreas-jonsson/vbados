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
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>

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
static inline void dputc(char c);

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

static inline void dputc(char c)
{
	while (!(inp(DLOG_TARGET_PORT + 5) & 0x20));
	outp(DLOG_TARGET_PORT, c);
}

#else /* DLOG_TARGET_SERIAL */

static inline void dlog_init()
{
	// No initialization required
}

static inline void dputc(char c)
{
	outp(DLOG_TARGET_PORT, c);
}

#endif /* DLOG_TARGET_SERIAL */

static void d_utoa(unsigned num, unsigned base)
{
	char buf[6];
	int i = 0;

	do {
		unsigned digit = num % base;

		if (digit < 10) {
			buf[i] = '0' + digit;
		} else {
			buf[i] = 'a' + (digit - 10);
		}

		i++;
		num /= base;
	} while (num > 0 && i < sizeof(buf));

	while (i--) {
		dputc(buf[i]);
	}
}

static void d_ultoa(unsigned long num, unsigned base)
{
	char buf[12];
	int i = 0;

	do {
		unsigned digit = 0;

		static inline void uldiv(void);
#pragma aux uldiv = \
			"push eax" \
			"push ecx" \
			"push edx" \
			"mov eax, [num]" \
			"movzx ecx, [base]" \
			"xor edx, edx" \
			\
			"div ecx" \
			\
			"mov [num], eax" \
			"mov [digit], dx" \
			\
			"pop edx" \
			"pop ecx" \
			"pop eax" \
			__modify __exact [];
		uldiv();

		if (digit < 10) {
			buf[i] = '0' + digit;
		} else {
			buf[i] = 'a' + (digit - 10);
		}

		i++;
	} while (num > 0 && i < sizeof(buf));

	while (i--) {
		dputc(buf[i]);
	}
}

static void d_itoa(int num, unsigned base)
{
	unsigned unum;

	// TODO
	if (num < 0) {
		dputc('-');
		unum = -num;
	} else {
		unum = num;
	}

	d_utoa(unum, base);
}

static void d_ltoa(long num, unsigned base)
{
	unsigned long unum;

	// TODO
	if (num < 0) {
		dputc('-');
		unum = -num;
	} else {
		unum = num;
	}

	d_ultoa(unum, base);
}

static unsigned d_strtou(const char * __far *str)
{
	unsigned i = 0;
	const char *s = *str;
	char c = *s;
	while (c >= '0' && c <= '9') {
		i = (i * 10) + (c - '0');
		c = *(++s);
	}
	*str = s;
	return i;
}

static void d_printstr(const char __far *s, unsigned l)
{
	while (l > 0 && *s) {
		dputc(*s++);
		l--;
	}
}

static void dprintf(const char *fmt, ...)
{
	va_list va;
	char c;

	va_start(va, fmt);

	while ((c = *(fmt++))) {
		if (c == '%') {
			unsigned width, precision, size;
			bool is_far = false;

			width = d_strtou(&fmt);

			if ((c = *fmt) == '.') {
				fmt++;
				precision = d_strtou(&fmt);
			} else {
				precision = UINT_MAX;
			}

			switch ((c = *fmt)) {
			case 'l':
				size = sizeof(long);
				fmt++;
				break;
			case 'h':
				size = sizeof(short);
				fmt++;
				break;
			case 'F':
				is_far = true;
				fmt++;
				break;
			default:
				size = sizeof(int);
				break;
			}

			switch ((c = *fmt)) {
			case 'd':
				switch (size) {
				case sizeof(int):
					d_itoa(va_arg(va, int), 10);
					break;
				case sizeof(long):
					d_ltoa(va_arg(va, long), 10);
					break;
				}
				break;
			case 'u':
				switch (size) {
				case sizeof(int):
					d_utoa(va_arg(va, int), 10);
					break;
				case sizeof(long):
					d_ultoa(va_arg(va, long), 10);
					break;
				}
				break;
			case 'x':
				switch (size) {
				case sizeof(int):
					d_utoa(va_arg(va, int), 16);
					break;
				case sizeof(long):
					d_ultoa(va_arg(va, long), 16);
					break;
				}
				break;
			case 'p':
				if (is_far) {
					void __far *p = va_arg(va, void __far *);
					d_utoa(FP_SEG(p), 16);
					dputc(':');
					d_utoa(FP_OFF(p), 16);
				} else {
					d_utoa(va_arg(va, unsigned), 16);
				}
				break;
			case 's':
				d_printstr(is_far ? va_arg(va, const char __far *)
				                  : va_arg(va, const char *),
				           precision);
				break;
			case '%':
				dputc('%');
				break;
			}
			fmt++;
		} else {
			dputc(c);
		}
	}

	va_end(va);
}

static void dputs(const char *s)
{
	char c;
	while ((c = *(s++))) {
		dputc(c);
	}
	dputc('\n');
}

#else /* ENABLE_DLOG */

#define dlog_nop()       do { } while(0)
#define dlog_init()      dlog_nop()
#define dputc(c)         dlog_nop()
#define dputs(s)         dlog_nop()
#define dprintf(...)     dlog_nop()


#endif /* ENABLE_DLOG */

#endif /* DLOG_H */
