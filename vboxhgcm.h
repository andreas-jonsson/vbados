/*
 * VBMouse - VirtualBox's HGCM protocol communication routines
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

#ifndef VBOXHGCM_H
#define VBOXHGCM_H

#include "vbox.h"
#include "vboxdev.h"

typedef uint32_t hgcm_client_id_t;

static void vbox_hgcm_wait(VMMDevHGCMRequestHeader __far * req)
{
	volatile uint32_t __far * req_flags = &req->fu32Flags;

	while (!(*req_flags & VBOX_HGCM_REQ_DONE)) {
		// TODO yield somehow?
	}
}

static vboxerr vbox_hgcm_connect_existing(LPVBOXCOMM vb, const char *service, hgcm_client_id_t __far *client_id)
{
	VMMDevHGCMConnect __far *req = (void __far *) vb->buf;

	vbox_init_req(&req->header.header, VMMDevReq_HGCMConnect, sizeof(VMMDevHGCMConnect));
	req->loc.type = VMMDevHGCMLoc_LocalHost_Existing;
	_fstrcpy(req->loc.u.host.achName, service);

	req->u32ClientID = 7;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	*client_id = req->u32ClientID;

	return req->header.result;
}

static vboxerr vbox_hgcm_disconnect(LPVBOXCOMM vb, hgcm_client_id_t client_id)
{
	VMMDevHGCMDisconnect __far *req = (void __far *) vb->buf;

	vbox_init_req(&req->header.header, VMMDevReq_HGCMDisconnect, sizeof(VMMDevHGCMDisconnect));
	req->u32ClientID = client_id;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	return req->header.result;
}

static void vbox_hgcm_init_call(VMMDevHGCMCall __far *req, hgcm_client_id_t client_id, uint32_t function, unsigned narg)
{
	vbox_init_req(&req->header.header, VMMDevReq_HGCMCall32, sizeof(VMMDevHGCMCall) + (narg * sizeof(HGCMFunctionParameter)));
	req->u32ClientID = client_id;
	req->u32Function = function;
	req->cParms = narg;
}

static vboxerr vbox_hgcm_do_call_sync(LPVBOXCOMM vb, VMMDevHGCMCall __far *req)
{
	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	if (req->header.header.rc < 0) {
		return req->header.header.rc;
	} else if (req->header.header.rc == VINF_HGCM_ASYNC_EXECUTE) {
		vbox_hgcm_wait(&req->header);
	}

	return 0;
}

static void vbox_hgcm_set_parameter_uint32(VMMDevHGCMCall __far *req, unsigned arg, uint32_t value)
{
	req->aParms[arg].type = VMMDevHGCMParmType_32bit;
	req->aParms[arg].u.value32 = value;
}

static inline uint32_t vbox_hgcm_get_parameter_uint32(VMMDevHGCMCall __far *req, unsigned arg)
{
	return req->aParms[arg].u.value32;
}

static void vbox_hgcm_set_parameter_uint64(VMMDevHGCMCall __far *req, unsigned arg, uint64_t value)
{
	req->aParms[arg].type = VMMDevHGCMParmType_64bit;
	req->aParms[arg].u.value64 = value;
}

static inline uint64_t vbox_hgcm_get_parameter_uint64(VMMDevHGCMCall __far *req, unsigned arg)
{
	return req->aParms[arg].u.value64;
}

static void vbox_hgcm_set_parameter_pointer(VMMDevHGCMCall __far *req, unsigned arg, unsigned size, void __far *ptr)
{
	req->aParms[arg].type = VMMDevHGCMParmType_LinAddr;
	req->aParms[arg].u.LinAddr.cb = size;
	req->aParms[arg].u.LinAddr.uAddr = linear_addr(ptr);
}

#endif // VBOXHGCM_H
