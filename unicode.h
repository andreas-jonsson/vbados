/*
 * VBSF - unix to DOS time conversion
 * Copyright (C) 2022 Javier S. Pedro
 *
 * unicode.h: Unicode conversion routines
 * Copyright (C) 2011-2022 Eduardo Casino
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General License for more details.
 *
 * You should have received a copy of the GNU General License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef UNICODE_H
#define UNICODE_H

#include <stdint.h>
#include "sftsr.h"


#ifdef __IN_SFTSR__
#define TSRDATAPTR PTSRDATA
#else
#define TSRDATAPTR LPTSRDATA
#endif

static inline uint8_t lookup_codepage( TSRDATAPTR data, uint16_t cp )
{
	uint8_t i;
	
	for ( i = 0; i < 128 && data->unicode_table[i] != cp; ++i );
	
	return ( i < 128 ? (uint8_t) i + 128 : '\0' );
}

// dst and src CAN'T BE THE SAME !!!!
// Returns resulting length or -1 if buffer overflow
//
static uint16_t local_to_utf8_n( TSRDATAPTR data, uint8_t *dst, const char far *src, uint16_t buflen, uint16_t count )
{
	uint16_t len = 0;	// Resulting length
	uint16_t cp;	// Unicode Code Point

	while ( *src && count )
	{
		// UTF-8 bytes: 0xxxxxxx
		// Binary CP:   0xxxxxxx
		// CP range:    U+0000 to U+007F (Direct ASCII translation)
		//
		if ( ! (*src & 0x80) )
		{	
			if ( buflen > len )
			{
				*dst++ = *src;
				++len;
				goto cont;
			}
			else
			{
				return -1;
			}
		} 

		cp = data->unicode_table[*src - 128];
		
		// UTF-8 bytes: 110yyyyy 10xxxxxx
		// Binary CP:   00000yyy yyxxxxxx
		// CP range:    U+0080 to U+07FF
		//		
		if ( ! (cp & 0xF000) )
		{
			if ( buflen > len + 1 )
			{
				*dst++ = (uint8_t)( cp >> 6 ) | 0xC0;
				*dst++ = (uint8_t)( cp & 0x3f ) | 0x80;
				len += 2;
			}
			else
			{
				return -1;
			}
		}
				
		// UTF-8 bytes: 1110zzzz 10yyyyyy 10xxxxxx
		// Binary CP:   zzzzyyyy yyxxxxxx
		// CP range:    U+0800 to U+FFFF
		//
		else
		{
			if ( buflen > len +2 )
			{
				*dst++ = (uint8_t)( cp >> 12 ) | 0xE0;
				*dst++ = (uint8_t)( (cp >> 6) & 0x3F ) | 0x80;
				*dst++ = (uint8_t)( cp & 0x3F ) | 0x80;
				len += 3;
			}
			else
			{
				return -1;
			}
		}
cont:
		++src, --count;
	};

	// Terminate string
	//
	*dst = '\0';
	
	return len;
	
}

static inline uint16_t local_to_utf8( TSRDATAPTR data, uint8_t *dst, const char far *src, uint16_t buflen )
{
	return local_to_utf8_n( data, dst, src, buflen, buflen );
}

// Returns true on success, false if any unsupported char is found
//
static bool utf8_to_local( TSRDATAPTR data, char *dst, char *src, uint16_t *len )
{
	bool ret = true;	// Return code
	uint16_t cp;		// Unicode Code point
	uint16_t l = 0;

	while ( *src )
	{
		// UTF-8 bytes: 0xxxxxxx
		// Binary CP:   0xxxxxxx
		// CP range:    U+0000 to U+007F (Direct ASCII translation)
		//
		if ( ! (*src & 0x80) )
		{	
			*dst = *src;
			++src;
			goto cont;
		} 
		
		// UTF-8 bytes: 110yyyyy 10xxxxxx
		// Binary CP:   00000yyy yyxxxxxx
		// CP range:    U+0080 to U+07FF
		//
		if ( ! (*src & 0x20) ) 
		{
			cp = ( (uint16_t)(*src & 0x1F) << 6 ) | *(src+1) & 0x3F;
			*dst = lookup_codepage( data, cp );
			if ( *dst == '\0' )
			{
				*dst = '_';
				ret = false;
			}
			src += 2;
			goto cont;
		}
		
		// UTF-8 bytes: 1110zzzz 10yyyyyy 10xxxxxx
		// Binary CP:   zzzzyyyy yyxxxxxx
		// CP range:    U+0800 to U+FFFF
		//
		if ( ! (*src & 0x10) )
		{
			cp = ( (uint16_t)(*src & 0xF) << 12 ) | ( (uint16_t)(*(src+1) & 0x3F) << 6 ) | *(src+2) & 0x3F;
			*dst = lookup_codepage( data, cp );
			if ( *dst == '\0' )
			{
				*dst = '_';
				ret = false;
			}
			src += 3;
			goto cont;
		}
		
		// UTF-8 bytes: 11110www 10zzzzzz 10yyyyyy 10xxxxxx
		// Binary CP:   000wwwzz zzzzyyyy yyxxxxxx
		// CP range:    U+010000 to U+10FFFF
		//
		if ( ! (*src & 0x08) )
		{
			*dst = '_';		// Currently unsupported
			ret = false;
			src += 4;
			goto cont;
		}
		
		// Should not reach here
		//
		*dst = '_';
		ret = false;
		++src;
cont:
		++dst, ++l;
		
	};
	
	// Terminate string
	//
	*dst = '\0';

	if (len) *len = l;

	return ret;

}

#endif
