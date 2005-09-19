/*	common/lun_packing.c
 *
 *	vi: set autoindent tabstop=8 shiftwidth=8 :
 *
 *	Copyright (C) 2001-2004 InterOperability Lab (IOL)
 *	University of New Hampshier (UNH)
 *	Durham, NH 03824
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2, or (at your option)
 *	any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *	USA.
 *
 *	The name of IOL and/or UNH may not be used to endorse or promote 
 *	products derived from this software without specific prior 
 * 	written permission.
*/
#include <inttypes.h>
#include <string.h>

#include "debug.h"
#include "lun_packing.h"


/*	routine to extract a lun number from an 8-byte LUN structure
	in network byte order.
	(see SAM-2, Section 4.12.3 page 39)
	Supports 2 types of lun unpacking: peripheral and flat space.
	Thanks to Bill Conway of EMC for information about this.
*/
uint32_t __attribute__ ((no_instrument_function))
unpack_lun(uint8_t * lun_ptr)
{
	uint32_t result, temp;

	result = *(lun_ptr + 1) ;    /* same in both supported packing methods */

	switch (temp = ((*lun_ptr) >> 6)) {	/* high 2 bits of byte 0 */
	case 0:	
		if (*lun_ptr != 0) {
			TRACE_ERROR
			    ("Illegal Byte 0 in LUN peripheral device addressing "
			     "method 0x%02x, expected 0\n", *lun_ptr);
		}
		break;

	case 1:		
		result += ((*lun_ptr) & 0x3f) << 8;
		break;

	default:		/* (extended) logical unit addressing */
		TRACE_ERROR("Unimplemented LUN addressing method %u, "
                    "PDA method used instead\n", temp);
		break;
	}			/* switch */

	return result;
}

/*
 * dumps "length" bytes from "buffer" in hex, 16 bytes per output line
 */
void __attribute__ ((no_instrument_function))
dump_buffer(uint8_t * buffer, int length)
{
	int k;

	for (k = 0; k < length; k++) {
		if (k % 16 == 0) {
			if (k != 0) {
				putchar('\n');
			}
			printf("%4d:", k);
		}
		printf(" %02x", *buffer++);
	}
	putchar('\n');
}
