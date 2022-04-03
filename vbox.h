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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dlog.h"
#include "vds.h"
#include "utils.h"
#include "vboxdev.h"

//#define VBOX_BUFFER_SIZE 64
#define VBOX_BUFFER_SIZE (1024 + 32 + 24 + 20)

typedef struct vboxcomm {
	uint16_t iobase;
	char buf[VBOX_BUFFER_SIZE];
	VDSDDS dds;
} vboxcomm_t;
typedef vboxcomm_t __far * LPVBOXCOMM;

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
	__modify [dx]

/** Finds the VirtualBox PCI device and reads the current IO base.
  * @returns 0 if the device was found. */
extern int vbox_init_device(LPVBOXCOMM vb);

/** Prepares the buffer used for communicating with VBox and
 *  computes its physical address (using VDS if necessary). */
extern int vbox_init_buffer(LPVBOXCOMM vb);

/** Releases/unlocks buffer, no further use possible. */
extern int vbox_release_buffer(LPVBOXCOMM vb);

/** Lets VirtualBox know that there are VirtualBox Guest Additions on this guest.
  * @param osType os installed on this guest. */
static int vbox_report_guest_info(LPVBOXCOMM vb, uint32_t osType)
{
	VMMDevReportGuestInfo __far *req = (void __far *) vb->buf;

	_fmemset(req, 0, sizeof(VMMDevReportGuestInfo));

	req->header.size = sizeof(VMMDevReportGuestInfo);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_ReportGuestInfo;
	req->header.rc = -1;
	req->guestInfo.interfaceVersion = VMMDEV_VERSION;
	req->guestInfo.osType = osType;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	return req->header.rc;
}

/** Tells VirtualBox whether we want absolute mouse information or not. */
static int vbox_set_mouse(LPVBOXCOMM vb, bool absolute, bool pointer)
{
	VMMDevReqMouseStatus __far *req = (void __far *) vb->buf;

	_fmemset(req, 0, sizeof(VMMDevReqMouseStatus));

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_SetMouseStatus;
	req->header.rc = -1;
	if (absolute) req->mouseFeatures |= VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE;
	if (pointer)  req->mouseFeatures |= VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	return req->header.rc;
}

/** Gets the current absolute mouse position from VirtualBox.
  * @param abs false if user has disabled mouse integration in VirtualBox,
  *  in which case we should fallback to PS/2 relative events. */
static int vbox_get_mouse(LPVBOXCOMM vb, bool __far *abs,
                          uint16_t __far *xpos, uint16_t __far *ypos)
{
	VMMDevReqMouseStatus __far *req = (void __far *) vb->buf;

	_fmemset(req, 0, sizeof(VMMDevReqMouseStatus));

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_GetMouseStatus;
	req->header.rc = -1;

	vbox_send_request(vb->iobase, vb->dds.physicalAddress);

	*abs = req->mouseFeatures & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE;
	*xpos = req->pointerXPos;
	*ypos = req->pointerYPos;

	return req->header.rc;
}

/** Asks the host to render the mouse cursor for us. */
static int vbox_set_pointer_visible(LPVBOXCOMM vb, bool visible)
{
	VMMDevReqMousePointer __far *req = (void __far *) vb->buf;

	_fmemset(req, 0, sizeof(VMMDevReqMousePointer));

	req->header.size = sizeof(VMMDevReqMousePointer);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_SetPointerShape;
	req->header.rc = -1;

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

#endif
