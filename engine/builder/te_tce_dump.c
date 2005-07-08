/** @file
 * @brief Test Coverage Estimation
 *
 * Retrieving TCE data from a TA
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "rcf_api.h"
#include "logger_api.h"
#include "logger_ten.h"


DEFINE_LGR_ENTITY("TCE dump");


int 
main(int argc, char *argv[])
{
    int rc;
    int result;
    int peer_id;

    static char ta_list[4096];
    size_t ta_list_len = sizeof(ta_list);
    char *ta;

    char buffer[PATH_MAX + 1];
    char buffer2[PATH_MAX + 1];

    UNUSED(argc);

    if (strcmp(argv[1], "--all") != 0)
    {
        strcpy(ta_list, argv[1]);        
    }
    else
    {
        rc = rcf_get_ta_list(ta_list, &ta_list_len);
        if (rc != 0)
        {
            ERROR("Unable to obtain TA list, error code = %d", rc);
            return EXIT_FAILURE;
        }
    }
    

    for (ta = ta_list; *ta; ta += strlen(ta) + 1)
    {
        VERB("Forcing TCE");
        rc = rcf_ta_call(ta, 0, "obtain_tce_peer_id", &peer_id, 0, 
                         FALSE, NULL);
        if (rc != 0)
        {
            ERROR("Unable to obtain TCE peer id, error code = %d", rc);
            return EXIT_FAILURE;
        }
        
        if (peer_id != 0)
        {
            rc = rcf_ta_call(ta, 0, "dump_collected_tce", &result, 0, 
                             FALSE, NULL);
            if (rc != 0 || result != 0)
            {
                ERROR("Unable to dump TCE, error code = %d", 
                      rc ? rc : result);
                return EXIT_FAILURE;
            }
            
            sprintf(buffer, "%s%d.tar", argv[2], peer_id);
            sprintf(buffer2, "%s%s.tar", argv[3], ta);
            if ((rc = rcf_ta_get_file(ta, 0, buffer, buffer2)) != 0)
            {
                ERROR("Unable to obtain TCE data file (%s -> %s), "
                      "error code = %d", 
                      buffer, buffer2, rc);
                return EXIT_FAILURE;
            }
            
            rc = rcf_ta_call(ta, 0, "stop_collect_tce", &result, 0, 
                             FALSE, NULL);
            if (rc != 0 || result != 0)
            {
                WARN("Unable to stop TCE, error code = %d", 
                      rc ? rc : result);
            }
            puts(ta);
        }    
    }
        
    return 0;
}
