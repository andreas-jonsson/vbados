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
#include <i86.h>

#include "pci.h"
#include "vboxdev.h"
#include "vbox.h"

/** VBox's PCI bus, device number, etc. */
static pcisel vbpci;
/** IO base of VBox's PCI device. */
static uint16_t vbiobase;
/** IRQ number of VBox's PCI device. Unused. */
static uint8_t vbirq;

/** Keep a pre-allocated VBox VMMDevRequest struct for the benefit of the callbacks,
    which may be called in a IRQ handler where I'd prefer to avoid making API calls. */
static VMMDevReqMouseStatus __far * reqMouse;
static HANDLE reqMouseH;
static uint32_t reqMouseAddr;

/** Actually send a request to the VirtualBox VMM device.
  * @param addr 32-bit linear address containing the VMMDevRequest struct.
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
	__value [dx ax] \

// Classic PCI defines
#define VBOX_PCI_VEND_ID 0x80ee
#define VBOX_PCI_PROD_ID 0xcafe
enum {
	CFG_COMMAND         = 0x04, /* Word  */
	CFG_INTERRUPT       = 0x3C, /* Byte */
	CFG_BAR0            = 0x10, /* DWord */
	CFG_BAR1            = 0x14, /* DWord */
};

/** Gets the 32-bit linear address of a 16:16 far pointer.
    Uses GetSelectorBase() since we are supposed to work under protected mode. */
static inline uint32_t get_physical_addr(void far *obj)
{
	return GetSelectorBase(FP_SEG(obj)) + FP_OFF(obj);
}

/** Send a request to the VirtualBox VMM device at vbiobase.
 *  @param request must be a VMMDevRequest (e.g. VMMDevReqMouseStatus). */
static inline void vbox_request(void far *request)
{
	uint32_t addr = get_physical_addr(request);
	vbox_send_request(addr);
}

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

/** Lets VirtualBox know that there are VirtualBox Guest Additions on this guest.
  * @param osType os installed on this guest. */
int vbox_report_guest_info(uint32_t osType)
{
	VMMDevReportGuestInfo req = {0};

	req.header.size = sizeof(req);
	req.header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req.header.requestType = VMMDevReq_ReportGuestInfo;
	req.header.rc = -1;
	req.guestInfo.interfaceVersion = VMMDEV_VERSION;
	req.guestInfo.osType = osType;

	vbox_request(&req);

	return req.header.rc;
}

/** Tells VirtualBox about the events we are interested in receiving.
    These events are notified via the PCI IRQ which we are not using, so this is not used either. */
int vbox_set_filter_mask(uint32_t add, uint32_t remove)
{
	VMMDevCtlGuestFilterMask req = {0};

	req.header.size = sizeof(req);
	req.header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req.header.requestType = VMMDevReq_CtlGuestFilterMask;
	req.header.rc = -1;
	req.u32OrMask = add;
	req.u32NotMask = remove;

	vbox_request(&req);

	return req.header.rc;
}

/** Tells VirtualBox whether we want absolute mouse information or not. */
int vbox_set_mouse(bool enable)
{
	VMMDevReqMouseStatus req = {0};

	req.header.size = sizeof(req);
	req.header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req.header.requestType = VMMDevReq_SetMouseStatus;
	req.header.rc = -1;
	req.mouseFeatures = enable ? VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE : 0;

	vbox_request(&req);

	return req.header.rc;
}

/** Gets the current absolute mouse position from VirtualBox.
  * @param abs false if user has disabled mouse integration in VirtualBox,
  *  in which case we should fallback to PS/2 relative events. */
int vbox_get_mouse(bool *abs, uint16_t *xpos, uint16_t *ypos)
{
	VMMDevReqMouseStatus req = {0};

	req.header.size = sizeof(req);
	req.header.version = VMMDEV_REQUEST_HEADER_VERSION;
	req.header.requestType = VMMDevReq_GetMouseStatus;
	req.header.rc = -1;

	vbox_request(&req);

	*abs = req.mouseFeatures & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE;
	*xpos = req.pointerXPos;
	*ypos = req.pointerYPos;

	return req.header.rc;
}

/** Just allocates the memory used by vbox_get_mouse_locked below. */
void vbox_init_callbacks(void)
{
	reqMouseH = GlobalAlloc(GMEM_FIXED|GMEM_SHARE, sizeof(VMMDevReqMouseStatus));
	reqMouse = (LPVOID) GlobalLock(reqMouseH);
	reqMouseAddr = get_physical_addr(reqMouse);
}

void vbox_deinit_callbacks(void)
{
	GlobalFree(reqMouseH);
	reqMouse = NULL;
	reqMouseAddr = 0;
}

#pragma code_seg ( "CALLBACKS" )

/** This is a version of vbox_get_mouse() that does not call any other functions. */
int vbox_get_mouse_locked(bool *abs, uint16_t *xpos, uint16_t *ypos)
{
	// Note that this may be called with interrupts disabled
	reqMouse->header.size = sizeof(VMMDevReqMouseStatus);
	reqMouse->header.version = VMMDEV_REQUEST_HEADER_VERSION;
	reqMouse->header.requestType = VMMDevReq_GetMouseStatus;
	reqMouse->header.rc = -1;
	reqMouse->mouseFeatures = 0;
	reqMouse->pointerXPos = 0;
	reqMouse->pointerYPos = 0;

	vbox_send_request(reqMouseAddr);

	*abs = reqMouse->mouseFeatures & VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE;
	*xpos = reqMouse->pointerXPos;
	*ypos = reqMouse->pointerYPos;

	return reqMouse->header.rc;
}

