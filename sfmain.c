/*
 * VBSF - Shared folders entry point
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
#include <time.h>
#include <dos.h>

#include "version.h"
#include "dlog.h"
#include "vboxshfl.h"
#include "dostsr.h"
#include "sftsr.h"

static char get_drive_letter(const char *path) {
	if (!path || path[0] == '\0') return '\0';
	if (path[1] == '\0' || (path[1] == ':' && path[2] == '\0')) {
		return path[0];
	} else {
		return '\0';
	}
}

static char find_free_drive_letter() {
	DOSLOL __far *lol = dos_get_list_of_lists();
	DOSCDS __far *cds = lol->cds;
	char letter;

	for (letter = 'A'; letter <= 'Z'; letter++) {
		int drive = drive_letter_to_index(letter);

		if (cds[drive].flags == 0) {
			// Looks free to me...
			return letter;
		}
	}

	return 0;
}

static int list_folders(LPTSRDATA data)
{
	int32_t err;
	SHFLMAPPING maps[SHFL_MAX_MAPPINGS];
	SHFLSTRING_WITH_BUF(str, SHFL_MAX_LEN);
	unsigned num_maps = sizeof(maps) / sizeof(SHFLMAPPING);
	unsigned i;

	printf("Mounted drives:\n");

	for (i = 0; i < NUM_DRIVES; i++) {
		if (data->drives[i].root == SHFL_ROOT_NIL) {
			// Not mounted
			continue;
		}

		err = vbox_shfl_query_map_name(&data->vb, data->hgcm_client_id, data->drives[i].root, &str.shflstr);
		if (err) {
			printf("Error on Query Map Name, err=%ld\n", err);
			continue;
		}

		printf(" %s on %c:\n", str.buf, drive_index_to_letter(i));
	}

	err = vbox_shfl_query_mappings(&data->vb, data->hgcm_client_id, 0, &num_maps, maps);
	if (err) {
		printf("Error on Query Mappings, err=%ld\n", err);
		return err;
	}

	printf("Available shared folders:\n");

	for (i = 0 ; i < num_maps; i++) {
		err = vbox_shfl_query_map_name(&data->vb, data->hgcm_client_id, maps[i].root, &str.shflstr);
		if (err) {
			printf("Error on Query Map Name, err=%ld\n", err);
			continue;
		}

		printf(" %s\n", str.buf);
	}

	return 0;
}

/** Closes all currently open files.
 *  @param drive drive number, or -1 to close files from all drives. */
static void close_openfiles(LPTSRDATA data, int drive)
{
	SHFLROOT search_root = SHFL_ROOT_NIL;
	int32_t err;
	unsigned i;

	if (drive >= 0) {
		search_root = data->drives[drive].root;
	}

	for (i = 0; i < NUM_FILES; i++) {
		if (data->files[i].root == SHFL_ROOT_NIL) {
			// Already closed
			continue;
		}
		if (search_root != SHFL_ROOT_NIL &&
		        data->files[i].root != search_root) {
			// File from a different shared folder
			continue;
		}

		err = vbox_shfl_close(&data->vb, data->hgcm_client_id, data->files[i].root, data->files[i].handle);
		if (err) {
			printf("Error on Close File, err=%ld\n", err);
			// Ignore it
		}

		data->files[i].root = SHFL_ROOT_NIL;
		data->files[i].handle = SHFL_HANDLE_NIL;
	}
}

static int mount_shfl(LPTSRDATA data, int drive, const char *folder)
{
	int32_t err;
	SHFLSTRING_WITH_BUF(str, SHFL_MAX_LEN);
	SHFLROOT root = SHFL_ROOT_NIL;

	shflstring_strcpy(&str.shflstr, folder);

	err = vbox_shfl_map_folder(&data->vb, data->hgcm_client_id, &str.shflstr, &root);
	if (err) {
		fprintf(stderr, "Cannot mount shared folder '%s', err=%d\n", folder, err);
		return -1;
	}

	data->drives[drive].root = root;

	return 0;
}

static int unmount_shfl(LPTSRDATA data, int drive)
{
	int32_t err;

	close_openfiles(data, drive);

	err = vbox_shfl_unmap_folder(&data->vb, data->hgcm_client_id,
	                             data->drives[drive].root);
	if (err) {
		fprintf(stderr, "Cannot unmount shared folder, err=%d\n", err);
		return -1;
	}

	data->drives[drive].root = SHFL_ROOT_NIL;

	return 0;
}

static int mount(LPTSRDATA data, const char *folder, char drive_letter)
{
	int drive = drive_letter_to_index(drive_letter);
	DOSLOL __far *lol = dos_get_list_of_lists();
	DOSCDS __far *cds;

	if (drive < 0) {
		fprintf(stderr, "Invalid drive %c:\n", drive_letter);
		return EXIT_FAILURE;
	}

	if (drive >= lol->last_drive || drive >= MAX_NUM_DRIVE) {
		fprintf(stderr, "Drive %c: is after LASTDRIVE\n", drive_letter);
		return EXIT_FAILURE;
	}

	if (data->drives[drive].root != SHFL_ROOT_NIL) {
		fprintf(stderr, "Drive %c already mounted\n", drive_letter);
		return EXIT_FAILURE;
	}

	cds = &lol->cds[drive];

	if (cds->flags) {
		fprintf(stderr, "Drive %c: already exists\n", drive_letter);
		return EXIT_FAILURE;
	}

	if (mount_shfl(data, drive, folder) != 0) {
		fprintf(stderr, "Cannot mount drive %c:\n", drive_letter);
		return EXIT_FAILURE;
	}

	// Ok, set the network flag.
	// By setting the physical flag, we also let DOS know the drive is present
	cds->flags = DOS_CDS_FLAG_NETWORK | DOS_CDS_FLAG_PHYSICAL;

	printf("Shared folder '%s' mounted as drive %c:\n", folder, drive_letter);

	return EXIT_SUCCESS;
}

static int unmount(LPTSRDATA data, char drive_letter)
{
	int drive = drive_letter_to_index(drive_letter);
	DOSLOL __far *lol = dos_get_list_of_lists();
	DOSCDS __far *cds;

	if (drive < 0) {
		fprintf(stderr, "Invalid drive %c:\n", drive_letter);
		return EXIT_FAILURE;
	}

	if (drive >= lol->last_drive || drive >= MAX_NUM_DRIVE) {
		fprintf(stderr, "Drive %c: is after LASTDRIVE\n", drive_letter);
		return EXIT_FAILURE;
	}

	cds = &lol->cds[drive];

	if (data->drives[drive].root == SHFL_ROOT_NIL) {
		fprintf(stderr, "Drive %c not mounted\n", drive_letter);
		return EXIT_FAILURE;
	}

	if (unmount_shfl(data, drive) != 0) {
		return EXIT_FAILURE;
	}

	// Hide the drive from DOS
	cds->flags = 0;

	// TODO Clear current directory ?

	printf("Drive %c: unmounted\n", drive_letter);

	return EXIT_SUCCESS;
}

static int automount(LPTSRDATA data)
{
	int32_t err;
	SHFLMAPPING maps[SHFL_MAX_MAPPINGS];
	SHFLSTRING_WITH_BUF(name, SHFL_MAX_LEN);
	SHFLSTRING_WITH_BUF(mountPoint, SHFL_MAX_LEN);
	unsigned num_maps = sizeof(maps) / sizeof(SHFLMAPPING);
	unsigned flags, version;
	unsigned i;

	err = vbox_shfl_query_mappings(&data->vb, data->hgcm_client_id,
	                               SHFL_MF_AUTOMOUNT, &num_maps, maps);
	if (err) {
		printf("Error on Query Mappings, err=%ld\n", err);
		return err;
	}

	for (i = 0 ; i < num_maps; i++) {
		unsigned flags = SHFL_MIQF_DRIVE_LETTER, version = 0;
		char drive_letter;

		err = vbox_shfl_query_map_info(&data->vb, data->hgcm_client_id, maps[i].root,
		                               &name.shflstr, &mountPoint.shflstr, &flags, &version);
		if (err) {
			printf("Error on Query Map Info, err=%ld\n", err);
			continue;
		}

		if (!(flags & SHFL_MIF_AUTO_MOUNT)) {
			// No automount...
			continue;
		}

		// Can we try to use the mountpoint as a drive letter?
		if (mountPoint.shflstr.u16Length >= 1) {
			drive_letter = get_drive_letter(mountPoint.shflstr.ach);
		}

		// Otherwise try to find a free one...
		if (!drive_letter) {
			drive_letter = find_free_drive_letter();
		}

		mount(data, name.buf, drive_letter);
	}

	return 0;
}

static int unmount_all(LPTSRDATA data)
{
	int err = 0;
	unsigned i;

	// Close all files
	close_openfiles(data, -1);

	// Unmount all drives
	for (i = 0 ; i < NUM_DRIVES; i++) {
		if (data->drives[i].root != SHFL_ROOT_NIL) {
			if (unmount(data, drive_index_to_letter(i)) != 0) {
				err = -1;
			}
		}
	}

	return err;
}

static int rescan(LPTSRDATA data)
{
	int err;

	if ((err = unmount_all(data))) {
		return err;
	}

	if ((err = automount(data))) {
		return err;
	}

	return 0;
}

static int configure_driver(LPTSRDATA data)
{
	unsigned i;
	int32_t err;

	// Clear up data structures
	for (i = 0; i < NUM_DRIVES; ++i) {
		data->drives[i].root = SHFL_ROOT_NIL;
	}
	for (i = 0; i < NUM_FILES; ++i) {
		data->files[i].root = SHFL_ROOT_NIL;
		data->files[i].handle = SHFL_HANDLE_NIL;
	}

	// Initialize TSR data
	data->dossda = dos_get_swappable_dos_area();

	// Get the current timezone offset
	if (getenv("TZ")) {
		tzset();
		printf("Using timezone from TZ variable (%s)\n", tzname[0]);
		data->tz_offset = timezone / 2;
	} else {
		data->tz_offset = 0;
	}

	// Now try to initialize VirtualBox communication
	err = vbox_init_device(&data->vb);
	if (err) {
		fprintf(stderr, "Cannot find VirtualBox PCI device, err=%ld\n", err);
		return -1;
	}

	err = vbox_init_buffer(&data->vb, VBOX_BUFFER_SIZE);
	if (err) {
		fprintf(stderr, "Cannot lock buffer used for VirtualBox communication, err=%ld\n", err);
		return -1;
	}

	err = vbox_report_guest_info(&data->vb, VBOXOSTYPE_DOS);
	if (err) {
		fprintf(stderr, "VirtualBox communication is not working, err=%ld\n", err);
		return -1;
	}

	err = vbox_hgcm_connect_existing(&data->vb, "VBoxSharedFolders", &data->hgcm_client_id);
	if (err) {
		printf("Cannot connect to shared folder service, err=%ld\n", err);
		return -1;
	}

	err = vbox_shfl_set_utf8(&data->vb, data->hgcm_client_id);
	if (err) {
		printf("Cannot configure UTF-8 on shared folder service, err=%ld\n", err);
		return -1;
	}

	printf("Connected to VirtualBox shared folder service\n");

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

	data->prev_int2f_handler = _dos_getvect(0x2f);
	_dos_setvect(0x2f, data:>int2f_isr);

	printf("Driver installed\n");

	// If we reallocated ourselves to UMB,
	// it's time to free our initial conventional memory allocation
	if (high) {
		finish_reallocation(_psp, FP_SEG(data));
	}

	_dos_keep(EXIT_SUCCESS, get_paragraphs(resident_size));

	// Shouldn't reach this part
	return EXIT_FAILURE;
}

static bool check_if_driver_uninstallable(LPTSRDATA data)
{
	void (__interrupt __far *cur_int2f_handler)() = _dos_getvect(0x2f);

	// Compare the segment of the installed handler to see if its ours
	// or someone else's
	if (FP_SEG(cur_int2f_handler) != FP_SEG(data)) {
		fprintf(stderr, "INT2F has been hooked by someone else, cannot safely remove\n");
		return false;
	}

	return true;
}

static int unconfigure_driver(LPTSRDATA data)
{
	unmount_all(data);

	vbox_hgcm_disconnect(&data->vb, data->hgcm_client_id);
	data->hgcm_client_id = 0;

	vbox_release_buffer(&data->vb);

	return 0;
}

static int uninstall_driver(LPTSRDATA data)
{
	_dos_setvect(0x2f, data->prev_int2f_handler);

	// Find and deallocate the PSP (including the entire program),
	// it is always 256 bytes (16 paragraphs) before the TSR segment
	dos_free(FP_SEG(data) - (DOS_PSP_SIZE/16));

	printf("Driver uninstalled\n");

	return 0;
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
	    "    VBSF <ACTION> <ARGS..>\n\n"
	    "Supported actions:\n"
	    "    install            install the driver (default)\n"
	    "        low                install in conventional memory (otherwise UMB)\n"
	    "    uninstall          uninstall the driver from memory\n"
	    "    list               list available shared folders\n"
	    "    mount <FOLD> <X:>  mount a shared folder into drive X:\n"
	    "    umount <X:>        unmount shared folder from drive X:\n"
	    "    rescan             unmount everything and recreate automounts\n"
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

		printf("\nVBSharedFolders %x.%x\n", VERSION_MAJOR, VERSION_MINOR);

		if (data) {
			printf("VBSF already installed\n");
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
		if (err) {
			if (high) cancel_reallocation(FP_SEG(data));
			return EXIT_FAILURE;
		}
		err = automount(data);
		if (err) {
			// Automount errors are not fatal
		}
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
	} else if (stricmp(argv[argi], "list") == 0) {
		if (!data) return driver_not_found();
		return list_folders(data);
	} else if (stricmp(argv[argi], "mount") == 0) {
		const char *folder;
		char drive;
		if (!data) return driver_not_found();

		argi++;
		if (argi >= argc) return arg_required("mount");
		folder = argv[argi];
		argi++;
		if (argi >= argc) return arg_required("mount");
		drive = get_drive_letter(argv[argi]);
		if (!drive) return invalid_arg(argv[argi]);

		return mount(data, folder, drive);
	} else if (stricmp(argv[argi], "umount") == 0 || stricmp(argv[argi], "unmount") == 0) {
		char drive;
		if (!data) return driver_not_found();

		argi++;
		if (argi >= argc) return arg_required("umount");
		drive = get_drive_letter(argv[argi]);
		if (!drive) return invalid_arg(argv[argi]);

		return unmount(data, drive);
	} else if (stricmp(argv[argi], "rescan") == 0) {
		if (!data) return driver_not_found();
		return rescan(data);
	} else {
		return invalid_arg(argv[argi]);
	}
}
