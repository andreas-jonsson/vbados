#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

#define BOUND(x,a,b) ( MIN(b, MAX(a, x)) )

static inline void breakpoint(void);
#pragma aux breakpoint = 0xcd 0x03;

static inline __segment get_cs(void);
#pragma aux get_cs = "mov ax, cs" value [ax] modify exact [];

static inline __segment get_ds(void);
#pragma aux get_ds = "mov ax, ds" value [ax] modify exact [];

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
