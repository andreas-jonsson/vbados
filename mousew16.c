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

#include "vbox.h"
#include "vboxdev.h"
#include "ps2.h"
#include "int2fwin.h"
#include "mousew16.h"

/** If 1, actually call VBox services.
 *  Thus, if 0, this should behave like a plain PS/2 mouse driver. */
#define ENABLE_VBOX 1
/** If 1, hook int2f to detect fullscreen DOSBoxes and auto-disable this driver. */
#define HOOK_INT2F  1

/** Logging through the virtualbox backdoor. */
#define log(...) vbox_logs(__VA_ARGS__)

#define MOUSE_NUM_BUTTONS 2

/** The routine Windows gave us which we should use to report events. */
static LPFN_MOUSEEVENT eventproc;
/** Current status of the mouse driver (see MOUSEFLAGS_*). */
static unsigned char mouseflags;
enum {
	MOUSEFLAGS_ENABLED      = 1 << 0,
	MOUSEFLAGS_HAS_VBOX     = 1 << 1,
	MOUSEFLAGS_VBOX_ENABLED = 1 << 2,
	MOUSEFLAGS_HAS_WIN386   = 1 << 3,
	MOUSEFLAGS_INT2F_HOOKED = 1 << 4
};
/** Last received pressed button status (to compare and see which buttons have been pressed). */
static unsigned char mousebtnstatus;
/** Existing interrupt2f handler. */
static LPFN prev_int2f_handler;

/* This is how events are delivered to Windows */

static void send_event(unsigned short Status, short deltaX, short deltaY, short ButtonCount, short extra1, short extra2);
#pragma aux (MOUSEEVENTPROC) send_event = \
	"call dword ptr [eventproc]"

/* PS/2 BIOS mouse callback. */

#pragma code_seg ( "CALLBACKS" )
static void FAR ps2_mouse_callback(uint8_t status, uint8_t x, uint8_t y, uint8_t z)
{
#pragma aux (PS2_CB) ps2_mouse_callback

	int sstatus = 0;
	int sx =   status & PS2M_STATUS_X_NEG ? 0xFF00 | x : x;
	int sy = -(status & PS2M_STATUS_Y_NEG ? 0xFF00 | y : y);

	if (!(mouseflags & MOUSEFLAGS_ENABLED)) {
		// Likely eventproc is invalid
		return;
	}

	if (sx || sy) {
		sstatus |= SF_MOVEMENT;
	}

#if ENABLE_VBOX
	if ((sstatus & SF_MOVEMENT) && (mouseflags & MOUSEFLAGS_VBOX_ENABLED)) {
		bool abs;
		uint16_t vbx, vby;
		// Even if we are connected to VBox, the user may have selected to disable abs positioning
		// So only report abs coordinates if it is still enabled.
		if (vbox_get_mouse_locked(&abs, &vbx, &vby) == 0 && abs) {
			sx = vbx;
			sy = vby;
			sstatus |= SF_ABSOLUTE;
		}
	}
#endif

	// Now proceed to see which buttons have been pressed down and/or released
	if ((mousebtnstatus & PS2M_STATUS_BUTTON_1) && !(status & PS2M_STATUS_BUTTON_1)) {
		sstatus |= SF_B1_UP;
	} else if (!(mousebtnstatus & PS2M_STATUS_BUTTON_1) && (status & PS2M_STATUS_BUTTON_1)) {
		sstatus |= SF_B1_DOWN;
	}

	if ((mousebtnstatus & PS2M_STATUS_BUTTON_2) && !(status & PS2M_STATUS_BUTTON_2)) {
		sstatus |= SF_B2_UP;
	} else if (!(mousebtnstatus & PS2M_STATUS_BUTTON_2) && (status & PS2M_STATUS_BUTTON_2)) {
		sstatus |= SF_B2_DOWN;
	}

	mousebtnstatus = status & (PS2M_STATUS_BUTTON_1 | PS2M_STATUS_BUTTON_2);

	if (sstatus) {
		send_event(sstatus, sx, sy, MOUSE_NUM_BUTTONS, 0, 0);
	}
}

#if HOOK_INT2F

static void display_switch_handler(int function)
#pragma aux display_switch_handler parm caller [ax] modify [ax bx cx dx si di]
{
	if (!(mouseflags & MOUSEFLAGS_ENABLED) || !(mouseflags & MOUSEFLAGS_VBOX_ENABLED)) {
		return;
	}

	switch (function) {
	case INT2F_NOTIFY_BACKGROUND_SWITCH:
		vbox_logs("Going background\n");
		vbox_set_mouse_locked(false);
		break;
	case INT2F_NOTIFY_FOREGROUND_SWITCH:
		vbox_logs("Going foreground\n");
		vbox_set_mouse_locked(true);
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
		mouseflags |= MOUSEFLAGS_HAS_WIN386;
	}
#endif

#if ENABLE_VBOX
	// However we will check whether VirtualBox exists:
	if (vbox_init() == 0) {
		vbox_logs("VirtualBox found\n");

		// VirtualBox connection was succesful, remember that
		mouseflags |= MOUSEFLAGS_HAS_VBOX;
	}
#endif

	return 1;
}

/** Called by Windows to retrieve information about the mouse hardware. */
WORD FAR PASCAL Inquire(LPMOUSEINFO lpMouseInfo)
{
	lpMouseInfo->msExist = 1;
	lpMouseInfo->msRelative = mouseflags & MOUSEFLAGS_HAS_VBOX ? 0 : 1;
	lpMouseInfo->msNumButtons = MOUSE_NUM_BUTTONS;
	lpMouseInfo->msRate = 40;
	return sizeof(MOUSEINFO);
}

/** Called by Windows to enable the mouse driver.
  * @param lpEventProc Callback function to call when a mouse event happens. */
VOID FAR PASCAL Enable(LPFN_MOUSEEVENT lpEventProc)
{
	cli(); // Write to far pointer may not be atomic, and we could be interrupted mid-write
	eventproc = lpEventProc;
	sti();

	if (!(mouseflags & MOUSEFLAGS_ENABLED)) {
		int err;
		if ((err = ps2_init())) {
			vbox_logs("PS2 init failure\n");
			return;
		}
		if ((err = ps2_set_callback(ps2_mouse_callback))) {
			vbox_logs("PS2 set handler failure\n");
			return;
		}
		if ((err = ps2_enable(true))) {
			vbox_logs("PS2 enable failure\n");
			return;
		}

		vbox_logs("PS/2 Enabled!\n");
		mouseflags |= MOUSEFLAGS_ENABLED;

#if ENABLE_VBOX
		if (mouseflags & MOUSEFLAGS_HAS_VBOX) {
			if ((err = vbox_alloc_buffers())) {
				vbox_logs("VBox alloc failure\n");
				return;
			}

			vbox_report_guest_info(VBOXOSTYPE_Win31);

			if ((err = vbox_set_mouse(true))) {
				vbox_logs("VBox enable failure\n");
				vbox_free_buffers();
				return;
			}

			vbox_logs("VBOX Enabled!\n");
			mouseflags |= MOUSEFLAGS_VBOX_ENABLED;
		}
#endif

#if HOOK_INT2F
		if ((mouseflags & MOUSEFLAGS_HAS_WIN386) && (mouseflags & MOUSEFLAGS_VBOX_ENABLED)) {
			cli();
			hook_int2f(&prev_int2f_handler, int2f_handler);
			sti();
			vbox_logs("int2F hooked!\n");
			mouseflags |= MOUSEFLAGS_INT2F_HOOKED;
		}
#endif
	}
}

/** Called by Windows to disable the mouse driver. */
VOID FAR PASCAL Disable(VOID)
{
	if (mouseflags & MOUSEFLAGS_ENABLED) {
#if HOOK_INT2F
		if (mouseflags & MOUSEFLAGS_INT2F_HOOKED) {
			cli();
			unhook_int2f(prev_int2f_handler);
			sti();
			vbox_logs("int2F unhooked!\n");
			mouseflags &= ~MOUSEFLAGS_INT2F_HOOKED;
		}
#endif

		ps2_enable(false);
		ps2_set_callback(NULL);
		vbox_logs("PS2 Disabled!\n");

		mouseflags &= ~MOUSEFLAGS_ENABLED;

#if ENABLE_VBOX
		if (mouseflags & MOUSEFLAGS_VBOX_ENABLED) {
			vbox_set_mouse(false);
			vbox_free_buffers();
			vbox_logs("VBOX Disabled!\n");
			mouseflags &= ~MOUSEFLAGS_VBOX_ENABLED;
		}
#endif
	}
}

/** Called by Window to retrieve the interrupt vector number used by this driver, or -1. */
int FAR PASCAL MouseGetIntVect(VOID)
{
	return 0x74; /* This corresponds to IRQ12, the PS/2 IRQ. At least in VirtualBox. */
}

