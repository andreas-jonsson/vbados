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

#include "utils.h"
#include "vboxdev.h"

//#define VBOX_BUFFER_SIZE 64
#define VBOX_BUFFER_SIZE (1024 + 32 + 24 + 20)

typedef struct vboxcomm {
	uint16_t iobase;
	char buf[VBOX_BUFFER_SIZE];
	uint32_t buf_physaddr;
} vboxcomm_t;
typedef vboxcomm_t __far * LPVBOXCOMM;

/** Logs a single character to the VBox debug message port. */
static void vbox_log_putc(char c);
#pragma aux vbox_log_putc = \
	"mov dx, 0x504" \
	"out dx, al" \
	__parm [al] \
	__modify [dx]

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

extern int vbox_init(LPVBOXCOMM vb);

/** Lets VirtualBox know that there are VirtualBox Guest Additions on this guest.
  * @param osType os installed on this guest. */
static inline int vbox_report_guest_info(LPVBOXCOMM vb, uint32_t osType)
{
	VMMDevReportGuestInfo __far *req = (void __far *) vb->buf;

	bzero(req, sizeof(VMMDevReportGuestInfo));

	req->header.size = sizeof(VMMDevReportGuestInfo);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_ReportGuestInfo;
	req->header.rc = -1;
	req->guestInfo.interfaceVersion = VMMDEV_VERSION;
	req->guestInfo.osType = osType;

	vbox_send_request(vb->iobase, vb->buf_physaddr);

	return req->header.rc;
}

/** Tells VirtualBox whether we want absolute mouse information or not. */
static inline int vbox_set_mouse(LPVBOXCOMM vb, bool absolute, bool pointer)
{
	VMMDevReqMouseStatus __far *req = (void __far *) vb->buf;

	bzero(req, sizeof(VMMDevReqMouseStatus));

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_SetMouseStatus;
	req->header.rc = -1;
	if (absolute) req->mouseFeatures |= VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE;
	if (pointer)  req->mouseFeatures |= VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR;

	vbox_send_request(vb->iobase, vb->buf_physaddr);

	return req->header.rc;
}

/** Gets the current absolute mouse position from VirtualBox.
  * @param abs false if user has disabled mouse integration in VirtualBox,
  *  in which case we should fallback to PS/2 relative events. */
static inline int vbox_get_mouse(LPVBOXCOMM vb, bool *abs,
                                 uint16_t *xpos, uint16_t *ypos)
{
	VMMDevReqMouseStatus __far *req = (void __far *) vb->buf;

	bzero(req, sizeof(VMMDevReqMouseStatus));

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_GetMouseStatus;
	req->header.rc = -1;

	vbox_send_request(vb->iobase, vb->buf_physaddr);

	*abs = req->mouseFeatures & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE;
	*xpos = req->pointerXPos;
	*ypos = req->pointerYPos;

	return req->header.rc;
}

/** @todo */
static inline int vbox_set_pointer_visible(LPVBOXCOMM vb, bool visible)
{
	VMMDevReqMousePointer __far *req = (void __far *) vb->buf;

	bzero(req, sizeof(VMMDevReqMousePointer));

	req->header.size = sizeof(VMMDevReqMousePointer);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_SetPointerShape;
	req->header.rc = -1;

	if (visible) req->fFlags |= VBOX_MOUSE_POINTER_VISIBLE;

	vbox_send_request(vb->iobase, vb->buf_physaddr);

	return req->header.rc;
}

static inline unsigned vbox_pointer_shape_data_size(unsigned width, unsigned height)
{
	//unsigned base_size = 24 + 20;
	unsigned and_mask_size = (width + 7) / 8 * height;
	unsigned xor_mask_size = width * height * 4;
	return ((and_mask_size + 3) & ~3) + xor_mask_size;
}

static inline int vbox_set_pointer_shape(LPVBOXCOMM vb,
                                         uint16_t xHot, uint16_t yHot,
                                         uint16_t width, uint16_t height,
                                         char __far *data)
{
	VMMDevReqMousePointer __far *req = (void __far *) vb->buf;
	unsigned data_size = vbox_pointer_shape_data_size(width, height);
	unsigned full_size = MAX(sizeof(VMMDevReqMousePointer), 24 + 20 + data_size);

	if (full_size >= VBOX_BUFFER_SIZE) {
		return -2;
	}

	bzero(req, full_size);

	req->header.size = full_size;
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_SetPointerShape;
	req->header.rc = -1;

	req->fFlags = VBOX_MOUSE_POINTER_SHAPE;
	req->xHot = xHot;
	req->yHot = yHot;
	req->width = width;
	req->height = height;

	ffmemcpy(req->pointerData, data, data_size);

	vbox_send_request(vb->iobase, vb->buf_physaddr);

	return req->header.rc;
}

#endif
