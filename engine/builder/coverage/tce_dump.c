/** @file
 * @brief TCE
 *
 * Retrieving TCE data from a TA
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
 * $Id: te_tee.c 12322 2005-03-10 14:58:56Z artem $
 */

#include "te_config.h"

#define TE_LGR_USER "Self"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "rcf_api.h"
#include "logger_api.h"
#include "logger_ten.h"

DEFINE_LGR_ENTITY("(TCE dump)");

int 
main(int argc, char *argv[])
{
    int rc;
    int result;
    int peer_id;
    char buffer[PATH_MAX + 1];
    char buffer2[PATH_MAX + 1];
    
    
    VERB("Forcing TCE");
    rc = rcf_ta_call(argv[1], 0, "dump_collected_tce", &result, 0, 
                     FALSE, NULL);
    if (rc != 0 || result != 0)
    {
        ERROR("Unable to dump TCE, error code = %d", rc ? rc : result);
        return EXIT_FAILURE;
    }
    
    rc = rcf_ta_call(argv[1], 0, "obtain_tce_peer_id", &peer_id, 0, 
                     FALSE, NULL);
    if (rc != 0)
    {
        ERROR("Unable to obtain TCE peer id, error code = %d", rc);
        return EXIT_FAILURE;
    }

    sprintf(buffer, "%s%d.tar", argv[2], peer_id);
    sprintf(buffer2, "%s%s.tar", argv[3], argv[1]);
    if ((rc = rcf_ta_get_file(argv[1], 0, buffer, buffer2)) != 0)
    {
        ERROR("Unable to obtain TCE data file, error code = %d", rc);
        return EXIT_FAILURE;
    }
    
    rc = rcf_ta_call(argv[1], 0, "stop_tce_collect", &result, 0, 
                     FALSE, NULL);
    if (rc != 0 || result != 0)
    {
        ERROR("Unable to stop TCE, error code = %d", rc ? rc : result);
        return EXIT_FAILURE;
    }
    
        
    return 0;
}
