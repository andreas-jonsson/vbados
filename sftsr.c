/*
 * VBSF - VirtualBox shared folders for DOS, resident part
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

#include <stdlib.h>
#include <string.h>
#include <i86.h>

#include "int21dos.h"
#include "unixtime.h"
#include "vboxshfl.h"
#include "sftsr.h"

TSRDATA data = {
	// TSR installation data
	NULL, /** Previous int2f ISR, storing it for uninstall. */
	NULL, /** Stored pointer for the DOS SDA. */

	// TSR configuration
	0,          /** Offset (in seconds/2) of the current timezone. */
	NULL, NULL, /** NLS support tables. */
	{           /** Codepage to unicode lookup table.
	             *  Initialised to cp437 */
		0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
		0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
		0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
		0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
		0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
		0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
		0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
		0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
		0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
		0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
		0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
		0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
		0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x03BC, 0x03C4,
		0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
		0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
		0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0 }
};

static union {
	SHFLVOLINFO volinfo;
	SHFLFSOBJINFO objinfo;
	SHFLCREATEPARMS create;
} parms;


/** Private buffer for VirtualBox filenames. */
static SHFLSTRING_WITH_BUF(shflstr, SHFL_MAX_LEN);

/** Private buffer where we store VirtualBox-obtained dir entries. */
static SHFLDIRINFO_WITH_NAME_BUF(shfldirinfo, SHFL_MAX_LEN);

#include "unicode.h"
#include "nls.h"
#include "lfn.h"

static uint8_t map_shfl_attr_to_dosattr(const SHFLFSOBJATTR *a)
{
	// DOS attributes are in the higher word of fMode (see RTFS_DOS_*)
	uint8_t attr = (a->fMode >> 16) & 0x3F;
	// Albeit also map the UNIX S_IFDIR attrib to DOS SUBDIR one,
	// since at least on Linux hosts the DOS one may not be set
	if ((a->fMode & 0xF000UL) == 0x4000UL) attr |= _A_SUBDIR;
	return attr;
}

static void map_shfl_info_to_dossft(DOSSFT __far *sft, SHFLFSOBJINFO *i)
{
	sft->attr = map_shfl_attr_to_dosattr(&i->Attr);
	sft->f_size = i->cbObject;
	timestampns_to_dos_time(&sft->f_time, &sft->f_date, i->ModificationTime, data.tz_offset);
}

static void map_shfl_info_to_dosdir(DOSDIR __far *dir, SHFLFSOBJINFO *i)
{
	dir->attr = map_shfl_attr_to_dosattr(&i->Attr);
	dir->f_size = i->cbObject;
	timestampns_to_dos_time(&dir->f_time, &dir->f_date, i->ModificationTime, data.tz_offset);
	dir->start_cluster = 0;
}

static void map_shfl_info_to_getattr(union INTPACK __far *r, SHFLFSOBJINFO *i)
{
	r->w.ax = map_shfl_attr_to_dosattr(&i->Attr);
	r->w.bx = ((uint32_t)i->cbObject) >> 16;
	r->w.di = ((uint16_t)i->cbObject);
	timestampns_to_dos_time(&r->w.cx, &r->w.dx, i->ModificationTime, data.tz_offset);
}

static bool is_valid_dos_file(SHFLFSOBJINFO *i)
{
	if (i->cbObject > UINT32_MAX) {
		// DOS uses 32-bit offsets for files, don't risk showing it large files
		return false;
	}

	return true;
}

/** Try to guess which drive the requested operation is for. */
static int get_op_drive_num(union INTPACK __far *r)
{
	DOSSFT __far *sft;

	switch (r->h.al) {
	case DOS_FN_CLOSE:
	case DOS_FN_COMMIT:
	case DOS_FN_READ:
	case DOS_FN_WRITE:
	case DOS_FN_LOCK:
	case DOS_FN_UNLOCK:
	case DOS_FN_SEEK_END:
		// Some operations use an SFT and we directly get the drive from it
		sft = MK_FP(r->w.es, r->w.di);
		return sft->dev_info & DOS_SFT_DRIVE_MASK;

	case DOS_FN_RMDIR:
	case DOS_FN_MKDIR:
	case DOS_FN_CHDIR:
	case DOS_FN_SET_FILE_ATTR:
	case DOS_FN_GET_FILE_ATTR:
	case DOS_FN_RENAME:
	case DOS_FN_DELETE:
	case DOS_FN_OPEN:
	case DOS_FN_CREATE:
	case DOS_FN_FIND_FIRST:
	case DOS_FN_OPEN_EX:
		// We can use the drive of the first filename argument for these operations
		return drive_letter_to_index(data.dossda->fn1[0]);

	case DOS_FN_GET_DISK_FREE:
		// Otherwise just get the current drive from dos
		return drive_letter_to_index(data.dossda->drive_cds->curr_path[0]);

	case DOS_FN_FIND_NEXT:
		// For find next, we stored the drive inside the SDB during find first.
		return data.dossda->sdb.drive & DOS_SDB_DRIVE_MASK;

	default:
		// We don't support this operation
		return -1;
	}
}

static bool is_call_for_mounted_drive(union INTPACK __far *r)
{
	int drive = get_op_drive_num(r);

	if (drive < 0 || drive >= NUM_DRIVES) {
		return false;
	}

	return data.drives[drive].root != SHFL_ROOT_NIL;
}

static void clear_dos_err(union INTPACK __far *r)
{
	dputs("->ok");
	r->w.flags &= ~INTR_CF;
}

static void set_dos_err(union INTPACK __far *r, int err)
{
	dprintf("->dos error %d\n", err);
	r->w.flags |= INTR_CF;
	r->w.ax = err;
}

static int vbox_err_to_dos(vboxerr err)
{
	switch (err) {
	case VINF_SUCCESS:
		return DOS_ERROR_SUCCESS;
	case VERR_INVALID_PARAMETER:
		return DOS_ERROR_INVALID_FUNCTION;
	case VERR_INVALID_HANDLE:
		return DOS_ERROR_INVALID_HANDLE;
	case VERR_ACCESS_DENIED:
	case VERR_PERMISSION_DENIED:
		return DOS_ERROR_ACCESS_DENIED;
	case VERR_FILE_NOT_FOUND:
		return DOS_ERROR_FILE_NOT_FOUND;
	case VERR_PATH_NOT_FOUND:
		return DOS_ERROR_PATH_NOT_FOUND;
	case VERR_NO_MORE_FILES:
		return DOS_ERROR_NO_MORE_FILES;
	case VERR_ALREADY_EXISTS:
		return DOS_ERROR_FILE_EXISTS;
	case VERR_TOO_MANY_OPEN_FILES:
		return DOS_ERROR_TOO_MANY_OPEN_FILES;
	case VERR_WRITE_PROTECT:
		return DOS_ERROR_WRITE_PROTECT;
	case VERR_NOT_A_DIRECTORY:
	case VERR_DIR_NOT_EMPTY: // Behavior seen in real DOS
		return DOS_ERROR_PATH_NOT_FOUND;
	case VERR_IO_GEN_FAILURE:
		return DOS_ERROR_NOT_READY;
	default:
		return DOS_ERROR_GEN_FAILURE;
	}
}

static void set_vbox_err(union INTPACK __far *r, vboxerr err)
{
	dprintf("->vbox error %ld\n", err);
	set_dos_err(r, vbox_err_to_dos(err));
}

static int my_strrchr(const char __far *str, char c)
{
	int last = -1;
	const char __far *s = str;
	while (*s) {
		if (*s == c) last = s - str;
		s++;
	}
	return last;
}

static inline bool translate_filename_from_host(SHFLSTRING *str, bool case_insensitive, bool uppercase)
{
	bool valid = utf8_to_local(&data, str->ach, str->ach, &str->u16Length);

	if (uppercase) {
		valid = (nls_uppercase(str) || case_insensitive) && valid;
	}

	return valid;
}

static const char * get_basename(const char *path)
{
	int last_sep = my_strrchr(path, '\\');
	if (last_sep >= 0) {
		return &path[last_sep+1];
	} else {
		return path;
	}
}

/** Tries to do some very simple heuristics to convert DOS-style wildcards
 *  into win32-like (as expected by VirtualBox). */
static void fix_wildcards(SHFLSTRING *str)
{
	unsigned i;

	// If this is the standard ????????.??? pattern, replace with *
	// (and not *.*, since we want to accept files without extension).
	if (str->u16Length >= 8+1+3) {
		i = str->u16Length - (8+1+3);
		if (memcmp(&str->ach[i], "????????.???", (8+1+3)) == 0) {
			strcpy(&str->ach[i], "*");
			str->u16Length = i + 1;
		}
	} else if (str->u16Length >= 1+3) {
		// If this ends with .???, remove it, since we want to accept files
		// without extension too.
		i = str->u16Length - (1+3);
		if (memcmp(&str->ach[i], ".???", (1+3)) == 0) {
			str->u16Length = i;
		}
	}

	for (i = 1; i < str->u16Length; i++) {
		// VirtualBox will only match N consecutive ?s to filenames longer than N,
		// while DOS doesn't care.
		// Replace consecutive ???? with ?***,
		// VirtualBox will merge consecutive *s so we don't have to.
		if ( str->ach[i] == '?' &&
		     (str->ach[i-1] == '*' || str->ach[i-1] == '?') ) {
			str->ach[i] = '*';
		}
	}

	// If there is a '~' in the pattern search, assume that this is a shortened
	// file name. Nothing to do for windows short names, as they exist in the
	// host, but for the hash ones, we have to replace the tilde and any following
	// characters with an '*'
	if (!data.short_fnames) {
		for (i = 0; i < str->u16Length; i++) {
			if ( str->ach[i] == '~') {
				strcpy(&str->ach[i], "*");
				str->u16Length = i + 1;
				break;
			}
		}
	}
}


static void copy_drive_relative_filename(SHFLROOT root, SHFLSTRING *str, char __far *path)
{
	// Assume X:.... path for now, i.e. drive_relative path starts at char 
	
	str->u16Length = get_true_host_name( root, &data, str->ach, path + 2, str->u16Size );
	
}

static void copy_drive_relative_dirname(SHFLROOT root, SHFLSTRING *str, char __far *path)
{
	int last_sep = my_strrchr(path + 2, '\\');
	if (last_sep >= 0) {
		str->u16Length = get_true_host_name_n( root, &data, str->ach, path + 2, str->u16Size, last_sep == 0 ? 1 : last_sep );
	} else {
		str->u16Length = get_true_host_name( root, &data, str->ach, path + 2, str->u16Size );
	}
}

static bool copy_to_8_3_filename(char __far *dst, const SHFLSTRING *str)
{
	int last_dot = my_strrchr(str->ach, '.');
	unsigned namelen, extlen;
	bool valid_8_3 = true, dotfile = false;

	namelen = last_dot >= 0 ? last_dot : str->u16Length;
	extlen = last_dot >= 0 ? str->u16Length - (last_dot + 1) : 0;

	if (extlen == 0 && (namelen <= 1 && str->ach[0] == '.')) {
		// . , .. files
		namelen = str->u16Length;
		extlen = 0;
		dotfile = true;
	}

	if (namelen == 0) {
		// Skip files with extension but no name (e.g. ".hid")
		valid_8_3 = false;
	} else if (namelen > 8) {
		namelen = 8;
		valid_8_3 = false;
	}

	if (extlen > 3) {
		extlen = 3;
		valid_8_3 = false;
	}

	_fmemcpy(&dst[0], str->ach, namelen);
	_fmemset(&dst[namelen], ' ', 8 - namelen);
	_fmemcpy(&dst[8], str->ach + last_dot + 1, extlen);
	_fmemset(&dst[8+extlen], ' ', 3 - extlen);

	return valid_8_3 && (dotfile || valid_8_3_file_chars(dst));
}

static bool is_8_3_wildcard(const char __far *name)
{
	return _fmemchr(name, '?', 8+3) != NULL;
}

static bool matches_8_3_wildcard(const char __far *name, const char __far *mask)
{
	unsigned i;
	for (i = 0; i < 8+3; i++) {
		if (*mask != '?' && *name != *mask) return false;
		name++; mask++;
	}

	return true;
}

static unsigned find_free_openfile()
{
	unsigned i;
	for (i = 0; i < NUM_FILES; i++) {
		if (data.files[i].root == SHFL_ROOT_NIL) {
			return i;
		}
	}
	return INVALID_OPENFILE;
}

static bool is_valid_openfile_index(unsigned index)
{
	if (index == INVALID_OPENFILE || index > NUM_FILES) return false;
	if (data.files[index].root == SHFL_ROOT_NIL) return false;
	return true;
}

/** Stores the index of an openfile inside the SFT struct,
  * in case we want to change the format in the future. */
static inline void set_sft_openfile_index(DOSSFT __far *sft, unsigned index)
{
	sft->start_cluster = index;
}

static inline unsigned get_sft_openfile_index(DOSSFT __far *sft)
{
	return sft->start_cluster;
}

static inline void clear_sft_openfile_index(DOSSFT __far *sft)
{
	sft->start_cluster = INVALID_OPENFILE;
}

static inline void set_sdb_openfile_index(DOSSDB __far *sdb, unsigned index)
{
	sdb->dir_entry = index;
}

static inline unsigned get_sdb_openfile_index(DOSSDB __far *sdb)
{
	return sdb->dir_entry;
}

static inline void clear_sdb_openfile_index(DOSSDB __far *sdb)
{
	sdb->dir_entry = INVALID_OPENFILE;
}

/** Closes an openfile entry by index, and marks it as free. */
static vboxerr close_openfile(unsigned openfile)
{
	vboxerr err;

	dprintf("close openfile=%u\n", openfile);

	err = vbox_shfl_close(&data.vb, data.hgcm_client_id,
	                      data.files[openfile].root, data.files[openfile].handle);

	// Even if we have an error on close,
	// assume the file is lost and leak the handle
	data.files[openfile].root = SHFL_ROOT_NIL;
	data.files[openfile].handle = SHFL_HANDLE_NIL;

	return err;
}

/** For an SFT corresponding to an openfile,
 *  flushes any pending metadata changes.
 *  Currently only time & date. */
static void flush_sft_metadata(DOSSFT __far *sft)
{
	unsigned openfile = get_sft_openfile_index(sft);
	vboxerr err;

	if (sft->dev_info & DOS_SFT_FLAG_TIME_SET) {
		unsigned buf_size = sizeof(SHFLFSOBJINFO);

		dputs("setting modified date/time");

		memset(&parms.objinfo, 0, sizeof(SHFLFSOBJINFO));

		timestampns_from_dos_time(&parms.objinfo.ModificationTime, sft->f_time, sft->f_date, data.tz_offset);

		// Set the current file size
		err = vbox_shfl_info(&data.vb, data.hgcm_client_id,
		                     data.files[openfile].root, data.files[openfile].handle,
		                     SHFL_INFO_SET | SHFL_INFO_FILE, &buf_size, &parms.objinfo);
		if (err == 0) {
			sft->dev_info &= ~DOS_SFT_FLAG_TIME_SET;
			if (buf_size == sizeof(SHFLFSOBJINFO)) {
				// VirtualBox returns updated information, let's use it
				map_shfl_info_to_dossft(sft, &parms.objinfo);
			}
		}
	}
}

static void handle_create_open_ex(union INTPACK __far *r)
{
	char __far *path = data.dossda->fn1;
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	DOSSFT __far *sft = MK_FP(r->w.es, r->w.di);
	unsigned int action, mode;
	unsigned openfile;
	bool save_result;
	vboxerr err;

	switch (r->h.al) {
	case DOS_FN_CREATE:
		action = OPENEX_CREATE_IF_NEW | OPENEX_REPLACE_IF_EXISTS;
		mode = OPENEX_MODE_WRITE;
		save_result = false;
		break;
	case DOS_FN_OPEN:
		action = OPENEX_FAIL_IF_NEW | OPENEX_OPEN_IF_EXISTS;
		mode = data.dossda->open_mode & 0x7F;
		save_result = false;
		break;
	case DOS_FN_OPEN_EX:
	default:
		action = data.dossda->openex_act;
		mode = data.dossda->openex_mode & 0x7F;
		save_result = true;
		break;
	}

	dprintf("handle_open for %Fs act=%x mode=%x\n", path, action, mode);

	openfile = find_free_openfile();
	if (openfile == INVALID_OPENFILE) {
		set_dos_err(r, DOS_ERROR_TOO_MANY_OPEN_FILES);
		return;
	}

	copy_drive_relative_filename(root, &shflstr.shflstr, path);

	memset(&parms.create, 0, sizeof(SHFLCREATEPARMS));
	if (action & OPENEX_REPLACE_IF_EXISTS) {
		parms.create.CreateFlags |= SHFL_CF_ACT_REPLACE_IF_EXISTS;
	} else if (action & OPENEX_OPEN_IF_EXISTS) {
		parms.create.CreateFlags |= SHFL_CF_ACT_OPEN_IF_EXISTS;
	} else {
		parms.create.CreateFlags |= SHFL_CF_ACT_FAIL_IF_EXISTS;
	}
	if (action & OPENEX_CREATE_IF_NEW) {
		parms.create.CreateFlags |= SHFL_CF_ACT_CREATE_IF_NEW;
	} else {
		parms.create.CreateFlags |= SHFL_CF_ACT_FAIL_IF_NEW;
	}

	if ((mode & OPENEX_MODE_RDWR) == OPENEX_MODE_RDWR) {
		parms.create.CreateFlags |= SHFL_CF_ACCESS_READWRITE;
	} else if (mode & OPENEX_MODE_WRITE) {
		parms.create.CreateFlags |= SHFL_CF_ACCESS_WRITE;
	} else {
		parms.create.CreateFlags |= SHFL_CF_ACCESS_READ;
	}

	if ((mode & OPENEX_SHARE_MASK) == OPENEX_SHARE_DENYALL) {
		parms.create.CreateFlags |= SHFL_CF_ACCESS_DENYALL;
	} else if ((mode & OPENEX_SHARE_MASK) == OPENEX_SHARE_DENYWRITE) {
		parms.create.CreateFlags |= SHFL_CF_ACCESS_DENYWRITE;
	} else if ((mode & OPENEX_SHARE_MASK) == OPENEX_SHARE_DENYREAD) {
		parms.create.CreateFlags |= SHFL_CF_ACCESS_DENYREAD;
	}

	if (!(parms.create.CreateFlags & SHFL_CF_ACCESS_WRITE)) {
		// Do we really want to create new files without opening them for writing?
		parms.create.CreateFlags |= SHFL_CF_ACT_FAIL_IF_NEW;
	}

	dprintf("vbox create flags=%lx\n", parms.create.CreateFlags);

	err = vbox_shfl_open(&data.vb, data.hgcm_client_id, root, &shflstr.shflstr, &parms.create);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	dprintf("vbox success result=%ld openfile=%u\n", parms.create.Result, openfile);

	switch (parms.create.Result) {
	case SHFL_PATH_NOT_FOUND:
		set_dos_err(r, DOS_ERROR_PATH_NOT_FOUND);
		return;
	case SHFL_FILE_NOT_FOUND:
		set_dos_err(r, DOS_ERROR_FILE_NOT_FOUND);
		return;
	case SHFL_FILE_EXISTS:
		if (save_result) r->w.cx = OPENEX_FILE_OPENED;
		break;
	case SHFL_FILE_CREATED:
		if (save_result) r->w.cx = OPENEX_FILE_CREATED;
		break;
	case SHFL_FILE_REPLACED:
		if (save_result) r->w.cx = OPENEX_FILE_REPLACED;
		break;
	}

	if (parms.create.Handle == SHFL_HANDLE_NIL) {
		set_dos_err(r, DOS_ERROR_GEN_FAILURE);
		return;
	}

	data.files[openfile].root = root;
	data.files[openfile].handle = parms.create.Handle;

	// Fill in the SFT
	map_shfl_info_to_dossft(sft, &parms.create.Info);
	sft->open_mode = mode;
	sft->dev_info = DOS_SFT_FLAG_NETWORK | DOS_SFT_FLAG_CLEAN | drive;
	sft->f_pos = 0;
	set_sft_openfile_index(sft, openfile);

	clear_dos_err(r);
}

static void handle_close(union INTPACK __far *r)
{
	DOSSFT __far *sft = MK_FP(r->w.es, r->w.di);
	unsigned openfile = get_sft_openfile_index(sft);

	dprintf("handle_close openfile=%u\n", openfile);

	if (!is_valid_openfile_index(openfile)) {
		set_dos_err(r, DOS_ERROR_INVALID_HANDLE);
		return;
	}

	flush_sft_metadata(sft);

	dos_sft_decref(sft);
	if (sft->num_handles == 0xFFFF) {
		// SFT is no longer referenced, really close file and clean it up
		vboxerr err = close_openfile(openfile);
		clear_sft_openfile_index(sft);
		sft->num_handles = 0;

		// Pass any error back to DOS, even though we always close the file
		if (err) {
			set_vbox_err(r, err);
			return;
		}
	}

	clear_dos_err(r);
}

static void handle_read(union INTPACK __far *r)
{
	DOSSFT __far *sft = MK_FP(r->w.es, r->w.di);
	unsigned openfile = get_sft_openfile_index(sft);
	uint8_t __far *buffer = data.dossda->cur_dta;
	unsigned long offset = sft->f_pos;
	unsigned bytes = r->w.cx;
	vboxerr err;

	dprintf("handle_read openfile=%u bytes=%u\n", openfile, bytes);

	if (!is_valid_openfile_index(openfile)) {
		set_dos_err(r, DOS_ERROR_INVALID_HANDLE);
		return;
	}

	err = vbox_shfl_read(&data.vb, data.hgcm_client_id,
	                     data.files[openfile].root, data.files[openfile].handle,
	                     offset, &bytes, buffer);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	dprintf("handle_read bytes_read=%u\n", bytes);

	// Advance the file position
	sft->f_pos += bytes;

	r->w.cx = bytes;
	clear_dos_err(r);
}

static void handle_write(union INTPACK __far *r)
{
	DOSSFT __far *sft = MK_FP(r->w.es, r->w.di);
	unsigned openfile = get_sft_openfile_index(sft);
	uint8_t __far *buffer = data.dossda->cur_dta;
	unsigned long offset = sft->f_pos;
	unsigned bytes = r->w.cx;
	vboxerr err;

	dprintf("handle_write openfile=%u bytes=%u\n", openfile, bytes);

	if (!is_valid_openfile_index(openfile)) {
		set_dos_err(r, DOS_ERROR_INVALID_HANDLE);
		return;
	}

	clear_dos_err(r);

	// Mark the SFT as dirty and set date not valid any more
	sft->dev_info &= ~(DOS_SFT_FLAG_CLEAN|DOS_SFT_FLAG_TIME_SET);

	// Special case: If size is 0, truncate to current file position
	if (!bytes) {
	
		err = vbox_shfl_set_file_size(&data.vb, data.hgcm_client_id,
		                              data.files[openfile].root, data.files[openfile].handle,
		                              sft->f_pos);
		if (err) {
			set_vbox_err(r, err);
			return;
		}

		sft->f_size = sft->f_pos;

		return;
	}
	
	err = vbox_shfl_write(&data.vb, data.hgcm_client_id,
	                      data.files[openfile].root, data.files[openfile].handle,
	                      offset, &bytes, buffer);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	dprintf("handle_write bytes_written=%u\n", bytes);

	// Advance the file position
	sft->f_pos += bytes;

	// Assume the file has grown if we've written past the end
	if (sft->f_pos > sft->f_size) sft->f_size = sft->f_pos;

	r->w.cx = bytes;
}

static void handle_commit(union INTPACK __far *r)
{
	DOSSFT __far *sft = MK_FP(r->w.es, r->w.di);
	unsigned openfile = get_sft_openfile_index(sft);
	vboxerr err;

	dprintf("handle_commit openfile=%u\n", openfile);

	if (!is_valid_openfile_index(openfile)) {
		set_dos_err(r, DOS_ERROR_INVALID_HANDLE);
		return;
	}

	flush_sft_metadata(sft);

	err = vbox_shfl_flush(&data.vb, data.hgcm_client_id,
	                      data.files[openfile].root, data.files[openfile].handle);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	clear_dos_err(r);
}

static void handle_lock(union INTPACK __far *r)
{
	DOSSFT __far *sft = MK_FP(r->w.es, r->w.di);
	unsigned openfile = get_sft_openfile_index(sft);
	bool unlock = r->h.bl ? true : false;
	unsigned numops = r->w.cx, i;
	unsigned flags = (unlock ? SHFL_LOCK_CANCEL : SHFL_LOCK_EXCLUSIVE)
	        | SHFL_LOCK_WAIT | SHFL_LOCK_PARTIAL;
	DOSLOCK __far *ops = MK_FP(r->w.ds, r->w.dx);
	vboxerr err;

	dprintf("handle_lock %s numops=%u\n", unlock ? "unlock" : "lock", numops);

	for (i = 0; i < numops; i++) {
		err = vbox_shfl_lock(&data.vb, data.hgcm_client_id,
		                     data.files[openfile].root, data.files[openfile].handle,
		                     ops[i].start_offset, ops[i].region_size, flags);
		if (err) {
			set_vbox_err(r, err);
			return;
		}
	}

	clear_dos_err(r);
}

static void handle_seek_end(union INTPACK __far *r)
{
	DOSSFT __far *sft = MK_FP(r->w.es, r->w.di);
	unsigned openfile = get_sft_openfile_index(sft);
	uint8_t __far *buffer = data.dossda->cur_dta;
	long offset = ((uint32_t)(r->w.cx) << 16) | (uint32_t)(r->w.dx);
	unsigned buf_size = sizeof(SHFLFSOBJINFO);
	vboxerr err;

	dprintf("handle_seek_end openfile=%u offset=%ld\n", openfile, offset);

	memset(&parms.objinfo, 0, sizeof(SHFLFSOBJINFO));

	// Get the current file size
	err = vbox_shfl_info(&data.vb, data.hgcm_client_id,
	                     data.files[openfile].root, data.files[openfile].handle,
	                     SHFL_INFO_GET | SHFL_INFO_FILE, &buf_size, &parms.objinfo);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	if (!is_valid_dos_file(&parms.objinfo)) {
		// If for any reason the file grows to be too long, fail..
		set_dos_err(r, DOS_ERROR_SEEK);
		return;
	}

	// Update current file size
	sft->f_size = parms.objinfo.cbObject;

	dprintf("seek_end filesize=%lu\n", sft->f_size);

	// Update the current offset pointer
	if (offset < 0 && sft->f_size < -offset ) {
		// Seeking beyond start of file
		set_dos_err(r, DOS_ERROR_SEEK);
		return;
	} else if (offset > 0) {
		dputs("seek_end enlarge");
		// Seeking past the end of the file, enlarge
		err = vbox_shfl_set_file_size(&data.vb, data.hgcm_client_id,
		                              data.files[openfile].root, data.files[openfile].handle,
		                              sft->f_size + offset);
		if (err) {
			set_vbox_err(r, err);
			return;
		}

		// Update the file size again, and move the pointer to the end
		sft->f_size = sft->f_size + offset;
		sft->f_pos  = sft->f_size;
	} else {
		// Regular seek
		sft->f_pos = sft->f_size + offset;
	}

	dprintf("seek_end new pos=%lu\n", sft->f_pos);

	// Return new file position in dx:ax
	r->w.dx = sft->f_pos >> 16;
	r->w.ax = sft->f_pos ;

	clear_dos_err(r);
}

static void handle_close_all(union INTPACK __far *r)
{
	unsigned i;

	dputs("handle_close_all");

	for (i = 0; i < NUM_FILES; ++i) {
		if (data.files[i].root != SHFL_ROOT_NIL) {
			close_openfile(i);
		}
	}

	clear_dos_err(r);
}

static void handle_delete(union INTPACK __far *r)
{
	char __far *path = data.dossda->fn1;
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	vboxerr err;

	dprintf("handle_delete %Fs\n", path);

	copy_drive_relative_filename(root, &shflstr.shflstr, path);

	err = vbox_shfl_remove(&data.vb, data.hgcm_client_id, root,
	                       &shflstr.shflstr, SHFL_REMOVE_FILE);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	clear_dos_err(r);
}

static void handle_rename(union INTPACK __far *r)
{
	char __far *src = data.dossda->fn1;
	int srcdrive = drive_letter_to_index(src[0]);
	char __far *dst = data.dossda->fn2;
	int dstdrive = drive_letter_to_index(dst[0]);
	SHFLROOT root = data.drives[srcdrive].root;
	vboxerr err;

	dprintf("handle_rename %Fs to %Fs\n", src, dst);

	if (srcdrive != dstdrive) {
		set_dos_err(r, DOS_ERROR_NOT_SAME_DEVICE);
		return;
	}

	copy_drive_relative_filename(root, &shflstr.shflstr, src);

	// Reusing shfldirinfo buffer space here for our second filename
	copy_drive_relative_filename(root, &shfldirinfo.dirinfo.name, dst);

	err = vbox_shfl_rename(&data.vb, data.hgcm_client_id, root,
	                       &shflstr.shflstr, &shfldirinfo.dirinfo.name,
	                       SHFL_RENAME_DIR | SHFL_RENAME_FILE);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	clear_dos_err(r);
}

static void handle_getattr(union INTPACK __far *r)
{
	char __far *path = data.dossda->fn1;
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	vboxerr err;

	dprintf("handle_getattr %Fs\n", path);

	copy_drive_relative_filename(root, &shflstr.shflstr, path);

	memset(&parms.create, 0, sizeof(SHFLCREATEPARMS));
	parms.create.CreateFlags = SHFL_CF_LOOKUP;

	err = vbox_shfl_open(&data.vb, data.hgcm_client_id, root,
	                     &shflstr.shflstr, &parms.create);
	if (err) {
		set_vbox_err(r, err);
		return;
	}
	switch (parms.create.Result) {
	case SHFL_PATH_NOT_FOUND:
		set_dos_err(r, DOS_ERROR_PATH_NOT_FOUND);
		return;
	case SHFL_FILE_NOT_FOUND:
		set_dos_err(r, DOS_ERROR_FILE_NOT_FOUND);
		return;
	default:
		break;
	}

	map_shfl_info_to_getattr(r, &parms.create.Info);
	clear_dos_err(r);
}

static void handle_setattr(union INTPACK __far *r)
{
	// TODO: Just silently ignoring setattr,
	// since the only attribute we could try to set in a multiplatform way
	// is A_RDONLY by messing with UNIX perms.
	clear_dos_err(r);
}

/** Opens directory corresponding to file in path (i.e. we use the dirname),
 *  filling corresponding openfile entry. */
static vboxerr open_search_dir(unsigned openfile, SHFLROOT root, char __far *path)
{
	vboxerr err;

	dprintf("open_search_dir openfile=%u path=%Fs\n", openfile, path);

	copy_drive_relative_dirname(root, &shflstr.shflstr, path);

	memset(&parms.create, 0, sizeof(SHFLCREATEPARMS));
	parms.create.CreateFlags = SHFL_CF_DIRECTORY
	        | SHFL_CF_ACT_OPEN_IF_EXISTS | SHFL_CF_ACT_FAIL_IF_NEW
	        | SHFL_CF_ACCESS_READ;

	err = vbox_shfl_open(&data.vb, data.hgcm_client_id, root,
	                     &shflstr.shflstr, &parms.create);
	if (err) {
		dputs("open search dir failed");
		return err;
	}

	switch (parms.create.Result) {
	case SHFL_PATH_NOT_FOUND:
		return VERR_PATH_NOT_FOUND;
	case SHFL_FILE_NOT_FOUND:
		return VERR_FILE_NOT_FOUND;
	default:
		break;
	}

	if (parms.create.Handle == SHFL_HANDLE_NIL) {
		dputs("open search dir returned no handle...");
		return VERR_INVALID_HANDLE;
	}

	data.files[openfile].root = root;
	data.files[openfile].handle = parms.create.Handle;

	return 0;
}

/** Simulates a directory entry with the current volume label. */
static vboxerr find_volume_label(SHFLROOT root)
{
	DOSDIR __far *found_file = &data.dossda->found_file;
	vboxerr err;

	shflstring_clear(&shflstr.shflstr);

	err = vbox_shfl_query_map_name(&data.vb, data.hgcm_client_id, root, &shflstr.shflstr);
	if (err) return err;

	(void) translate_filename_from_host(&shflstr.shflstr, false, false);

	dprintf("label: %s\n", shflstr.buf);

	found_file->attr = _A_VOLID;
	copy_to_8_3_filename(found_file->filename, &shflstr.shflstr);
	found_file->f_date = 0;
	found_file->f_time = 0;
	found_file->f_size = 0;
	found_file->start_cluster = 0;

	return 0;
}

/** Gets and fills in the next directory entry from VirtualBox. */
static vboxerr find_next_from_vbox(unsigned openfile, char __far *path)
{
	DOSSDB __far *sdb = &data.dossda->sdb;
	DOSDIR __far *found_file = &data.dossda->found_file;
	uint16_t search_mask;
	vboxerr err;
	uint32_t hash;
	int drive;
	bool case_insensitive;

	// Always accept files with these attributes, even if mask says otherwise
	search_mask = ~(sdb->search_attr | _A_ARCH | _A_RDONLY);

	// It is important that at least for the first call we pass in
	// a correct absolute mask with the correct wildcards;
	// this is what VirtualBox will use in future calls.
	if (path) {
		SHFLROOT root;
		drive = drive_letter_to_index(path[0]);
		root = data.drives[drive].root;

		copy_drive_relative_filename(root, &shflstr.shflstr, path);
		fix_wildcards(&shflstr.shflstr);

		dprintf("fixed path=%s\n", shflstr.buf);

		if (shflstr.shflstr.ach[shflstr.shflstr.u16Length-1] == '\\') {
			// No wildcard?
			return VERR_NO_MORE_FILES;
		}
	} else {
		drive = sdb->drive & DOS_SDB_DRIVE_MASK;
		// For find next calls, it's not really important what we pass here,
		// as long as it's not empty.
		shflstring_strcpy(&shflstr.shflstr, " ");
	}

	case_insensitive = data.drives[drive].case_insensitive;

	while (1) { // Loop until we have a valid file (or an error)
		unsigned size = sizeof(shfldirinfo), resume = 0, count = 0;
		bool valid;

		err = vbox_shfl_list(&data.vb, data.hgcm_client_id,
		                     data.files[openfile].root, data.files[openfile].handle,
		                     SHFL_LIST_RETURN_ONE, &size, &shflstr.shflstr, &shfldirinfo.dirinfo,
		                     &resume, &count);

		// Reset the size of the buffer here since VirtualBox "shortens" it,
		// (since it expects to fit in more entries in the same space, but
		//  we won't allow that via SHFL_LIST_RETURN_ONE).
		shfldirinfo.dirinfo.name.u16Size = sizeof(shfldirinfo.buf);

		if (err) {
			return err;
		}

		if (count != 1) {
			return VERR_IO_BAD_LENGTH;
		}

		dprintf("got diritem name=%s\n", shfldirinfo.dirinfo.name.ach);

		if (!is_valid_dos_file(&shfldirinfo.dirinfo.Info)) {
			dputs("hiding file with invalid info");
			continue;
		}

		// Read file metadata (except for the name)
		map_shfl_info_to_dosdir(found_file, &shfldirinfo.dirinfo.Info);

		// See if this file has the right attributes,
		// we are searching for files that have no attribute bits set other
		// than the ones in search_attr .
		// Except for the ARCH and RDONLY attributes, which are always accepted.
		if (found_file->attr & search_mask) {
			dputs("hiding file with unwanted attrs");
			continue; // Skip this one
		}

		// Now convert the filename

		// Calculate hash using host file name
		hash = lfn_name_hash( shfldirinfo.dirinfo.name.ach, shfldirinfo.dirinfo.name.u16Length );

		if (data.short_fnames && shfldirinfo.dirinfo.cucShortName != 0) {
			valid = utf16_to_local( &data, &shfldirinfo.dirinfo.name.ach, &shfldirinfo.dirinfo.uszShortName, shfldirinfo.dirinfo.cucShortName);
 
			if (!valid) {
				// Should not happen as Windows short names are pure ascii
				dputs("hiding file with illegal character(s)");
				continue;
			}
			shfldirinfo.dirinfo.name.u16Length = shfldirinfo.dirinfo.cucShortName;
			dprintf("  Host short filename: '%s'\n", shfldirinfo.dirinfo.name.ach);
		} else {
			valid = translate_filename_from_host(&shfldirinfo.dirinfo.name, case_insensitive, true);
		}
		
		if (valid) {
			if (!copy_to_8_3_filename(found_file->filename, &shfldirinfo.dirinfo.name)) {
				dputs("Mangling long filename");
				mangle_to_8_3_filename(hash, found_file->filename, &shfldirinfo.dirinfo.name);
			}
		} else {
			dputs("Mangling filename with illegal character(s)");
			mangle_to_8_3_filename(hash, found_file->filename, &shfldirinfo.dirinfo.name);
		}

		if (!matches_8_3_wildcard(found_file->filename, sdb->search_templ)) {
			dputs("hiding file with unwanted filename");
			continue;
		}

		// This file is OK to return, break out of the loop
		break;
	};

	dprintf("accepted file name='%.11Fs' attr=0x%x\n", found_file->filename, found_file->attr);

	return 0;
}

/** Find first file.
 *  Searches in drive/path indicated by fn1,
 *  using the search mask (wildcard) in fcb_fn1.
 *  Should use dossda.sdb to store our internal data,
 *  since programs expect to swap this structure to pause/continue searches.
 *  Return the results in dossda.found_file (see find_next). */
static void handle_find_first(union INTPACK __far *r)
{
	char __far *path = data.dossda->fn1;
	const char __far *search_mask = data.dossda->fcb_fn1;
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	uint8_t search_attr = data.dossda->search_attr;
	unsigned openfile;
	vboxerr err;

	dprintf("find_first path=%Fs mask='%.11Fs' attr=0x%x\n", path, search_mask, search_attr);

	// Initialize the search data block; we'll use it on future calls
	// Even DOS seems to look and check that we did initialize it;
	// it may never call us again if we don't e.g. copy the search mask.
	data.dossda->sdb.drive = DOS_SDB_DRIVE_FLAG_NETWORK | drive;
	data.dossda->sdb.search_attr = search_attr;
	_fmemcpy(data.dossda->sdb.search_templ, search_mask, 8+3);
	data.dossda->sdb.dir_entry = 0;
	data.dossda->sdb.par_clstr = 0;

	// Special case: the search for volume label
	if (search_attr == _A_VOLID) {
		// Simulate an initial entry with the volume label
		// if we are searching for it.
		// DOS actually expects to always find it first, and nothing else.
		dputs("search volid");
		err = find_volume_label(root);
		if (err) {
			dputs("search volid err");
			set_vbox_err(r, err);
			return;
		}
		dputs("search volid OK");
		clear_dos_err(r);
		return;
	} 

	// First, open the desired directory for searching
	openfile = find_free_openfile();
	if (openfile == INVALID_OPENFILE) {
		set_dos_err(r, DOS_ERROR_TOO_MANY_OPEN_FILES);
		return;
	}

	err = open_search_dir(openfile, root, path);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	// Remember it for future calls
	set_sdb_openfile_index(&data.dossda->sdb, openfile);

	err = find_next_from_vbox(openfile, path);
	if (err) {
		// If we are finished, or any other error, close the dir handle
		close_openfile(openfile);
	    clear_sdb_openfile_index(&data.dossda->sdb);
		set_vbox_err(r, err);
		return;
	}

	// Normally we expect the user to repeteadly call FindNext until the last file
	// is returned, so that we can close the directory handle/openfile.
	// However, some programs will call FindFirst to check if a file exists.
	// In this case they pass a full path without any wildcards,
	// and then never call FindNext.
	// Detect this case and free the directory handle immediately.
	if (!is_8_3_wildcard(search_mask)) {
		close_openfile(openfile);
		clear_sdb_openfile_index(&data.dossda->sdb);
	}

	// We will still leak the directory handle if the program stops calling
	// FindNext before reaching the end of the directory, e.g. if it expects
	// a specific file. But why would a program use a wildcard search just to
	// check the existence of a particular file?
	// Hopefully this is a rare situation and we don't leak too many fds.

	// Naturally, Windows 3.x's Winfile does exactly what is described above.
	// On mkdir, it will create a testdir.tmp file on the new directory,
	// then proceed to findfirst "*.*" on it and expect the testdir.tmp file,
	// never to call findnext again.
	// Cue hack to avoid leaking one dirfd for each mkdir...
	if (_fmemcmp(data.dossda->found_file.filename, "TESTDIR TMP", 8+3) == 0) {
		dputs("win3.x testdir detected");
		close_openfile(openfile);
		clear_sdb_openfile_index(&data.dossda->sdb);
	}

	clear_dos_err(r);
}

static void handle_find_next(union INTPACK __far *r)
{
	unsigned openfile = get_sdb_openfile_index(&data.dossda->sdb);
	vboxerr err;

	dprintf("find_next openfile=%u\n", openfile);

	if (!is_valid_openfile_index(openfile)) {
		set_dos_err(r, DOS_ERROR_NO_MORE_FILES);
		return;
	}

	err = find_next_from_vbox(openfile, NULL);
	if (err) {
		close_openfile(openfile);
	    clear_sdb_openfile_index(&data.dossda->sdb);
		set_vbox_err(r, err);
		return;
	}

	clear_dos_err(r);
}

static void handle_chdir(union INTPACK __far *r)
{
	char __far *path = data.dossda->fn1;
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	vboxerr err;

	dprintf("handle_chdir %Fs\n", path);

	// Just have to check if the directory exists
	copy_drive_relative_filename(root, &shflstr.shflstr, path);

	memset(&parms.create, 0, sizeof(SHFLCREATEPARMS));
	parms.create.CreateFlags = SHFL_CF_LOOKUP;

	err = vbox_shfl_open(&data.vb, data.hgcm_client_id, root,
	                     &shflstr.shflstr, &parms.create);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	switch (parms.create.Result) {
	case SHFL_PATH_NOT_FOUND:
	case SHFL_FILE_NOT_FOUND:
		set_dos_err(r, DOS_ERROR_PATH_NOT_FOUND);
		return;
	default:
		break;
	}

	// Also check whether it is really a directory
	if (!(map_shfl_attr_to_dosattr(&parms.create.Info.Attr) & _A_SUBDIR)) {
		set_dos_err(r, DOS_ERROR_PATH_NOT_FOUND);
		return;
	}

	clear_dos_err(r);
}

static void handle_mkdir(union INTPACK __far *r)
{
	char __far *path = data.dossda->fn1;
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	vboxerr err;

	dprintf("handle_mkdir %Fs\n", path);

	copy_drive_relative_filename(root, &shflstr.shflstr, path);

	memset(&parms.create, 0, sizeof(SHFLCREATEPARMS));
	parms.create.CreateFlags = SHFL_CF_DIRECTORY
	        | SHFL_CF_ACT_FAIL_IF_EXISTS | SHFL_CF_ACT_CREATE_IF_NEW;

	err = vbox_shfl_open(&data.vb, data.hgcm_client_id, root,
	                     &shflstr.shflstr, &parms.create);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	switch (parms.create.Result) {
	case SHFL_PATH_NOT_FOUND:
	case SHFL_FILE_NOT_FOUND:
		set_dos_err(r, DOS_ERROR_PATH_NOT_FOUND);
		return;
	case SHFL_FILE_EXISTS:
		set_dos_err(r, DOS_ERROR_FILE_EXISTS);
		return;
	default:
		break;
	}

	// Immediately close newly created directory
	vbox_shfl_close(&data.vb, data.hgcm_client_id, root, parms.create.Handle);

	clear_dos_err(r);
}

static void handle_rmdir(union INTPACK __far *r)
{
	char __far *path = data.dossda->fn1;
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	vboxerr err;

	dprintf("handle_rmdir %Fs\n", path);

	copy_drive_relative_filename(root, &shflstr.shflstr, path);

	err = vbox_shfl_remove(&data.vb, data.hgcm_client_id, root,
	                       &shflstr.shflstr, SHFL_REMOVE_DIR);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	clear_dos_err(r);
}

static uint16_t disk_bytes_to_clusters(uint64_t bytes);
#if BYTES_PER_CLUSTER == 32768
// Avoid the 64-bit divide by hardcoding the following:
#pragma aux disk_bytes_to_clusters = \
	"shrd dx, cx, 15" /* Just need to shift everything by 15 bits to do the division. */ \
	"shrd cx, bx, 15" \
	"shrd bx, ax, 15" \
	"shr  ax,     15" \
	"or ax, bx"  /* Now we need to take the lower 16bits, so if any of the upper bits is set (ax:bx:cx), overflow. */ \
	"or ax, cx" \
	"jz end" \
	"overflow:" \
	"mov dx, 0xFFFF" \
	"end:" \
	__parm [ax bx cx dx] \
	__value [dx] \
	__modify [ax bx cx dx]
#endif

static void handle_get_disk_free(union INTPACK __far *r)
{
	const char __far *path = data.dossda->drive_cds->curr_path; // Use the current path
	int drive = drive_letter_to_index(path[0]);
	SHFLROOT root = data.drives[drive].root;
	unsigned buf_size = sizeof(SHFLVOLINFO);
	vboxerr err;

	dputs("handle disk free");

	memset(&parms.volinfo, 0, sizeof(SHFLVOLINFO));

	// Ask VirtualBox for disk space info
	err = vbox_shfl_info(&data.vb, data.hgcm_client_id, root, SHFL_HANDLE_ROOT,
	                     SHFL_INFO_GET | SHFL_INFO_VOLUME, &buf_size, &parms.volinfo);
	if (err) {
		set_vbox_err(r, err);
		return;
	}

	r->h.ah = 0; // media ID byte
	r->h.al = SECTORS_PER_CLUSTER;
	r->w.cx = BYTES_PER_SECTOR;
	r->w.bx = disk_bytes_to_clusters(parms.volinfo.ullTotalAllocationBytes);
	r->w.dx = disk_bytes_to_clusters(parms.volinfo.ullAvailableAllocationBytes);

	clear_dos_err(r);
}

static bool int2f_11_handler(union INTPACK r)
#pragma aux int2f_11_handler "*" parm caller [] value [al] modify [ax bx cx dx si di es gs fs]
{
	if (r.h.ah != 0x11) return false; // Only interested in network redirector functions
	if (r.h.al == 0xff && r.w.bx == 0x5742 && r.w.cx == 0x5346) {
		// These are the magic numbers to our private "Get TSR data" function
		dputs("Get TSR data");
		r.w.es = get_ds();
		r.w.di = FP_OFF(&data);
		r.w.bx = 0x5444;
		r.w.cx = 1;
		return true;
	}

#if TRACE_CALLS
	dprintf("2f al=%hx\n", r.h.al);
#endif

	// Handle special functions that target all redirectors first
	switch (r.h.al) {
	case DOS_FN_CLOSE_ALL:
		handle_close_all(&r);
		return false; // Let others do the same
	}

	// Now handle normal functions if they refer to our mounted drives
	if (!is_call_for_mounted_drive(&r)) {
		return false;
	}

	switch (r.h.al) {
	case DOS_FN_CLOSE:
		handle_close(&r);
		return true;
	case DOS_FN_CREATE:
	case DOS_FN_OPEN:
	case DOS_FN_OPEN_EX:
		handle_create_open_ex(&r);
		return true;
	case DOS_FN_READ:
		handle_read(&r);
		return true;
	case DOS_FN_WRITE:
		handle_write(&r);
		return true;
	case DOS_FN_COMMIT:
		handle_commit(&r);
		return true;
	case DOS_FN_LOCK:
		handle_lock(&r);
		return true;
	case DOS_FN_SEEK_END:
		handle_seek_end(&r);
		return true;
	case DOS_FN_DELETE:
		handle_delete(&r);
		return true;
	case DOS_FN_RENAME:
		handle_rename(&r);
		return true;
	case DOS_FN_GET_FILE_ATTR:
		handle_getattr(&r);
		return true;
	case DOS_FN_SET_FILE_ATTR:
		handle_setattr(&r);
		return true;
	case DOS_FN_FIND_FIRST:
		handle_find_first(&r);
		return true;
	case DOS_FN_FIND_NEXT:
		handle_find_next(&r);
		return true;
	case DOS_FN_CHDIR:
		handle_chdir(&r);
		return true;
	case DOS_FN_MKDIR:
		handle_mkdir(&r);
		return true;
	case DOS_FN_RMDIR:
		handle_rmdir(&r);
		return true;
	case DOS_FN_GET_DISK_FREE:
		handle_get_disk_free(&r);
		return true;
	}

	return false;
}

void __declspec(naked) __far int2f_isr(void)
{
	__asm {
		pusha
		push ds
		push es
		push fs
		push gs

		mov bp, sp
		push cs
		pop ds

		call int2f_11_handler
		test al, al
		jnz handled

		pop gs
		pop fs
		pop es
		pop ds
		popa

		; Jump to the next handler in the chain
		jmp dword ptr cs:[data + 0] ; wasm doesn't support structs, this is data.prev_int2f_handler

	handled:
		pop gs
		pop fs
		pop es
		pop ds
		popa
		iret
	}
}

static LPTSRDATA int2f_get_tsr_data(void);
#pragma aux int2f_get_tsr_data = \
	"mov ax, 0x11ff" \
	"mov bx, 0x5742" /* Add magic numbers */ \
	"mov cx, 0x5346" \
	"int 0x2f"   \
	"cmp bx, 0x5444" /* Test output magic number */ \
	"jne fail" \
	"cmp cx, 1" \
	"jne fail" \
	"jmp end" \
	"fail:" \
	"xor ax, ax" \
	"mov es, ax" \
	"mov di, ax" \
	"end:" \
	__value [es di] \
	__modify [ax bx cx]

LPTSRDATA __far get_tsr_data(bool installed)
{
	if (installed) {
		return int2f_get_tsr_data();
	} else {
		return MK_FP(get_cs(), FP_OFF(&data));
	}
}

int resident_end;
