/** @file
 * @brief Test Environment
 *
 * Test rebootability of TA
 * 
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Elena A. Vengerova <Mamadou.Ngom@oktetlabs.ru>
 * 
 * $Id: tareboot.c  10100 2005-01-26 21:25:42 mamadou $
 */
 
#include "config.h"

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

DEFINE_LGR_ENTITY("tareboot");

int
main()
{
    char ta_list[40];
    char *agentA;
    char *agentB;
    const char *delim = "\0";
    int retval;
    size_t list_len;
    
    list_len = sizeof(ta_list);
    retval = rcf_get_ta_list(ta_list, &list_len);
    if (retval != 0)
    {
        ERROR("Cannot get TA list.");
	exit(1);
    }
    
    if (list_len == 0 || list_len <= strlen(ta_list) + 1)
    {
        ERROR("Cannot get TA names");
	exit(1);
    }
    agentA = ta_list;
    agentB = ta_list + strlen(agentA) + 1;
    
    retval = rcf_ta_reboot(agentA, NULL, NULL);
    if (retval != 0)
    {
        if (EPERM == TE_RC_GET_ERROR(retval))
            ERROR("/*****TA %s is not rebootable.********/", agentA);
        else
	    ERROR("/****rcf_ta_reboot failed. errno=%X ****/",
                   TE_RC_GET_ERROR(retval)); 
    }
    else
        ERROR("/********TA %s rebooted *******/", agentA);
    
    retval = rcf_ta_reboot(agentB, NULL, NULL);

    if (retval != 0)
    {
        if (EPERM == TE_RC_GET_ERROR(retval))
            ERROR("/*****TA %s is not rebootable.********/", agentB);
        else
	    ERROR("rcf_ta_reboot failed. errno=%X",
                   TE_RC_GET_ERROR(retval)); 
    }
    else
        ERROR("/********TA %s rebooted *******/", agentB);    
    return 0;
}
