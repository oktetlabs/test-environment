/*	common/lun_packing.h
 *
 *	vi: set autoindent tabstop=8 shiftwidth=8 :
 *
 *	Copyright (C) 2001-2004 InterOperability Lab (IOL)
 *	University of New Hampshire (UNH)
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
#ifndef _LUN_PACKING_H
#define _LUN_PACKING_H

/* routine to pack an ordinary lun number into an 8-byte LUN structure
 * (see SAM-2, Section 4.12.3 page 39)
 * The result is always in network byte order, regardless of the
 * machine's endianness.
 * Supports 2 types of lun packing: peripheral and flat space.
 * Thanks to Bill Conway of EMC for information about this.
 *
 * When called, lun should be NON-zero, and result should be ALL ZERO!
 *
 * prototype:
 *	void __attribute__ ((no_instrument_function))
 *	pack_lun(__u32 lun, __u32 use_flat_space_lun, __u8 result[8])
 *
 * flat space addressing method for non-zero LUN, SAM-2 Section 4.12.4
 * -   high-order 2 bits of byte 0 are 01
 * -   low-order 6 bits of byte 0 are MSB of the lun
 * -   all 8 bits of byte 1 are LSB of the lun
 * -   all other bytes (2 thru 7) are 0
 * use peripheral device addressing method, SAM-2 Section 4.12.5
 * -    high-order 2 bits of byte 0 are 00
 * -    low-order 6 bits of byte 0 are all 0
 * -    all 8 bits of byte 1 are the lun
 * -    all other bytes (2 thru 7) are 0
 */
#define pack_lun(lun, use_flat_space_lun, result)			\
	do {								\
		if (use_flat_space_lun) {				\
			*(result) = 0x40 | ((lun >> 8) & 0x3f);		\
		}							\
		/* same in both supported packing methods */		\
		*((result) + 1) = lun;					\
	} while(0)


/*	routine to extract a lun number from an 8-byte LUN structure
	in network byte order.
	(see SAM-2, Section 4.12.3 page 39)
	Supports 2 types of lun unpacking: peripheral and flat space.
	Thanks to Bill Conway of EMC for information about this.
*/
uint32_t __attribute__ ((no_instrument_function))
unpack_lun(uint8_t * lun_ptr);

/*
 * dumps "length" bytes from "buffer" in hex, 16 bytes per output line
 */
void __attribute__ ((no_instrument_function))
dump_buffer(uint8_t * buffer, int length);

#endif
