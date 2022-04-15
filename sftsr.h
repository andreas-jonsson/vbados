/*
 * VBSF - VirtualBox shared folders for DOS, resident interface
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

#ifndef SFTSR_H
#define SFTSR_H

#include <stdbool.h>
#include <stdint.h>

#include "vbox.h"
#include "int21dos.h"

#define LASTDRIVE     'Z'
#define MAX_NUM_DRIVE (LASTDRIVE - 'A')
#define NUM_DRIVES    (MAX_NUM_DRIVE + 1)

/** Maximum number of open files */
#define NUM_FILES     40

/** Directory enumeration needs an open file, this is its index in the "openfile" table. */
#define SEARCH_DIR_FILE 0

/** Size of the VBox buffer. The maximum message length that may be sent.
 *  Enough to fit an HGCM connect call, which is actually larger than most other calls we use ( <= 7 args ).  */
#define VBOX_BUFFER_SIZE (200)

typedef struct {
	uint32_t root;
	uint64_t handle;
} OPENFILE;

typedef struct {
	// TSR installation data
	/** Previous int2f ISR, storing it for uninstall. */
	void (__interrupt __far *prev_int2f_handler)();
	/** Stored pointer for the DOS SDA. */
	DOSSDA __far *dossda;

	// TSR configuration
	/** Offset (in seconds/2) of the current timezone */
	int32_t tz_offset;

	// Current status
	/** Array of all possible DOS drives. */
	struct {
		/** VirtualBox "root" for this drive, or NIL if unmounted. */
		uint32_t root;
	} drives[NUM_DRIVES];

	/** All currently open files.
	 *  index 0 is reserved for the file opened during directory enumeration. */
	OPENFILE files[NUM_FILES];

	// VirtualBox communication
	struct vboxcomm vb;
	char vbbuf[VBOX_BUFFER_SIZE];
	uint32_t hgcm_client_id;
} TSRDATA;

typedef TSRDATA * PTSRDATA;
typedef TSRDATA __far * LPTSRDATA;

extern void __declspec(naked) __far int2f_isr(void);

extern LPTSRDATA __far get_tsr_data(bool installed);

/** This symbol is always at the end of the TSR segment */
extern int resident_end;

/** This is not just data, but the entire segment. */
static inline unsigned get_resident_size(void)
{
	return FP_OFF(&resident_end);
}

#endif // SFTSR_H
