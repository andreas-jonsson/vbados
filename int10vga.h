#ifndef INT10_H
#define INT10_H

#include <stdint.h>

#define BIOS_DATA_AREA_SEGMENT 0x40

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

static inline uint16_t bda_get_word(unsigned int offset) {
	uint16_t __far *p = MK_FP(BIOS_DATA_AREA_SEGMENT, offset);
	return *p;
}

static inline uint8_t bda_get_byte(unsigned int offset) {
	uint8_t __far *p = MK_FP(BIOS_DATA_AREA_SEGMENT, offset);
	return *p;
}

#define bda_get_video_mode()      bda_get_byte(0x49)
#define bda_get_num_columns()     bda_get_word(0x4a)
#define bda_get_video_page_size() bda_get_word(0x4c)
//#define bda_get_tick_count()      bda_get_dword(0x6c)
#define bda_get_tick_count_lo()   bda_get_word(0x6c)

static inline uint16_t __far * get_video_char(uint8_t page, unsigned int x, unsigned int y)
{
	return MK_FP(0xB800, (page * bda_get_video_page_size())
	                     + ((y * bda_get_num_columns()) + x) * 2);
}

#endif
