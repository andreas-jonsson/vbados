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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <dos.h>
#include "utils.h"

#define DOS_PSP_SIZE 256

enum dos_error {
	DOS_ERROR_SUCCESS = 0,
	DOS_ERROR_INVALID_FUNCTION = 1,
	DOS_ERROR_FILE_NOT_FOUND = 2,
	DOS_ERROR_PATH_NOT_FOUND = 3,
	DOS_ERROR_TOO_MANY_OPEN_FILES = 4,
	DOS_ERROR_ACCESS_DENIED = 5,
	DOS_ERROR_INVALID_HANDLE = 6,
	DOS_ERROR_ARENA_TRASHED = 7,
	DOS_ERROR_NOT_ENOUGH_MEMORY = 8,
	DOS_ERROR_INVALID_BLOCK = 9,
	DOS_ERROR_BAD_ENVIRONMENT = 10,
	DOS_ERROR_BAD_FORMAT = 11,
	DOS_ERROR_INVALID_ACCESS = 12,
	DOS_ERROR_INVALID_DATA = 13,
	DOS_ERROR_OUT_OF_MEMORY = 14,
	DOS_ERROR_INVALID_DRIVE = 15,
	DOS_ERROR_CURRENT_DIRECTORY = 16,
	DOS_ERROR_NOT_SAME_DEVICE = 17,
	DOS_ERROR_NO_MORE_FILES = 18,
	DOS_ERROR_WRITE_PROTECT = 19,
	DOS_ERROR_BAD_UNIT = 20,
	DOS_ERROR_NOT_READY = 21,
	DOS_ERROR_BAD_COMMAND = 22,
	DOS_ERROR_CRC = 23,
	DOS_ERROR_BAD_LENGTH = 24,
	DOS_ERROR_SEEK = 25,
	DOS_ERROR_NOT_DOS_DISK = 26,
	DOS_ERROR_SECTOR_NOT_FOUND = 27,

	DOS_ERROR_GEN_FAILURE = 31,

	DOS_ERROR_HANDLE_EOF = 38,
	DOS_ERROR_HANDLE_DISK_FULL = 39,

	DOS_ERROR_FILE_EXISTS = 80,
	DOS_ERROR_CANNOT_MAKE = 82,
};

// General functions
static inline void dos_print_string(const char *s);
#pragma aux dos_print_string = \
	"mov ah, 0x09" \
	"int 0x21" \
	__parm [dx] \
	__modify [ah]

static inline uint8_t dos_read_character(void);
#pragma aux dos_read_character = \
	"mov ah, 0x08" \
	"int 0x21" \
	__value [al] \
	__modify [ax]

// Memory manipulation functions

typedef __segment segment_t;

enum dos_allocation_strategy {
	DOS_FIT_FIRST    = 0,
	DOS_FIT_BEST     = 1,
	DOS_FIT_LAST     = 2,

	DOS_FIT_HIGH     = 0x80,
	DOS_FIT_HIGHONLY = 0x40,
};

/** Converts bytes to paragraphs (16 bytes), rounding up. */
static inline unsigned get_paragraphs(unsigned bytes)
{
	return (bytes + 15) / 16;
}

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
static void dos_free(segment_t segment);
#pragma aux dos_free = \
	"mov ah, 0x49" \
	"int 0x21" \
	__parm [es] \
	__modify [ax]

/** Sets a given PSP as the current active process. */
static void dos_set_psp(segment_t psp);
#pragma aux dos_set_psp = \
	"mov ah, 0x50" \
	"int 0x21" \
	__parm [bx] \
	__modify [ax]

// Internal DOS structures

// For documentation of these, better grab "Undocumented DOS" or similar book
// These are probably valid only on DOS >= 4

typedef _Packed struct dos_current_directory_structure {
	char curr_path[67];
	uint16_t flags;
	uint32_t disk_blk;
	void __far * info;
	char pad1[11];
} DOSCDS;
STATIC_ASSERT(sizeof(DOSCDS) == 88);

enum dos_cds_flags {
	DOS_CDS_FLAG_PHYSICAL = 0x4000,
	DOS_CDS_FLAG_NETWORK = 0x8000
};

typedef _Packed struct dos_system_file_table_entry {
	uint16_t num_handles;
	uint16_t open_mode;
	uint8_t attr;
	uint16_t dev_info;
	void __far *dpb;
	uint16_t start_cluster;
	uint16_t f_time;
	uint16_t f_date;
	uint32_t f_size;
	uint32_t f_pos;
	uint16_t last_rel_cluster;
	uint16_t last_abs_cluster;
	uint16_t dir_sector;
	uint8_t dir_entry;
	char filename[11];
} DOSSFT;
STATIC_ASSERT(sizeof(DOSSFT) == 43);

enum dos_sft_dev_flags {
	DOS_SFT_FLAG_NETWORK  = 0x8000,
	DOS_SFT_FLAG_TIME_SET = 0x4000,
	DOS_SFT_FLAG_CLEAN    = 0x0040,
	DOS_SFT_DRIVE_MASK    = 0x001F
};

typedef _Packed struct dos_search_data_block {
	uint8_t drive;
	char search_templ[11];
	uint8_t search_attr;
	uint16_t dir_entry;
	uint16_t par_clstr;
	char pad1[4];
} DOSSDB;
STATIC_ASSERT(sizeof(DOSSDB) == 21);

enum dos_sdb_drive_flags {
	DOS_SDB_DRIVE_FLAG_NETWORK = 0x80,
	DOS_SDB_DRIVE_MASK         = 0x1F
};

typedef _Packed struct dos_lock_params {
	uint32_t start_offset;
	uint32_t region_size;
} DOSLOCK;

typedef _Packed struct dos_direntry {
	char filename[11];
	uint8_t attr;
	char pad1[10];
	uint16_t f_time;
	uint16_t f_date;
	uint16_t start_cluster;
	uint32_t f_size;
} DOSDIR;
STATIC_ASSERT(sizeof(DOSDIR) == 32);

typedef _Packed struct dos_swappable_area {
	uint8_t criterr;
	uint8_t indos;
	uint8_t drive_num;
	uint8_t lasterr_dum[9];
	uint8_t __far *cur_dta;
	uint16_t cur_psp;
	uint8_t pad1[4];
	uint8_t cur_drive;
	uint8_t pad2[135];
	char fn1[128];
	char fn2[128];
	DOSSDB sdb;
	DOSDIR found_file;
	DOSCDS drive_cdscopy;
	uint8_t fcb_fn1[11];
	uint8_t pad3[1];
	uint8_t fcb_fn2[11];
	uint8_t pad4[11];
	uint8_t search_attr;
	uint8_t open_mode;
	uint8_t pad5[51];
	DOSCDS __far *drive_cds;
	uint8_t pad6[12];
	uint16_t fn1_csoffset;
	uint16_t fn2_csoffset;
	uint8_t pad7[71];
	uint16_t openex_act;
	uint16_t openex_attr;
	uint16_t openex_mode;
	uint8_t pad8[29];
	DOSSDB rename_srcfile;
	DOSDIR rename_file;
} DOSSDA;
STATIC_ASSERT(offsetof(DOSSDA, cur_dta) == 0xC);
STATIC_ASSERT(offsetof(DOSSDA, cur_drive) == 0x16);
STATIC_ASSERT(offsetof(DOSSDA, fn1) == 0x9E);
STATIC_ASSERT(offsetof(DOSSDA, fn2) == 0x11E);
STATIC_ASSERT(offsetof(DOSSDA, sdb) == 0x19E);
STATIC_ASSERT(offsetof(DOSSDA, found_file) == 0x1B3);
STATIC_ASSERT(offsetof(DOSSDA, search_attr) == 0x24D);
STATIC_ASSERT(offsetof(DOSSDA, open_mode) == 0x24E);
STATIC_ASSERT(offsetof(DOSSDA, drive_cds) == 0x282);
STATIC_ASSERT(offsetof(DOSSDA, openex_act) == 0x2DD);
STATIC_ASSERT(offsetof(DOSSDA, openex_mode) == 0x2E1);

typedef _Packed struct dos_list_of_lists {
	char pad1[22];
	DOSCDS __far *cds;
	char pad2[7];
	uint8_t last_drive;
} DOSLOL;

typedef _Packed struct dos_nls_table {
	uint8_t table_id;
	void __far *table_data;
} NLSTABLE;

typedef _Packed struct file_char_table {
	uint16_t	size;		// table size (not counting this word)
	uint8_t		unk1;		// ??? (01h for MS-DOS 3.30-6.00)
	uint8_t		lowest;		// lowest permissible character value for filename
	uint8_t		highest;	// highest permissible character value for filename
	uint8_t		unk2;		// ??? (00h for MS-DOS 3.30-6.00)
	uint8_t		first_x;	// first excluded character in range \ all characters in this
	uint8_t		last_x;		// last excluded character in range  / range are illegal
	uint8_t		unk3;		// ??? (02h for MS-DOS 3.30-6.00)
	uint8_t		n_illegal;	// number of illegal (terminator) characters
	uint8_t		illegal[1];	// characters which terminate a filename:       ."/\[]:|<>+=;,
} FCHAR;

static inline int drive_letter_to_index(char letter)
{
	if (letter >= 'A' && letter <= 'Z') return letter - 'A';
	else if (letter >= 'a' && letter <= 'z') return letter - 'a';
	else return -1;
}

static inline char drive_index_to_letter(int index)
{
	return 'A' + index;
}

static inline DOSSDA __far * dos_get_swappable_dos_area(void);
#pragma aux dos_get_swappable_dos_area = \
	"push ds" \
	"mov ax, 0x5D06" \
	"int 0x21" \
	"jnc success" \
	"xor si, si" \
	"mov es, si" \
	"jmp end" \
	"success: mov ax, ds" \
	"mov es, ax" \
	"end: pop ds" \
	__value [es si] \
	__modify [ax cx dx]

static inline DOSLOL __far * dos_get_list_of_lists(void);
#pragma aux dos_get_list_of_lists = \
	"mov ax, 0x5200" \
	"int 0x21" \
	__value [es bx] \
	__modify [ax]

static inline uint16_t dos_sft_decref(DOSSFT __far *sft);
#pragma aux dos_sft_decref = \
	"mov ax, 0x1208" \
	"int 0x2F" \
	__parm [es di] \
	__value [ax]

// Network redirector interface

enum DOS_REDIR_SUBFUNCTION {
	DOS_FN_RMDIR     = 0x01,
	DOS_FN_MKDIR     = 0x03,
	DOS_FN_CHDIR     = 0x05,
	DOS_FN_CLOSE     = 0x06,
	DOS_FN_COMMIT    = 0x07,
	DOS_FN_READ      = 0x08,
	DOS_FN_WRITE     = 0x09,
	DOS_FN_LOCK      = 0x0A,
	DOS_FN_UNLOCK    = 0x0B,
	DOS_FN_GET_DISK_FREE = 0x0C,
	DOS_FN_SET_FILE_ATTR = 0x0E,
	DOS_FN_GET_FILE_ATTR = 0x0F,
	DOS_FN_RENAME    = 0x11,
	DOS_FN_DELETE    = 0x13,
	DOS_FN_OPEN      = 0x16,
	DOS_FN_CREATE    = 0x17,
	DOS_FN_FIND_FIRST = 0x1B,
	DOS_FN_FIND_NEXT  = 0x1C,
	DOS_FN_CLOSE_ALL  = 0x1D,
	DOS_FN_DO_REDIR   = 0x1E,
	DOS_FN_PRINTSETUP = 0x1F,
	DOS_FN_FLUSH      = 0x20,
	DOS_FN_SEEK_END   = 0x21,
	DOS_FN_ATEXIT     = 0x22,
	DOS_FN_QUALIFY    = 0x23,
	DOS_FN_OPEN_EX    = 0x2E
};

enum OPENEX_ACTIONS {
	OPENEX_FAIL_IF_EXISTS     = 0x00,
	OPENEX_OPEN_IF_EXISTS     = 0x01,
	OPENEX_REPLACE_IF_EXISTS  = 0x02,
	OPENEX_FAIL_IF_NEW        = 0x00,
	OPENEX_CREATE_IF_NEW      = 0x10,
};

enum OPENEX_MODE {
	OPENEX_MODE_READ       = 0,
	OPENEX_MODE_WRITE      = 1,
	OPENEX_MODE_RDWR       = 2,
	OPENEX_MODE_MASK       = 3,
	OPENEX_SHARE_COMPAT    = 0x00,
	OPENEX_SHARE_DENYALL   = 0x10,
	OPENEX_SHARE_DENYWRITE = 0x20,
	OPENEX_SHARE_DENYREAD  = 0x30,
	OPENEX_SHARE_DENYNONE  = 0x40,
	OPENEX_SHARE_MASK      = 0x70,
};

enum OPENEX_RESULT {
	OPENEX_FILE_OPENED        = 1,
	OPENEX_FILE_CREATED       = 2,
	OPENEX_FILE_REPLACED      = 3,
};

#endif // INT21DOS_H
