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
#include "int33.h"
#include "ps2.h"
#include "vbox.h"
#include "vmware.h"
#include "dostsr.h"

static void detect_wheel(LPTSRDATA data)
{
	// Do a quick check for a mouse wheel here.
	// The TSR will do its own check when it is reset anyway
	if (data->haswheel = ps2m_detect_wheel()) {
		printf("Wheel mouse found and enabled\n");
	}
}

static int set_wheel(LPTSRDATA data, bool enable)
{
	printf("Setting wheel support to %s\n", enable ? "enabled" : "disabled");
	data->usewheel = enable;

	if (data->usewheel) {
		detect_wheel(data);
	} else {
		data->haswheel = false;
	}

	return 0;
}

#if USE_VIRTUALBOX
static int set_virtualbox_integration(LPTSRDATA data, bool enable)
{
	if (enable) {
		int err;

		data->vbavail = false; // Reinitialize it even if already enabled

		err = vbox_init_device(&data->vb);
		if (err) {
			fprintf(stderr, "Cannot find VirtualBox PCI device, err=%d\n", err);
			return err;
		}

		printf("Found VirtualBox device at IO 0x%x\n", data->vb.iobase);

		err = vbox_init_buffer(&data->vb);
		if (err) {
			fprintf(stderr, "Cannot lock buffer used for VirtualBox communication, err=%d\n", err);
			return err;
		}

		err = vbox_report_guest_info(&data->vb, VBOXOSTYPE_DOS);
		if (err) {
			fprintf(stderr, "VirtualBox communication is not working, err=%d\n", err);
			return err;
		}

		printf("VirtualBox integration enabled\n");
		data->vbavail = true;
		data->vbhaveabs = true;
	} else {
		if (data->vbavail) {
			vbox_set_mouse(&data->vb, false, false);

			vbox_release_buffer(&data->vb);

			printf("Disabled VirtualBox integration\n");
			data->vbavail = false;
			data->vbhaveabs = false;
		} else {
			printf("VirtualBox integration already disabled or not available\n");
		}
	}

	return 0;
}

static int set_virtualbox_host_cursor(LPTSRDATA data, bool enable)
{
	printf("Setting host cursor to %s\n", enable ? "enabled" : "disabled");
	data->vbwantcursor = enable;

	return 0;
}
#endif

#if USE_VMWARE
static int set_vmware_integration(LPTSRDATA data, bool enable)
{
	if (enable) {
		int32_t version;
		uint32_t status;
		uint16_t data_avail;

		data->vmwavail = false;

		version = vmware_get_version();
		if (version < 0) {
			fprintf(stderr, "Could not detect VMware, err=%ld\n", version);
			return -1;
		}

		printf("Found VMware protocol version %ld\n", version);

		vmware_abspointer_cmd(VMWARE_ABSPOINTER_CMD_ENABLE);

		status = vmware_abspointer_status();
		if ((status & VMWARE_ABSPOINTER_STATUS_MASK_ERROR)
		        == VMWARE_ABSPOINTER_STATUS_MASK_ERROR) {
			fprintf(stderr, "VMware absolute pointer error, err=0x%lx\n",
			        status & VMWARE_ABSPOINTER_STATUS_MASK_ERROR);
			return -1;
		}

		vmware_abspointer_data_clear();

		// TSR part will enable the absolute mouse when reset

		printf("VMware integration enabled\n");
		data->vmwavail = true;
	} else {
		if (data->vmwavail) {
			vmware_abspointer_cmd(VMWARE_ABSPOINTER_CMD_REQUEST_RELATIVE);
			vmware_abspointer_cmd(VMWARE_ABSPOINTER_CMD_DISABLE);

			data->vmwavail = false;
			printf("Disabled VMware integration\n");
		} else {
			printf("VMware integration already disabled or not available\n");
		}
	}

	return 0;
}
#endif

static int set_integration(LPTSRDATA data, bool enable)
{
	if (enable) {
		int err = -1;

#if USE_VIRTUALBOX
		// First check if we can enable the VirtualBox integration,
		// since it's a PCI device it's easier to check if it's not present
		err = set_virtualbox_integration(data, true);
		if (!err) return 0;
#endif

#if USE_VMWARE
		// Afterwards try VMWare integration
		err = set_vmware_integration(data, true);
		if (!err) return 0;
#endif

		printf("Neither VirtualBox nor VMware integration available\n");
		return err;
	} else {
#if USE_VIRTUALBOX
		if (data->vbavail) {
			set_virtualbox_integration(data, false);
		}
#endif
#if USE_VMWARE
		if (data->vmwavail) {
			set_vmware_integration(data, false);
		}
#endif
		return 0;
	}
}

static int set_host_cursor(LPTSRDATA data, bool enable)
{
#if USE_VIRTUALBOX
	if (data->vbavail) {
		return set_virtualbox_host_cursor(data, enable);
	}
#endif
	printf("VirtualBox integration not available\n");
	return -1;
}

static int configure_driver(LPTSRDATA data)
{
	int err;

	// Configure the debug logging port
	dlog_init();

	// Check for PS/2 mouse BIOS availability
	if ((err = ps2m_init(PS2M_PACKET_SIZE_PLAIN))) {
		fprintf(stderr, "Cannot init PS/2 mouse BIOS, err=%d\n", err);
		// Can't do anything without PS/2
		return err;
	}

#if USE_WHEEL
	// Let's utilize the wheel by default
	data->usewheel = true;
	detect_wheel(data);
#else
	data->usewheel = false;
#endif

#if USE_INTEGRATION
	// Enable integration by default
	set_integration(data, true);
#endif

#if USE_VIRTUALBOX
	// Assume initially that we want host cursor
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

#if USE_WIN386
	data->prev_int2f_handler = _dos_getvect(0x2f);
	_dos_setvect(0x2f, int2f_isr);
#endif

	printf("Driver installed\n");

	_dos_keep(EXIT_SUCCESS, (256 + resident_size + 15) / 16);
	return 0;
}

static bool check_if_driver_uninstallable(LPTSRDATA data)
{
	void (__interrupt __far *cur_int33_handler)() = _dos_getvect(0x33);
	void (__interrupt __far *our_int33_handler)() = MK_FP(FP_SEG(data), FP_OFF(int33_isr));

	if (cur_int33_handler != our_int33_handler) {
		fprintf(stderr, "INT33 has been hooked by someone else, removing anyway\n");
		return true;
	}

#if USE_WIN386
	{
		void (__interrupt __far *cur_int2f_handler)() = _dos_getvect(0x2f);
		void (__interrupt __far *our_int2f_handler)() = MK_FP(FP_SEG(data), FP_OFF(int2f_isr));

		if (cur_int2f_handler != our_int2f_handler) {
			fprintf(stderr, "INT2F has been hooked by someone else, removing anyway\n");
			return true;
		}
	}
#endif

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

#if USE_WIN386
	_dos_setvect(0x2f, data->prev_int2f_handler);
#endif

	// Find and deallocate the PSP (including the entire program),
	// it is always 256 bytes (16 paragraphs) before the TSR segment
	_dos_freemem(FP_SEG(data) - 16);

	printf("Driver uninstalled\n");

	return 0;
}

static int driver_reset(void)
{
	printf("Reset mouse driver\n");
	return int33_reset() == 0xFFFF;
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
#if USE_WHEEL
	    "\twheel <ON|OFF>    enable/disable wheel API support\n"
#endif
#if USE_INTEGRATION
	    "\tinteg <ON|OFF>    enable/disable virtualbox integration\n"
	    "\thostcur <ON|OFF>  enable/disable mouse cursor rendering in host\n"
#endif
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

	printf("\nVBMouse %x.%x (like MSMOUSE %x.%x)\n", VERSION_MAJOR, VERSION_MINOR, REPORTED_VERSION_MAJOR, REPORTED_VERSION_MINOR);

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
#if USE_WHEEL
	} else if (stricmp(argv[argi], "wheel") == 0) {
		bool enable = true;

		if (!data) return driver_not_found();

		argi++;
		if (argi < argc) {
			if (is_false(argv[argi])) enable = false;
		}

		return set_wheel(data, enable);
#endif
#if USE_INTEGRATION
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

		// Reset before changing this to ensure the cursor is not drawn
		int33_reset();

		return set_host_cursor(data, enable);
#endif
	} else if (stricmp(argv[argi], "reset") == 0) {
		return driver_reset();
	}

	print_help();
	return EXIT_FAILURE;
}
