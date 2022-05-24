/*
 * VBSF - NLS support functions
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

#ifndef NLS_H
#define NLS_H

static bool illegal_char( unsigned char c )
{
	int i= 0;
	
	for ( i= 0; i < data.file_char->n_illegal; ++i )
	{
		if ( c == data.file_char->illegal[i] )
		{
			return true;
		}
	}
	if (  ( c < data.file_char->lowest || c > data.file_char->highest ) ||
	     !( c < data.file_char->first_x || c > data.file_char->last_x ) )
	{
		return true;
	}
	
	return false;
}

static unsigned char nls_toupper( unsigned char c )
{
	if ( c > 0x60 && c < 0x7b )
	{
		return c & 0xDF;
	}
	
	return ( c < 0x80 ? c : data.file_upper_case[c - 0x80] );
}

static inline bool nls_uppercase(SHFLSTRING *str)
{
	int i;
	bool upper_case = true;

	for (i= 0; i < str->u16Length; ++i)
	{
		unsigned char upper = nls_toupper(str->ach[i]);
		if (upper != str->ach[i]) {
			str->ach[i] = upper;
			upper_case = false;
		}
	}
	
	return upper_case;
}

static inline bool valid_8_3_file_chars(char __far *fname )
{
	int i;

	for (i = 0; i < 11; ++i)
	{
		if ( fname[i] != ' ' && illegal_char(fname[i]) )
		{
			break;
		}
	}

	return ( i == 11 );
}

#endif // NLS_H
