#ifndef INT10_H
#define INT10_H

#include <stdint.h>

#define BIOS_DATA_AREA_SEGMENT 0x40

// TODO Assuming screenwidth, videopage on ss rather than using far
static uint8_t int10_get_video_mode(uint8_t *screenwidth, uint8_t *videopage);
#pragma aux int10_get_video_mode = \
	"mov al, 0" \
	"mov ah, 0x0F" \
	"int 0x10" \
	"mov ss:[si], ah" \
	"mov ss:[di], bh" \
	__parm [si] [di] \
	__value [al] \
	__modify [ax bh]

static inline uint32_t bda_get_dword(unsigned int offset) {
	uint32_t __far *p = MK_FP(BIOS_DATA_AREA_SEGMENT, offset);
	return *p;
}

static inline uint16_t bda_get_word(unsigned int offset) {
	uint16_t __far *p = MK_FP(BIOS_DATA_AREA_SEGMENT, offset);
	return *p;
}

static inline uint8_t bda_get_byte(unsigned int offset) {
	uint8_t __far *p = MK_FP(BIOS_DATA_AREA_SEGMENT, offset);
	return *p;
}

#define bda_get_ebda_segment()    bda_get_word(0x0e)

#define bda_get_video_mode()      bda_get_byte(0x49)
#define bda_get_num_columns()     bda_get_word(0x4a)
#define bda_get_video_page_size() bda_get_word(0x4c)
#define bda_get_cur_video_page()  bda_get_word(0x62)
#define bda_get_last_row()        bda_get_byte(0x84)

#define bda_get_tick_count()      bda_get_dword(0x6c)
#define bda_get_tick_count_lo()   bda_get_word(0x6c)

static inline uint16_t __far * get_video_char(uint8_t page, unsigned int x, unsigned int y)
{
	return MK_FP(0xB800, (page * bda_get_video_page_size())
	                     + ((y * bda_get_num_columns()) + x) * 2);
}

static inline uint8_t __far * get_video_scanline(uint8_t mode, uint8_t page, unsigned int y)
{
	switch (mode) {
	case 4:
	case 5:
	case 6: // CGA modes
		// Scalines are 80 bytes long, however they are interleaved.
		// Even numbered scanlines begin at 0, odd lines begin at 0x2000.
		// So offset by y%2 * 0x2000.
		return MK_FP(0xB800, (page * bda_get_video_page_size())
		                      + ((y/2) * 80) + (y%2) * 0x2000);

	default:
		return 0;
	}
}



#endif
