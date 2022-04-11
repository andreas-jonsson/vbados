#ifndef SFTSR_H
#define SFTSR_H

#include <stdbool.h>
#include <stdint.h>

#include "vbox.h"
#include "int21dos.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

#define LASTDRIVE     'Z'
#define MAX_NUM_DRIVE (LASTDRIVE - 'A')
#define NUM_DRIVES    (MAX_NUM_DRIVE + 1)

typedef struct tsrdata {
	// TSR installation data
	/** Previous int2f ISR, storing it for uninstall. */
	void (__interrupt __far *prev_int2f_handler)();

	/** Array of all possible DOS drives. */
	struct {
		/** VirtualBox "root" for this drive, or NIL if unmounted. */
		uint32_t root;
	} drives[NUM_DRIVES];

	/** Stored pointer for the DOS SDA. */
	DOSSDA __far *dossda;

	/** Handle for the directory we are currently searching in. */
	uint64_t search_dir_handle;

	struct vboxcomm vb;
	uint32_t hgcm_client_id;
} TSRDATA;

typedef TSRDATA * PTSRDATA;
typedef TSRDATA __far * LPTSRDATA;

extern void __declspec(naked) __far int2f_isr(void);

extern LPTSRDATA __far get_tsr_data(bool installed);

/** This symbol is always at the end of the TSR segment */
extern int resident_end;

/** This is not just data, but the entire segment. */
static inline unsigned get_resident_size(void)
{
	return FP_OFF(&resident_end);
}

#endif // SFTSR_H
