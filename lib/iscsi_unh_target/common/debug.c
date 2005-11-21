/*	common/debug.c
 * 
 *	vi: set autoindent tabstop=8 shiftwidth=8 :
 * 
 *	This file initializes the global iscsi_trace_mask
 *	to the default tracing options
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

#include "te_config.h"

#include <inttypes.h>
#include "te_defs.h"
#include "debug.h"

static int verbosity_level = ISCSI_VERBOSITY_SILENT;

static char *level_map[] = {
    "silent", "minimal",
    "normal", "verbose", 
    "debug",  "printall", NULL
};

te_bool
iscsi_set_verbose(const char *level)
{
    char **iter;
    for (iter = level_map; *iter != NULL; iter++)
    {
        if (strcmp(level, *iter) == 0)
        {
            verbosity_level = iter - level_map;
            return TRUE;
        }
    }
    ERROR("Unknown verbosity level '%s'", level);
    return FALSE;
}

const char *
iscsi_get_verbose(void)
{
    return level_map[verbosity_level];
}

te_bool
iscsi_check_verbose(int level)
{
    return level <= verbosity_level;
}

