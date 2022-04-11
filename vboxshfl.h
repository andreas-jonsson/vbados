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

#define SHFLSTRING_WITH_BUF(name, size) \
	struct { \
	    SHFLSTRING shflstr; \
	    char buf[size]; \
	} name = { {size, 0} }

#define SHFLDIRINFO_WITH_BUF(name, size) \
	struct { \
	    SHFLDIRINFO dirinfo; \
	    char buf[size]; \
	} name

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

static int32_t vbox_shfl_query_mappings(LPVBOXCOMM vb, hgcm_client_id_t client_id, uint32_t flags, unsigned __far *num_maps, SHFLMAPPING __far *maps)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_QUERY_MAPPINGS, 3);

	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = flags;

	req->aParms[1].type = VMMDevHGCMParmType_32bit;
	req->aParms[1].u.value32 = *num_maps;

	req->aParms[2].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[2].u.Pointer.size = sizeof(SHFLMAPPING) * *num_maps;
	req->aParms[2].u.Pointer.u.linearAddr = linear_addr(maps);

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	*num_maps = req->aParms[1].u.value32;

	return req->header.result;
}

static int32_t vbox_shfl_query_map_name(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLSTRING *name)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_QUERY_MAP_NAME, 2);

	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	req->aParms[1].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[1].u.Pointer.size = shflstring_size_with_buf(name);
	req->aParms[1].u.Pointer.u.linearAddr = linear_addr(name);

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	return req->header.result;
}

static int32_t vbox_shfl_map_folder(LPVBOXCOMM vb, hgcm_client_id_t client_id, const SHFLSTRING *name, SHFLROOT *root)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_MAP_FOLDER, 4);

	// arg 0 in shflstring "name"
	req->aParms[0].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[0].u.Pointer.size = shflstring_size_with_buf(name);
	req->aParms[0].u.Pointer.u.linearAddr = linear_addr(name);

	// arg 1 out uint32 "root"
	req->aParms[1].type = VMMDevHGCMParmType_32bit;
	req->aParms[1].u.value32 = *root;

	// arg 2 in uint32 "delimiter"
	req->aParms[2].type = VMMDevHGCMParmType_32bit;
	req->aParms[2].u.value32 = '\\';

	// arg 3 in uint32 "caseSensitive"
	req->aParms[3].type = VMMDevHGCMParmType_32bit;
	req->aParms[3].u.value32 = 0;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	*root = req->aParms[1].u.value32;

	return req->header.result;
}

static int32_t vbox_shfl_unmap_folder(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_UNMAP_FOLDER, 1);

	// arg 0 in uint32 "root"
	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	return req->header.result;
}

static int32_t vbox_shfl_open(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, const SHFLSTRING *name, SHFLCREATEPARMS *parms)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_CREATE, 3);

	// arg 0 in uint32 "root"
	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	// arg 1 in shflstring "name"
	req->aParms[1].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[1].u.Pointer.size = shflstring_size_with_buf(name);
	req->aParms[1].u.Pointer.u.linearAddr = linear_addr(name);

	// arg 2 in shflcreateparms "parms"
	req->aParms[2].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[2].u.Pointer.size = sizeof(SHFLCREATEPARMS);
	req->aParms[2].u.Pointer.u.linearAddr = linear_addr(parms);

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	return req->header.result;
}

static int32_t vbox_shfl_close(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_CLOSE, 2);

	// arg 0 in uint32 "root"
	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	// arg 1 in uint64 "handle"
	req->aParms[1].type = VMMDevHGCMParmType_64bit;
	req->aParms[1].u.value64 = handle;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	return req->header.result;
}

static int32_t vbox_shfl_read(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                               unsigned long offset, unsigned __far *size, void __far *buffer)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_READ, 5);

	// arg 0 in uint32 "root"
	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	// arg 1 in uint64 "handle"
	req->aParms[1].type = VMMDevHGCMParmType_64bit;
	req->aParms[1].u.value64 = handle;

	// arg 2 in uint64 "offset"
	req->aParms[2].type = VMMDevHGCMParmType_64bit;
	req->aParms[2].u.value64 = offset;

	// arg 3 inout uint32 "size"
	req->aParms[3].type = VMMDevHGCMParmType_32bit;
	req->aParms[3].u.value32 = *size;

	// arg 4 out void "buffer"
	req->aParms[4].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[4].u.Pointer.size = *size;
	req->aParms[4].u.Pointer.u.linearAddr = linear_addr(buffer);

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	*size = req->aParms[3].u.value32;

	return req->header.result;
}

static int32_t vbox_shfl_write(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                               unsigned long offset, unsigned __far *size, void __far *buffer)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_WRITE, 5);

	// arg 0 in uint32 "root"
	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	// arg 1 in uint64 "handle"
	req->aParms[1].type = VMMDevHGCMParmType_64bit;
	req->aParms[1].u.value64 = handle;

	// arg 2 in uint64 "offset"
	req->aParms[2].type = VMMDevHGCMParmType_64bit;
	req->aParms[2].u.value64 = offset;

	// arg 3 inout uint32 "size"
	req->aParms[3].type = VMMDevHGCMParmType_32bit;
	req->aParms[3].u.value32 = *size;

	// arg 4 in void "buffer"
	req->aParms[4].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[4].u.Pointer.size = *size;
	req->aParms[4].u.Pointer.u.linearAddr = linear_addr(buffer);

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	*size = req->aParms[3].u.value32;

	return req->header.result;
}

static int32_t vbox_shfl_list(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root, SHFLHANDLE handle,
                              unsigned flags, unsigned __far *size, const SHFLSTRING *path, SHFLDIRINFO *dirinfo, unsigned __far *resume, unsigned __far *count)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_LIST, 8);

	// arg 0 in uint32 "root"
	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	// arg 1 in uint64 "handle"
	req->aParms[1].type = VMMDevHGCMParmType_64bit;
	req->aParms[1].u.value64 = handle;

	// arg 2 in uint32 "flags"
	req->aParms[2].type = VMMDevHGCMParmType_32bit;
	req->aParms[2].u.value32 = flags;

	// arg 3 inout uint32 "size"
	req->aParms[3].type = VMMDevHGCMParmType_32bit;
	req->aParms[3].u.value32 = *size;

	// arg 4 in shflstring "path"
	req->aParms[4].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[4].u.Pointer.size = shflstring_size_optional_in(path);
	req->aParms[4].u.Pointer.u.linearAddr = linear_addr(path);

	// arg 5 out void "dirinfo"
	req->aParms[5].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[5].u.Pointer.size = *size;
	req->aParms[5].u.Pointer.u.linearAddr = linear_addr(dirinfo);

	// arg 6 inout uint32 "resume_point"
	req->aParms[6].type = VMMDevHGCMParmType_32bit;
	req->aParms[6].u.value32 = *resume;

	// arg 7 out uint32 "count"
	req->aParms[7].type = VMMDevHGCMParmType_32bit;
	req->aParms[7].u.value32 = 0;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	*size = req->aParms[3].u.value32;
	*resume = req->aParms[6].u.value32;
	*count = req->aParms[7].u.value32;

	return req->header.result;
}

static int32_t vbox_shfl_set_utf8(LPVBOXCOMM vb, hgcm_client_id_t client_id)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_SET_UTF8, 0);

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	return req->header.result;
}

static int32_t vbox_shfl_query_map_info(LPVBOXCOMM vb, hgcm_client_id_t client_id, SHFLROOT root,
                                        SHFLSTRING *name, SHFLSTRING *mountPoint, unsigned *flags, unsigned *version)
{
	VMMDevHGCMCall __far *req = (void __far *) vb->buf;
	vbox_hgcm_init_call(req, client_id, SHFL_FN_QUERY_MAP_INFO, 5);

	// arg 0 in uint32 "root"
	req->aParms[0].type = VMMDevHGCMParmType_32bit;
	req->aParms[0].u.value32 = root;

	// arg 1 inout shflstring "name"
	req->aParms[1].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[1].u.Pointer.size = shflstring_size_with_buf(name);
	req->aParms[1].u.Pointer.u.linearAddr = linear_addr(name);

	// arg 2 inout shflstring "mountPoint"
	req->aParms[2].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[2].u.Pointer.size = shflstring_size_with_buf(mountPoint);
	req->aParms[2].u.Pointer.u.linearAddr = linear_addr(mountPoint);

	// arg 3 inout uint64 "flags"
	req->aParms[3].type = VMMDevHGCMParmType_64bit;
	req->aParms[3].u.value64 = *flags;

	// arg 4 out uint32 "version"
	req->aParms[4].type = VMMDevHGCMParmType_32bit;
	req->aParms[4].u.value32 = *version;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	*flags = req->aParms[3].u.value64;
	*version = req->aParms[4].u.value32;

	return req->header.result;
}

#endif // VBOXSHFL_H
