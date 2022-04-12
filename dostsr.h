/*
 * VBMouse - utility functions for DOS TSRs
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

#ifndef DOSTSR_H
#define DOSTSR_H

#include <stdint.h>

#include "int21dos.h"

/** Deallocates the environment block from the passed PSP segment. */
static void deallocate_environment(segment_t psp)
{
	// TODO : Too lazy to make PSP struct;
	// 0x2C is offsetof the environment block field on the PSP
	uint16_t __far *envblockP = (uint16_t __far *) MK_FP(psp, 0x2C);
	dos_free(*envblockP);
	*envblockP = 0;
}

/** Copies a program to another location.
 *  @param new_seg PSP segment for the new location
 *  @param old_seg PSP segment for the old location
 *  @param size size of the program to copy including PSP size. */
static void copy_program(segment_t new_seg, segment_t old_seg, unsigned size)
{
	// The MCB is always 1 segment before.
	uint8_t __far *new_mcb = MK_FP(new_seg - 1, 0);
	uint8_t __far *old_mcb = MK_FP(old_seg - 1, 0);
	uint16_t __far *new_mcb_owner = (uint16_t __far *) &new_mcb[1];
	char __far *new_mcb_owner_name = &new_mcb[8];
	char __far *old_mcb_owner_name = &old_mcb[8];

	// Copy entire resident segment including PSP
	_fmemcpy(MK_FP(new_seg, 0), MK_FP(old_seg, 0), size);

	// Make the new MCB point to itself as owner
	*new_mcb_owner = new_seg;

	// Copy the program name, too.
	_fmemcpy(new_mcb_owner_name, old_mcb_owner_name, 8);
}

/** Allocates a UMB of the given size.
 *  If no UMBs are available, this may still return a block in conventional memory. */
static __segment allocate_umb(unsigned size)
{
	bool old_umb_link = dos_query_umb_link_state();
	unsigned int old_strategy = dos_query_allocation_strategy();
	segment_t new_segment;

	dos_set_umb_link_state(true);
	dos_set_allocation_strategy(DOS_FIT_BEST | DOS_FIT_HIGHONLY);

	new_segment = dos_alloc(get_paragraphs(size));

	dos_set_umb_link_state(old_umb_link);
	dos_set_allocation_strategy(old_strategy);

	return new_segment;
}

static __segment reallocate_to_umb(segment_t cur_seg, unsigned segment_size)
{
	segment_t old_segment_psp = cur_seg - (DOS_PSP_SIZE/16);
	segment_t new_segment_psp;

	deallocate_environment(_psp);

	// If we are already in UMA, don't bother
	if (old_segment_psp >= 0xA000) {
		return 0;
	}

	new_segment_psp = allocate_umb(segment_size);

	if (new_segment_psp && new_segment_psp >= 0xA000) {
		segment_t new_segment = new_segment_psp + (DOS_PSP_SIZE/16);

		// Create a new program instance including PSP at the new_segment
		copy_program(new_segment_psp, old_segment_psp, segment_size);

		// Tell DOS to "switch" to the new program
		dos_set_psp(new_segment_psp);

		// Return the new segment
		return new_segment;
	} else {
		if (new_segment_psp) {
			// In case we got another low-memory segment...
			dos_free(new_segment_psp);
		}

		return 0;
	}
}

#endif // DOSTSR_H
