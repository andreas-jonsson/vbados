/*
 * VBMouse - Routines to access the PS2 BIOS services
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

#ifndef PS2_H
#define PS2_H

#include <stdbool.h>
#include <stdint.h>

/** Standard PS/2 mouse IRQ. At least on VirtualBox. */
#define PS2_MOUSE_IRQ        12
/** The corresponding interrupt vector for IRQ 12. */
#define PS2_MOUSE_INT_VECTOR 0x74

/** Packet size for plain PS/2 in default protocol. */
#define PS2_MOUSE_PLAIN_PACKET_SIZE 3

typedef uint8_t ps2m_err;
enum ps2m_errors {
	PS2M_ERR_INVALID_FUNCTION = 1,
	PSM2_ERR_INVALID_INPUT    = 2,
	PSM2_ERR_INTERFACE_ERROR  = 3,
	PSM2_ERR_RESEND           = 4,
	PSM2_ERR_NO_CALLBACK      = 5,
};

enum ps2m_status {
	PS2M_STATUS_BUTTON_1 = 1 << 0,
	PS2M_STATUS_BUTTON_2 = 1 << 1,
	PS2M_STATUS_BUTTON_3 = 1 << 2,
	PS2M_STATUS_X_NEG    = 1 << 4,
	PS2M_STATUS_Y_NEG    = 1 << 5,
	PS2M_STATUS_X_OVF    = 1 << 6,
	PS2M_STATUS_Y_OVF    = 1 << 7,
};

enum ps2m_device_ids {
	/** Standard PS/2 mouse, 2 buttons. */
	PS2M_DEVICE_ID_PLAIN = 0,
	/** IntelliMouse PS/2, with wheel. */
	PS2M_DEVICE_ID_IMPS2 = 3,
	/** IntelliMouse Explorer, wheel and 5 buttons. */
	PS2M_DEVICE_ID_IMEX = 4,
	/** IntelliMouse Explorer, wheel, 5 buttons, and horizontal scrolling. */
	PS2M_DEVICE_ID_IMEX_HORZ = 5
};

#pragma aux PS2_CB far loadds parm reverse caller []
// TODO: ax and es look not be preserved with this. VBox BIOS already preserves them though, as well as 32-bit registers

/** Invoked by the BIOS when there is a mouse event.
 *  @param status combination of PS2M_STATUS_* flags */
typedef void (__far * LPFN_PS2CALLBACK)(uint8_t status, uint8_t x, uint8_t y, uint8_t z);

static ps2m_err ps2m_init(uint8_t packet_size);
#pragma aux ps2m_init = \
	"stc"               /* If nothing happens, assume failure */ \
	"mov ax, 0xC205"    /* Pointing device: initialization (packet size in bh) */ \
	"int 0x15" \
	"jnc end"           /* Success */ \
	"fail: test ah, ah" /* Ensure we have some error code back, set ah to -1 if we don't */ \
	"jnz end" \
	"dec ah" \
	"end:" \
	__parm [bh] \
	__value [ah] \
	__modify [ax]

static ps2m_err ps2m_reset(void);
#pragma aux ps2m_reset = \
	"stc" \
	"mov ax, 0xC201"    /* Pointing device: reset */ \
	"int 0x15" \
	"jnc end" \
	"fail: test ah, ah" \
	"jnz end" \
	"dec ah" \
	"end:" \
	__value [ah] \
	__modify [ax]

static ps2m_err ps2m_get_device_id(uint8_t *device_id);
#pragma aux ps2m_get_device_id = \
	"stc" \
	"mov ax, 0xC204"    /* Pointing device: get device ID */ \
	"int 0x15" \
	"mov es:[di], bh"   /* Save returned value */ \
	"jnc end" \
	"fail: test ah, ah" \
	"jnz end" \
	"dec ah" \
	"end:" \
	__parm [es di] \
	__value [ah] \
	__modify [ax]

//      0 =  25 dpi, 1 count  per millimeter
//      1 =  50 dpi, 2 counts per millimeter
//      2 = 100 dpi, 4 counts per millimeter
//      3 = 200 dpi, 8 counts per millimeter
static ps2m_err ps2m_set_resolution(uint8_t resolution);
#pragma aux ps2m_set_resolution = \
	"stc" \
	"mov ax, 0xC203"    /* Pointing device: set resolution (in bh) */ \
	"int 0x15" \
	"jnc end" \
	"fail: test ah, ah" \
	"jnz end" \
	"dec ah" \
	"end:" \
	__parm [bh] \
	__value [ah] \
	__modify [ax]

//   0 = 10 reports/sec
//   1 = 20 reports/sec
//   2 = 40 reports/sec
//   3 = 60 reports/sec
//   4 = 80 reports/sec
//   5 = 100 reports/sec (default)
//   6 = 200 reports/sec
static ps2m_err ps2m_set_sample_rate(uint8_t sample_rate);
#pragma aux ps2m_set_sample_rate = \
	"stc" \
	"mov ax, 0xC202"    /* Pointing device: set sample rate (in bh) */ \
	"int 0x15" \
	"jnc end" \
	"fail: test ah, ah" \
	"jnz end" \
	"dec ah" \
	"end:" \
	__parm [bh] \
	__value [ah] \
	__modify [ax]

static ps2m_err ps2m_set_scaling_factor(uint8_t scaling_factor);
#pragma aux ps2m_set_scaling_factor = \
	"stc" \
	"mov ax, 0xC206"    /* Pointing device: extended commands (with bh > 0, set scaling factor) */ \
	"int 0x15" \
	"jnc end" \
	"fail: test ah, ah" \
	"jnz end" \
	"dec ah" \
	"end:" \
	__parm [bh] \
	__value [ah] \
	__modify [ax]

static ps2m_err ps2m_set_callback(LPFN_PS2CALLBACK callback);
#pragma aux ps2m_set_callback = \
	"stc" \
	"mov ax, 0xC207"   /* Pointing device: set interrupt callback (in es:bx) */ \
	"int 0x15" \
	"jnc end" \
	"fail: test ah, ah" \
	"jnz end" \
	"dec ah" \
	"end:" \
	__parm [es bx] \
	__value [ah] \
	__modify [ax]

static ps2m_err ps2m_enable(bool enable);
#pragma aux ps2m_enable = \
	"test bh, bh"      /* Ensure enable is either 1 or 0 */ \
	"setnz bh" \
	"stc" \
	"mov ax, 0xC200"   /* Pointing device enable/disable (in bh) */ \
	"int 0x15" \
	"jnc end" \
	"fail: test ah, ah" \
	"jnz end" \
	"dec ah" \
	"end:" \
	__parm [bh] \
	__value [ah] \
	__modify [ax]

#endif
