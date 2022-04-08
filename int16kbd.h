#ifndef INT16KBD_H
#define INT16KBD_H

static bool int16_store_keystroke(uint16_t scancode);
#pragma aux int16_store_keystroke = \
	"mov ah, 0x05" \
	"int 0x16" \
	__parm [cx] \
	__value [al] \
	__modify [ax]

#endif // INT16KBD_H
