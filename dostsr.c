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

#include <stdlib.h>
#include <string.h>
#include <i86.h>

#include "dlog.h"
#include "ps2.h"
#include "int10vga.h"
#include "int2fwin.h"
#include "int33.h"
#include "vbox.h"
#include "vmware.h"
#include "dostsr.h"

#define MSB_MASK 0x8000U

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

/** Constraint current mouse position to the user-set window. */
static void bound_position_to_window(void)
{
	if (data.pos.x < data.min.x) data.pos.x = data.min.x;
	if (data.pos.x > data.max.x) data.pos.x = data.max.x;
	if (data.pos.y < data.min.y) data.pos.y = data.min.y;
	if (data.pos.y > data.max.y) data.pos.y = data.max.y;
}

/** Constraints coordinate value to the desired granularity,
 *  which must be a power of two. */
static inline int16_t snap_to_grid(int16_t val, int16_t granularity)
{
	// Build a bitmask that masks away the undesired low-order bits
	return val & (-granularity);
}

static void hide_text_cursor(void)
{
	// Restore the character under the old position of the cursor
	uint16_t __far *ch = get_video_char(&data.video_mode,
	                                    data.cursor_pos.x / 8, data.cursor_pos.y / 8);
	*ch = data.cursor_prev_char;
	data.cursor_visible = false;
}

static void show_text_cursor(void)
{
	uint16_t __far *ch = get_video_char(&data.video_mode,
	                                    data.pos.x / 8, data.pos.y / 8);
	data.cursor_prev_char = *ch;
	data.cursor_pos = data.pos;
	*ch = (*ch & data.cursor_text_and_mask) ^ data.cursor_text_xor_mask;
	data.cursor_visible = true;
}

/** Given the new position of the cursor, compute the both our source
 *  (i.e. the mouse cursor shape) and target clipping areas.
 *  @param cursor_pos cursor position
 *  @param start top left corner of clipping box in screen
 *  @param size size of the clipping box in screen as well as cursor shape
 *  @param offset top left corner of clipping box in mouse cursor shape */
static bool get_graphic_cursor_area(struct point __far *cursor_pos,
                                    struct point __far *start,
                                    struct point __far *size,
                                    struct point __far *offset)
{
	start->x = (cursor_pos->x / data.screen_scale.x) - data.cursor_hotspot.x;
	start->y = (cursor_pos->y / data.screen_scale.y) - data.cursor_hotspot.y;
	size->x = GRAPHIC_CURSOR_WIDTH;
	size->y = GRAPHIC_CURSOR_HEIGHT;
	offset->x = 0;
	offset->y = 0;

	// Start clipping around
	// Cursor is left/top of visible area
	if (start->x <= -size->x) {
		return false;
	} else if (start->x < 0) {
		offset->x += -start->x;
		size->x   -= -start->x;
		start->x = 0;
	}
	if (start->y <= -size->y) {
		return false;
	} else if (start->y < 0) {
		offset->y += -start->y;
		size->y   -= -start->y;
		start->y = 0;
	}
	// Cursor is right/bottom of visible area
	if (start->x > data.video_mode.pixels_width) {
		return false; // Don't render cursor
	} else if (start->x + size->x > data.video_mode.pixels_width) {
		size->x -= (start->x + size->x) - data.video_mode.pixels_width;
	}
	if (start->y > data.video_mode.pixels_height) {
		return false;
	} else if (start->y + size->y > data.video_mode.pixels_height) {
		size->y -= (start->y + size->y) - data.video_mode.pixels_height;
	}

	return true;
}

static inline uint8_t * get_prev_graphic_cursor_scanline(unsigned bytes_per_line,
                                                         unsigned num_lines,
                                                         unsigned plane,
                                                         unsigned y)
{
	return &data.cursor_prev_graphic[((plane * num_lines) + y) * bytes_per_line];
}

static inline uint16_t get_graphic_cursor_and_mask_line(unsigned y)
{
	return data.cursor_graphic[y];
}

static inline uint16_t get_graphic_cursor_xor_mask_line(unsigned y)
{
	return data.cursor_graphic[GRAPHIC_CURSOR_HEIGHT + y];
}

/** Compute the total number of bytes between start and end pixels,
 *  rounding up as necessary to cover all bytes. */
static inline unsigned get_scanline_segment_bytes(unsigned bits_per_pixel, unsigned start, unsigned size)
{
	// Get starting byte (round down)
	unsigned start_byte = (start * bits_per_pixel) / 8;
	// Get end byte (round up)
	unsigned end_byte = (((start + size) * bits_per_pixel) + (8-1)) / 8;

	return end_byte - start_byte;
}

/** Creates a bitmask that extracts the topmost N bits of a byte. */
static inline uint8_t build_pixel_mask(unsigned bits_per_pixel)
{
	if      (bits_per_pixel == 1) return 0x80;
	else if (bits_per_pixel == 2) return 0xC0;
	else if (bits_per_pixel == 4) return 0xF0;
	else                          return 0xFF;
}

/** Hides the graphical mouse cursor, by restoring the contents of
 *  data.cursor_prev_graphic (i.e. what was below the cursor before we drew it)
 *  to video memory. */
static void hide_graphic_cursor(void)
{
	struct modeinfo *info = &data.video_mode;
	struct point start, size, offset;
	unsigned cursor_bytes_per_line;
	unsigned plane, y;

	// Compute the area where the cursor is currently positioned
	if (!get_graphic_cursor_area(&data.cursor_pos, &start, &size, &offset)) {
		return;
	}

	// For each scanline, we will copy this amount of bytes
	cursor_bytes_per_line = get_scanline_segment_bytes(info->bits_per_pixel,
	                                                   start.x, size.x);

	for (plane = 0; plane < info->num_planes; plane++) {
		if (info->num_planes > 1) vga_select_plane(plane);

		for (y = 0; y < size.y; y++) {
			uint8_t __far *line = get_video_scanline(info, start.y + y)
			                      + (start.x * info->bits_per_pixel) / 8;
			uint8_t *prev = get_prev_graphic_cursor_scanline(cursor_bytes_per_line, size.y,
			                                                 plane, y);

			// Restore this scaline from cursor_prev
			_fmemcpy(line, prev, cursor_bytes_per_line);
		}
	}

	data.cursor_visible = false;
}

/** Renders the graphical cursor.
 *  It will also backup whatever pixels are below
 *  the cursor area to cursor_prev_graphic. */
static void show_graphic_cursor(void)
{
	const struct modeinfo *info = &data.video_mode;
	struct point start, size, offset;
	unsigned cursor_bytes_per_line;
	const uint8_t msb_pixel_mask = build_pixel_mask(info->bits_per_pixel);
	unsigned plane, y;

	// Compute the area where the cursor is supposed to be drawn
	if (!get_graphic_cursor_area(&data.pos, &start, &size, &offset)) {
		return;
	}

	// For each scanline, we will copy this amount of bytes
	cursor_bytes_per_line = get_scanline_segment_bytes(info->bits_per_pixel,
	                                                   start.x, size.x);

	for (plane = 0; plane < info->num_planes; plane++) {
		if (info->num_planes > 1) vga_select_plane(plane);

		for (y = 0; y < size.y; y++) {
			uint8_t __far *line = get_video_scanline(info, start.y + y)
			                      + (start.x * info->bits_per_pixel) / 8;
			uint8_t *prev = get_prev_graphic_cursor_scanline(cursor_bytes_per_line, size.y,
			                                                 plane, y);
			uint16_t cursor_and_mask = get_graphic_cursor_and_mask_line(offset.y + y)
			                           << offset.x;
			uint16_t cursor_xor_mask = get_graphic_cursor_xor_mask_line(offset.y + y)
			                           << offset.x;
			unsigned x;

			// First, backup this scanline to prev before any changes
			_fmemcpy(prev, line, cursor_bytes_per_line);

			if (info->bits_per_pixel < 8) {
				uint8_t pixel_mask = msb_pixel_mask;
				uint8_t pixel = *line;

				// when start.x is not pixel aligned,
				// scaline points the previous multiple of pixels_per_byte;
				// and the initial pixel will not be at the MSB of it.
				// advance the pixel_mask accordingly
				pixel_mask >>= (start.x * info->bits_per_pixel) % 8;

				for (x = 0; x < size.x; x++) {
					// The MSBs of each mask correspond to the current pixel
					if (!(cursor_and_mask & MSB_MASK)) {
						pixel &= ~pixel_mask;
					}
					if (cursor_xor_mask & MSB_MASK) {
						pixel ^= pixel_mask;
					}

					// Advance to the next pixel
					pixel_mask >>= info->bits_per_pixel;
					if (!pixel_mask) {
						// Time to advance to the next byte
						*line = pixel; // Save current byte first
						pixel = *(++line);
						pixel_mask = msb_pixel_mask;
					}

					// Advance to the next bit in the cursor mask
					cursor_and_mask <<= 1;
					cursor_xor_mask <<= 1;
				}

				if (pixel_mask != msb_pixel_mask) {
					// We ended up in the middle of a byte, save it
					*line = pixel;
				}
			} else if (info->bits_per_pixel == 8) {
				// Simplified version for byte-aligned pixels
				for (x = 0; x < size.x; x++) {
					uint8_t pixel = 0;

					if (cursor_and_mask & MSB_MASK) {
						pixel = *line;
					}
					if (cursor_xor_mask & MSB_MASK) {
						pixel ^= 0x0F; // Use 0x0F as "white pixel"
					}

					// Advance to the next pixel
					*line = pixel;
					++line;

					// Advance to the next bit in the cursor mask
					cursor_and_mask <<= 1;
					cursor_xor_mask <<= 1;
				}
			}
		}
	}

	data.cursor_pos = data.pos;
	data.cursor_visible = true;
}

/** Refreshes cursor position and visibility. */
static void refresh_cursor(void)
{
	bool should_show = data.visible_count >= 0;
	bool pos_changed, needs_refresh;

#if USE_WIN386
	// Windows 386 is already rendering the cursor for us.
	// Hide our own.
	if (data.w386cursor) should_show = false;
#endif

#if USE_VIRTUALBOX
	if (data.vbavail && data.vbwantcursor) {
		// We want to use the VirtualBox host cursor.
		// See if we have to update its visibility.
		int err = 0;
		if (should_show != data.cursor_visible) {
			err = vbox_set_pointer_visible(&data.vb, should_show);
			if (err == 0 && data.vbhaveabs) {
				data.cursor_visible = should_show;
			}
		}
		if (err == 0 && data.vbhaveabs) {
			// No need to refresh the cursor; VirtualBox is already showing it for us.
			return;
		}
	}
#endif

	pos_changed = data.cursor_pos.x != data.pos.x || data.cursor_pos.y != data.pos.y;
	needs_refresh = should_show && pos_changed || should_show != data.cursor_visible;

	if (!needs_refresh) {
		// Nothing to do
		return;
	}

	if (data.video_mode.type == VIDEO_TEXT) {
		// Text video mode
		if (data.cursor_visible) {
			// Hide the cursor at the old position if any
			hide_text_cursor();
		}
		if (should_show) {
			// Show the cursor at the new position
			show_text_cursor();
		}
	} else if (data.video_mode.type != VIDEO_UNKNOWN) {
		// Graphic video modes

		bool video_planar = data.video_mode.num_planes > 1;
		struct videoregs regs;
		// If current video mode is planar,
		// we will have to play with the VGA registers
		// so let's save and restore them.
		if (video_planar) {
			vga_save_registers(&regs);
			vga_set_graphics_mode(&regs, 0, 0);
		}

		if (data.cursor_visible) {
			hide_graphic_cursor();
		}
		if (should_show) {
			show_graphic_cursor();
		}

		if (video_planar) {
			vga_restore_register(&regs);
		}
	} else {
		// Unknown video mode, don't render cursor.
	}
}

/** Forcefully hides the mouse cursor if shown. */
static void hide_cursor(void)
{
#if USE_VIRTUALBOX
	if (data.vbavail && data.vbwantcursor) {
		vbox_set_pointer_visible(&data.vb, false);
		if (data.vbhaveabs) {
			data.cursor_visible = false;
		}
	}
#endif

	if (data.cursor_visible) {
		if (data.video_mode.type == VIDEO_TEXT) {
			hide_text_cursor();
		} else if (data.video_mode.type != VIDEO_UNKNOWN) {
			hide_graphic_cursor();
		}
	}
}

/** Loads the current graphic cursor,
 *  which in this case means uploading it to the host. */
static void load_cursor(void)
{
#if USE_VIRTUALBOX
	if (data.vbavail && data.vbwantcursor) {
		VMMDevReqMousePointer *req = (VMMDevReqMousePointer *) data.vb.buf;
		const unsigned width = GRAPHIC_CURSOR_WIDTH, height = GRAPHIC_CURSOR_HEIGHT;
		uint8_t  *output = req->pointerData;
		unsigned int y, x;

		memset(req, 0, sizeof(VMMDevReqMousePointer));

		req->header.size = vbox_req_mouse_pointer_size(width, height);
		req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
		req->header.requestType = VMMDevReq_SetPointerShape;
		req->header.rc = -1;

		req->fFlags = VBOX_MOUSE_POINTER_SHAPE;
		req->xHot = BOUND(data.cursor_hotspot.x, 0, width);
		req->yHot = BOUND(data.cursor_hotspot.y, 0, height);
		req->width = width;
		req->height = height;

		// AND mask
		// int33 format is 1-bit per pixel packed into 16-bit LE values,
		// while VirtualBox wants 1-bit per pixel packed into 8-bit.
		// All we have to do is byteswap 16-bit values.
		for (y = 0; y < height; ++y) {
			uint16_t cursor_line = get_graphic_cursor_and_mask_line(y);
			output[0] = (cursor_line >> 8) & 0xFF;
			output[1] = cursor_line & 0xFF;
			output += GRAPHIC_CURSOR_SCANLINE_LEN;
		}

		// XOR mask
		// int33 format is again 1-bit per pixel packed into 16-bit LE values,
		// however VirtualBox wants 4-byte per pixel packed "RGBA".
		for (y = 0; y < height; ++y) {
			uint16_t cursor_line = get_graphic_cursor_xor_mask_line(y);

			for (x = 0; x < width; ++x) {
				// MSB of line is current mask bit, we shift it on each iteration
				uint8_t val = (cursor_line & MSB_MASK) ? 0xFF : 0;

				output[0] = val;
				output[1] = val;
				output[2] = val;
				output[3] = 0;

				cursor_line <<= 1;
				output += 4;
			}
		}

		dlog_puts("Loading cursor to VBox");

		vbox_send_request(data.vb.iobase, data.vb.dds.physicalAddress);

		if (req->header.rc != 0) {
			dlog_puts("Could not send cursor to VirtualBox");
			return;
		}

		// After we send this message, it looks like VirtualBox shows the cursor
		// even if we didn't actually want it to be visible at this point.
		vbox_set_pointer_visible(&data.vb, false);
	}
#endif
}

/** Reloads the information about the current video mode. */
static void reload_video_info(void)
{
	get_current_video_mode_info(&data.video_mode);

	data.screen_max.x = data.video_mode.pixels_width - 1;
	data.screen_max.y = data.video_mode.pixels_height - 1;
	data.screen_scale.x = 1;
	data.screen_scale.y = 1;
	data.screen_granularity.x = 1;
	data.screen_granularity.y = 1;

	// The actual range of coordinates expected by int33 clients
	// is, for some reason, different than real resolution in some modes.
	// For example, 320x... modes are mapped to 640x... pixels.
	if (data.video_mode.pixels_width == 320) {
		data.screen_max.x = 640 - 1;
		data.screen_scale.x = 640 / 320;
	}

	// In text modes, we are supposed to always round the mouse cursor
	// to character boundaries.
	if (data.video_mode.type == VIDEO_TEXT) {
		// Always assume 8x8 character size irregardless of true font
		data.screen_granularity.x = 8;
		data.screen_granularity.y = 8;
	}

	dlog_print("Current video mode=");
	dlog_printx(data.video_mode.mode);
	dlog_print(" screen_max=");
	dlog_printd(data.screen_max.x);
	dlog_putc(',');
	dlog_printd(data.screen_max.y);
	dlog_endline();
}

/** True if the current video is different from what we have stored
 *  and we probably need to recompute video mode info. */
static inline bool video_mode_changed(void)
{
	uint8_t cur_mode = bda_get_video_mode() & ~0x80;

	if (cur_mode != data.video_mode.mode)
		return true;

	if (data.video_mode.type == VIDEO_TEXT) {
		// Also check to see if the font size was changed..
		return data.video_mode.pixels_height != (bda_get_last_row()+1) * 8;
	} else {
		return false;
	}
}

/** Checks if the video mode has changed and if so
 *  refreshes the information about the current video mode.  */
static void refresh_video_info(void)
{
	if (video_mode_changed()) {
		if (data.cursor_visible) {
			// Assume cursor is lost with no way to restore prev contents
			data.cursor_visible = false;
		}

		reload_video_info();

		if (data.video_mode.type != VIDEO_UNKNOWN) {
			// If we know the screen size for this mode, then reset the window to it
			data.min.x = 0;
			data.min.y = 0;
			data.max.x = data.screen_max.x;
			data.max.y = data.screen_max.y;
		}
	}
}

/** Calls the application-registered event handler. */
static void call_event_handler(void (__far *handler)(), uint16_t events,
                               uint16_t buttons, int16_t x, int16_t y,
                               int16_t delta_x, int16_t delta_y)
{
#if TRACE_EVENTS
	dlog_print("calling event handler events=");
	dlog_printx(events);
	dlog_print(" buttons=");
	dlog_printx(buttons);
	dlog_print(" x=");
	dlog_printd(x);
	dlog_print(" y=");
	dlog_printd(y);
	dlog_print(" dx=");
	dlog_printd(delta_x);
	dlog_print(" dy=");
	dlog_printd(delta_y);
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

/** Process a mouse event internally.
 *  @param buttons currently pressed buttons as a bitfield
 *  @param absolute whether mouse coordinates are an absolute value
 *  @param x y if absolute, then absolute coordinates in screen pixels
 *             if relative, then relative coordinates in mickeys
 *  @param z relative wheel mouse movement
 */
static void handle_mouse_event(uint16_t buttons, bool absolute, int x, int y, int z)
{
	uint16_t events = 0;
	int i;

#if TRACE_EVENTS
	dlog_print("handle mouse event");
	if (absolute) dlog_print(" absolute");
	dlog_print(" buttons=");
	dlog_printx(buttons);
	dlog_print(" x=");
	dlog_printd(x);
	dlog_print(" y=");
	dlog_printd(y);
	dlog_print(" z=");
	dlog_printd(z);
	dlog_endline();
#endif

	if (absolute) {
		// Absolute movement: x,y are in screen pixels units
		events |= INT33_EVENT_MASK_ABSOLUTE;

		if (x != data.pos.x || y != data.pos.y) {
			events |= INT33_EVENT_MASK_MOVEMENT;

			// Simulate a fake relative movement in mickeys
			// This is almost certainly broken.
			// Programs that expect relative movement data
			// will almost never set a mickeyPerPixel value.
			// So all we can do is guess.
			if (data.abs_pos.x >= 0 && data.abs_pos.y >= 0) {
				data.delta.x += (x - data.abs_pos.x) * 8;
				data.delta.y += (y - data.abs_pos.y) * 8;
			}
			data.abs_pos.x = x;
			data.abs_pos.y = y;

			// Set the new absolute position
			data.pos.x = x;
			data.pos.y = y;
			data.pos_frac.x = 0;
			data.pos_frac.y = 0;
		}
	} else if (x || y) {
		// Relative movement: x,y are in mickeys
		uint16_t ticks = bda_get_tick_count_lo();
		unsigned ax = abs(x), ay = abs(y);

		events |= INT33_EVENT_MASK_MOVEMENT;

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
		data.delta_frac.x = 0;
		data.delta_frac.y = 0;
		data.abs_pos.x = -1;
		data.abs_pos.y = -1;

		// Convert mickeys into pixels
		data.pos.x += scalei_rem(x, data.mickeysPerLine.x, 8, &data.pos_frac.x);
		data.pos.y += scalei_rem(y, data.mickeysPerLine.y, 8, &data.pos_frac.y);
	}

	bound_position_to_window();

	if (data.haswheel && z) {
		events |= INT33_EVENT_MASK_WHEEL_MOVEMENT;
		// Higher byte of buttons contains wheel movement
		buttons |= (z & 0xFF) << 8;
		// Accumulate delta wheel movement
		data.wheel_delta += z;
		data.wheel_last.x = data.pos.x;
		data.wheel_last.y = data.pos.y;
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
		events |= evt;
	}
	data.buttons = buttons;

	refresh_cursor();

	events &= data.event_mask;
	if (data.event_handler && events) {
		x = snap_to_grid(data.pos.x, data.screen_granularity.x);
		y = snap_to_grid(data.pos.y, data.screen_granularity.y);

		call_event_handler(data.event_handler, events,
		                   buttons, x, y, data.delta.x, data.delta.y);
	}
}

/** PS/2 BIOS calls this routine to notify mouse events. */
static void ps2_mouse_handler(uint16_t word1, uint16_t word2, uint16_t word3, uint16_t word4)
{
#pragma aux ps2_mouse_handler "*" parm caller [ax] [bx] [cx] [dx] modify [ax bx cx dx si di es fs gs]

	unsigned status;
	int x, y, z;
	bool abs = false;

#if TRACE_EVENTS
	dlog_print("ps2 callback ");
	dlog_printx(word1);
	dlog_putc(' ');
	dlog_printx(word2);
	dlog_putc(' ');
	dlog_printx(word3);
	dlog_putc(' ');
	dlog_printx(word4);
	dlog_endline();
#endif /* TRACE_EVENTS */

	// Decode the PS2 event args
	// In a normal IBM PS/2 BIOS (incl. VirtualBox/Bochs/qemu/SeaBIOS):
	//  word1 low byte = status (following PS2M_STATUS_*)
	//  word2 low byte = x
	//  word3 low byte = y
	//  word4 = always zero
	// In a PS/2 BIOS with wheel support (incl. VMware/DOSBox-X):
	// taken from CuteMouse/KoKo:
	//  word1 high byte = x
	//  word1 low byte = status
	//  word2 low byte = y
	//  word3 low byte = z
	//  word4 = always zero
	// VirtualBox/Bochs/qemu/SeaBIOS behave like a normal one,
	// but they also store the raw contents of all mouse packets in the EBDA (starting 0x28 = packet 0).
	// Other BIOSes don't do that so it is not a reliable option either.
	// So, how to detect which BIOS we have?
	// For now we are always assuming "normal" PS/2 BIOS.
	// But with VirtualBox integration on we'll get the wheel packet from the EBDA,
	// and with VMWare integration on we'll get it from the VMware protocol.
	status = (uint8_t) word1;
	x = (uint8_t) word2;
	y = (uint8_t) word3;
	z = 0;
	(void) word4;

	// Sign-extend X, Y as per the status byte
	x =   status & PS2M_STATUS_X_NEG ? 0xFF00 | x : x;
	y = -(status & PS2M_STATUS_Y_NEG ? 0xFF00 | y : y);

#if USE_VIRTUALBOX
	if (data.vbavail) {
		uint16_t vbx, vby;
		if ((vbox_get_mouse(&data.vb, &abs, &vbx, &vby) == 0) && abs) {
			// VirtualBox gives unsigned coordinates from 0...0xFFFFU,
			// scale to 0..screen_size (in pixels).
			refresh_video_info();
			// If the user is using a window larger than the screen, use it.
			x = scaleu(vbx, 0xFFFFU, MAX(data.max.x, data.screen_max.x));
			y = scaleu(vby, 0xFFFFU, MAX(data.max.y, data.screen_max.y));
			data.vbhaveabs = true;
		} else {
			// VirtualBox does not support absolute coordinates,
			// or user has disabled them.
			data.vbhaveabs = false;
			// Rely on PS/2 relative coordinates.
		}

		// VirtualBox/Bochs BIOS does not pass wheel data to the callback,
		// so we will fetch it directly from the BIOS data segment.
		if (data.haswheel) {
			int8_t __far * mouse_packet = MK_FP(bda_get_ebda_segment(), 0x28);
			z = mouse_packet[3];
		}
	}
#endif /* USE_VIRTUALBOX */

#if USE_VMWARE
	if (data.vmwavail) {
		uint32_t vmwstatus = vmware_abspointer_status();
		uint16_t data_avail = vmwstatus & VMWARE_ABSPOINTER_STATUS_MASK_DATA;

#if TRACE_EVENTS
		dlog_print("vmware status=0x");
		dlog_printx(vmwstatus >> 16);
		dlog_print(" ");
		dlog_printx(vmwstatus & 0xFFFF);
		dlog_endline();
#endif

		if (data_avail >= VMWARE_ABSPOINTER_DATA_PACKET_SIZE) {
			struct vmware_abspointer_data vmw;
			vmware_abspointer_data(VMWARE_ABSPOINTER_DATA_PACKET_SIZE, &vmw);

#if TRACE_EVENTS
			dlog_print("vmware pstatus=0x");
			dlog_printx(status);
			dlog_print(" x=0x");
			dlog_printx(vmw.x);
			dlog_print(" z=");
			dlog_printd((int8_t) (uint8_t) vmw.z);
			dlog_endline();
#endif

			if (vmw.status & VMWARE_ABSPOINTER_STATUS_RELATIVE) {
				x = (int16_t) vmw.x;
				y = (int16_t) vmw.y;
				z = (int8_t) (uint8_t) vmw.z;
			} else {
				abs = true;
				// Scale to screen coordinates
				refresh_video_info();
				x = scaleu(vmw.x & 0xFFFFU, 0xFFFFU,
				           MAX(data.max.x, data.screen_max.x));
				y = scaleu(vmw.y & 0xFFFFU, 0xFFFFU,
				           MAX(data.max.y, data.screen_max.y));
				z = (int8_t) (uint8_t) vmw.z;
			}

			if (vmw.status & VMWARE_ABSPOINTER_STATUS_BUTTON_LEFT) {
				status |= PS2M_STATUS_BUTTON_1;
			}
			if (vmw.status & VMWARE_ABSPOINTER_STATUS_BUTTON_RIGHT) {
				status |= PS2M_STATUS_BUTTON_2;
			}
			if (vmw.status & VMWARE_ABSPOINTER_STATUS_BUTTON_MIDDLE) {
				status |= PS2M_STATUS_BUTTON_3;
			}
		} else {
			return; // Ignore the PS/2 packet otherwise, it is likely garbage
		}
	}
#endif /* USE_VMWARE */

	handle_mouse_event(status & (PS2M_STATUS_BUTTON_1 | PS2M_STATUS_BUTTON_2 | PS2M_STATUS_BUTTON_3),
	                   abs, x, y, z);
}

void __declspec(naked) __far ps2_mouse_callback()
{
	__asm {
		pusha
		push ds
		push es
		push fs
		push gs

		; 8 + 4 saved registers, 24 bytes
		; plus 4 bytes for retf address
		; = 28 bytes of stack before callback args

		mov bp, sp
		push cs
		pop ds

		mov	ax,[bp+28+6]	; Status
		mov	bx,[bp+28+4]	; X
		mov	cx,[bp+28+2]	; Y
		mov	dx,[bp+28+0]	; Z

		call ps2_mouse_handler

		pop gs
		pop fs
		pop es
		pop ds
		popa

		retf
	}
}

#if USE_INTEGRATION
static void set_absolute(bool enable)
{
#if USE_VIRTUALBOX
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
#endif /* USE_VIRTUALBOX */
#if USE_VMWARE
	if (data.vmwavail) {
		if (enable) {
			uint16_t data_avail;
			// It looks like a reset of the PS/2 mouse completely disables
			// the vmware interface, so we have to reenable it from scratch.
			vmware_abspointer_cmd(VMWARE_ABSPOINTER_CMD_ENABLE);
			vmware_abspointer_data_clear();
			vmware_abspointer_cmd(VMWARE_ABSPOINTER_CMD_REQUEST_ABSOLUTE);

			dlog_puts("VMware absolute mouse enabled");
		} else {
			vmware_abspointer_cmd(VMWARE_ABSPOINTER_CMD_REQUEST_RELATIVE);
			vmware_abspointer_cmd(VMWARE_ABSPOINTER_CMD_DISABLE);

			dlog_puts("VMware absolute mouse disabled");
		}
	}
#endif /* USE_VMWARE */
}
#endif /* USE_INTEGRATION */

static void reset_mouse_hardware()
{
	ps2m_enable(false);

#if USE_WHEEL
	if (data.usewheel && ps2m_detect_wheel()) {
		// Detect wheel also reinitializes the mouse to the proper packet size
		data.haswheel = true;
	} else {
		// Otherwise do an extra reset to return back to initial state, just in case
		data.haswheel = false;
		ps2m_init(PS2M_PACKET_SIZE_PLAIN);
	}
#else
	data.haswheel = false;
	ps2m_init(PS2M_PACKET_SIZE_PLAIN);
#endif

	ps2m_set_resolution(3);     // 3 = 200 dpi, 8 counts per millimeter
	ps2m_set_sample_rate(4);    // 4 = 80 reports per second
	ps2m_set_scaling_factor(1); // 1 = 1:1 scaling

	ps2m_set_callback(get_cs():>ps2_mouse_callback);

#if USE_INTEGRATION
	// By default, enable absolute mouse
	set_absolute(true);
	// Reload hardware cursor
	load_cursor();
#endif

	ps2m_enable(true);
}

/** Reset "software" mouse settings, i.e. those configurable by the client program. */
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
	memcpy(data.cursor_graphic, default_cursor_graphic, sizeof(data.cursor_graphic));

	refresh_cursor(); // This will hide the cursor and update data.cursor_visible
}

/** Reset the current mouse state and throw away past events. */
static void reset_mouse_state()
{
	int i;
	data.pos.x = data.min.x;
	data.pos.y = data.min.y;
	data.pos_frac.x = 0;
	data.pos_frac.y = 0;
	data.delta.x = 0;
	data.delta.y = 0;
	data.delta_frac.x = 0;
	data.delta_frac.y = 0;
	data.abs_pos.x = -1;
	data.abs_pos.y = -1;
	data.buttons = 0;
	for (i = 0; i < NUM_BUTTONS; i++) {
		data.button[i].pressed.count = 0;
		data.button[i].pressed.last.x = 0;
		data.button[i].pressed.last.y = 0;
		data.button[i].released.count = 0;
		data.button[i].released.last.x = 0;
		data.button[i].released.last.y = 0;
	}
	data.wheel_delta = 0;
	data.cursor_visible = false;
	data.cursor_pos.x = 0;
	data.cursor_pos.y = 0;
	data.cursor_prev_char = 0;
	memset(data.cursor_prev_graphic, 0, sizeof(data.cursor_prev_graphic));
}

static void return_clear_wheel_counter(union INTPACK __far *r)
{
	r->x.cx = snap_to_grid(data.wheel_last.x, data.screen_granularity.x);
	r->x.dx = snap_to_grid(data.wheel_last.y, data.screen_granularity.y);
	r->x.bx = data.wheel_delta;
	data.wheel_last.x = 0;
	data.wheel_last.y = 0;
	data.wheel_delta = 0;
}

static void return_clear_button_counter(union INTPACK __far *r, struct buttoncounter *c)
{
	r->x.cx = snap_to_grid(c->last.x, data.screen_granularity.x);
	r->x.dx = snap_to_grid(c->last.y, data.screen_granularity.y);
	r->x.bx = c->count;
	c->last.x = 0;
	c->last.y = 0;
	c->count = 0;
}

/** Entry point for our int33 API. */
static void int33_handler(union INTPACK r)
#pragma aux int33_handler "*" parm caller [] modify [ax bx cx dx si di es fs gs]
{
	switch (r.x.ax) {
	case INT33_RESET_MOUSE:
		dlog_puts("Mouse reset");
		reload_video_info();
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
#if TRACE_EVENTS
		dlog_puts("Mouse get position");
#endif
		r.x.cx = snap_to_grid(data.pos.x, data.screen_granularity.x);
		r.x.dx = snap_to_grid(data.pos.y, data.screen_granularity.y);
		r.x.bx = data.buttons;
		if (data.haswheel) {
			r.h.bh = data.wheel_delta;
			data.wheel_delta = 0;
		}
		break;
	case INT33_SET_MOUSE_POSITION:
#if TRACE_EVENTS
		dlog_puts("Mouse set position");
#endif
		data.pos.x = r.x.cx;
		data.pos.y = r.x.dx;
		data.pos_frac.x = 0;
		data.pos_frac.y = 0;
		data.delta.x = 0;
		data.delta.y = 0;
		data.delta_frac.x = 0;
		data.delta_frac.y = 0;
		bound_position_to_window();
		break;
	case INT33_GET_BUTTON_PRESSED_COUNTER:
		r.x.ax = data.buttons;
		if (data.haswheel) {
			r.h.bh = data.wheel_delta;
		}
		if (data.haswheel && r.x.bx == -1) {
			// Wheel information
			return_clear_wheel_counter(&r);
		} else {
			// Regular button information
			int btn = MIN(r.x.bx, NUM_BUTTONS - 1);
			return_clear_button_counter(&r, &data.button[btn].pressed);
		}
		break;
	case INT33_GET_BUTTON_RELEASED_COUNTER:
		r.x.ax = data.buttons;
		if (data.haswheel) {
			r.h.bh = data.wheel_delta;
		}
		if (data.haswheel && r.x.bx == -1) {
			// Wheel information
			return_clear_wheel_counter(&r);
		} else {
			int btn = MIN(r.x.bx, NUM_BUTTONS - 1);
			return_clear_button_counter(&r, &data.button[btn].released);
		}
		break;
	case INT33_SET_HORIZONTAL_WINDOW:
		dlog_print("Mouse set horizontal window [");
		dlog_printd(r.x.cx);
		dlog_putc(',');
		dlog_printd(r.x.dx);
		dlog_puts("]");
		// Recheck in case someone changed the video mode
		refresh_video_info();
		data.min.x = r.x.cx;
		data.max.x = r.x.dx;
		bound_position_to_window();
		break;
	case INT33_SET_VERTICAL_WINDOW:
		dlog_print("Mouse set vertical window [");
		dlog_printd(r.x.cx);
		dlog_putc(',');
		dlog_printd(r.x.dx);
		dlog_puts("]");
		refresh_video_info();
		data.min.y = r.x.cx;
		data.max.y = r.x.dx;
		bound_position_to_window();
		break;
	case INT33_SET_GRAPHICS_CURSOR:
		dlog_puts("Mouse set graphics cursor");
		hide_cursor();
		data.cursor_hotspot.x = r.x.bx;
		data.cursor_hotspot.y = r.x.cx;
		_fmemcpy(data.cursor_graphic, MK_FP(r.x.es, r.x.dx), sizeof(data.cursor_graphic));
		load_cursor();
		refresh_cursor();
		break;
	case INT33_SET_TEXT_CURSOR:
		dlog_print("Mouse set text cursor ");
		dlog_printd(r.x.bx);
		dlog_endline();
		hide_cursor();
		data.cursor_text_type = r.x.bx;
		data.cursor_text_and_mask = r.x.cx;
		data.cursor_text_xor_mask = r.x.dx;
		refresh_cursor();
		break;
	case INT33_GET_MOUSE_MOTION:
#if TRACE_EVENTS
		dlog_puts("Mouse get motion");
#endif
		r.x.cx = data.delta.x;
		r.x.dx = data.delta.y;
		data.delta.x = 0;
		data.delta.y = 0;
		break;
	case INT33_SET_EVENT_HANDLER:
		dlog_puts("Mouse set event handler");
		data.event_mask = r.x.cx;
		data.event_handler = MK_FP(r.x.es, r.x.dx);
		break;
	case INT33_SET_MOUSE_SPEED:
		dlog_print("Mouse set speed x=");
		dlog_printd(r.x.cx);
		dlog_print(" y=");
		dlog_printd(r.x.dx);
		dlog_endline();
		data.mickeysPerLine.x = r.x.cx;
		data.mickeysPerLine.y = r.x.dx;
		break;
	case INT33_SET_SPEED_DOUBLE_THRESHOLD:
		dlog_print("Mouse set speed double threshold=");
		dlog_printd(r.x.dx);
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
		_fmemcpy(MK_FP(r.x.es, r.x.dx), &data, sizeof(TSRDATA));
		break;
	case INT33_LOAD_MOUSE_STATUS:
		dlog_puts("Mouse load status");
		_fmemcpy(&data, MK_FP(r.x.es, r.x.dx), sizeof(TSRDATA));
		break;
	case INT33_SET_MOUSE_SENSITIVITY:
		dlog_print("Mouse set speed x=");
		dlog_printd(r.x.bx);
		dlog_print(" y=");
		dlog_printd(r.x.cx);
		dlog_print(" threshold=");
		dlog_printd(r.x.dx);
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
		reload_video_info();
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
		r.h.bh = REPORTED_VERSION_MAJOR;
		r.h.bl = REPORTED_VERSION_MINOR;
		r.h.ch = INT33_MOUSE_TYPE_PS2;
		r.h.cl = 0;
		break;
	case INT33_GET_MAX_COORDINATES:
		r.x.bx = 0;
		r.x.cx = MAX(data.screen_max.x, data.max.x);
		r.x.dx = MAX(data.screen_max.y, data.max.y);
		break;
	case INT33_GET_WINDOW:
		r.x.ax = data.min.x;
		r.x.bx = data.min.y;
		r.x.cx = data.max.x;
		r.x.dx = data.max.y;
		break;
#if USE_WHEEL
	// Wheel API extensions:
	case INT33_GET_CAPABILITIES:
		r.x.ax = INT33_WHEEL_API_MAGIC; // Driver supports wheel API
		r.x.bx = 0;
		r.x.cx = data.haswheel ? INT33_CAPABILITY_MOUSE_API : 0;
		break;
#endif
	// Our internal API extensions:
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

#if USE_WIN386
/** Windows will call this function to notify events when we are inside a DOS box. */
static void windows_mouse_handler(int action, int x, int y, int buttons, int events)
#pragma aux windows_mouse_handler "*" parm [ax] [bx] [cx] [dx] [si] modify [ax bx cx dx es]
{
	switch (action) {
	case VMD_ACTION_MOUSE_EVENT:
		(void) events;
		// Forward event to our internal system
		handle_mouse_event(buttons, true, x, y, 0);
		break;
	case VMD_ACTION_HIDE_CURSOR:
		dlog_puts("VMD_ACTION_HIDE_CURSOR");
		data.w386cursor = true;
		refresh_cursor();
		break;
	case VMD_ACTION_SHOW_CURSOR:
		dlog_puts("VMD_ACTION_SHOW_CURSOR");
		data.w386cursor = false;
		refresh_cursor();
		break;
	}
}

void __declspec(naked) __far windows_mouse_callback()
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

		call windows_mouse_handler

		pop gs
		pop fs
		pop es
		pop ds
		popa

		retf
	}
}

static void int2f_handler(union INTPACK r)
#pragma aux int2f_handler "*" parm caller [] modify [ax bx cx dx es]
{
	switch (r.x.ax) {
	case INT2F_NOTIFY_WIN386_STARTUP:
		dlog_print("Windows is starting, version=");
		dlog_printx(r.x.di);
		dlog_endline();
		data.w386_startup.version = 3;
		data.w386_startup.next = MK_FP(r.x.es, r.x.bx);
		data.w386_startup.device_driver = 0;
		data.w386_startup.device_driver_data = 0;
		data.w386_startup.instance_data = &data.w386_instance;
		data.w386_instance[0].ptr = &data;
		data.w386_instance[0].size = sizeof(data);
		data.w386_instance[1].ptr = 0;
		data.w386_instance[1].size = 0;
		r.x.es = FP_SEG(&data.w386_startup);
		r.x.bx = FP_OFF(&data.w386_startup);
		break;
	case INT2F_NOTIFY_DEVICE_CALLOUT:
		switch (r.x.bx) {
		case VMD_DEVICE_ID:
			switch (r.x.cx) {
			case VMD_CALLOUT_TEST:
				r.x.cx = 1; // Yes, we are here!
				break;
			case VMD_CALLOUT_GET_DOS_MOUSE_API:
				// Windows is asking our mouse driver for the hook function address
				r.x.ds = get_cs();
				r.x.si = FP_OFF(windows_mouse_callback);
				r.x.ax = 0; // Yes, we are here!
				break;
			}
			break;
		}
		break;
	}
}

void __declspec(naked) __far int2f_isr(void)
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

		call int2f_handler

		pop gs
		pop fs
		pop es
		pop ds
		popa

		; Jump to the next handler in the chain
		jmp dword ptr cs:[data + 4] ; wasm doesn't support structs, this is data.prev_int2f_handler
	}
}
#endif

static LPTSRDATA int33_get_tsr_data(void);
#pragma aux int33_get_tsr_data = \
	"xor ax, ax" \
	"mov es, ax" \
	"mov di, ax" \
	"mov ax, 0x73" \
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
