/*
 * VBSF - Long File Name support
 * Copyright (C) 2011-2022 Eduardo Casino
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General License for more details.
 *
 * You should have received a copy of the GNU General License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LFN_H
#define LFN_H

#include <stdint.h>
#include "unicode.h"
#include "vboxshfl.h"
#include "sftsr.h"

#define REVERSE_HASH 1

#define DEF_HASH_CHARS 3
#define MIN_HASH_CHARS 2
#define MAX_HASH_CHARS 6

#ifdef IN_TSR

static inline bool translate_filename_from_host(SHFLSTRING *, bool, bool);
static bool matches_8_3_wildcard(const char __far *, const char __far *);
static int my_strrchr(const char __far *, char);

/** Private buffer for resolving VirtualBox long filenames. */
static SHFLSTRING_WITH_BUF(shflstrlfn, SHFL_MAX_LEN);

static char fcb_name[12];

static char __far *_fstrchr_local(const char __far *str, char c)
{
	int i;

	for (i = 0; str[i] != '\0' && str[i] != c; ++i)
		;

	return (char __far *)&str[i];
}

static inline char __far *_fstrrchr_local(const char __far *str, char c)
{
	int i;

	for (i = 0; str[i] != '\0'; ++i)
		;
	for (; i && str[i] != c; --i)
		;

	return (char __far *)&str[i];
}

static inline char *_fstrcpy_local(char *dst, const char __far *src)
{
	while (*dst++ = *src++)
		;

	return dst - 1;
}

static inline char hex_digit(uint8_t val)
{
	return val > 9 ? val + 55 : val + 48;
}

/*
 * NOTE: The hash algorithm is from sdbm, a public domain clone of ndbm
 *       by Ozan Yigit (http://www.cse.yorku.ca/~oz/), with the result
 *       folded to the desired hash lengh (method from
 *		 http://isthe.com/chongo/tech/comp/fnv). Hence, lfn_name_hash() is
 *       also in the PUBLIC DOMAIN. (The actual code is fom the version
 *       included in gawk, which uses bit rotations instead of
 *       multiplications)
 */
// Claculates and returns hash_len bits length hash
//
#define FOLD_MASK(x) (((uint32_t)1 << (x)) - 1)
uint32_t lfn_name_hash(
	uint8_t *name, // in : File name in UTF-8
	uint16_t len   // in : File name length
)
{
	uint8_t *be = name + len; /* beyond end of buffer */
	uint32_t hval = 0;
	uint8_t hash_len = data.hash_chars << 2;

	while (name < be)
	{
		/* It seems that calculating the hash backwards gives an slightly
		 * better dispersion for files with names which are the same in their
		 * first part ( like image_000001.jpg, image_000002.jpg, ... )
		 */
#ifdef REVERSE_HASH
		hval = *--be + (hval << 6) + (hval << 16) - hval;
#else
		hval = *name++ + (hval << 6) + (hval << 16) - hval;
#endif
	}

	// Fold result to the desired len
	hval = (((hval >> hash_len) ^ hval) & FOLD_MASK(hash_len));

	return hval;
}

// Generates a valid (and hopefully unique) 8.3 DOS path from an LFN
// This function assumes that fname is already in DOS character set
//
void mangle_to_8_3_filename(
	uint32_t hash,		  // in : Pre-calculated hash of the filename in UTF-8
	char __far *fcb_name, // out : File name in FCB format. Must be 11 bytes long
	SHFLSTRING *str		  // in : File name in DOS codepage
)
{
	char *d, *p, __far *s, *fname = str->ach;
	int i;
	uint8_t mangle = data.hash_chars;
	static int ror[] = {0, 4, 8, 12, 16, 20};

	*(uint32_t __far *)(&fcb_name[0]) = 0x20202020;
	*(uint32_t __far *)(&fcb_name[4]) = 0x20202020;
	*(uint16_t __far *)(&fcb_name[8]) = 0X2020;
	*(uint8_t __far *)(&fcb_name[10]) = 0X20;

	// First, skip leading dots and spaces
	//
	while (*fname == ' ' || *fname == '.')
	{
		++fname;
	}

	// Then look for the last dot in the filename
	//
	d = p = (char *)_fstrrchr_local(fname, '.');

	// There is an extension
	//
	if (p != fname)
	{
		*p = '\0';
		i = 0;
		s = &fcb_name[8];
		while (*++p != '\0' && i < 3)
		{
			if (*p != ' ')
			{
				*s++ = illegal_char(*p) ? '_' : nls_toupper(*p);
				++i;
			}
		}
	}

	i = 0;
	s = fcb_name;
	p = fname;
	while (*p && i < 8)
	{
		if (*p != ' ' && *p != '.')
		{
			*s++ = illegal_char(*p) ? '_' : nls_toupper(*p);
			++i;
		}
		++p;
	}

	if (*d == '\0')
	{
		*d = '.';
	}

	// It is 7 because 1 char is for the tilde '~'
	//
	if (i > 7 - mangle)
	{
		i = 7 - mangle;
	}

	fcb_name[i] = '~';

	while (mangle--)
	{
		fcb_name[++i] = hex_digit((hash >> ror[mangle]) & 0xf);
	}
}

static inline bool match_to_8_3_filename(const char __far *dos_filename, const char *fcb_name)
{
	int len;

	for (len = 0; dos_filename[len] != '.' && dos_filename[len] != '\0' && len < 8; ++len)
	{
		if (dos_filename[len] != fcb_name[len])
		{
			return false;
		}
	}

	// No extension
	if (dos_filename[len] == '\0')
	{
		while (len < 11)
		{
			if (fcb_name[len++] != ' ')
			{
				return false;
			}
		}
		return true;
	}

	if (dos_filename[len] == '.')
	{
		int len2 = len + 1;

		while (len < 8)
		{
			if (fcb_name[len++] != ' ')
			{
				return false;
			}
		}

		while (dos_filename[len2] != '\0' && len < 11)
		{
			if (dos_filename[len2++] != fcb_name[len++])
			{
				return false;
			}
		}
		while (len < 11)
		{
			if (fcb_name[len++] != ' ')
			{
				return false;
			}
		}
		return true;
	}

	// Should not reach here
	return false;
}

static inline char *find_real_name(
	SHFLROOT root,
	TSRDATAPTR data,
	char *dest,			  // out : destination buffer
	char __far *path,	  // in : search path
	char __far *filename, // in : DOS file name
	uint16_t namelen,	  // in : file name length
	uint16_t bufsiz		  // in : buffer size
)
{
	vboxerr err;

	dprintf("find_real_name path=%Fs filename=%Fs\n", path, filename);

	memset(&parms.create, 0, sizeof(SHFLCREATEPARMS));
	parms.create.CreateFlags = SHFL_CF_DIRECTORY | SHFL_CF_ACT_OPEN_IF_EXISTS | SHFL_CF_ACT_FAIL_IF_NEW | SHFL_CF_ACCESS_READ;
	shflstring_strcpy(&shflstrlfn.shflstr, path);

	err = vbox_shfl_open(&data->vb, data->hgcm_client_id, root, &shflstrlfn.shflstr, &parms.create);
	if (err)
	{
		dputs("open search dir failed");
		goto not_found;
	}

	// Check there is room for appending "\*" to the path
	if (shflstrlfn.shflstr.u16Size < shflstrlfn.shflstr.u16Length + 2)
	{
		dputs("path too long");
		vbox_shfl_close(&data->vb, data->hgcm_client_id, root, parms.create.Handle);
		goto not_found;
	}

	*(uint16_t *)&shflstrlfn.shflstr.ach[shflstrlfn.shflstr.u16Length] = '*\\';
	shflstrlfn.shflstr.u16Length += 2;
	shflstrlfn.shflstr.ach[shflstrlfn.shflstr.u16Length] = '\0';

	for (;;)
	{
		unsigned size = sizeof(shfldirinfo), resume = 0, count = 0;
		char *d;
		uint32_t hash;
		bool valid;

		err = vbox_shfl_list(&data->vb, data->hgcm_client_id, root, parms.create.Handle,
							 SHFL_LIST_RETURN_ONE, &size, &shflstrlfn.shflstr, &shfldirinfo.dirinfo,
							 &resume, &count);

		// Reset the size of the buffer
		shfldirinfo.dirinfo.name.u16Size = sizeof(shfldirinfo.buf);

		if (err)
		{
			dputs("vbox_shfl_list() failed");
			vbox_shfl_close(&data->vb, data->hgcm_client_id, root, parms.create.Handle);
			break;
		}

		// Calculate hash using host file name
		hash = lfn_name_hash(shfldirinfo.dirinfo.name.ach, shfldirinfo.dirinfo.name.u16Length);

		// Copy now, because translate_filename_from_host() converts fName to DOS codepage
		//
		if (shfldirinfo.dirinfo.name.u16Length > bufsiz)
		{
			vbox_shfl_close(&data->vb, data->hgcm_client_id, root, parms.create.Handle);
			return (char *)0;
		}
		d = _fstrcpy_local(dest, &shfldirinfo.dirinfo.name.ach);

		translate_filename_from_host(&shfldirinfo.dirinfo.name, false, true);
		mangle_to_8_3_filename(hash, fcb_name, &shfldirinfo.dirinfo.name);

		if (match_to_8_3_filename(filename, fcb_name))
		{
			vbox_shfl_close(&data->vb, data->hgcm_client_id, root, parms.create.Handle);
			return d;
		}
	}

not_found:
	if (namelen > bufsiz)
	{
		return (char *)0;
	}
	else
	{
		return _fstrcpy_local(dest, filename);
	}
}

static uint16_t get_true_host_name_n(SHFLROOT root, TSRDATAPTR data, uint8_t *dst, char __far *src, uint16_t buflen, uint16_t count)
{
	char __far *s, *d;
	char __far *tl, __far *ps, __far *ns, save_ns, save_end;
	uint16_t ret, len = 0;

	if (data->short_fnames)
	{
		return local_to_utf8_n(data, dst, src, buflen, count);
	}

	if (count < buflen)
	{
		save_end = src[count];
		src[count] = '\0';
	}

	dprintf("get_true_host_name_n() src: %Fs\n", src);

	s = src;
	d = dst;

	tl = _fstrchr_local(src, '~');

	while (*tl != '\0')
	{
		//            tl
		//            |
		//  ps ---+   |    +--- ns
		//        |   |    |
		//   /path/fil~enam/
		//        0123456789

		*tl = '\0';
		ps = _fstrrchr_local(s, '\\');
		*tl = '~';

		ns = _fstrchr_local(tl, '\\');

		save_ns = *ns; // Can be '\\' or '\0'
		*ps = *ns = '\0';

		ret = local_to_utf8(data, d, s, buflen - len);

		d += ret;
		len += ret;

		if (buflen <= len)
		{
			goto return_name;
		}

		*d++ = '\\';
		*d = '\0';
		++len;

		d = find_real_name(root, data, d, dst, ps + 1, (uint16_t)(ns - ps + 1), buflen - len);

		if (d == (char *)0)
		{
			goto return_name;
		}

		len = (uint16_t)(d - dst);

		*d = '\0';

		*ps = '\\';
		*ns = save_ns;

		tl = _fstrchr_local(ns, '~');
		s = ns;
	}

	len += local_to_utf8(data, d, s, buflen - len);

return_name:
	if (count < buflen)
	{
		src[count] = save_end;
	}

	dprintf("True host name: '%s', len: %d\n", dst, len);

	return len;
}

static inline uint16_t get_true_host_name(SHFLROOT root, TSRDATAPTR data, uint8_t *dst, char __far *src, uint16_t buflen)
{
	return get_true_host_name_n(root, data, dst, src, buflen, buflen);
}

#endif // IN_TSR
#endif // LFN_H
