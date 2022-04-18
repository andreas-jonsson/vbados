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

#include <string.h>
#include <limits.h>
#include <windows.h>

#include "utils.h"
#include "int33.h"
#include "int21dos.h"
#include "int2fwin.h"
#include "mousew16.h"

/** Whether to enable wheel mouse handling. */
#define USE_WHEEL 1
/** Verbosely log events as they happen. */
#define TRACE_EVENTS 0

#define MOUSE_NUM_BUTTONS 2

/** The routine Windows gave us which we should use to report events. */
static LPFN_MOUSEEVENT eventproc;
/** Current status of the driver. */
static struct {
	/** Whether the driver is currently enabled. */
	bool enabled : 1;
#if USE_WHEEL
	/** Whether a mouse wheel was detected. */
	bool wheel : 1;
#endif
} flags;
/** Previous deltaX, deltaY from the int33 mouse callback (for relative motion) */
static short prev_delta_x, prev_delta_y;
/** Maximum X and Y coordinates expected from the int33 driver. */
static unsigned short max_x, max_y;
#if USE_WHEEL
/** Some delay-loaded functions from USER.EXE to provide wheel mouse events. */
static struct {
	HINSTANCE hUser;
	void WINAPI (*GetCursorPos)( POINT FAR * );
	HWND WINAPI (*WindowFromPoint)( POINT );
	HWND WINAPI (*GetParent)( HWND );
	int WINAPI  (*GetClassName)( HWND, LPSTR, int );
	LONG WINAPI (*GetWindowLong)( HWND, int );
	BOOL WINAPI (*EnumChildWindows)( HWND, WNDENUMPROC, LPARAM );
	BOOL WINAPI (*PostMessage)( HWND, UINT, WPARAM, LPARAM );
} userapi;
#endif

/* This is how events are delivered to Windows */

static void send_event(unsigned short Status, short deltaX, short deltaY, short ButtonCount, short extra1, short extra2);
#pragma aux (MOUSEEVENTPROC) send_event = \
	"call dword ptr [eventproc]"

/* Our "CALLBACKS" segment which is fixed and non-relocatable. */

#pragma code_seg ( "CALLBACKS" )

#include "dlog.h"

#if USE_WHEEL
typedef struct {
	HWND scrollbar;
} FINDSCROLLBARDATA, FAR * LPFINDSCROLLBARDATA;

static BOOL CALLBACK __loadds find_scrollbar(HWND hWnd, LPARAM lParam)
{
	LPFINDSCROLLBARDATA data = (LPFINDSCROLLBARDATA) lParam;
	char buffer[16];

	userapi.GetClassName(hWnd, &buffer, sizeof(buffer));

	if (_fstrcmp(buffer, "ScrollBar") == 0) {
		data->scrollbar = hWnd;
		return FALSE;
	}

	return TRUE;
}

static void send_wheel_movement(int8_t z)
{
	POINT point;
	HWND hWnd;
	WPARAM wParam;

#if TRACE_EVENTS
	dlog_print("w16mouse: wheel=");
	dlog_printd(z);
	dlog_endline();
#endif

	// TODO It's highly unlikely that we can call this many functions from
	// an interrupt handler without causing a re-entrancy issue somewhere.
	// Likely it would be better to just move all of this into a hook .DLL

	userapi.GetCursorPos(&point);

	hWnd = userapi.WindowFromPoint(point);

	while (hWnd) {
		LONG style = userapi.GetWindowLong(hWnd, GWL_STYLE);

		if (style & WS_VSCROLL) {
			wParam = z < 0 ? SB_LINEUP : SB_LINEDOWN;
			userapi.PostMessage(hWnd, WM_VSCROLL, wParam, 0);
			break;
		} else if (style & WS_HSCROLL) {
			wParam = z < 0 ? SB_LINELEFT : SB_LINERIGHT;
			userapi.PostMessage(hWnd, WM_HSCROLL, wParam, 0);
			break;
		} else {
			FINDSCROLLBARDATA data = {0};

			// Let's check if we can find a scroll bar in this window..
			userapi.EnumChildWindows(hWnd, find_scrollbar, (LONG) (LPVOID) &data);
			if (data.scrollbar) {
				// Assume vertical scrolling
				wParam = z < 0 ? SB_LINEUP : SB_LINEDOWN;
				userapi.PostMessage(hWnd, WM_VSCROLL, wParam, 0);
				break;
			}

			if (style & WS_CHILD) {
				// Otherwise try again with a parent window
				hWnd = userapi.GetParent(hWnd);
			} else {
				// This was already a topmost
				break;
			}
		}
	}
}
#endif /* USE_WHEEL */

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

#if USE_WHEEL
	if (flags.wheel && (events & INT33_EVENT_MASK_WHEEL_MOVEMENT)) {
		// If wheel API is enabled, higher byte of buttons contains wheel movement
		int8_t z = (buttons & 0xFF00) >> 8;
		if (z) {
			send_wheel_movement(z);
		}
	}
#endif

	if (events & INT33_EVENT_MASK_ABSOLUTE) {
		status |= SF_ABSOLUTE;

		// Clip the coordinates, we don't want any overflow error below...
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x > max_x) x = max_x;
		if (y > max_y) y = max_y;

		x = scaleu(x, max_x, 0xFFFF);
		y = scaleu(y, max_y, 0xFFFF);
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
	dlog_print("w16mouse: post status=");
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
		dos_print_string("To use VBMOUSE.DRV you have to run/install VBMOUSE.EXE first\r\n(or at least any other DOS mouse driver).\r\n\n$");
		dos_print_string("Press any key to terminate.$");
		dos_read_character();
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

	if (!flags.enabled) {
		int33_reset();

#if USE_WHEEL
		// Detect whether the int33 driver has wheel support
		flags.wheel = int33_get_capabilities() & 1;
		if (flags.wheel) {
			userapi.hUser = (HINSTANCE) GetModuleHandle("USER");
			userapi.GetCursorPos = (LPVOID) GetProcAddress(userapi.hUser, "GetCursorPos");
			userapi.WindowFromPoint = (LPVOID) GetProcAddress(userapi.hUser, "WindowFromPoint");
			userapi.GetParent = (LPVOID) GetProcAddress(userapi.hUser, "GetParent");
			userapi.GetClassName = (LPVOID) GetProcAddress(userapi.hUser, "GetClassName");
			userapi.GetWindowLong = (LPVOID) GetProcAddress(userapi.hUser, "GetWindowLong");
			userapi.EnumChildWindows = (LPVOID) GetProcAddress(userapi.hUser, "EnumChildWindows");
			userapi.PostMessage = (LPVOID) GetProcAddress(userapi.hUser, "PostMessage");
		}
#endif

		// Set the speed to 1,1 to enlarge dosemu coordinate range by 8x16 times.
		// In other absolute drivers, this doesn't change coordinates nor actual speed (which is inherited from host)
		// In normal relative drivers, we'll use the raw mickeys anyways, so speed should also have no effect.
		int33_set_mouse_speed(1, 1);

		// Get what the int33 driver thinks the current coordinate range is
		max_x = 0; max_y = 0;
		int33_get_max_coordinates(&max_x, &max_y);
		if (!max_x || !max_y) { max_x = 640; max_y = 200; } // For really bad drivers

		// Multiply that by 8x16 to match dosemu coordinate range
		// (and also in case the driver-provided coordinate range is too small)
		max_x *= 8; max_y *= 16;

		// And use that to define the window size which is what most
		// absolute drivers will use to define the coordinate range
		int33_set_horizontal_window(0, max_x);
		int33_set_vertical_window(0, max_y);

		int33_set_event_handler(INT33_EVENT_MASK_ALL, int33_mouse_callback);

		flags.enabled = true;
	}
}

/** Called by Windows to disable the mouse driver. */
VOID FAR PASCAL Disable(VOID)
{
	if (flags.enabled) {
		int33_reset(); // This removes our handler and all other settings
		flags.enabled = false;
	}
}

/** Called by Window to retrieve the interrupt vector number used by this driver, or -1. */
int FAR PASCAL MouseGetIntVect(VOID)
{
	return 0x33;
}
