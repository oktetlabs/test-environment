/*	common/debug.h
 * 
 *	vi: set autoindent tabstop=8 shiftwidth=8 :
 * 
 *	This file defines the tracing macros and masks
 *
 *	Copyright (C) 2003 InterOperability Lab (IOL)
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
 *	The name IOL and/or UNH may not be used to endorse or promote 
 *	products derived from this software without specific prior 
 * 	written permission.
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#define PRINT printf

#if !defined(CONFIG_ISCSI_DEBUG)

	#define TRACE_SET(mask) do { } while(0)
	#define TRACE_GET(mask) do { } while(0)
	#define TRACE_TEST(mask) (0)
	#define TRACE(mask, args...) do { } while(0)
	#define TRACE_BUFFER(mask, buffer, len, args...) do { } while(0)

#else

#define TRACE_ENDING		0x0000
#define TRACE_DEBUG		0x0001
#define TRACE_ISCSI_FULL	0x0002
#define TRACE_ISCSI		0x0004
#define TRACE_NET		0x0008
#define TRACE_BUF		0x0010
#define TRACE_SEM		0x0020
#define TRACE_ENTER_LEAVE	0x0040
#define TRACE_MY_MEMORY		0x0080
#define TRACE_TIMERS		0x0100
#define TRACE_ERROR_RECOVERY	0x0200
#define TRACE_VERBOSE		0x0400
#define TRACE_ALL		0xffff

extern uint32_t iscsi_trace_mask;

#define TRACE_SET(mask)		iscsi_trace_mask = mask

#define TRACE_GET(mask)		mask = iscsi_trace_mask

#define TRACE_TEST(mask)	(iscsi_trace_mask & mask)
 
#define TRACE(mask, args...)						   \
	do {								   \
		if (iscsi_trace_mask & mask) { 				   \
			if (iscsi_trace_mask & TRACE_VERBOSE) { 	   \
				PRINT("%s:%d:", __FUNCTION__, __LINE__);   \
			}						   \
			PRINT(args);		   			   \
		}							   \
	} while(0)

#define TRACE_BUFFER(mask, buffer, len, args...)			   \
	do {								   \
		if (iscsi_trace_mask & mask) { 				   \
			int ndx;					   \
			PRINT(args);					   \
			for (ndx = 0; ndx < len; ndx++)	{		   \
				if ((ndx & 0xf) == 0) {			   \
					PRINT("%3d:", ndx);		   \
				}					   \
				PRINT(" %02x",				   \
					*((uint8_t *)(buffer)+ndx)); \
				if ((ndx & 0xf) == 0xf) {		   \
					PRINT("\n");			   \
				}					   \
			}						   \
			if ((ndx & 0xf) != 0) {				   \
				PRINT("\n");				   \
			}						   \
		}							   \
	} while(0)

#endif

#define TRACE_ERROR(args...)						   \
	do  {								   \
		PRINT("%s:%d:", __FUNCTION__, __LINE__);		   \
		PRINT(__FILE__ ": ***ERROR*** " args);			   \
	} while(0)

#define TRACE_WARNING(args...)						   \
	do  {								   \
		PRINT("***Warning*** " args);				   \
	} while(0)

#endif
