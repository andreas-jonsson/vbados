#ifndef INT10_H
#define INT10_H

#include <stdint.h>
#include <conio.h>

#define BIOS_DATA_AREA_SEGMENT 0x40

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
#define bda_get_char_height()     bda_get_word(0x85)

#define bda_get_tick_count()      bda_get_dword(0x6c)
#define bda_get_tick_count_lo()   bda_get_word(0x6c)

enum videotype {
	VIDEO_UNKNOWN,
	VIDEO_TEXT,
	VIDEO_CGA,
	VIDEO_EGA,
	VIDEO_VGA
};

struct modeinfo {
	uint8_t mode;
	uint8_t page;
	enum videotype type;

	uint16_t pixels_width, pixels_height;
	uint16_t bytes_per_line;
	uint16_t odd_scanline_offset;
	uint8_t bits_per_pixel;
	uint8_t num_planes;

	/** Pointer to video memory. */
	uint8_t __far * begin;
};

static void get_current_video_mode_info(struct modeinfo *info)
{
	uint16_t segment;

	info->mode = bda_get_video_mode() & ~0x80;
	info->page = bda_get_cur_video_page();

	info->odd_scanline_offset = 0;

	switch (info->mode) {
	case 0:
	case 1:
	case 2:
	case 3: // CGA text modes with 25 rows and variable columns
	case 7: // MDA Mono text mode
		info->type = VIDEO_TEXT;
		info->pixels_width = bda_get_num_columns() * 8;
		info->pixels_height = (bda_get_last_row()+1) * 8;
		info->bytes_per_line = bda_get_num_columns() * 2;
		info->bits_per_pixel = 2 * 8;
		info->num_planes = 1;
		segment = 0xB800;
		break;

	case 4:
	case 5: // CGA 320x200 4-color
		info->type = VIDEO_CGA;
		info->pixels_width = 320;
		info->pixels_height = 200;
		info->bytes_per_line = 80;
		info->odd_scanline_offset = 0x2000;
		info->bits_per_pixel = 2;
		info->num_planes = 1;
		segment = 0xB800;
		break;

	case 6: // CGA 640x200 2-color
		info->type = VIDEO_CGA;
		info->pixels_width = 640;
		info->pixels_height = 200;
		info->bytes_per_line = 80;
		info->odd_scanline_offset = 0x2000;
		info->bits_per_pixel = 1;
		info->num_planes = 1;
		segment = 0xB800;
		break;

	case 0xd: // EGA 320x200 16-color
		info->type = VIDEO_EGA;
		info->pixels_width = 320;
		info->pixels_height = 200;
		info->bytes_per_line = 40;
		info->bits_per_pixel = 1;
		info->num_planes = 4;
		segment = 0xA000;
		break;

	case 0xe: // EGA 640x200 16-color
		info->type = VIDEO_EGA;
		info->pixels_width = 640;
		info->pixels_height = 200;
		info->bytes_per_line = 80;
		info->bits_per_pixel = 1;
		info->num_planes = 4;
		segment = 0xA000;
		break;

	case 0xf: // EGA 640x350 4-color
		info->type = VIDEO_EGA;
		info->pixels_width = 640;
		info->pixels_height = 350;
		info->bytes_per_line = 80;
		info->bits_per_pixel = 1;
		info->num_planes = 2;
		segment = 0xA000;
		break;

	case 0x10: // EGA 640x350 16-color
		info->type = VIDEO_EGA;
		info->pixels_width = 640;
		info->pixels_height = 350;
		info->bytes_per_line = 80;
		info->bits_per_pixel = 1;
		info->num_planes = 4;
		segment = 0xA000;
		break;

	case 0x11: // VGA 640x480 2-color
	case 0x12: // VGA 640x480 16-color
		info->type = VIDEO_VGA;
		info->pixels_width = 640;
		info->pixels_height = 480;
		info->bytes_per_line = 80;
		info->bits_per_pixel = 1;
		info->num_planes = 4;
		segment = 0xA000;
		break;

	case 0x13: // VGA 320x200 256-color
		info->type = VIDEO_VGA;
		info->pixels_width = 320;
		info->pixels_height = 200;
		info->bytes_per_line = 320;
		info->bits_per_pixel = 8;
		info->num_planes = 1;
		segment = 0xA000;
		break;


	default:
		info->type = VIDEO_UNKNOWN;
		// Let's put in some default coordinates at leas
		info->pixels_width = 640;
		info->pixels_height = 200;
		segment = 0;
	}

	info->begin = MK_FP(segment, info->page * bda_get_video_page_size());
}

static inline uint16_t __far * get_video_char(const struct modeinfo *info, unsigned int x, unsigned int y)
{
	return (uint16_t __far *) (info->begin + (y * info->bytes_per_line) + (x * 2));
}

static inline uint8_t __far * get_video_scanline(const struct modeinfo *info, unsigned int y)
{
	if (info->odd_scanline_offset) {
		return info->begin
		        + (y%2 * info->odd_scanline_offset)
		        + (y/2 * info->bytes_per_line);
	} else {
		return info->begin
		        + (y * info->bytes_per_line);
	}
}

enum vga_io_ports {
	VGA_PORT_SC_ADDRESS = 0x3c4,
	VGA_PORT_SC_DATA    = 0x3c5,
	VGA_PORT_GC_ADDRESS = 0x3ce,
	VGA_PORT_GC_DATA    = 0x3cf
};

enum vga_sc_regs {
	VGA_SC_REG_MAP_MASK = 2
};

enum vga_gc_regs {
	VGA_GC_REG_SET_RESET = 0,
	VGA_GC_REG_ENABLE_SET_RESET = 1,
	VGA_GC_REG_DATA_ROTATE = 3,
	VGA_GC_REG_READ_MAP = 4,
	VGA_GC_REG_GRAPHICS_MODE = 5,
	VGA_GC_REG_BIT_MASK = 8
};

struct videoregs {
	uint8_t sc_reg;
	uint8_t sc_map_mask;
	uint8_t gc_reg;
	uint8_t gc_enable_set_reset;
	uint8_t gc_data_rotate;
	uint8_t gc_graphics_mode;
	uint8_t gc_read_map;
	uint8_t gc_bit_mask;
};

static inline uint8_t vga_sc_reg_read(uint8_t reg)
{
	outp(VGA_PORT_SC_ADDRESS, reg);
	return inp(VGA_PORT_SC_DATA);
}

static inline void vga_sc_reg_write(uint8_t reg, uint8_t val)
{
	// Do one 16-bit write, the higher part will be the data while the lower byte the register.
	outpw(VGA_PORT_SC_ADDRESS, (val << 8) | reg);
}

static inline uint8_t vga_gc_reg_read(uint8_t reg)
{
	outp(VGA_PORT_GC_ADDRESS, reg);
	return inp(VGA_PORT_GC_DATA);
}

static inline void vga_gc_reg_write(uint8_t reg, uint8_t val)
{
	outpw(VGA_PORT_GC_ADDRESS, (val << 8) | reg);
}

static void vga_save_registers(struct videoregs __far *regs)
{
	regs->sc_reg = inp(VGA_PORT_SC_ADDRESS);
	regs->sc_map_mask = vga_sc_reg_read(VGA_SC_REG_MAP_MASK);
	regs->gc_reg = inp(VGA_PORT_GC_ADDRESS);
	regs->gc_enable_set_reset = vga_gc_reg_read(VGA_GC_REG_ENABLE_SET_RESET);
	regs->gc_data_rotate = vga_gc_reg_read(VGA_GC_REG_DATA_ROTATE);
	regs->gc_read_map = vga_gc_reg_read(VGA_GC_REG_READ_MAP);
	regs->gc_graphics_mode = vga_gc_reg_read(VGA_GC_REG_GRAPHICS_MODE);
	regs->gc_bit_mask = vga_gc_reg_read(VGA_GC_REG_BIT_MASK);
}

static void vga_restore_register(struct videoregs __far *regs)
{
	vga_sc_reg_write(VGA_SC_REG_MAP_MASK, regs->sc_map_mask);
	outp(VGA_PORT_SC_ADDRESS, regs->sc_reg);
	vga_gc_reg_write(VGA_GC_REG_ENABLE_SET_RESET, regs->gc_enable_set_reset);
	vga_gc_reg_write(VGA_GC_REG_DATA_ROTATE, regs->gc_data_rotate);
	vga_gc_reg_write(VGA_GC_REG_READ_MAP, regs->gc_read_map);
	vga_gc_reg_write(VGA_GC_REG_GRAPHICS_MODE, regs->gc_graphics_mode);
	vga_gc_reg_write(VGA_GC_REG_BIT_MASK, regs->gc_bit_mask);
	outp(VGA_PORT_GC_ADDRESS, regs->gc_reg);
}

static void vga_set_graphics_mode(struct videoregs __far *regs, unsigned read, unsigned write)
{
	vga_gc_reg_write(VGA_GC_REG_GRAPHICS_MODE, (regs->gc_graphics_mode & ~(8|3))
	                                           | ((read << 3) & 8) | write & 3);
	if (write == 0) {
		// Set registers to a reasonable default ...
		vga_gc_reg_write(VGA_GC_REG_ENABLE_SET_RESET, 0);
		vga_gc_reg_write(VGA_GC_REG_DATA_ROTATE, 0);
		vga_gc_reg_write(VGA_GC_REG_BIT_MASK, 0xFF);
	}
}

static inline void vga_select_plane(unsigned plane)
{
	vga_gc_reg_write(VGA_GC_REG_READ_MAP, plane & 0x3);
	vga_sc_reg_write(VGA_SC_REG_MAP_MASK, 1 << plane);

}

#endif
