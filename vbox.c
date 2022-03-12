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

#include <windows.h>
#include <string.h>
#include <i86.h>

#include "pci.h"
#include "vds.h"
#include "vboxdev.h"
#include "vbox.h"

// PCI device information
/** VBox's PCI bus, device number, etc. */
static pcisel vbpci;
/** IO base of VBox's PCI device. */
static uint16_t vbiobase;
/** IRQ number of VBox's PCI device. Unused. */
static uint8_t vbirq;

/** Handle to fixed buffer used for communication with VBox device.
  * We also use Virtual DMA Service to obtain the physical address of this buffer,
  * so it must remain fixed in memory. */
static HANDLE hBuf;
static LPVOID pBuf;
/** The DMA descriptor used by VDS corresponding to the above buffer. */
static VDS_DDS bufdds;

/** Actually send a request to the VirtualBox VMM device.
  * @param addr 32-bit physical address containing the VMMDevRequest struct.
  */
static void vbox_send_request(uint32_t addr);
#pragma aux vbox_send_request = \
	"movzx eax, ax" /* Need to make a single 32-bit write so combine both regs. */ \
	"movzx ebx, bx" \
	"shl ebx, 16" \
	"or eax, ebx" \
	"mov dx, vbiobase" \
	"out dx, eax" \
	"shr ebx, 16" \
	__parm [bx ax] \
	__modify [dx]

/** Acknowledge an interrupt from the VirtualBox VMM device at iobase.
 *  @return bitmask with all pending events (e.g. VMMDEV_EVENT_MOUSE_CAPABILITIES_CHANGED). */
static uint32_t vbox_irq_ack();
#pragma aux vbox_irq_ack = \
	"mov dx, vbiobase" \
	"add dx, 8" \
	"in eax, dx" \
	"mov edx, eax" \
	"shr edx, 16" \
	__value [dx ax]

// Classic PCI defines
#define VBOX_PCI_VEND_ID 0x80ee
#define VBOX_PCI_PROD_ID 0xcafe
enum {
	CFG_COMMAND         = 0x04, /* Word  */
	CFG_INTERRUPT       = 0x3C, /* Byte */
	CFG_BAR0            = 0x10, /* DWord */
	CFG_BAR1            = 0x14, /* DWord */
};

/** Finds the VirtualBox PCI device and reads the current IO base.
  * @returns 0 if the device was found. */
int vbox_init(void)
{
	int err;
	uint16_t command;
	uint32_t bar;

	if ((err = pci_init_bios())) {
		return -1;
	}

	if ((err = pci_find_device(&vbpci, VBOX_PCI_VEND_ID, VBOX_PCI_PROD_ID, 0))) {
		return -1;
	}

	if ((err = pci_read_config_word(vbpci, CFG_COMMAND, &command))
	    || !(command & 1)) {
		return -2;
	}

	if ((err = pci_read_config_byte(vbpci, CFG_INTERRUPT, &vbirq))) {
		return -2;
	}

	if ((err = pci_read_config_dword(vbpci, CFG_BAR0, &bar))) {
		return -2;
	}

	if (!(bar & 1)) {
		return -2;
	}

	vbiobase = bar & 0xFFFC;

	return 0;
}

/** Allocates the buffers that will be used to communicate with the VirtualBox device. */
int vbox_alloc_buffers(void)
{
	const unsigned int bufferSize = 36; // This should be the largest of all VMMDevRequest* structs that we can send
	int err;

	// Allocate the buffer (double the size for reasons explained below)
	hBuf = GlobalAlloc(GMEM_FIXED|GMEM_SHARE, bufferSize * 2);
	if (!hBuf) return -1;

	// Keep it in memory at a fixed location
	GlobalFix(hBuf);
	GlobalPageLock(hBuf);

	// Get the usable pointer / logical address of the buffer
	pBuf = (LPVOID) GlobalLock(hBuf);
	if (!pBuf) return -1;

	if (vds_available()) {
		// Use the Virtual DMA Service to get the physical address of this buffer
		bufdds.regionSize = bufferSize;
		bufdds.segOrSelector = FP_SEG(pBuf);
		bufdds.offset = FP_OFF(pBuf);
		bufdds.bufferId = 0;
		bufdds.physicalAddress = 0;

		err = vds_lock_dma_buffer_region(&bufdds, VDS_NO_AUTO_ALLOC);
		if (err == VDS_REGION_NOT_CONTIGUOUS) {
			// This is why we made the allocation double the required size
			// If the buffer happens to be on a page boundary,
			// it may not be contiguous and cause the call to fail.
			// So, we try to lock the 2nd half of the allocation,
			// which should not be on a page boundary.
			vbox_logs("VDS try again\n");
			pBuf = (char FAR*) pBuf + bufferSize;
			bufdds.regionSize = bufferSize;
			bufdds.offset += bufferSize;
			err = vds_lock_dma_buffer_region(&bufdds, VDS_NO_AUTO_ALLOC);
		}
		if (err) {
			vbox_logs("VDS lock failure\n");
		}
	} else {
		bufdds.regionSize = 0;  // Indicates we don't have to unlock this later on

		// If VDS is not available, assume we are not paging
		// Just use the linear address as physical
		bufdds.physicalAddress = GetSelectorBase(FP_SEG(pBuf)) + FP_OFF(pBuf);
		err = 0;
	}

	return err;
}

/** Frees the buffers allocated by vbox_alloc_buffers(). */
int vbox_free_buffers(void)
{
	if (bufdds.regionSize > 0) {
		vds_unlock_dma_buffer_region(&bufdds, 0);
	}
	bufdds.regionSize = 0;
	bufdds.segOrSelector = 0;
	bufdds.offset = 0;
	bufdds.bufferId = 0;
	bufdds.physicalAddress = 0;
	GlobalFree(hBuf);
	hBuf = NULL;
	pBuf = NULL;

	return 0;
}

/** Lets VirtualBox know that there are VirtualBox Guest Additions on this guest.
  * @param osType os installed on this guest. */
int vbox_report_guest_info(uint32_t osType)
{
	VMMDevReportGuestInfo *req = pBuf;

	memset(req, 0, sizeof(VMMDevReportGuestInfo));

	req->header.size = sizeof(VMMDevReportGuestInfo);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_ReportGuestInfo;
	req->header.rc = -1;
	req->guestInfo.interfaceVersion = VMMDEV_VERSION;
	req->guestInfo.osType = osType;

	vbox_send_request(bufdds.physicalAddress);

	return req->header.rc;
}

/** Tells VirtualBox about the events we are interested in receiving.
    These events are notified via the PCI IRQ which we are not using, so this is not used either. */
int vbox_set_filter_mask(uint32_t add, uint32_t remove)
{
	VMMDevCtlGuestFilterMask *req = pBuf;

	memset(req, 0, sizeof(VMMDevCtlGuestFilterMask));

	req->header.size = sizeof(VMMDevCtlGuestFilterMask);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_CtlGuestFilterMask;
	req->header.rc = -1;
	req->u32OrMask = add;
	req->u32NotMask = remove;

	vbox_send_request(bufdds.physicalAddress);

	return req->header.rc;
}

/** Tells VirtualBox whether we want absolute mouse information or not. */
int vbox_set_mouse(bool enable)
{
	VMMDevReqMouseStatus *req = pBuf;

	memset(req, 0, sizeof(VMMDevReqMouseStatus));

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_SetMouseStatus;
	req->header.rc = -1;
	req->mouseFeatures = enable ? VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE : 0;

	vbox_send_request(bufdds.physicalAddress);

	return req->header.rc;
}

/** Gets the current absolute mouse position from VirtualBox.
  * @param abs false if user has disabled mouse integration in VirtualBox,
  *  in which case we should fallback to PS/2 relative events. */
int vbox_get_mouse(bool *abs, uint16_t *xpos, uint16_t *ypos)
{
	VMMDevReqMouseStatus *req = pBuf;

	memset(req, 0, sizeof(VMMDevReqMouseStatus));

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_GetMouseStatus;
	req->header.rc = -1;

	vbox_send_request(bufdds.physicalAddress);

	*abs = req->mouseFeatures & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE;
	*xpos = req->pointerXPos;
	*ypos = req->pointerYPos;

	return req->header.rc;
}

#pragma code_seg ( "CALLBACKS" )

/** This is a version of vbox_set_mouse() that does not call any other functions,
  * and may be called inside an interrupt handler.  */
int vbox_set_mouse_locked(bool enable)
{
	VMMDevReqMouseStatus *req = pBuf;

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_SetMouseStatus;
	req->header.rc = -1;
	req->mouseFeatures = enable ? VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE : 0;
	req->pointerXPos = 0;
	req->pointerYPos = 0;

	vbox_send_request(bufdds.physicalAddress);

	return req->header.rc;
}

/** Likewise for vbox_get_mouse() */
int vbox_get_mouse_locked(bool *abs, uint16_t *xpos, uint16_t *ypos)
{
	VMMDevReqMouseStatus *req = pBuf;

	req->header.size = sizeof(VMMDevReqMouseStatus);
	req->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req->header.requestType = VMMDevReq_GetMouseStatus;
	req->header.rc = -1;
	req->mouseFeatures = 0;
	req->pointerXPos = 0;
	req->pointerYPos = 0;

	vbox_send_request(bufdds.physicalAddress);

	*abs = req->mouseFeatures & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE;
	*xpos = req->pointerXPos;
	*ypos = req->pointerYPos;

	return req->header.rc;
}

