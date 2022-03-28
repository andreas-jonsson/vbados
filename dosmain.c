/*
 * VBMouse - DOS mouse driver exec entry point
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <i86.h>

#include "dlog.h"
#include "ps2.h"
#include "vbox.h"
#include "dostsr.h"

static unsigned get_resident_size(void)
{
	return FP_OFF(&resident_end);
}

static int set_integration(LPTSRDATA data, bool enable)
{
	if (enable) {
		int err;

		data->vbavail = false; // Reinitialize it even if already enabled

		if ((err = vbox_init(&data->vb)) == 0) {
			printf("Found VirtualBox device at IO 0x%x\n", data->vb.iobase);
			printf("Found physical address for VBox communication 0x%lx\n", data->vb.buf_physaddr);

			if ((err = vbox_report_guest_info(&data->vb, VBOXOSTYPE_DOS)) == 0) {
				printf("VirtualBox integration enabled\n");
				data->vbavail = true;
			} else {
				fprintf(stderr, "VirtualBox communication is not working, err=%d\n", err);
				return err;
			}
		} else {
			fprintf(stderr, "Cannot find VirtualBox PCI device, err=%d\n", err);
			return err;
		}
	} else {
		if (data->vbavail) {
			vbox_set_mouse(&data->vb, false, false);
			printf("Disabled VirtualBox integration\n");
			data->vbavail = false;
		} else {
			printf("VirtualBox already disabled or not available\n");
		}
	}

	return 0;
}

static int set_host_cursor(LPTSRDATA data, bool enable)
{
#if USE_VIRTUALBOX
	if (data->vbavail) {
		printf("Setting host cursor to %s\n", enable ? "enabled" : "disabled");
		data->vbwantcursor = enable;
	} else {
		printf("VirtualBox integration is not available\n");
	}
#endif
	return EXIT_SUCCESS;
}

static int configure_driver(LPTSRDATA data)
{
	int err;

	// First check for PS/2 mouse availability
	if ((err = ps2m_init(PS2_MOUSE_PLAIN_PACKET_SIZE))) {
		fprintf(stderr, "Cannot init PS/2 mouse BIOS, err=%d\n", err);
		// Can't do anything without PS/2
		return err;
	}

#if USE_VIRTUALBOX
	// Assume initially that we want integration and host cursor
	 set_integration(data, true);
	 data->vbwantcursor = data->vbavail;
#endif

	return 0;
}

static void deallocate_environment(uint16_t psp)
{
	// TODO : Too lazy to make PSP struct;
	// 0x2C is offsetof the environment block field on the PSP
	uint16_t __far *envblockP = (uint16_t __far *) MK_FP(psp, 0x2C);
	_dos_freemem(*envblockP);
	*envblockP = 0;
}

static int install_driver(LPTSRDATA data)
{
	unsigned int resident_size = get_resident_size();

	// Not that this will do anything other than fragment memory, but why not...
	deallocate_environment(_psp);

	data->prev_int33_handler = _dos_getvect(0x33);

	_dos_setvect(0x33, int33_isr);

	_dos_keep(EXIT_SUCCESS, (256 + resident_size + 15) / 16);
	return 0;
}

static bool check_if_driver_uninstallable(LPTSRDATA data)
{
	void (__interrupt __far *cur_int33_handler)() = _dos_getvect(0x33);
	void (__interrupt __far *our_int33_handler)() = MK_FP(FP_SEG(data), FP_OFF(int33_isr));

	if (cur_int33_handler != our_int33_handler) {
		fprintf(stderr, "INT33 has been hooked by some other driver, removing anyway\n");
		return true;
	}

	return true;
}

static int unconfigure_driver(LPTSRDATA data)
{
#if USE_VIRTUALBOX
	if (data->vbavail) {
		set_integration(data, false);
	}
#endif

	ps2m_enable(false);
	ps2m_set_callback(0);

	return 0;
}

static int uninstall_driver(LPTSRDATA data)
{
	_dos_setvect(0x33, data->prev_int33_handler);

	// Find and deallocate the PSP (including the entire program),
	// it is always 256 bytes (16 paragraphs) before the TSR segment
	_dos_freemem(FP_SEG(data) - 16);

	printf("Driver uninstalled\n");

	return 0;
}

static unsigned int33_call(unsigned ax);
#pragma aux int33_call parm [ax] value [ax] = \
	"int 0x33";

static int driver_reset(void)
{
	printf("Reset mouse driver\n");
	return int33_call(0x0) == 0xFFFF;
}

static int driver_not_found(void)
{
	fprintf(stderr, "Driver data not found (driver not installed?)\n");
	return EXIT_FAILURE;
}

static void print_help(void)
{
	printf("\n"
	    "Usage: \n"
	    "\tVBMOUSE <ACTION>\n\n"
	    "Supported actions:\n"
	    "\tinstall           install the driver (default)\n"
	    "\tuninstall         uninstall the driver from memory\n"
	    "\tinteg <ON|OFF>    enable/disable virtualbox integration\n"
	    "\thostcur <ON|OFF>  enable/disable mouse cursor rendering in host\n"
	    "\treset             reset mouse driver settings\n"
	);
}

static bool is_true(const char *s)
{
	return stricmp(s, "yes") == 0
	       || stricmp(s, "y") == 0
	       || stricmp(s, "on") == 0
	       || stricmp(s, "true") == 0
	       || stricmp(s, "enabled") == 0
	       || stricmp(s, "enable") == 0
	       || stricmp(s, "1") == 0;
}

static bool is_false(const char *s)
{
	return stricmp(s, "no") == 0
	       || stricmp(s, "n") == 0
	       || stricmp(s, "off") == 0
	       || stricmp(s, "false") == 0
	       || stricmp(s, "disabled") == 0
	       || stricmp(s, "disable") == 0
	       || stricmp(s, "0") == 0;
}

int main(int argc, const char *argv[])
{
	LPTSRDATA data = get_tsr_data(true);
	int err, argi = 1;

	printf("VBMouse %d.%d\n", DRIVER_VERSION_MAJOR, DRIVER_VERSION_MINOR);

	if (argi >= argc || stricmp(argv[argi], "install") == 0) {
		if (data) {
			printf("VBMouse already installed\n");
			return EXIT_SUCCESS;
		}

		data = get_tsr_data(false);
		err = configure_driver(data);
		if (err) return EXIT_FAILURE;
		return install_driver(data);
	} else if (stricmp(argv[argi], "uninstall") == 0) {
		if (!data) return driver_not_found();
		if (!check_if_driver_uninstallable(data)) {
			return EXIT_FAILURE;
		}
		err = unconfigure_driver(data);
		if (err) {
			return EXIT_FAILURE;
		}
		return uninstall_driver(data);
	} else if (stricmp(argv[argi], "integ") == 0) {
		bool enable = true;

		if (!data) return driver_not_found();

		argi++;
		if (argi < argc) {
			if (is_false(argv[argi])) enable = false;
		}

		return set_integration(data, enable);
	} else if (stricmp(argv[argi], "hostcur") == 0) {
		bool enable = true;

		if (!data) return driver_not_found();

		argi++;
		if (argi < argc) {
			if (is_false(argv[argi])) enable = false;
		}

		return set_host_cursor(data, enable);
	} else if (stricmp(argv[argi], "reset") == 0) {
		return driver_reset();
	}

	print_help();
	return EXIT_FAILURE;
}
