/*
 * VBMouse - Interface to some DOS int 21h services
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

#ifndef INT21DOS_H
#define INT21DOS_H

#include <stdbool.h>
#include <dos.h>

#define DOS_PSP_SIZE 256

enum dos_allocation_strategy {
	DOS_FIT_FIRST    = 0,
	DOS_FIT_BEST     = 1,
	DOS_FIT_LAST     = 2,

	DOS_FIT_HIGH     = 0x80,
	DOS_FIT_HIGHONLY = 0x40,
};

static unsigned dos_query_allocation_strategy(void);
#pragma aux dos_query_allocation_strategy = \
	"mov ax, 0x5800" \
	"int 0x21" \
	__value [ax]

static int dos_set_allocation_strategy(unsigned strategy);
#pragma aux dos_set_allocation_strategy = \
	"clc" \
	"mov ax, 0x5801" \
	"int 0x21" \
	"jc end" \
	"mov ax, 0" \
	"end:" \
	__parm [bx] \
	__value [ax]

static bool dos_query_umb_link_state(void);
#pragma aux dos_query_umb_link_state = \
	"mov ax, 0x5802" \
	"int 0x21" \
	__value [al]

static int dos_set_umb_link_state(bool state);
#pragma aux dos_set_umb_link_state = \
	"clc" \
	"mov ax, 0x5803" \
	"int 0x21" \
	"jc end" \
	"mov ax, 0" \
	"end:" \
	__parm [bx] \
	__value [ax]

/** Allocates a new DOS segment.
 *  @returns either the allocated segment or 0 if any error. */
static __segment dos_alloc(unsigned paragraphs);
#pragma aux dos_alloc = \
	"clc" \
	"mov ah, 0x48" \
	"int 0x21" \
	"jnc end" \
	"mov ax, 0xB" \
	"end:" \
	__parm [bx] \
	__value [ax]

/** Frees DOS segment. */
static void dos_free(_segment segment);
#pragma aux dos_free = \
	"mov ah, 0x49" \
	"int 0x21" \
	__parm [es] \
	__modify [ax]

/** Sets a given PSP as the current active process. */
static void dos_set_psp(_segment psp);
#pragma aux dos_set_psp = \
	"mov ah, 0x50" \
	"int 0x21" \
	__parm [bx] \
	__modify [ax]

#endif // INT21DOS_H
