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

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <i86.h>

#include "pci.h"
#include "vds.h"
#include "dlog.h"
#include "vboxdev.h"
#include "vbox.h"

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
static int vbox_find_iobase(uint16_t __far *iobase)
{
	int err;
	pcisel pcidev;
	uint16_t command;
	uint32_t bar;

	if ((err = pci_init_bios())) {
		return err;
	}

	if ((err = pci_find_device(&pcidev, VBOX_PCI_VEND_ID, VBOX_PCI_PROD_ID, 0))) {
		return err;
	}

	if ((err = pci_read_config_word(pcidev, CFG_COMMAND, &command))) {
		return err;
	}

	if (!(command & 1)) {
		// The card is not configured
		return -1;
	}

	if ((err = pci_read_config_dword(pcidev, CFG_BAR0, &bar))) {
		return err;
	}

	if (!(bar & 1)) {
		// This is not an IO BAR
		return -2;
	}

	*iobase = bar & 0xFFFC;

	return 0;
}

static int vbox_get_phys_addr(uint32_t __far *physaddr, void __far *buf)
{
	int err = 0;
#if 0
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
		bufdds.physicalAddress = FP_SEG(pBuf + FP_OFF(pBuf);
		err = 0;
	}
#endif

	*physaddr = ((uint32_t)(FP_SEG(buf)) << 4) + FP_OFF(buf);

	return err;
}

int vbox_init(LPVBOXCOMM vb)
{
	int err;

	if ((err = vbox_find_iobase(&vb->iobase))) {
		return err;
	}

	if (err = vbox_get_phys_addr(&vb->buf_physaddr, vb->buf)) {
		return err;
	}

	return 0;
}
