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
    const char *nut = argv[1];
    const char *ta = argv[2];
    const char *filename = argv[3];
    const char *map_filename = argv[4];

    int rc;
    int result;
    int peer_id;

    char buffer[RCF_MAX_PATH + 1];


    if (argc != 4 && argc != 5)
    {
        ERROR("Invalid number of arguments");
        return EXIT_FAILURE;
    }

    VERB("Obtain principal peer ID for %s on TA %s", nut, ta);
    rc = rcf_ta_call(ta, 0, "tce_obtain_principal_peer_id", &peer_id,
                     0, FALSE, NULL);
    if (rc != 0)
    {
        ERROR("Unable to obtain TCE peer id, error code = %d", rc);
        return EXIT_FAILURE;
    }
    RING("TCE principal peer ID for '%s' on TA '%s' is %d",
         nut, ta, peer_id);

    if (peer_id != 0)
    {
        VERB("Dump TCE collector for peer ID %d on TA %s", peer_id, ta);
        rc = rcf_ta_call(ta, 0, "tce_dump_collector", &result,
                         0, FALSE, NULL);
        if (rc != 0 || result != 0)
        {
            ERROR("Unable to dump TCE, error code = %d",
                  (rc != 0) ? rc : result);
            return EXIT_FAILURE;
        }

        VERB("Get TCE dump for peer ID %d from TA %s, put in %s",
             peer_id, ta, filename);
        sprintf(buffer, "/tmp/tcedump%d.tar", peer_id);
        if ((rc = rcf_ta_get_file(ta, 0, buffer, filename)) != 0)
        {
            ERROR("Unable to obtain TCE data file (%s -> %s), "
                  "error code = %d", buffer, filename, rc);
            return EXIT_FAILURE;
        }

        if (map_filename != NULL)
        {
            VERB("Get TCE module map for peer ID %d from TA %s, put in %s",
                 peer_id, ta, filename);
            sprintf(buffer, "/tmp/tcedump%d.map", peer_id);
            if ((rc = rcf_ta_get_file(ta, 0, buffer, map_filename)) != 0)
            {
                ERROR("Unable to obtain TCE data file (%s -> %s), "
                      "error code = %d", buffer, filename, rc);
                return EXIT_FAILURE;
            }
        }


        VERB("Stop TCE collector for peer ID %d on TA %s", peer_id, ta);
        rc = rcf_ta_call(ta, 0, "tce_stop_collector", &result,
                         0, FALSE, NULL);
        if (rc != 0 || result != 0)
        {
            WARN("Unable to stop TCE, error code = %d",
                  rc ? rc : result);
        }
    }

    return 0;
}
