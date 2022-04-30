/*
 * VBMouse - VirtualBox Shared Folders service client functions
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

#ifndef VBOXSHFL_H
#define VBOXSHFL_H

#include "vboxhgcm.h"

#define SHFLSTRING_WITH_BUF(varname, bufsize) \
	struct { \
	    SHFLSTRING shflstr; \
	    char buf[bufsize]; \
	} varname = { .shflstr = { .u16Size = bufsize} }

#define SHFLDIRINFO_WITH_NAME_BUF(varname, bufsize) \
	struct { \
	    SHFLDIRINFO dirinfo; \
	    char buf[bufsize]; \
	} varname = { \
	    .dirinfo = { .name = { .u16Size = bufsize} } \
	}

static inline unsigned shflstring_size_with_buf(const SHFLSTRING *str)
{
	return sizeof(SHFLSTRING) + str->u16Size;
}

static inline unsigned shflstring_size_optional_in(const SHFLSTRING *str)
{
	if (str->u16Length > 0) {
		return sizeof(SHFLSTRING) + str->u16Size;
	} else {
		return 0;
	}
}

static inline void shflstring_clear(SHFLSTRING *str)
{
	str->u16Length = 0;
}

static void shflstring_strcpy(SHFLSTRING *str, const char __far *src)
{
	str->u16Length = MIN(_fstrlen(src), str->u16Size - 1);
	_fmemcpy(str->ach, src, str->u16Length);
	str->ach[str->u16Length] = '\0';
}

static void shflstring_strncpy(SHFLSTRING *str, const char __far *src, unsigned len)
{
	str->u16Length = MIN(len, str->u16Size - 1);
	_fmemcpy(str->ach, src, str->u16Length);
	str->ach[str->u16Length] = '\0';
}

static inline void vbox_hgcm_set_parameter_shflroot(VMMDevHGCMCall __far *req, unsigned arg, SHFLROOT root)
{
	vbox_hgcm_set_parameter_uint32(req, arg, root);
}

static inline void vbox_hgcm_set_parameter_shflhandle(VMMDevHGCMCall __far *req, unsigned arg, SHFLHANDLE handle)
{
	vbox_hgcm_set_parameter_uint64(req, arg, handle);
}

static void vbox_hgcm_set_parameter_shflstring(VMMDevHGCMCall __far *req, unsigned arg, const SHFLSTRING *str)
{
	vbox_hgcm_set_parameter_pointer(req, arg, shflstring_size_with_buf(str), str);
}

static vboxerr vbox_shfl_query_mappings(LPVBOXCOMM vb, hgcm_client_id_t client_id, uint32_t flags, unsigned __far *num_maps, SHFLMAPPING __far *maps)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_QUERY_MAPPINGS, 3);

	vbox_hgcm_set_parameter_uint32(req, 0, flags);
	vbox_hgcm_set_parameter_uint32(req, 1, *num_maps);
	vbox_hgcm_set_parameter_pointer(req, 2, sizeof(SHFLMAPPING) * *num_maps, maps);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	*num_maps = vbox_hgcm_get_parameter_uint32(req, 1);

	return req->header.result;
}

static vboxerr vbox_shfl_query_map_name(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLSTRING *name)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_QUERY_MAP_NAME, 2);

	vbox_hgcm_set_parameter_shflroot(req, 0, root);
	vbox_hgcm_set_parameter_shflstring(req, 1, name);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_map_folder(LPVBOXCOMM vb, hgcm_client_id_t client_id, const SHFLSTRING *name, SHFLROOT *root)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_MAP_FOLDER, 4);

	// arg 0 in shflstring "name"
	vbox_hgcm_set_parameter_pointer(req, 0, shflstring_size_with_buf(name), name);

	// arg 1 out uint32 "root"
	vbox_hgcm_set_parameter_uint32(req, 1, 0);

	// arg 2 in uint32 "delimiter"
	vbox_hgcm_set_parameter_uint32(req, 2, '\\');

	// arg 3 in uint32 "caseSensitive"
	vbox_hgcm_set_parameter_uint32(req, 3, 0);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	*root = vbox_hgcm_get_parameter_uint32(req, 1);

	return req->header.result;
}

static vboxerr vbox_shfl_unmap_folder(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_UNMAP_FOLDER, 1);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	if (err = vbox_hgcm_do_call_sync(vb, req))
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_open(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, const SHFLSTRING *name, SHFLCREATEPARMS *parms)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_CREATE, 3);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in shflstring "name"
	vbox_hgcm_set_parameter_shflstring(req, 1, name);

	// arg 2 in shflcreateparms "parms"
	vbox_hgcm_set_parameter_pointer(req, 2, sizeof(SHFLCREATEPARMS), parms);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_close(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_CLOSE, 2);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_read(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                               unsigned long offset, unsigned __far *size, void __far *buffer)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_READ, 5);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	// arg 2 in uint64 "offset"
	vbox_hgcm_set_parameter_uint64(req, 2, offset);

	// arg 3 inout uint32 "size"
	vbox_hgcm_set_parameter_uint32(req, 3, *size);

	// arg 4 out void "buffer"
	vbox_hgcm_set_parameter_pointer(req, 4, *size, buffer);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	*size = vbox_hgcm_get_parameter_uint32(req, 3);

	return req->header.result;
}

static vboxerr vbox_shfl_write(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                               unsigned long offset, unsigned __far *size, void __far *buffer)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_WRITE, 5);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	// arg 2 in uint64 "offset"
	vbox_hgcm_set_parameter_uint64(req, 2, offset);

	// arg 3 inout uint32 "size"
	vbox_hgcm_set_parameter_uint32(req, 3, *size);

	// arg 4 in void "buffer"
	vbox_hgcm_set_parameter_pointer(req, 4, *size, buffer);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	*size = vbox_hgcm_get_parameter_uint32(req, 3);

	return req->header.result;
}

static vboxerr vbox_shfl_lock(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                              unsigned long offset, unsigned long length, unsigned flags)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_LOCK, 5);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	// arg 2 in uint64 "offset"
	vbox_hgcm_set_parameter_uint64(req, 2, offset);

	// arg 3 in uint64 "length"
	vbox_hgcm_set_parameter_uint64(req, 3, length);

	// arg 4 in uint32 "flags"
	vbox_hgcm_set_parameter_uint32(req, 4, flags);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_flush(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_FLUSH, 2);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_list(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                              unsigned flags, unsigned __far *size, const SHFLSTRING *path, SHFLDIRINFO *dirinfo, unsigned __far *resume, unsigned __far *count)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_LIST, 8);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	// arg 2 in uint32 "flags"
	vbox_hgcm_set_parameter_uint32(req, 2, flags);

	// arg 3 inout uint32 "size"
	vbox_hgcm_set_parameter_uint32(req, 3, *size);

	// arg 4 in shflstring "path"
	vbox_hgcm_set_parameter_pointer(req, 4, shflstring_size_optional_in(path), path);

	// arg 5 out void "dirinfo"
	vbox_hgcm_set_parameter_pointer(req, 5, *size, dirinfo);

	// arg 6 inout uint32 "resume_point"
	vbox_hgcm_set_parameter_uint32(req, 6, *resume);

	// arg 7 out uint32 "count"
	vbox_hgcm_set_parameter_uint32(req, 7, 0);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	*size = vbox_hgcm_get_parameter_uint32(req, 3);
	*resume = vbox_hgcm_get_parameter_uint32(req, 6);
	*count = vbox_hgcm_get_parameter_uint32(req, 7);

	return req->header.result;
}

static vboxerr vbox_shfl_info(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                               unsigned flags, unsigned __far *size, void __far *buffer)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_INFORMATION, 5);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	// arg 2 in uint32 "flags"
	vbox_hgcm_set_parameter_uint32(req, 2, flags);

	// arg 3 inout uint32 "size"
	vbox_hgcm_set_parameter_uint32(req, 3, *size);

	// arg 4 inout void "buffer"
	vbox_hgcm_set_parameter_pointer(req, 4, *size, buffer);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	*size = vbox_hgcm_get_parameter_uint32(req, 3);

	return req->header.result;
}

static vboxerr vbox_shfl_remove(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root,
                                const SHFLSTRING *path, unsigned flags)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_REMOVE, 3);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in shflstring "path"
	vbox_hgcm_set_parameter_shflstring(req, 1, path);

	// arg 2 in uint32 "flags"
	vbox_hgcm_set_parameter_uint32(req, 2, flags);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_rename(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root,
                                const SHFLSTRING *src, const SHFLSTRING *dst, unsigned flags)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_RENAME, 4);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in shflstring "src"
	vbox_hgcm_set_parameter_shflstring(req, 1, src);

	// arg 2 in shflstring "dst"
	vbox_hgcm_set_parameter_shflstring(req, 2, dst);

	// arg 3 in uint32 "flags"
	vbox_hgcm_set_parameter_uint32(req, 3, flags);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_set_utf8(LPVBOXCOMM vb, hgcm_client_id_t client_id)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_SET_UTF8, 0);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

static vboxerr vbox_shfl_query_map_info(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root,
                                        SHFLSTRING *name, SHFLSTRING *mountPoint, unsigned *flags, unsigned *version)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_QUERY_MAP_INFO, 5);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 inout shflstring "name"
	vbox_hgcm_set_parameter_shflstring(req, 1, name);

	// arg 2 inout shflstring "mountPoint"
	vbox_hgcm_set_parameter_shflstring(req, 2, mountPoint);

	// arg 3 inout uint64 "flags"
	vbox_hgcm_set_parameter_uint64(req, 3, *flags);

	// arg 4 out uint32 "version"
	vbox_hgcm_set_parameter_uint32(req, 4, 0);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	*flags = vbox_hgcm_get_parameter_uint64(req, 3);
	*version = vbox_hgcm_get_parameter_uint32(req, 4);

	return req->header.result;
}

static vboxerr vbox_shfl_set_file_size(LPVBOXCOMM vb, hgcm_client_id_t client_id,
                                       SHFLROOT root, SHFLHANDLE handle, unsigned long size)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vboxerr err;

	vbox_hgcm_init_call(req, client_id, SHFL_FN_SET_FILE_SIZE, 3);

	// arg 0 in uint32 "root"
	vbox_hgcm_set_parameter_shflroot(req, 0, root);

	// arg 1 in uint64 "handle"
	vbox_hgcm_set_parameter_shflhandle(req, 1, handle);

	// arg 2 in uint64 "new_size"
	vbox_hgcm_set_parameter_uint32(req, 2, size);

	if ((err = vbox_hgcm_do_call_sync(vb, req)) < 0)
		return err;

	return req->header.result;
}

#endif // VBOXSHFL_H
