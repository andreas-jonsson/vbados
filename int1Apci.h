/*
 * VBMouse - PCI BIOS communication routines
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

#ifndef INT1APCI_H
#define INT1APCI_H

typedef unsigned char pcierr;
enum {
	PCI_SUCCESSFUL = 0,
	PCI_GENERIC_ERROR = 1,
	PCI_FUNC_NOT_SUPPORTED = 0x81,
	PCI_BAD_VENDOR_ID = 0x83,
	PCI_DEVICE_NOT_FOUND = 0x86,
	PCI_BAD_REGISTER_NUMBER = 0x87,
	PCI_SET_FAILED = 0x88,
	PCI_BUFFER_TOO_SMALL = 0x89
};

typedef unsigned short pcisel;

static pcierr pci_init_bios(void);
#pragma aux pci_init_bios = \
	"mov ax, 0xB101" \
	"int 0x1A" \
	"jc not_found" \
	"cmp ah, 0" \
	"jne not_found" \
	"cmp edx, ' ICP'" \
	"jne not_found" \
	"mov ah, 0" \
	"jmp end" \
	"not_found: mov ah, 1" \
	"end:" \
	__value [ah] \
	__modify [ax bx cx dx]

static pcierr pci_find_device(pcisel __far *sel, unsigned short vendor_id, unsigned short dev_id, unsigned short index);
#pragma aux pci_find_device = \
	"mov ax, 0xB102" \
	"int 0x1A" \
	"jnc success" \
	"mov ah, 1" \
	"success: mov [es:di], bx" \
	__parm [es di] [dx] [cx] [si] \
	__value [ah] \
	__modify [ax bx]

/* Reading from configuration space */

static pcierr pci_read_config_byte(pcisel sel, unsigned char reg, unsigned char __far *data);
#pragma aux pci_read_config_byte = \
	"mov ax, 0xB108" \
	"int 0x1A" \
	"mov [es:si], cl" \
	__parm [bx] [di] [es si] \
	__value [ah] \
	__modify [ax cx]

static pcierr pci_read_config_word(pcisel sel, unsigned char reg, unsigned short __far *data);
#pragma aux pci_read_config_word = \
	"mov ax, 0xB109" \
	"int 0x1A" \
	"mov [es:si], cx" \
	__parm [bx] [di] [es si] \
	__value [ah] \
	__modify [ax cx]

static pcierr pci_read_config_dword(pcisel sel, unsigned char reg, unsigned long __far *data);
#pragma aux pci_read_config_dword = \
	"mov ax, 0xB10A" \
	"int 0x1A" \
	"mov [es:si], ecx" \
	__parm [bx] [di] [es si] \
	__value [ah] \
	__modify [ax cx]

/* Writing to configuration space */

static pcierr pci_write_config_byte(pcisel sel, unsigned char reg, unsigned char data);
#pragma aux pci_write_config_byte = \
	"mov ax, 0xB10B" \
	"int 0x1A" \
	__parm [bx] [di] [cl] \
	__value [ah] \
	__modify [ax]

static pcierr pci_write_config_word(pcisel sel, unsigned char reg, unsigned short data);
#pragma aux pci_write_config_word = \
	"mov ax, 0xB10C" \
	"int 0x1A" \
	__parm [bx] [di] [cx] \
	__value [ah] \
	__modify [ax cx]

static pcierr pci_write_config_dword(pcisel sel, unsigned char reg, unsigned long data);
#pragma aux pci_write_config_dword = \
	"mov ax, 0xB10D" \
	"shl esi, 16" \
	"and ecx, 0xFFFF" \
	"or ecx, esi" \
	"int 0x1A" \
	__parm [bx] [di] [si cx] \
	__value [ah] \
	__modify [ax cx]

#endif /* INT1APCI_H */
