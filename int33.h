/*
 * VBMouse - int33 API defines
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
#ifndef INT33_H
#define INT33_H

enum INT33_API {
	/** Reinitializes mouse hardware and resets mouse to default driver values.
	 *  On return, ax = 0xFFFF, bx = number of buttons. */
	INT33_RESET_MOUSE = 0,

	/** Increment the cursor visible counter. The cursor is shown when the counter is >= 0. */
	INT33_SHOW_CURSOR = 1,
	/** Decrement the cursor visible counter. The cursor is shown when the counter is >= 0. */
	INT33_HIDE_CURSOR = 2,

	/** Gets the current mouse position and button status.
	 *  @returns cx = horizontal pos, dx = vertical pos,
	 *           bx = button status (bitfield, lsb = button 0/left). */
	INT33_GET_MOUSE_POSITION = 3,
	/** Sets the current mouse position.
	 *  @param cx = horizontal pos, dx = vertical pos. */
	INT33_SET_MOUSE_POSITION = 4,

	/** Gets the number of times a mouse button was pressed since the last call.
	 *  @param bx = button number
	 *  @returns ax = current all button status (bitfield),
	 *           bx = count of times button was pressed,
	 *           cx = horizontal pos at last press,
	 *           dx = vetical pos at last press */
	INT33_GET_BUTTON_PRESSED_COUNTER = 5,
	/** Gets the number of times a mouse button was released since the last call.
	 *  @param bx = button number
	 *  @returns ax = current all button status (bitfield),
	 *           bx = count of times button was released,
	 *           cx = horizontal pos at last release,
	 *           dx = vetical pos at last release */
	INT33_GET_BUTTON_RELEASED_COUNTER = 6,

	/** @param cx = minimum horizontal position, dx = maximum horizontal position. */
	INT33_SET_HORIZONTAL_WINDOW = 7,

	/** @param cx = minimum vertical position, dx = maximum vertical position. */
	INT33_SET_VERTICAL_WINDOW = 8,

	/** Configures graphicsmode mouse cursor.
	 *  @param bx horizontal hotspot , cx vertical hotspot
	 *  @param es:dx address of cursor shape bitmap */
	INT33_SET_GRAPHICS_CURSOR = 9,

	/** Configures textmode mouse cursor.
	 *  @param bx 1 for hardware cursor, 0 for software cursor
	 *  @param cx for software cursor, AND mask; for hardware cursor, start scanline.
	 *  @param dx for software cursor, XOR mask; for hardware cursor, end scanline. */
	INT33_SET_TEXT_CURSOR = 0xA,

	/** Gets total relative mouse distance (in mickeys) since the last call.
	 *  @param cx horizontal distance , dx vertical distance */
	INT33_GET_MOUSE_MOTION = 0xB,

	/** Replaces the current routine for event handling.
	 *  @param es:dx address of the new event handler
	 *  @param cx event mask (see INT33_EVENT_MASK) */
	INT33_SET_EVENT_HANDLER = 0xC,

	/** Sets the mickey-per-8-pixels ratio, controlling the cursor speed.
	 *  @param cx horizontal speed, dx vertical speed */
	INT33_SET_MOUSE_SPEED = 0xF,

	/** If the mouse is moved more than this mickeys in one second,
	 *  the mouse motion is doubled.
	 *  @param cx doubling threshold (mickeys per second) */
	INT33_SET_SPEED_DOUBLE_THRESHOLD = 0x13,

	/** Replaces the current routine for event handling,
	 *  but also returns the old one.
	 *  @param es:dx address of the new event handler
	 *  @param cx event mask (see INT33_EVENT_MASK)
	 *  @return es:dx address of the previous event handler. */
	INT33_EXCHANGE_EVENT_HANDLER = 0x14,

	/** Query the size of the memory required to save a copy of the mouse status.
	 *  @return bx buffer needed. */
	INT33_GET_MOUSE_STATUS_SIZE = 0x15,

	INT33_SAVE_MOUSE_STATUS = 0x16,
	INT33_LOAD_MOUSE_STATUS = 0x17,

	/** Sets both speed and speed-doubling threshold in one call.
	 *  @param bx horizontal speed, cx vertical speed
	 *  @param dx doubling threshold (mickeys per second). */
	INT33_SET_MOUSE_SENSITIVITY = 0x1A,
	/** Gets current speed and speed-doubling threshold.
	 *  @return bx horizontal speed, cx vertical speed
	 *  @return dx doubling threshold (mickeys per second). */
	INT33_GET_MOUSE_SENSITIVITY = 0x1B,

	/** Resets mouse to default driver values.
	 *  On return, ax = 0xFFFF, bx = number of buttons. */
	INT33_RESET_SETTINGS = 0x21,

	/** Gets language for messages.
	 *  @return bx language code. */
	INT33_GET_LANGUAGE = 0x23,

	/** Gets driver information.
	 *  On return, bx = major:minor version, ch = INT33_MOUSE_TYPE, CL = irq number (or 0). */
	INT33_GET_DRIVER_INFO = 0x24,

	/** Gets the current video mode maximum X & Y positions.
	 *  @return cx max x, dx max y. */
	INT33_GET_MAX_COORDINATES = 0x26,

	/** Gets the current window coordinates (set by INT33_SET_HORIZONTAL_WINDOW).
	 *  @return ax min_x, bx min_y, cx max_x, d max_y. */
	INT33_GET_WINDOW = 0x31,

	// Wheel API Extensions:
	INT33_GET_CAPABILITIES = 0x11,

	// Our internal API functions:
	/** Obtains a pointer to the driver's data. */
	INT33_GET_TSR_DATA = 0x7f
};

enum INT33_CAPABILITY_BITS {
	INT33_CAPABILITY_MOUSE_API = 1 << 0
};

#define INT33_WHEEL_API_MAGIC 'WM'
#define INT33_MOUSE_FOUND 0xFFFF

enum INT33_MOUSE_TYPE {
	INT33_MOUSE_TYPE_BUS = 1,
	INT33_MOUSE_TYPE_SERIAL = 2,
	INT33_MOUSE_TYPE_INPORT = 3,
	INT33_MOUSE_TYPE_PS2 = 4,
	INT33_MOUSE_TYPE_HP = 5
};

enum INT33_BUTTON_MASK {
	INT33_BUTTON_MASK_LEFT   = 1 << 0,
	INT33_BUTTON_MASK_RIGHT  = 1 << 1,
	INT33_BUTTON_MASK_CENTER = 1 << 2
};

enum INT33_EVENT_MASK {
	INT33_EVENT_MASK_MOVEMENT               = 1 << 0,
	INT33_EVENT_MASK_LEFT_BUTTON_PRESSED    = 1 << 1,
	INT33_EVENT_MASK_LEFT_BUTTON_RELEASED   = 1 << 2,
	INT33_EVENT_MASK_RIGHT_BUTTON_PRESSED   = 1 << 3,
	INT33_EVENT_MASK_RIGHT_BUTTON_RELEASED  = 1 << 4,
	INT33_EVENT_MASK_CENTER_BUTTON_PRESSED  = 1 << 5,
	INT33_EVENT_MASK_CENTER_BUTTON_RELEASED = 1 << 6,

	// Wheel API Extensions:
	/** Wheel mouse movement. */
	INT33_EVENT_MASK_WHEEL_MOVEMENT         = 1 << 7,

	// Absolute API extensions:
	/** The source of the event is an absolute pointing device. */
	INT33_EVENT_MASK_ABSOLUTE               = 1 << 8,

	INT33_EVENT_MASK_ALL                    = 0xFFFF
};

#pragma aux INT33_CB far loadds parm [ax] [bx] [cx] [dx] [si] [di]

static uint16_t int33_reset(void);
#pragma aux int33_reset = \
	"mov ax, 0x0" \
	"int 0x33" \
	__value [ax]

static void int33_set_horizontal_window(int16_t min, int16_t max);
#pragma aux int33_set_horizontal_window = \
	"mov ax, 0x7" \
	"int 0x33" \
	__parm [cx] [dx] \
	__modify [ax]

static void int33_set_vertical_window(int16_t min, int16_t max);
#pragma aux int33_set_vertical_window = \
	"mov ax, 0x8" \
	"int 0x33" \
	__parm [cx] [dx] \
	__modify [ax]

static void int33_set_event_handler(uint16_t event_mask, void (__far *handler)());
#pragma aux int33_set_event_handler = \
	"mov ax, 0xC" \
	"int 0x33" \
	__parm [cx] [es dx] \
	__modify [ax]

static uint16_t int33_get_driver_version(void);
#pragma aux int33_get_driver_version = \
	"mov bx, 0" \
	"mov ax, 0x24" \
	"int 0x33" \
	"cmp ax, -1" \
	"jne end" \
	"error:" \
	"mov bx, 0" \
	"end:" \
	__value [bx] \
	__modify [ax bx cx dx]

#endif
