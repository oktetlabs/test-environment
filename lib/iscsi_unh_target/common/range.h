/*	common/range.h
 *
 *	These are the routines to manipulate range lists
 *	in the iSCSI initiator and iSCSI target implementations
 * 
 *	vi: set autoindent tabstop=8 shiftwidth=4 :
 *
 *	This file contains auxilliary functions for iscsi initiator 
 *	code that are responsible for dealing with error recovery.
 *
 *	Copyright (C) 2001-2003 InterOperability Lab (IOL)
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
#ifndef	_RANGE_H
#define	_RANGE_H

/*	Holds information to control out-of-order sequences and/or data pdus */
struct order_range {
	uint32_t offset;		/* base offset of range */
	uint32_t limit;		/* == offset + length of range */
	struct order_range *next;	/* next range element in list */
};

/*	frees all elements in a range list and then sets head->next to NULL */
void __attribute__ ((no_instrument_function))
free_range_list(struct order_range *head);

/*	Accepts the new range [new_offset..new_offset+new_length]
 *	and merges it into the existing list pointed to by head.
 *	Messages are printed if the new range overlaps any existing
 *	range in the list.
 *	A new list element is created only if no existing element can be
 *	extended by the new range.
 *	In addition, elements in the list are collapsed as holes are filled.
 */
void merge_offset_length(struct order_range *head, uint32_t new_offset,
			 uint32_t new_length);

/*	Checks that range list covers the complete range it was supposed to cover.
 *	On entry, both head and head->next must NOT be NULL!
 *	Returns total number of bytes not covered by items in the range list.
 */
int check_range_list_complete(struct order_range *head);

#endif
