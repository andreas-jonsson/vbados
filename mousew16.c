/*
 * VBMouse - win16 mouse driver entry points
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

#include <windows.h>
#include <limits.h>

#include "utils.h"
#include "int33.h"
#include "int2fwin.h"
#include "mousew16.h"

#define TRACE_EVENTS 0

#define MOUSE_NUM_BUTTONS 2

/** The routine Windows gave us which we should use to report events. */
static LPFN_MOUSEEVENT eventproc;
/** Current status of the mouse driver. */
static bool enabled;
/** Previous deltaX, deltaY from the int33 mouse callback (for relative motion) */
static short prev_delta_x, prev_delta_y;

/* This is how events are delivered to Windows */

static void send_event(unsigned short Status, short deltaX, short deltaY, short ButtonCount, short extra1, short extra2);
#pragma aux (MOUSEEVENTPROC) send_event = \
	"call dword ptr [eventproc]"

/* Our "CALLBACKS" segment which is fixed and non-relocatable. */

#pragma code_seg ( "CALLBACKS" )

#include "dlog.h"

static void FAR int33_mouse_callback(uint16_t events, uint16_t buttons, int16_t x, int16_t y, int16_t delta_x, int16_t delta_y)
#pragma aux (INT33_CB) int33_mouse_callback
{
	int status = 0;

#if TRACE_EVENTS
	dlog_print("w16mouse: events=");
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

    if (events & INT33_EVENT_MASK_LEFT_BUTTON_PRESSED)   status |= SF_B1_DOWN;
	if (events & INT33_EVENT_MASK_LEFT_BUTTON_RELEASED)  status |= SF_B1_UP;
	if (events & INT33_EVENT_MASK_RIGHT_BUTTON_PRESSED)  status |= SF_B2_DOWN;
	if (events & INT33_EVENT_MASK_RIGHT_BUTTON_RELEASED) status |= SF_B2_UP;

	if (events & INT33_EVENT_MASK_MOVEMENT) {
		status |= SF_MOVEMENT;
	}

	if (events & INT33_EVENT_MASK_ABSOLUTE) {
		status |= SF_ABSOLUTE;

		// We set the window to be 0..0x7FFF, so just scale to 0xFFFF
		x = (uint16_t)(x) * 2;
		y = (uint16_t)(y) * 2;
	} else {
		// Prefer to use mickeys for relative motion if we don't have absolute data
		x = delta_x - prev_delta_x;
		y = delta_y - prev_delta_y;

		prev_delta_x = delta_x;
		prev_delta_y = delta_y;
	}

	// Unused
	(void) buttons;

#if TRACE_EVENTS
	dlog_print("w16mouse: event status=");
	dlog_printx(status);
	dlog_print(" x=");
	if (status & SF_ABSOLUTE) dlog_printu(x);
	                     else dlog_printd(x);
	dlog_print(" y=");
	if (status & SF_ABSOLUTE) dlog_printu(y);
	                     else dlog_printd(y);
	dlog_endline();
#endif

	send_event(status, x, y, MOUSE_NUM_BUTTONS, 0, 0);
}

#pragma code_seg ()

/* Driver exported functions. */

/** DLL entry point (or driver initialization routine).
 * The initialization routine should check whether a mouse exists.
 * @return nonzero value indicates a mouse exists.
 */
#pragma off (unreferenced)
BOOL FAR PASCAL LibMain(HINSTANCE hInstance, WORD wDataSegment,
                        WORD wHeapSize, LPSTR lpszCmdLine)
#pragma pop (unreferenced)
{
	uint16_t version = int33_get_driver_version();

	// For now we just check for the presence of any int33 driver version
	if (version == 0) {
		// No one responded to our request, we can assume no driver
		// This will cause a "can't load .drv" message from Windows
		return 0;
	}

	return 1;
}

/** Called by Windows to retrieve information about the mouse hardware. */
WORD FAR PASCAL Inquire(LPMOUSEINFO lpMouseInfo)
{
	lpMouseInfo->msExist = 1;
	lpMouseInfo->msRelative = 0;
	lpMouseInfo->msNumButtons = MOUSE_NUM_BUTTONS;
	lpMouseInfo->msRate = 80;
	return sizeof(MOUSEINFO);
}

/** Called by Windows to enable the mouse driver.
  * @param lpEventProc Callback function to call when a mouse event happens. */
VOID FAR PASCAL Enable(LPFN_MOUSEEVENT lpEventProc)
{
	// Store the windows-given callback
	_disable(); // Write to far pointer may not be atomic, and we could be interrupted mid-write
	eventproc = lpEventProc;
	_enable();

	if (!enabled) {
		int33_reset();

		// Since the mouse driver will likely not know the Windows resolution,
		// let's manually set up a large window of coordinates so as to have
		// as much precision as possible.
		// We use 0x7FFF instead of 0xFFFF because this parameter is officially a signed value.
		int33_set_horizontal_window(0, 0x7FFF);
		int33_set_vertical_window(0, 0x7FFF);

		int33_set_event_handler(INT33_EVENT_MASK_ALL, int33_mouse_callback);

		enabled = true;
	}
}

/** Called by Windows to disable the mouse driver. */
VOID FAR PASCAL Disable(VOID)
{
	if (enabled) {
		int33_reset(); // This removes our handler and all other settings
		enabled = false;
	}
}

/** Called by Window to retrieve the interrupt vector number used by this driver, or -1. */
int FAR PASCAL MouseGetIntVect(VOID)
{
	return 0x33;
}
