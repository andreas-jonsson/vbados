/*
 * VBMouse - VirtualBox communication routines
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

#ifndef VBOX_H
#define VBOX_H

#include <stdbool.h>
#include <stdint.h>

/** Logs a single character to the VBox debug message port. */
static void vbox_logc(char c);
#pragma aux vbox_logc = \
	"mov dx, 0x504" \
	"out dx, al" \
	__parm [al] \
	__modify [dx]

/** Logs a string to the VBox debug message port. */
static void vbox_logs(const char __far *str);
#pragma aux vbox_logs = \
	"mov di, si" \
	"xor cx, cx" \
	"not cx"                         /* First we have to strlen(str), prepare -1 as max length */ \
	"xor al, al"                     /* The '\0' that we're searching for */ \
	"cld" \
	"repne scas byte ptr es:[di]"    /* afterwards, cx = -(len+2) */ \
	"not cx"                         /* cx = (len+1) */ \
	"dec cx"                         /* cx = len */ \
	"mov dx, 0x504"                  /* And now we can write the string */ \
	"rep outs dx, byte ptr es:[si]" \
	__parm [es si] \
	__modify [ax cx dx si di]

/** Logs a single character followed by '\n' to force log flush.
  * A life-saver when there is no data segment available. */
#define vbox_putc(c) do { \
		vbox_logc(c); vbox_logc('\n'); \
	} while (0);

extern int vbox_init(void);

extern int vbox_report_guest_info(uint32_t osType);

extern int vbox_set_filter_mask(uint32_t add, uint32_t remove);

extern int vbox_set_mouse(bool enable);

extern int vbox_get_mouse(bool *abs, uint16_t *xpos, uint16_t *ypos);

extern void vbox_init_callbacks();
extern void vbox_deinit_callbacks();

// In CALLBACKS segment:

extern int vbox_get_mouse_locked(bool *abs, uint16_t *xpos, uint16_t *ypos);

#endif
