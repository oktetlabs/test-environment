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

#ifndef TE_LGR_USER
#define TE_LGR_USER "UNH Target"
#endif

#include <stdio.h>
#include "te_defs.h"
#include "logger_api.h"
#include "logger_defs.h"
#include "te_errno.h"

enum iscsi_verbosity_levels {
    ISCSI_VERBOSITY_SILENT,
    ISCSI_VERBOSITY_MINIMAL,
    ISCSI_VERBOSITY_NORMAL,
    ISCSI_VERBOSITY_VERBOSE,
    ISCSI_VERBOSITY_DEBUG,
    ISCSI_VERBOSITY_PRINTALL
};

/**
 * Sets the verbosity level for the target.
 * The following levels are defined:
 * -# silent   The target logs nothing)
 * -# minimal  Only errors and warnings are reported (the default level)
 * -# normal   Important events are reported
 * -# verbose  All non-debug info is reported
 * -# debug    All debug info is reported, excluding raw PDU dumps
 * -# printall Everything is reported
 * 
 * @param level Verbosity level
 * 
 * @return TRUE if the level is a valid string
 */
extern te_bool iscsi_set_verbose(const char *level);
extern const char *iscsi_get_verbose(void);
extern te_bool iscsi_check_verbose(int level);

#define TRACE(level, args...) do {                          \
        if (iscsi_check_verbose(ISCSI_VERBOSITY_ ## level)) \
            RING(args);                                     \
    } while(0)

#define TRACE_BUFFER(level, buffer, len, args...)               \
	do {                                                        \
			TRACE(level, args);                                 \
            if (iscsi_check_verbose(ISCSI_VERBOSITY_PRINTALL))  \
                print_payload(buffer, len);                     \
	} while(0)

#define TRACE_ERROR(args...) do {                           \
        if (iscsi_check_verbose(ISCSI_VERBOSITY_MINIMAL))   \
            ERROR(args);                                    \
    } while(0)

#define TRACE_WARNING(args...) do {                           \
        if (iscsi_check_verbose(ISCSI_VERBOSITY_MINIMAL))   \
            WARN(args);                                     \
    } while(0)


#endif
