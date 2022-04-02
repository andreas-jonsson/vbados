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
#include "int2fwin.h"

#define USE_VIRTUALBOX 1
#define USE_INT2F 1
#define TRACE_EVENTS 0

#define NUM_BUTTONS 3

#define GRAPHIC_CURSOR_WIDTH 16
#define GRAPHIC_CURSOR_HEIGHT 16

#define VERSION_MAJOR 0
#define VERSION_MINOR 3
#define REPORTED_VERSION_MAJOR 8
#define REPORTED_VERSION_MINOR 0x20

struct point {
	int16_t x, y;
};

typedef struct tsrdata {
	// TSR installation data
	/** Previous int33 ISR, storing it for uninstall. */
	void (__interrupt __far *prev_int33_handler)();
#if USE_INT2F
	void (__interrupt __far *prev_int2f_handler)();
#endif
	/** Whether to enable & use wheel mouse. */
	bool usewheel;

	// Video settings
	/** Current video mode. */
	uint8_t screen_mode;
	/** Active video page. */
	uint8_t screen_page;
	/** Max (virtual) coordinates of full screen in the current mode.
	 *  Used for rendering graphic cursor, mapping absolute coordinates,
	 *  and initializing the default min/max window. */
	struct point screen_max;
	/** Some graphic modes have pixel doubling, so the virtual coordinates
	 *  are double vs real framebuffer coordinates. */
	struct point screen_scale;

	// Detected mouse hardware
	/** Whether the current mouse has a wheel (and support is enabled). */
	bool haswheel;

	// Current mouse settings
	/** Mouse sensitivity/speed. */
	struct point mickeysPerLine; // mickeys per 8 pixels
	/** Mouse acceleration "double-speed threshold". */
	uint16_t doubleSpeedThreshold; // mickeys
	/** Current window min coordinates. */
	struct point min;
	/** Current window max coordinates. */
	struct point max;
	/** Current cursor visible counter. If >= 0, cursor should be shown. */
	int16_t visible_count;
	/** For text cursor, whether this is a software or hardware cursor. */
	uint8_t cursor_text_type;
	/** Masks for the text cursor. */
	uint16_t cursor_text_and_mask, cursor_text_xor_mask;
	/** Hotspot for the graphic cursor. */
	struct point cursor_hotspot;
	/** Masks for the graphic cursor. */
	uint16_t cursor_graphic[GRAPHIC_CURSOR_HEIGHT*2];

	// Current mouse status
	/** Current cursor position (in pixels). */
	struct point pos;
	/** Current remainder of movement that does not yet translate to an entire pixel
	 *  (8ths of pixel). */
	struct point pos_frac;
	/** Current delta movement (in mickeys) since the last report. */
	struct point delta;
	/** Current remainder of delta movement that does not yet translate to an entire mickey
	 *  Usually only when mickeysPerLine is not a multiple of 8. */
	struct point delta_frac;
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
	/** Total delta movement of the wheel since the last wheel report. */
	int16_t wheel_delta;
	/** Last position where the wheel was moved. */
	struct point wheel_last;

	// Cursor information
	/** Whether the cursor is currently displayed or not. */
	bool cursor_visible;
	/** The current position at which the cursor is displayed. */
	struct point cursor_pos;
	/** For text mode cursor, the character data that was displayed below the cursor. */
	uint16_t cursor_prev_char;
	/** For graphical mode cursor, contents of the screen that were displayed below
	 *  the cursor before the cursor was drawn. */
	uint8_t cursor_prev_graphic[GRAPHIC_CURSOR_WIDTH * GRAPHIC_CURSOR_HEIGHT];

	// Current handlers
	/** Address of the event handler. */
	void (__far *event_handler)();
	/** Events for which we should call the event handler. */
	uint16_t event_mask;

#if USE_INT2F
	/** Information that we pass to Windows 386 on startup. */
	win386_startup_info w386_startup;
	win386_instance_item w386_instance[2];
	/** Whether Windows 386 is rendering the cursor for us,
	 *  and therefore we should hide our own. */
	bool w386cursor : 1;
#endif

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

extern void __declspec(naked) __far int2f_isr(void);

extern LPTSRDATA __far get_tsr_data(bool installed);

/** This symbol is always at the end of the TSR segment */
extern int resident_end;

/** This is not just data, but the entire segment. */
static inline unsigned get_resident_size(void)
{
	return FP_OFF(&resident_end);
}

#endif
