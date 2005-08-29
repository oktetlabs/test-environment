/*	common/range.c
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
#include <stdlib.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include "../userland_lib/my_memory.h"
#include "iscsi_common.h"
#include "debug.h"
#include "range.h"

/*	frees all elements in a range list and then sets head->next to NULL */
void __attribute__ ((no_instrument_function))
free_range_list(struct order_range *head)
{
	struct order_range *ptr, *next;

	for (ptr = head->next; ptr != NULL; ptr = next) {
		next = ptr->next;
		TRACE(TRACE_ISCSI_FULL, "free range [%u..%u]\n", ptr->offset,
		      ptr->limit);
		my_free((void **) &ptr);
	}
	head->next = NULL;
}

/*	
 * here to collapse two items into 1 if they are adjacent or overlap 
 */
static void
collapse(struct order_range *here, struct order_range *next)
{
	if (next != NULL && here->limit >= next->offset) {	
		/* ranges are adjacent or overlap */
		if (here->limit >= next->limit) {	
			/* here range completely subsumes next range */
			TRACE(TRACE_ISCSI, "range [%u..%u] subsumes [%u..%u]\n",
			      here->offset, here->limit, next->offset,
			      next->limit);
		} else {	
			/* upper end of here range overlaps 
				lower end of next range */
			if (here->limit > next->offset) {	
				/* really have overlap, not just adjacency */
				TRACE(TRACE_ISCSI,
				      "range [%u..%u] overlaps [%u..%u]\n",
				      here->offset, here->limit, next->offset,
				      next->limit);
			} else {	
				/* have adjacency */
				TRACE(TRACE_ISCSI_FULL,
				      "range [%u..%u] precedes [%u..%u]\n",
				      here->offset, here->limit, next->offset,
				      next->limit);
			}
			here->limit = next->limit;
		}
		here->next = next->next;
		my_free((void **) &next);
		/* note recurse */
		collapse(here, here->next);
	}
}

/*	Accepts the new range [new_offset..new_offset+new_length]
 *	and merges it into the existing list pointed to by head.
 *	Messages are printed if the new range overlaps any existing
 *	range in the list.
 *	A new list element is created only if no existing element can be
 *	extended by the new range.
 *	In addition, elements in the list are collapsed as holes are filled.
 */
void
merge_offset_length(struct order_range *head, uint32_t new_offset,
		    uint32_t new_length)
{
	struct order_range *ptr, *prev, *newptr;
	uint32_t new_limit;

	new_limit = new_offset + new_length;

	for (prev = NULL, ptr = head->next; ptr != NULL;
	     prev = ptr, ptr = ptr->next) {
		if (new_offset < ptr->offset) {	
			/* the new range starts below the ptr range,
			 * insert new item here in list, then try 
			 * collapsing it 
			 */
			break;
		} else if (new_offset <= ptr->limit) {	
			/* new range starts at or before end of ptr range */
			if (new_limit <= ptr->limit) {	
				/* new range completely within ptr range */
				TRACE(TRACE_ISCSI,
				      "range [%u..%u] subsumes [%u..%u]\n",
				      ptr->offset, ptr->limit, new_offset,
				      new_limit);
			} else {	
				/* lower end of new range overlaps 
				 * upper end of ptr range 
				 */
				if (new_offset == ptr->limit) {	
					/* actually have adjacency */
					TRACE(TRACE_ISCSI_FULL,
					      "range [%u..%u] precedes [%u..%u]\n",
					      ptr->offset, ptr->limit,
					      new_offset, new_limit);
				} else {	/* true overlap */
					TRACE(TRACE_ISCSI,
					      "range [%u..%u] overlaps [%u..%u]\n",
					      ptr->offset, ptr->limit,
					      new_offset, new_limit);
				}
				ptr->limit = new_limit;
				collapse(ptr, ptr->next);
			}
			return;
		}
	}
	/* if loop finishes, need to add a new range after prev, before ptr */
	if ((newptr = (struct order_range *) 
		malloc(sizeof (struct order_range))) == NULL) {	
			/* no memory for new range structure */
		return;
	}

	TRACE(TRACE_ISCSI_FULL, "new range [%u..%u]\n", new_offset, new_limit);

	newptr->offset = new_offset;
	newptr->limit = new_limit;
	newptr->next = ptr;
	if (prev == NULL)
		head->next = newptr;	/* first element on list */
	else
		prev->next = newptr;
	collapse(newptr, ptr);
	return;
}

/*	
 * 	Checks that range list covers the complete range it was 
 * 	supposed to cover.
 *	On entry, both head and head->next must NOT be NULL!
 *	Returns total number of bytes not covered by items in the range list.
 */
int
check_range_list_complete(struct order_range *head)
{
	struct order_range *ptr, *prev;
	uint32_t gap;
	int missing;

	TRACE((TRACE_ENTER_LEAVE | TRACE_ISCSI_FULL),
	      "Enter check_range_list_complete [%u..%u]\n", head->offset,
	      head->limit);

	missing = 0;
	if (head->offset < head->next->offset) {	
		/* first range starts later than it 
			was supposed to start */
		gap = head->next->offset - head->offset;
		TRACE(TRACE_ISCSI, "gap of %u before first range [%u..%u]\n",
		      gap, head->next->offset, head->next->limit);
		missing += gap;
	}
	for (ptr = head->next, prev = NULL; ptr != NULL;
	     prev = ptr, ptr = ptr->next) {
		if (prev != NULL && prev->limit < ptr->offset) {
			/* have a gap between the ranges of two list items */
			gap = ptr->offset - prev->limit;
			TRACE(TRACE_ISCSI,
			      "gap of %u between range [%u..%u] and [%u..%u]\n",
			      gap, prev->offset, prev->limit, ptr->offset,
			      ptr->limit);
			missing += gap;
		}
	}
	if (prev->limit < head->limit) {	
		/* last range ends before it was supposed to end */
		gap = head->limit - prev->limit;
		TRACE(TRACE_ISCSI, "gap of %u after last range [%u..%u]\n", gap,
		      prev->offset, prev->limit);
		missing += gap;
	}

	TRACE((TRACE_ENTER_LEAVE | TRACE_ISCSI_FULL),
	      "Leave check_range_list_complete, missing %d\n", missing);

	return missing;
}
