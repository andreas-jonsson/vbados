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
#include "int21dos.h"
#include "ps2.h"
#include "vbox.h"
#include "vmware.h"
#include "dostsr.h"
#include "mousetsr.h"

#if USE_WHEEL
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
		if (!data->haswheel) {
			fprintf(stderr, "Could not find PS/2 wheel mouse\n");
		}
	} else {
		data->haswheel = false;
	}

	return 0;
}

static int set_wheel_key(LPTSRDATA data, const char *keyname)
{
	if (!data->usewheel || !data->haswheel) {
		fprintf(stderr, "Wheel not detected or support not enabled\n");
		return EXIT_FAILURE;
	}
	if (keyname) {
		if (stricmp(keyname, "updn") == 0) {
			data->wheel_up_key = 0x4800;
			data->wheel_down_key = 0x5000;
			printf("Generate Up Arrow / Down Arrow key presses on wheel movement\n");
		} else if (stricmp(keyname, "pageupdn") == 0) {
			data->wheel_up_key = 0x4900;
			data->wheel_down_key = 0x5100;
			printf("Generate PageUp / PageDown key presses on wheel movement\n");
		} else {
			fprintf(stderr, "Unknown key '%s'\n", keyname);
			return EXIT_FAILURE;
		}
	} else {
		printf("Disabling wheel keystroke generation\n");
		data->wheel_up_key = 0;
		data->wheel_down_key = 0;
	}
	return EXIT_SUCCESS;
}
#endif /* USE_WHEEL */

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

		err = vbox_init_buffer(&data->vb, VBOX_BUFFER_SIZE);
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
	data->wheel_up_key = 0;
	data->wheel_down_key = 0;
	detect_wheel(data);
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

static int move_driver_to_umb(LPTSRDATA __far * data)
{
	segment_t cur_seg = FP_SEG(*data);
	segment_t umb_seg = reallocate_to_umb(cur_seg,  get_resident_size() + DOS_PSP_SIZE);

	if (umb_seg) {
		// Update the data pointer with the new segment
		*data = MK_FP(umb_seg, FP_OFF(*data));
		return 0;
	} else {
		return -1;
	}
}

static __declspec(aborts) int install_driver(LPTSRDATA data, bool high)
{
	const unsigned int resident_size = DOS_PSP_SIZE + get_resident_size();

	// No more interruptions from now on and until we TSR.
	// Inserting ourselves in the interrupt chain should be atomic.
	_disable();

	data->prev_int33_handler = _dos_getvect(0x33);
	_dos_setvect(0x33, data:>int33_isr);

#if USE_WIN386
	data->prev_int2f_handler = _dos_getvect(0x2f);
	_dos_setvect(0x2f, data:>int2f_isr);
#endif

	printf("Driver installed\n");

	// If we reallocated ourselves to UMB,
	// it's time to free our initial conventional memory allocation
	if (high) {
		// We are about to free() our own code segment.
		// Nothing should try to allocate memory between this and the TSR call
		// below, since it could overwrite our code...
		dos_free(_psp);
	}

	_dos_keep(EXIT_SUCCESS, get_paragraphs(resident_size));

	// Shouldn't reach this part
	return EXIT_FAILURE;
}

static bool check_if_driver_uninstallable(LPTSRDATA data)
{
	void (__interrupt __far *cur_int33_handler)() = _dos_getvect(0x33);

	// Compare the segment of the installed handler to see if its ours
	// or someone else's
	if (FP_SEG(cur_int33_handler) != FP_SEG(data)) {
		fprintf(stderr, "INT33 has been hooked by someone else, cannot safely remove\n");
		return false;
	}

#if USE_WIN386
	{
		void (__interrupt __far *cur_int2f_handler)() = _dos_getvect(0x2f);

		if (FP_SEG(cur_int2f_handler) != FP_SEG(data)) {
			fprintf(stderr, "INT2F has been hooked by someone else, cannot safely remove\n");
			return false;
		}
	}
#endif

	return true;
}

static int unconfigure_driver(LPTSRDATA data)
{
#if USE_INTEGRATION
	set_integration(data, false);
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
	dos_free(FP_SEG(data) - (DOS_PSP_SIZE/16));

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
	    "    VBMOUSE <ACTION> <ARGS..>\n\n"
	    "Supported actions:\n"
	    "    install            install the driver (default)\n"
	    "        low                install in conventional memory (otherwise UMB)\n"
	    "    uninstall          uninstall the driver from memory\n"
#if USE_WHEEL
	    "    wheel <ON|OFF>     enable/disable wheel API support\n"
	    "    wheelkey <KEY|OFF> emulate a specific keystroke on wheel scroll\n"
	    "                          supported keys: updn, pageupdn\n"
#endif
#if USE_INTEGRATION
	    "    integ <ON|OFF>     enable/disable virtualbox integration\n"
	    "    hostcur <ON|OFF>   enable/disable mouse cursor rendering in host\n"
#endif
	    "    reset              reset mouse driver settings\n"
	);
}

static int invalid_arg(const char *s)
{
	fprintf(stderr, "Invalid argument '%s'\n", s);
	print_help();
	return EXIT_FAILURE;
}

static int arg_required(const char *s)
{
	fprintf(stderr, "Argument required for '%s'\n", s);
	print_help();
	return EXIT_FAILURE;
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
		bool high = true;

		argi++;
		for (; argi < argc; argi++) {
			if (stricmp(argv[argi], "low") == 0) {
				high = false;
			} else if (stricmp(argv[argi], "high") == 0) {
				high = true;
			} else {
				return invalid_arg(argv[argi]);
			}
		}

		if (data) {
			printf("VBMouse already installed\n");
			print_help();
			return EXIT_SUCCESS;
		}

		data = get_tsr_data(false);
		if (high) {
			err = move_driver_to_umb(&data);
			if (err) high = false; // Not fatal
		} else {
			deallocate_environment(_psp);
		}
		err = configure_driver(data);
		if (err) return EXIT_FAILURE;
		return install_driver(data, high);
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
	} else if (stricmp(argv[argi], "wheelkey") == 0) {
		bool enable = true;
		const char *key = 0;

		if (!data) return driver_not_found();

		argi++;
		if (argi < argc) {
			if (is_false(argv[argi])) enable = false;
			else                      key = argv[argi];
		}

		if (enable) {
			if (!key) return arg_required("wheelkey");
			return set_wheel_key(data, key);
		} else {
			return set_wheel_key(data, 0);
		}
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
	} else {
		return invalid_arg(argv[argi]);
	}
}
