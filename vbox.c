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

#include "dlog.h"
#include "utils.h"
#include "int1Apci.h"
#include "int4Bvds.h"
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

int vbox_init_device(LPVBOXCOMM vb)
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

	vb->iobase = bar & 0xFFFC;

	return 0;
}

int vbox_init_buffer(LPVBOXCOMM vb, unsigned size)
{
	vb->dds.regionSize = size;
	vb->dds.segOrSelector = FP_SEG(&vb->buf);
	vb->dds.offset = FP_OFF(&vb->buf);
	vb->dds.bufferId = 0;
	vb->dds.physicalAddress = 0;
	vb->vds = false;

	if (vds_available()) {
		// Use the Virtual DMA Service to get the physical address of this buffer
		int err;

		err = vds_lock_dma_buffer_region(&vb->dds, VDS_NO_AUTO_ALLOC);
		if (err) {
			// As far as I have seen, most VDS providers always keep low memory contiguous,
			// so I'm not handling VDS_REGION_NOT_CONTIGUOUS here.
			dprintf("Error while VDS locking, err=%d\n", err);
			return err;
		}

		vb->vds = true;
	} else {
		// If VDS is not available,
		// we assume a 1:1 mapping between linear and physical addresses
		vb->dds.physicalAddress = linear_addr(&vb->buf);
	}

	return 0;
}

int vbox_release_buffer(LPVBOXCOMM vb)
{
	if (vb->vds && vds_available()) {
		int err = vds_unlock_dma_buffer_region(&vb->dds, 0);
		if (err) {
			dprintf("Error while VDS unlocking, err=%d\n", err);
			// Ignore the error, it's not like we can do anything
		}
	}
	vb->vds = false;
	vb->dds.regionSize = 0;
	vb->dds.segOrSelector = 0;
	vb->dds.offset = 0;
	vb->dds.bufferId = 0;
	vb->dds.physicalAddress = 0;

	return 0;
}
