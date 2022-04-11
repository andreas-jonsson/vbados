/*
 * VBMouse - small inline utilities
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

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <i86.h>

#define STATIC_ASSERT(expr)           typedef int STATIC_ASSERT_FAILED[(expr) ? 1 : -1]

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

#define BOUND(x,a,b) ( MIN(b, MAX(a, x)) )

static inline void breakpoint(void);
#pragma aux breakpoint = 0xcd 0x03;

static inline __segment get_cs(void);
#pragma aux get_cs = "mov ax, cs" value [ax] modify exact [];

static inline __segment get_ds(void);
#pragma aux get_ds = "mov ax, ds" value [ax] modify exact [];

/** Converts a far pointer into equivalent linear address.
 *  Note that under protected mode linear != physical (for that, need VDS). */
static inline uint32_t linear_addr(const void __far * ptr)
{
	return ((uint32_t)(FP_SEG(ptr)) << 4) + FP_OFF(ptr);
}

/** Map x linearly from range [0, srcmax] to [0, dstmax].
 *  Equivalent of (x * dstmax) / srcmax but with 32-bit unsigned precision. */
static unsigned scaleu(unsigned x, unsigned srcmax, unsigned dstmax);
#pragma aux scaleu = \
	"mul cx" /* dx:ax = x * dstmax */\
	"div bx" /* ax = dx:ax / srcmax */\
	__parm [ax] [bx] [cx] \
	__value [ax] \
	__modify [ax dx]

/** Map x linearly from range [0, srcmax] to [0, dstmax].
 *  Equivalent of (x * dstmax) / srcmax but with 32-bit signed precision. */
static int scalei(int x, int srcmax, int dstmax);
#pragma aux scalei = \
	"imul cx" /* dx:ax = x * dstmax */ \
	"idiv bx" /* ax = dx:ax / srcmax */ \
	__parm [ax] [bx] [cx] \
	__value [ax] \
	__modify [ax dx]

/** Map x linearly from range [0, srcmax] to [0, dstmax].
 *  Equivalent of (x * dstmax) / srcmax but with 32-bit signed precision.
 *  Division remainder is returned in rem, which should be reused
 *  in future calls to reduce rounding error. */
static int scalei_rem(int x, int srcmax, int dstmax, short *rem);
#pragma aux scalei_rem = \
	"imul cx"       /* dx:ax = x * dstmax */ \
	"mov cx, [si]"  /* cx = rem */ \
	"test cx, cx" \
	"setns cl"      /* cl = 1 if rem positive, 0 if negative */ \
	"movzx cx, cl" \
	"dec cx"        /* cx = 0 if rem positive, -1 if negative */ \
	"add ax, [si]"  /* ax += *rem */ \
	"adc dx, cx"    /* dx += 1 if carry and rem positive, 0 if carry and rem negative (aka. signed addition) */ \
	"idiv bx"       /* ax = dx:ax / srcmax, dx = new remainder */ \
	"mov [si], dx"  /* store the new remainder */ \
	__parm [ax] [bx] [cx] [si] \
	__value [ax] \
	__modify [ax cx dx si]

#endif
