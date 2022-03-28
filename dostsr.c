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

#include <i86.h>

#include "dlog.h"
#include "ps2.h"
#include "int10vga.h"
#include "int33.h"
#include "dostsr.h"

TSRDATA data;

static const uint16_t default_cursor_graphic[] = {
    0x3FFF, 0x1FFF, 0x0FFF, 0x07FF,
    0x03FF, 0x01FF, 0x00FF, 0x007F,
    0x003F, 0x001F, 0x01FF, 0x00FF,
    0x30FF, 0xF87F, 0xF87F, 0xFCFF,
    0x0000, 0x4000, 0x6000, 0x7000,
    0x7800, 0x7C00, 0x7E00, 0x7F00,
    0x7F80, 0x7C00, 0x6C00, 0x4600,
    0x0600, 0x0300, 0x0300, 0x0000
};

/** This is going to end up at offset 0 of our segment,
 *  have something here in case we end up calling a NULL near pointer by mistake. */
void tsr_null(void)
{
	breakpoint();
}

static void bound_position_to_window(void)
{
	if (data.pos.x < data.min.x) data.pos.x = data.min.x;
	if (data.pos.x > data.max.x) data.pos.x = data.max.x;
	if (data.pos.y < data.min.y) data.pos.y = data.min.y;
	if (data.pos.y > data.max.y) data.pos.y = data.max.y;
}

/** Refreshes cursor position and visibility. */
static void refresh_cursor(void)
{
	bool should_show = data.visible_count >= 0;

#if USE_VIRTUALBOX
	if (data.vbwantcursor) {
		int err = 0;
		if (should_show != data.cursor_visible) {
			int err = vbox_set_pointer_visible(&data.vb, should_show);
			if (err == 0 && data.vbhaveabs) {
				data.cursor_visible = should_show;
			}
		}
		if (err == 0 & data.vbhaveabs) {
			// No need to show the cursor; VirtualBox is already showing it for us.
			return;
		}
	}
#endif

	if (data.screen_text_mode) {
		if (data.cursor_visible) {
			// Restore the character under the old position of the cursor
			uint16_t __far *ch = get_video_char(data.screen_page,
			                                    data.cursor_pos.x / 8, data.cursor_pos.y / 8);
			*ch = data.cursor_prev_char;
			data.cursor_visible = false;
		}

		if (should_show) {
			uint16_t __far *ch = get_video_char(data.screen_page,
			                                    data.pos.x / 8, data.pos.y / 8);
			data.cursor_prev_char = *ch;
			data.cursor_pos = data.pos;
			*ch = (*ch & data.cursor_text_and_mask) ^ data.cursor_text_xor_mask;
			data.cursor_visible = true;
		}
	} else {
		dlog_puts("Graphic mode cursor is not implemented");
	}
}

static void load_cursor(void)
{
#if USE_VIRTUALBOX
	if (data.vbwantcursor) {
		VMMDevReqMousePointer *req = (VMMDevReqMousePointer *) data.vb.buf;
		const unsigned width = 16, height = 16;
		unsigned and_mask_size = (width + 7) / 8 * height;
		unsigned xor_mask_size = width * height * 4;
		unsigned data_size = and_mask_size + xor_mask_size;
		unsigned full_size = MAX(sizeof(VMMDevReqMousePointer), 24 + 20 + data_size);
		unsigned int offset = 0, y, x;

		bzero(req, full_size);

		req->header.size = full_size;
		req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
		req->header.requestType = VMMDevReq_SetPointerShape;
		req->header.rc = -1;

		req->fFlags = VBOX_MOUSE_POINTER_SHAPE;
		req->xHot = BOUND(data.cursor_hotspot.x, 0, width);
		req->yHot = BOUND(data.cursor_hotspot.y, 0, height);
		req->width = width;
		req->height = height;

		// Just byteswap the and mask
		for (y = 0; y < height; ++y) {
			uint16_t line = data.cursor_graphic[y];
			req->pointerData[(y*2)]   = (line >> 8) & 0xFF;
			req->pointerData[(y*2)+1] = line & 0xFF;
		}
		offset += and_mask_size;

		// Store the XOR mask in "RGBA" format.
		for (y = 0; y < height; ++y) {
			uint16_t line = data.cursor_graphic[height + y];

			for (x = 0; x < width; ++x) {
				unsigned int pos = offset + (y * width * 4) + (x*4) + 0;
				uint8_t val = (line & 0x8000) ? 0xFF : 0;

				req->pointerData[pos + 0] = val;
				req->pointerData[pos + 1] = val;
				req->pointerData[pos + 2] = val;
				req->pointerData[pos + 3] = 0;

				line <<= 1;
			}
		}

		dlog_puts("Loading cursor to VBox");

		vbox_send_request(data.vb.iobase, data.vb.buf_physaddr);

		if (req->header.rc != 0) {
			dlog_puts("Could not send cursor to VirtualBox");
		}

		// VirtualBox shows the cursor even if we don't want;
		// rehide if necessary.
		data.cursor_visible = true;
		refresh_cursor();
	}
#endif
}

static void refresh_video_info(void)
{
	uint8_t screen_columns;
	uint8_t video_page;
	uint8_t mode = int10_get_video_mode(&screen_columns, &video_page);
	bool mode_change = mode != data.screen_mode;

	if (mode_change && data.cursor_visible) {
		// Assume cursor is lost
		data.cursor_visible = false;
	}

	dlog_print("Current video mode=");
	dlog_printx(mode);
	dlog_print(" with cols=");
	dlog_printd(screen_columns, 10);
	dlog_endline();

	data.screen_mode = mode;
	data.screen_page = video_page;

	switch (mode) {
	case 0:
	case 1:
	case 2:
	case 3: /* VGA text modes with 25 rows and variable columns */
		data.screen_max.x = (screen_columns * 8) - 1;
		data.screen_max.y = (25 * 8) - 1;
		data.screen_text_mode = true;
		break;

	case 4:
	case 5:
	case 6: /* Graphic CGA modes */
		data.screen_max.x = 640 - 1;
		data.screen_max.y = 200 - 1;
		data.screen_text_mode = false;
		break;

	case 0x11:
	case 0x12:
	case 0x13: /* Graphical 640x480 modes. */
		data.screen_max.x = 640 - 1;
		data.screen_max.y = 480 - 1;
		data.screen_text_mode = false;
		break;

	default:
		data.screen_max.x = 0;
		data.screen_max.y = 0;
		data.screen_text_mode = false;
		break;
	}

	dlog_print(" screen_x=");
	dlog_printd(data.screen_max.x, 10);
	dlog_print(" y=");
	dlog_printd(data.screen_max.y, 10);
	dlog_endline();
}

static void call_event_handler(void (__far *handler)(), uint16_t events,
                               uint16_t buttons, int16_t x, int16_t y,
                               int16_t delta_x, int16_t delta_y)
{
#if TOO_VERBOSE
	dlog_print("calling event handler events=");
	dlog_printx(events);
	dlog_print(" buttons=");
	dlog_printx(buttons);
	dlog_print(" x=");
	dlog_printd(x, 10);
	dlog_print(" y=");
	dlog_printd(y, 10);
	dlog_print(" dx=");
	dlog_printd(delta_x, 10);
	dlog_print(" dy=");
	dlog_printd(delta_y, 10);
	dlog_endline();
#endif

	__asm {
		mov ax, [events]
		mov bx, [buttons]
		mov cx, [x]
		mov dx, [y]
		mov si, [delta_x]
		mov di, [delta_y]

		call dword ptr [handler]
	}
}

static void handle_mouse_event(uint8_t buttons, bool absolute, int x, int y, int z)
{
	uint16_t events = 0;
	int i;

#if TOO_VERBOSE
	dlog_print("handle mouse event");
	if (absolute) dlog_print(" absolute");
	dlog_print(" buttons=");
	dlog_printx(buttons);
	dlog_print(" x=");
	dlog_printd(x, 10);
	dlog_print(" y=");
	dlog_printd(y, 10);
	dlog_endline();
#endif

	if (absolute) {
		// Absolute movement: x,y are in screen pixels units
		// Translate to mickeys for delta movement
		// TODO: we are not storing the remainder here (e.g. delta_frac),
		// but does anyone care about it ?
		data.delta.x += ((x - data.pos.x) * 8) / data.mickeysPerLine.x;
		data.delta.y += ((y - data.pos.y) * 8) / data.mickeysPerLine.y;

		// Store the new absolute position
		data.pos.x = x;
		data.pos.y = y;
		data.pos_frac.x = 0;
		data.pos_frac.y = 0;
	} else {
		// Relative movement: x,y are in mickeys
		uint16_t ticks = bda_get_tick_count_lo();
		unsigned ax = ABS(x), ay = ABS(y);

		// Check if around one second has passed
		if ((ticks - data.last_ticks) >= 18) {
			data.total_motion = 0;
			data.last_ticks = ticks;
		}

		// If more than the double speed threshold has been moved in the last second,
		// double the speed
		data.total_motion += ax * ax + ay * ay;
		if (data.total_motion > data.doubleSpeedThreshold * data.doubleSpeedThreshold) {
			x *= 2;
			y *= 2;
		}

		data.delta.x += x;
		data.delta.y += y;

		data.pos.x += scalei_rem(x, 8, data.mickeysPerLine.x, &data.pos_frac.x);
		data.pos.y += scalei_rem(y, 8, data.mickeysPerLine.y, &data.pos_frac.y);
	}
	bound_position_to_window();

	// TODO: Wheel
	(void) z;

	// Report movement if there was any
	if (data.delta.x || data.delta.y) {
		events |= INT33_EVENT_MASK_MOVEMENT;
	}

	// Update button status
	for (i = 0; i < NUM_BUTTONS; ++i) {
		uint8_t btn = 1 << i;
		uint8_t evt = 0;
		if ((buttons & btn) && !(data.buttons & btn)) {
			// Button pressed
			evt = 1 << (1 + (i * 2)); // Press event mask
			data.button[i].pressed.count++;
			data.button[i].pressed.last.x = data.pos.x;
			data.button[i].pressed.last.y = data.pos.y;
		} else if (!(buttons & btn) && (data.buttons & btn)) {
			// Button released
			evt = 1 << (2 + (i * 2)); // Release event mask
			data.button[i].released.count++;
			data.button[i].released.last.x = data.pos.x;
			data.button[i].released.last.y = data.pos.y;
		}
		if (evt & data.event_mask) {
			events |= evt;
		}
	}
	data.buttons = buttons;

	refresh_cursor();

	events &= data.event_mask;

	if (data.event_handler && events) {
		call_event_handler(data.event_handler, events,
		                   buttons, data.pos.x, data.pos.y, data.delta.x, data.delta.y);

		// If we succesfully reported movement, clear it to avoid reporting movement again
		if (events & INT33_EVENT_MASK_MOVEMENT) {
			//data.delta.x = 0;
			//data.delta.y = 0;
		}
	}
}

static void __far ps2_mouse_callback(uint8_t status, uint8_t x, uint8_t y, uint8_t z)
{
#pragma aux (PS2_CB) ps2_mouse_callback

	int sx =   status & PS2M_STATUS_X_NEG ? 0xFF00 | x : x;
	int sy = -(status & PS2M_STATUS_Y_NEG ? 0xFF00 | y : y);
	bool abs = false;

#if TOO_VERBOSE
	dlog_print("ps2 callback status=");
	dlog_printx(status);
	dlog_print(" sx=");
	dlog_printd(sx, 10);
	dlog_print(" sy=");
	dlog_printd(sy, 10);
	dlog_print(" z=");
	dlog_printd(z, 10);
	dlog_endline();
#endif

#if USE_VIRTUALBOX
	if (data.vbavail) {
		uint16_t vbx, vby;
		if ((vbox_get_mouse(&data.vb, &abs, &vbx, &vby) == 0) && abs) {
			sx = scaleu(vbx, 0xFFFFU, MAX(data.max.x, data.screen_max.x));
			sy = scaleu(vby, 0xFFFFU, MAX(data.max.y, data.screen_max.y));
			data.vbhaveabs = true;
		} else {
			// VirtualBox does not support absolute coordinates,
			// or user has disabled them.
			data.vbhaveabs = false;
		}
	}
#endif

	handle_mouse_event(status & (PS2M_STATUS_BUTTON_1 | PS2M_STATUS_BUTTON_2 | PS2M_STATUS_BUTTON_3),
	                   abs, sx, sy, z);
}

#if USE_VIRTUALBOX
static void enable_vbox_absolute(bool enable)
{
	data.vbhaveabs = false;

	if (data.vbavail) {
		int err = vbox_set_mouse(&data.vb, enable, false);
		if (enable && !err) {
			dlog_puts("VBox absolute mouse enabled");
			data.vbhaveabs = true;
		} else if (!enable) {
			dlog_puts("VBox absolute mouse disabled");
		}
	}
}
#endif

static void reset_mouse_hardware()
{
	ps2m_enable(false);

#if USE_VIRTUALBOX
	// By default, enable the integration
	enable_vbox_absolute(true);
	load_cursor();
#endif

	ps2m_init(PS2_MOUSE_PLAIN_PACKET_SIZE);
	ps2m_reset();

	ps2m_set_resolution(3);     // 3 = 200 dpi, 8 counts per millimeter
	ps2m_set_sample_rate(4);    // 4 = 80 reports per second
	ps2m_set_scaling_factor(1); // 1 = 1:1 scaling

	ps2m_set_callback(ps2_mouse_callback);

	ps2m_enable(true);
}

static void reset_mouse_settings()
{
	data.event_mask = 0;
	data.event_handler = 0;

	data.mickeysPerLine.x = 8;
	data.mickeysPerLine.y = 16;
	data.doubleSpeedThreshold = 64;
	data.min.x = 0;
	data.max.x = data.screen_max.x;
	data.min.y = 0;
	data.max.y = data.screen_max.y;
	data.visible_count = -1;
	data.cursor_text_type = 0;
	data.cursor_text_and_mask = 0xFFFFU;
	data.cursor_text_xor_mask = 0x7700U;
	data.cursor_hotspot.x = 0;
	data.cursor_hotspot.y = 0;
	nnmemcpy(data.cursor_graphic, default_cursor_graphic, 32+32);

	refresh_cursor(); // This will hide the cursor and update data.cursor_visible
}

static void reset_mouse_state()
{
	int i;
	data.pos.x = data.min.x;
	data.pos.y = data.min.y;
	data.delta.x = 0;
	data.delta.y = 0;
	data.delta.x = 0;
	data.delta.y = 0;
	data.buttons = 0;
	for (i = 0; i < NUM_BUTTONS; i++) {
		data.button[i].pressed.count = 0;
		data.button[i].pressed.last.x = 0;
		data.button[i].pressed.last.y = 0;
		data.button[i].released.count = 0;
		data.button[i].released.last.x = 0;
		data.button[i].released.last.y = 0;
	}
	data.cursor_visible = false;
	data.cursor_pos.x = 0;
	data.cursor_pos.y = 0;
	data.cursor_prev_char = 0;
}

static void return_clear_button_counter(union INTPACK __far *r, struct buttoncounter *c)
{
	r->x.cx = c->last.x;
	r->x.dx = c->last.y;
	r->x.bx = c->count;
	c->last.x = 0;
	c->last.y = 0;
	c->count = 0;
}

static void int33_handler(union INTPACK r)
#pragma aux int33_handler "*" parm caller [] modify [ax bx cx dx es]
{
	switch (r.x.ax) {
	case INT33_RESET_MOUSE:
		dlog_puts("Mouse reset");
		refresh_video_info();
		reset_mouse_settings();
		reset_mouse_hardware();
		reset_mouse_state();
		r.x.ax = INT33_MOUSE_FOUND;
		r.x.bx = NUM_BUTTONS;
		break;
	case INT33_SHOW_CURSOR:
		data.visible_count++;
		refresh_cursor();
		break;
	case INT33_HIDE_CURSOR:
		data.visible_count--;
		refresh_cursor();
		break;
	case INT33_GET_MOUSE_POSITION:
		r.x.cx = data.pos.x;
		r.x.dx = data.pos.y;
		r.x.bx = data.buttons;
		break;
	case INT33_SET_MOUSE_POSITION:
		data.pos.x = r.x.cx;
		data.pos.y = r.x.dx;
		data.delta.x = 0;
		data.delta.y = 0;
		bound_position_to_window();
		break;
	case INT33_GET_BUTTON_PRESSED_COUNTER:
		r.x.ax = data.buttons;
		return_clear_button_counter(&r, &data.button[MIN(r.x.bx, NUM_BUTTONS - 1)].pressed);
		break;
	case INT33_GET_BUTTON_RELEASED_COUNTER:
		r.x.ax = data.buttons;
		return_clear_button_counter(&r, &data.button[MIN(r.x.bx, NUM_BUTTONS - 1)].released);
		break;
	case INT33_SET_HORIZONTAL_WINDOW:
		dlog_print("Mouse set horizontal window [");
		dlog_printd(r.x.cx, 10);
		dlog_putc(',');
		dlog_printd(r.x.dx, 10);
		dlog_puts("]");
		// Recheck in case someone changed the video mode
		refresh_video_info();
		data.min.x = r.x.cx;
		data.max.x = r.x.dx;
		bound_position_to_window();
		break;
	case INT33_SET_VERTICAL_WINDOW:
		dlog_print("Mouse set vertical window [");
		dlog_printd(r.x.cx, 10);
		dlog_putc(',');
		dlog_printd(r.x.dx, 10);
		dlog_puts("]");
		refresh_video_info();
		data.min.y = r.x.cx;
		data.max.y = r.x.dx;
		bound_position_to_window();
		break;
	case INT33_SET_GRAPHICS_CURSOR:
		dlog_puts("Mouse set graphics cursor");
		data.cursor_hotspot.x = r.x.bx;
		data.cursor_hotspot.y = r.x.cx;
		fnmemcpy(data.cursor_graphic, MK_FP(r.x.es, r.x.dx), 64);
		load_cursor();
		refresh_cursor();
		break;
	case INT33_SET_TEXT_CURSOR:
		dlog_print("Mouse set text cursor ");
		dlog_printd(r.x.bx, 10);
		dlog_endline();
		data.cursor_text_type = r.x.bx;
		data.cursor_text_and_mask = r.x.cx;
		data.cursor_text_xor_mask = r.x.dx;
		refresh_cursor();
		break;
	case INT33_GET_MOUSE_MOTION:
		r.x.cx = data.delta.x;
		r.x.dx = data.delta.y;
		data.delta.x = 0;
		data.delta.y = 0;
#if USE_VIRTUALBOX
		// Likely this means we need a relative mouse, or we will get out of sync
		//if (data.vbabs) enable_vbox_absolute(false);
#endif
		break;
	case INT33_SET_EVENT_HANDLER:
		dlog_puts("Mouse set event handler");
		data.event_mask = r.x.cx;
		data.event_handler = MK_FP(r.x.es, r.x.dx);
		break;
	case INT33_SET_MOUSE_SPEED:
		dlog_print("Mouse set speed x=");
		dlog_printd(r.x.cx, 10);
		dlog_print(" y=");
		dlog_printd(r.x.dx, 10);
		dlog_endline();
		data.mickeysPerLine.x = r.x.cx;
		data.mickeysPerLine.y = r.x.dx;
		break;
	case INT33_SET_SPEED_DOUBLE_THRESHOLD:
		dlog_print("Mouse set speed double threshold=");
		dlog_printd(r.x.dx, 10);
		dlog_endline();
		data.doubleSpeedThreshold = r.x.dx;
		break;
	case INT33_EXCHANGE_EVENT_HANDLER:
		dlog_puts("Mouse exchange event handler");
		data.event_mask = r.x.cx;
	    {
		    void (__far *prev_event_handler)() = data.event_handler;
			data.event_handler = MK_FP(r.x.es, r.x.dx);
			r.x.es = FP_SEG(prev_event_handler);
			r.x.dx = FP_OFF(prev_event_handler);
	    }
		break;
	case INT33_GET_MOUSE_STATUS_SIZE:
		dlog_puts("Mouse get status size");
		r.x.bx = sizeof(TSRDATA);
		break;
	case INT33_SAVE_MOUSE_STATUS:
		dlog_puts("Mouse save status");
		nfmemcpy(MK_FP(r.x.es, r.x.dx), &data, sizeof(TSRDATA));
		break;
	case INT33_LOAD_MOUSE_STATUS:
		dlog_puts("Mouse load status");
		fnmemcpy(&data, MK_FP(r.x.es, r.x.dx), sizeof(TSRDATA));
		break;
	case INT33_SET_MOUSE_SENSITIVITY:
		dlog_print("Mouse set speed x=");
		dlog_printd(r.x.bx, 10);
		dlog_print(" y=");
		dlog_printd(r.x.cx, 10);
		dlog_print(" threshold=");
		dlog_printd(r.x.dx, 10);
		dlog_endline();
		data.mickeysPerLine.x = r.x.bx;
		data.mickeysPerLine.y = r.x.cx;
		data.doubleSpeedThreshold = r.x.dx;
		break;
	case INT33_GET_MOUSE_SENSITIVITY:
		r.x.bx = data.mickeysPerLine.x;
		r.x.cx = data.mickeysPerLine.y;
		r.x.dx = data.doubleSpeedThreshold;
		break;
	case INT33_RESET_SETTINGS:
		dlog_puts("Mouse reset settings");
		refresh_video_info();
		reset_mouse_settings();
		reset_mouse_state();
		r.x.ax = INT33_MOUSE_FOUND;
		r.x.bx = NUM_BUTTONS;
		break;
	case INT33_GET_LANGUAGE:
		r.x.bx = 0;
		break;
	case INT33_GET_DRIVER_INFO:
		dlog_puts("Mouse get driver info");
		r.h.bh = DRIVER_VERSION_MAJOR;
		r.h.bl = DRIVER_VERSION_MINOR;
		r.h.ch = INT33_MOUSE_TYPE_PS2;
		r.h.cl = 0;
		break;
	case INT33_GET_MAX_COORDINATES:
		r.x.cx = data.screen_max.x;
		r.x.dx = data.screen_max.y;
		break;
	case INT33_GET_WINDOW:
		r.x.ax = data.min.x;
		r.x.bx = data.min.y;
		r.x.cx = data.max.x;
		r.x.dx = data.max.y;
		break;
	case INT33_GET_TSR_DATA:
		dlog_puts("Get TSR data");
		r.x.es = FP_SEG(&data);
		r.x.di = FP_OFF(&data);
		break;
	default:
		dlog_print("Unknown mouse function ax=");
		dlog_printx(r.x.ax);
		dlog_endline();
		break;
	}
}

// Can't use __interrupt, it makes a call to GETDS on the runtime
void __declspec(naked) __far int33_isr(void)
{
	__asm {
		pusha
		push ds
		push es
		push fs
		push gs

		mov bp, sp
		push cs
		pop ds

		call int33_handler

		pop gs
		pop fs
		pop es
		pop ds
		popa
		iret
	}
}

static LPTSRDATA int33_get_tsr_data(void);
#pragma aux int33_get_tsr_data = \
	"xor ax, ax" \
	"mov es, ax" \
	"mov di, ax" \
	"mov ax, 0x7f" \
	"int 0x33"   \
	__value [es di] \
	__modify [ax]

static LPTSRDATA local_get_tsr_data(void);
#pragma aux local_get_tsr_data = \
	"mov ax, cs" \
	"mov es, ax" \
	"mov di, offset data" \
	__value [es di] \
	__modify [ax]

LPTSRDATA __far get_tsr_data(bool installed)
{
	if (installed) {
		return int33_get_tsr_data();
	} else {
		return local_get_tsr_data();
	}
}

int resident_end;
