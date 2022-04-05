/*
 * VBMouse - VMware communication routines
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

#ifndef VMWARE_H
#define VMWARE_H

#include <stdint.h>

/* Ideas from https://wiki.osdev.org/VMware_tools */

#define VMWARE_MAGIC 0x564D5868UL
#define VMWARE_PORT  0x5658

enum vmware_cmds {
	VMWARE_CMD_GETVERSION          = 10,
	VMWARE_CMD_ABSPOINTER_DATA     = 39,
	VMWARE_CMD_ABSPOINTER_STATUS   = 40,
	VMWARE_CMD_ABSPOINTER_COMMAND  = 41
};

enum vmware_abspointer_cmds {
	VMWARE_ABSPOINTER_CMD_ENABLE           = 0x45414552UL,
	VMWARE_ABSPOINTER_CMD_DISABLE          = 0x000000f5UL,
	VMWARE_ABSPOINTER_CMD_REQUEST_RELATIVE = 0x4c455252UL,
	VMWARE_ABSPOINTER_CMD_REQUEST_ABSOLUTE = 0x53424152UL,
};

enum vmware_abspointer_status {
	VMWARE_ABSPOINTER_STATUS_MASK_ERROR    = 0xFFFF0000UL,
	VMWARE_ABSPOINTER_STATUS_MASK_DATA     = 0x0000FFFFUL
};

enum vmware_abspointer_data_status {
	VMWARE_ABSPOINTER_STATUS_RELATIVE      = 0x00010000U,
	VMWARE_ABSPOINTER_STATUS_BUTTON_LEFT   = 0x20,
	VMWARE_ABSPOINTER_STATUS_BUTTON_RIGHT  = 0x10,
	VMWARE_ABSPOINTER_STATUS_BUTTON_MIDDLE = 0x08,
};

struct vmware_call_regs {
	union {
		uint32_t eax;
		uint32_t magic;
	};
	union {
		uint32_t ebx;
		uint32_t size;
	};
	union {
		uint32_t ecx;
		uint32_t command;
	};
	union {
		uint32_t edx;
		uint32_t port;
	};
};

struct vmware_abspointer_data {
	uint32_t status;
	int32_t x;
	int32_t y;
	int32_t z;
};

#define VMWARE_ABSPOINTER_DATA_PACKET_SIZE 4

static inline void vmware_call(struct vmware_call_regs __far * regs);
#pragma aux vmware_call = \
	"pushad" /* First make a copy of all 32-bit registers, to avoid clobbering them. */ \
	"mov eax, es:[di + 4*0]" \
	"mov ebx, es:[di + 4*1]" \
	"mov ecx, es:[di + 4*2]" \
	"mov edx, es:[di + 4*3]" \
	"in eax, dx"  /* This performs the actual backdoor call. */ \
	"mov es:[di + 4*0], eax" \
	"mov es:[di + 4*1], ebx" \
	"mov es:[di + 4*2], ecx" \
	"mov es:[di + 4*3], edx" \
	"popad" \
	__parm [es di] \
	__modify []

/** Checks the presence of the VMware backdoor by comparing magic numbers.
 *  @return detected version of the vmware protocol (0-6), -1 if not detected. */
static int32_t vmware_get_version(void)
{
	struct vmware_call_regs regs;
	regs.magic = VMWARE_MAGIC;
	regs.port = VMWARE_PORT;
	regs.command = VMWARE_CMD_GETVERSION;
	regs.ebx = ~VMWARE_MAGIC;
	vmware_call(&regs);
	if (regs.ebx != VMWARE_MAGIC) {
		return -1;
	}
	return regs.eax;
}

/** Sends a command to the VMware absolute pointing interface.
 *  @param cmd see VMWARE_ABSPOINTER_CMD_*. */
static void vmware_abspointer_cmd(uint32_t cmd)
{
	struct vmware_call_regs regs;
	regs.magic = VMWARE_MAGIC;
	regs.port = VMWARE_PORT;
	regs.command = VMWARE_CMD_ABSPOINTER_COMMAND;
	regs.ebx = cmd;
	vmware_call(&regs);
}

/** Gets the status reg from the VMware absolute pointing interface.
 *  @return status
 *     VMWARE_ABSPOINTER_STATUS_MASK_ERROR bits: error flags.
 *     VMWARE_ABSPOINTER_STATUS_MASK_DATA bits: amount of data available to read. */
static uint32_t vmware_abspointer_status(void)
{
	struct vmware_call_regs regs;
	regs.magic = VMWARE_MAGIC;
	regs.port = VMWARE_PORT;
	regs.command = VMWARE_CMD_ABSPOINTER_STATUS;
	regs.ebx = 0;
	vmware_call(&regs);
	return regs.eax;
}

/** Reads data from the VMware absolute pointing interface.
 *  @param size amount of data to read (value between 1 and 4)
 *    ensure it is less than the amount of data available,
 *    qemu disconnects us if we read more than we can.
 *  @return status
 *     VMWARE_ABSPOINTER_STATUS_MASK_ERROR bits: error flags.
 *     VMWARE_ABSPOINTER_STATUS_MASK_DATA bits: amount of data available to read. */
static void vmware_abspointer_data(unsigned size, struct vmware_abspointer_data __far * data)
{
	union u {
		struct vmware_call_regs regs;
		struct vmware_abspointer_data data;
	} __far *p = (union u __far *) data;
	p->regs.magic = VMWARE_MAGIC;
	p->regs.port = VMWARE_PORT;
	p->regs.command = VMWARE_CMD_ABSPOINTER_DATA;
	p->regs.size = size;
	vmware_call(&p->regs);
}

/** Reads (and discards) all available data in the VMware absolute pointing interface. */
static void vmware_abspointer_data_clear(void)
{
	uint16_t data_avail;
	// Drop any data in the queue
	while ((data_avail = vmware_abspointer_status())) {
		struct vmware_abspointer_data data;
		vmware_abspointer_data(MIN(4, data_avail), &data);
	}
}

#endif // VMWARE_H
