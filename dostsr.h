/*
 * VBMouse - DOS mouse driver resident part
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

#ifndef DOSTSR_H
#define DOSTSR_H

#include <stdbool.h>
#include <stdint.h>

#include "vbox.h"

#define USE_VIRTUALBOX 1

#define NUM_BUTTONS 3

#define DRIVER_VERSION_MAJOR 8
#define DRIVER_VERSION_MINOR 0x20

struct point {
	int16_t x, y;
};

typedef struct tsrdata {
	// TSR installation data
	/** Previous int33 ISR, storing it for uninstall. */
	void (__interrupt __far *prev_int33_handler)();

	// Video settings
	uint8_t screen_mode;
	uint8_t screen_page;
	bool screen_text_mode;
	struct point screen_max;

	// Current mouse settings
	struct point mickeysPerLine; // mickeys per 8 pixels
	uint16_t doubleSpeedThreshold; // mickeys
	struct point min;
	struct point max;
	int16_t visible_count;
	uint8_t cursor_text_type;
	uint16_t cursor_text_and_mask, cursor_text_xor_mask;
	struct point cursor_hotspot;
	uint16_t cursor_graphic[16+16];

	// Current mouse status
	/** Current cursor position (in pixels). */
	struct point pos;
	/** Current remainder of movement that does not yet translate to an entire pixel
	 *  (8ths of pixel right now). */
	struct point pos_frac;
	/** Current delta movement (in mickeys) since the last report. */
	struct point delta;
	/** Total mickeys moved in the last second. */
	uint16_t total_motion;
	/** Ticks when the above value was last reset. */
	uint16_t last_ticks;
	/** Current status of buttons (as bitfield). */
	uint16_t buttons;
	struct {
		struct buttoncounter {
			struct point last;
			uint16_t count;
		} pressed, released;
	} button[NUM_BUTTONS];
	/** Whether the cursor is currently displayed or not. */
	bool cursor_visible;
	struct point cursor_pos;
	uint16_t cursor_prev_char;

	// Current handlers
	void (__far *event_handler)();
	uint8_t event_mask;

#if USE_VIRTUALBOX
	/** VirtualBox is available. */
	bool vbavail : 1;
	/** Want to use the VirtualBox "host" cursor. */
	bool vbwantcursor : 1;
	/** Have VirtualBox absolute coordinates. */
	bool vbhaveabs : 1;
	struct vboxcomm vb;
#endif
} TSRDATA;

typedef TSRDATA * PTSRDATA;
typedef TSRDATA __far * LPTSRDATA;

extern void __declspec(naked) __far int33_isr(void);

extern LPTSRDATA __far get_tsr_data(bool installed);

extern int resident_end;

#endif
