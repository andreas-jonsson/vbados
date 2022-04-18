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

#ifndef INT15PS2_H
#define INT15PS2_H

#include <stdbool.h>
#include <stdint.h>

/** Standard PS/2 mouse IRQ. At least on VirtualBox. */
#define PS2_MOUSE_IRQ        12
/** The corresponding interrupt vector for IRQ 12. */
#define PS2_MOUSE_INT_VECTOR 0x74

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

enum ps2m_packet_size {
	PS2M_PACKET_SIZE_PLAIN = 3,
	PS2M_PACKET_SIZE_EXT   = 4,
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

/** Valid PS/2 mouse resolutions in DPI. */
enum ps2m_resolution {
	PS2M_RESOLUTION_25  = 0,
	PS2M_RESOLUTION_50  = 1,
	PS2M_RESOLUTION_100 = 2,
	PS2M_RESOLUTION_200 = 3
};

/** Valid PS/2 mouse sampling rates in Hz. */
enum ps2m_sample_rate {
	PS2M_SAMPLE_RATE_10  = 0,
	PS2M_SAMPLE_RATE_20  = 1,
	PS2M_SAMPLE_RATE_40  = 2,
	PS2M_SAMPLE_RATE_60  = 3,
	PS2M_SAMPLE_RATE_80  = 4,
	PS2M_SAMPLE_RATE_100 = 5,
	PS2M_SAMPLE_RATE_200 = 6
};

/** Invoked by the BIOS when there is a mouse event. */
typedef void (__far * LPFN_PS2CALLBACK)();

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

static ps2m_err ps2m_get_device_id(uint8_t __far *device_id);
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

/** Sends the magic sequence to switch the mouse to the IMPS2 protocol. */
static void ps2m_send_imps2_sequence(void)
{
	ps2m_set_sample_rate(PS2M_SAMPLE_RATE_200);
	ps2m_set_sample_rate(PS2M_SAMPLE_RATE_100);
	ps2m_set_sample_rate(PS2M_SAMPLE_RATE_80);
}

/** Detects whether we have a IMPS2 mouse with wheel support. */
static bool ps2m_detect_wheel(void)
{
	int err;
	uint8_t device_id;

	// Get the initial mouse device id
	err = ps2m_get_device_id(&device_id);
	if (err) {
		return false;
	}

	if (device_id == PS2M_DEVICE_ID_IMPS2) {
		// Already wheel
		return true;
	}

	if (device_id != PS2M_DEVICE_ID_PLAIN) {
		// TODO: Likely we have to accept more device IDs here
		dlog_print("Unknown initial mouse device_id=");
		dlog_printx(device_id);
		dlog_endline();
		return false;
	}

	// Send the knock sequence to activate the extended packet
	ps2m_send_imps2_sequence();

	// Now check if the device id has changed
	err = ps2m_get_device_id(&device_id);

	return err == 0 && device_id == PS2M_DEVICE_ID_IMPS2;
}

#endif /* INT15PS2_H */
