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

#include "utils.h"
#include "int33.h"
#include "int2fwin.h"
#include "w16mouse.h"

#define TRACE_EVENT 1

/** If 1, hook int2f to detect fullscreen DOSBoxes and auto-disable this driver. */
#define HOOK_INT2F  0

#define MOUSE_NUM_BUTTONS 2

/** The routine Windows gave us which we should use to report events. */
static LPFN_MOUSEEVENT eventproc;
/** Current status of the mouse driver (see MOUSEFLAGS_*). */
static struct mouseflags {
	bool enabled : 1;
	bool haswin386 : 1;
	bool int2f_hooked : 1;
} flags;
/** Previous deltaX, deltaY from the int33 mouse callback (for relative motion) */
static short prev_delta_x, prev_delta_y;
#if HOOK_INT2F
/** Existing interrupt2f handler. */
static LPFN prev_int2f_handler;
#endif

/* This is how events are delivered to Windows */

static void send_event(unsigned short Status, short deltaX, short deltaY, short ButtonCount, short extra1, short extra2);
#pragma aux (MOUSEEVENTPROC) send_event = \
	"call dword ptr [eventproc]"

#pragma code_seg ( "CALLBACKS" )

#include "dlog.h"

static void FAR int33_mouse_callback(uint16_t events, uint16_t buttons, int16_t x, int16_t y, int16_t delta_x, int16_t delta_y)
#pragma aux (INT33_CB) int33_mouse_callback
{
	int status = 0;

#if TRACE_EVENT_IN
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
		// Use mickeys for relative motion
		x = delta_x - prev_delta_x;
		y = delta_y - prev_delta_y;

		prev_delta_x = delta_x;
		prev_delta_y = delta_y;
	}

	// Unused
	(void) buttons;

#if TRACE_EVENT
	dlog_print("w16mouse: event status=");
	dlog_printx(status);
	dlog_print(" x=");
	dlog_printd(x);
	dlog_print(" y=");
	dlog_printd(y);
	dlog_endline();
#endif

	send_event(status, x, (uint16_t)(y), MOUSE_NUM_BUTTONS, 0, 0);
}

#if HOOK_INT2F

static void display_switch_handler(int function)
#pragma aux display_switch_handler parm caller [ax] modify [ax bx cx dx si di]
{
	if (!flags.enabled) {
		return;
	}

	switch (function) {
	case INT2F_NOTIFY_BACKGROUND_SWITCH:
		dlog_puts("Going background\n");
		break;
	case INT2F_NOTIFY_FOREGROUND_SWITCH:
		dlog_puts("Going foreground\n");
		break;
	}
}

/** Interrupt 2F handler, which will be called on some Windows 386 mode events.
 * This is more complicated than it should be becaused we have to fetch our DS
 * without clobbering any registers whatsoever, and chain back to the previous handler.
 * @todo OpenWatcom 2.x insists on calling GETDS from a different code segment, so we can't use __interrupt. */
static void __declspec(naked) __far int2f_handler(void)
{
	_asm {
		; Preserve data segment
		push ds

		; Load our data segment
		push ax
		mov ax, SEG prev_int2f_handler ; Let's hope that Windows relocates segments with interrupts disabled
		mov ds, ax
		pop ax

		; Check functions we are interested in hooking
		cmp ax, 0x4001  ; Notify Background Switch
		je handle_it
		cmp ax, 0x4002  ; Notify Foreground Switch
		je handle_it

		; Otherwise directly jump to next handler
		jmp next_handler

	handle_it:
		pushad ; Save and restore 32-bit registers, we may clobber them from C
		call display_switch_handler
		popad

	next_handler:
		; Store the address of the previous handler
		push dword ptr [prev_int2f_handler]

		; Restore original data segment without touching the stack,
		; since we want to keep the prev handler address at the top
		push bp
		mov bp, sp
		mov ds, [bp + 6]  ; Stack looks like 0: bp, 2: prev_int2f_handler, 6: ds
		pop bp

		retf 2
	}
}

#endif /* HOOK_INT2F */

#pragma code_seg ()

/* Driver exported functions. */

/** DLL entry point (or driver initialization routine).
 * The initialization routine should check whether a mouse exists.
 * @return nonzero value indicates a mouse exists.
 */
#pragma off (unreferenced);
BOOL FAR PASCAL LibMain(HINSTANCE hInstance, WORD wDataSegment,
                        WORD wHeapSize, LPSTR lpszCmdLine)
#pragma pop (unreferenced);
{
	// We are not going to bother checking whether a PS2 mouse exists and just assume it does

#if HOOK_INT2F
	// Check now whether we are running under protected mode windows
	if (windows_386_enhanced_mode()) {
		flags.haswin386 = true;
	}
#endif

#if 0
	// When running under protected mode Windows, let's tell VMD (the mouse virtualizer)
	// what type of mouse we are going to be using
	if (flags.haswin386) {
		LPFN vmd_entry = win_get_vxd_api_entry(VMD_DEVICE_ID);
		if (vmd_entry) {
			vmd_set_mouse_type(&vmd_entry, VMD_TYPE_PS2, 0x33, 0);
		}
	}
#endif

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
	cli(); // Write to far pointer may not be atomic, and we could be interrupted mid-write
	eventproc = lpEventProc;
	sti();

	if (!flags.enabled) {
		int33_reset();

		int33_set_horizontal_window(0, 0x7FFF);
		int33_set_vertical_window(0, 0x7FFF);

		int33_set_event_handler(INT33_EVENT_MASK_ALL, int33_mouse_callback);

		flags.enabled = true;

#if HOOK_INT2F
		if (flags.haswin386) {
			cli();
			hook_int2f(&prev_int2f_handler, int2f_handler);
			sti();
			flags.int2f_hooked = true;
		}
#endif
	}
}

/** Called by Windows to disable the mouse driver. */
VOID FAR PASCAL Disable(VOID)
{
	if (flags.enabled) {
#if HOOK_INT2F
		if (flags.int2f_hooked) {
			cli();
			unhook_int2f(prev_int2f_handler);
			sti();
			flags.int2f_hooked = false;
		}
#endif

		int33_reset();

		flags.enabled = false;
	}
}

/** Called by Window to retrieve the interrupt vector number used by this driver, or -1. */
int FAR PASCAL MouseGetIntVect(VOID)
{
	return 0x33;
}

