/*
 * VBMouse - VirtualBox communication routines
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

#ifndef VBOX_H
#define VBOX_H

#pragma off (unreferenced)

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dlog.h"
#include "int4Bvds.h"
#include "utils.h"
#include "vboxdev.h"

/** Struct containing all the information required to send a VirtualBox message. */
typedef struct vboxcomm {
	/** The IO port of the VirtualBox pci device, found by vbox_init_device(). */
	uint16_t iobase;
	/** Whether we are using VDS or not. */
	bool vds;
	/** The VDS (Virtual DMA service) descriptor corresponding to the buffer that we will use.
	 *  Initialized by vbox_init_buffer(), even if we don't use VDS. */
	VDSDDS dds;
	/** We assume the actual buffer comes in memory after this struct. */
	char buf[];
} vboxcomm_t;
typedef vboxcomm_t * PVBOXCOMM;
typedef vboxcomm_t __far * LPVBOXCOMM;

typedef int32_t vboxerr;

/** Actually send a request to the VirtualBox VMM device.
  * @param addr 32-bit physical address containing the VMMDevRequest struct.
  */
static void vbox_send_request(uint16_t iobase, uint32_t addr);
#pragma aux vbox_send_request = \
	"push eax"     /* Preserve 32-bit register, specifically the higher word. */ \
	"push bx"      /* Combine bx:ax into single 32-bit eax */ \
	"push ax" \
	"pop eax" \
	"out dx, eax" \
	"pop eax" \
	__parm [dx] [bx ax] \
	__modify []

/** Finds the VirtualBox PCI device and reads the current IO base.
  * @returns 0 if the device was found. */
extern int vbox_init_device(LPVBOXCOMM vb);

/** Prepares the buffer used for communicating with VBox and
 *  computes its physical address (using VDS if necessary). */
extern int vbox_init_buffer(LPVBOXCOMM vb, unsigned size);

/** Releases/unlocks buffer, no further use possible. */
extern int vbox_release_buffer(LPVBOXCOMM vb);

static void vbox_init_req(VMMDevRequestHeader __far *hdr, VMMDevRequestType type, unsigned size)
{
	_fmemset(hdr, 0, size);

	hdr->size = size;
	hdr->version = VMMDEV_REQUEST_HEADER_VERSION;
	hdr->requestType = type;
	hdr->rc = VERR_DEV_IO_ERROR; // So that we trigger an error if VirtualBox doesn't actually reply anything
}

/** Lets VirtualBox know that there are VirtualBox Guest Additions on this guest.
  * @param osType os installed on this guest. */
static vboxerr vbox_report_guest_info(LPVBOXCOMM vb, uint32_t osType)
{
	VMMDevReportGuestInfo __far *req = (void __far *) vb->buf;

	vbox_init_req(&req->header, VMMDevReq_ReportGuestInfo, sizeof(VMMDevReportGuestInfo));
	req->guestInfo.interfaceVersion = VMMDEV_VERSION;
	req->guestInfo.osType = osType;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	return req->header.rc;
}

/** Tells VirtualBox whether we want absolute mouse information or not. */
static vboxerr vbox_set_mouse(LPVBOXCOMM vb, bool absolute, bool pointer)
{
	VMMDevReqMouseStatus __far *req = (void __far *) vb->buf;

	vbox_init_req(&req->header, VMMDevReq_SetMouseStatus, sizeof(VMMDevReqMouseStatus));
	if (absolute) req->mouseFeatures |= VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE;
	if (pointer)  req->mouseFeatures |= VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	return req->header.rc;
}

/** Gets the current absolute mouse position from VirtualBox.
  * @param abs false if user has disabled mouse integration in VirtualBox,
  *  in which case we should fallback to PS/2 relative events. */
static vboxerr vbox_get_mouse(LPVBOXCOMM vb, bool __far *abs,
                          uint16_t __far *xpos, uint16_t __far *ypos)
{
	VMMDevReqMouseStatus __far *req = (void __far *) vb->buf;

	vbox_init_req(&req->header, VMMDevReq_GetMouseStatus, sizeof(VMMDevReqMouseStatus));

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	*abs = req->mouseFeatures & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE;
	*xpos = req->pointerXPos;
	*ypos = req->pointerYPos;

	return req->header.rc;
}

/** Asks the host to render the mouse cursor for us. */
static vboxerr vbox_set_pointer_visible(LPVBOXCOMM vb, bool visible)
{
	VMMDevReqMousePointer __far *req = (void __far *) vb->buf;

	vbox_init_req(&req->header, VMMDevReq_SetPointerShape, sizeof(VMMDevReqMousePointer));
	if (visible) req->fFlags |= VBOX_MOUSE_POINTER_VISIBLE;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	return req->header.rc;
}

/** Computes size of a VMMDevReqMousePointer message. */
static inline unsigned vbox_req_mouse_pointer_size(unsigned width, unsigned height)
{
	const unsigned and_mask_size = (width + 7) / 8 * height;
	const unsigned xor_mask_size = width * height * 4;
	const unsigned data_size = and_mask_size + xor_mask_size;
	return MAX(sizeof(VMMDevReqMousePointer), 24 + 20 + data_size);
}

static vboxerr vbox_idle(LPVBOXCOMM vb)
{
	VMMDevReqIdle __far *req = (void __far *) vb->buf;

	vbox_init_req(&req->header, VMMDevReq_Idle, sizeof(VMMDevReqIdle));

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	return req->header.rc;
}

#pragma pop (unreferenced)

#endif
