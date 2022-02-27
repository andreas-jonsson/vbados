/*
 * VBMouse - Routines to access the PS2 BIOS services
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

#ifndef PS2_H
#define PS2_H

#include <stdbool.h>
#include <stdint.h>

enum {
	PS2M_STATUS_BUTTON_1 = 1 << 0,
	PS2M_STATUS_BUTTON_2 = 1 << 1,
	PS2M_STATUS_BUTTON_3 = 1 << 2,
	PS2M_STATUS_X_NEG    = 1 << 4,
	PS2M_STATUS_Y_NEG    = 1 << 5,
	PS2M_STATUS_X_OVF    = 1 << 6,
	PS2M_STATUS_Y_OVF    = 1 << 7,
};

#pragma aux PS2_CB far loadds parm reverse caller []
// TODO: ax and es look not be preserved with this. VBox BIOS already preserves them though.

/** Invoked by the BIOS when there is a mouse event.
 *  @param status combination of PS2M_STATUS_* flags */
typedef void (__far * LPFN_PS2CALLBACK)(uint8_t status, uint8_t x, uint8_t y, uint8_t z);

static inline void cli(void);
#pragma aux cli = "cli"

static inline void sti(void);
#pragma aux sti = "sti"

static uint8_t ps2_init(void);
#pragma aux ps2_init = \
	"stc"              /* If nothing happens, assume failure */ \
	"mov ax, 0xC205"   /* Pointing device: initialization */ \
	"mov bh, 3"        /* Use 3 byte packets */ \
	"int 0x15" \
	"jc fail" \
	\
	"mov ax, 0xC201"   /* Pointing device: reset */ \
	"int 0x15" \
	"jc fail" \
	\
	"mov ax, 0xC203"   /* Pointing device: set resolution */ \
	"mov bh, 3"        /* 3 count per mm = ~ 200 ppi */ \
	"int 0x15" \
	"jc fail" \
	\
	"mov ax, 0xC202"   /* Pointing device: set sample rate*/ \
	"mov bh, 5"        /* 2: 40 samples per second */ \
	"int 0x15" \
	"jc fail" \
	\
	"mov ax, 0xC206"   /* Pointing device: extended commands */ \
	"mov bh, 1"        /* Set 1:1 scaling */ \
	"int 0x15" \
	"jc fail" \
	\
	"mov ah, 0"        /* Success */ \
	"jmp end" \
	\
	"fail: test ah, ah" /* Ensure we have some error code back, set ah to ff if we don't */ \
	"jnz end" \
	"mov ah, 0xFF" \
	"end:" \
	__value [ah] \
	__modify [ax]

static uint8_t ps2_set_callback(LPFN_PS2CALLBACK callback);
#pragma aux ps2_set_callback = \
	"stc"              /* If nothing happens, assume failure */ \
	"mov ax, 0xC207"   /* Pointing device: set interrupt callback (in es:bx) */ \
	"int 0x15" \
	"jc fail" \
	\
	"mov ah, 0"        /* Success */ \
	"jmp end" \
	\
	"fail: test ah, ah" /* Ensure we have some error code back */ \
	"jnz end" \
	"mov ah, 0xFF" \
	"end:" \
	__parm [es bx] \
	__value [ah] \
	__modify [ax]

static uint8_t ps2_enable(bool enable);
#pragma aux ps2_enable = \
	"test bh, bh"      /* Ensure enable is either 1 or 0 */ \
	"setnz bh" \
	"stc"              /* If nothing happens, assume failure */ \
	"mov ax, 0xC200"   /* Pointing device enable/disable (in bh) */ \
	"int 0x15" \
	"jc fail" \
	\
	"mov ah, 0"        /* Success */ \
	"jmp end" \
	\
	"fail: test ah, ah" /* Ensure we have some error code back */ \
	"jnz end" \
	"mov ah, 0xFF" \
	"end:" \
	__parm [bh] \
	__value [ah] \
	__modify [ax]

#endif
