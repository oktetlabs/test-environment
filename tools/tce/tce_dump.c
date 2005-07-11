/** @file
 * @brief Test Coverage Estimation
 *
 * Force TCE collector to dump all data.
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

#include <config.h>

#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

int 
main(int argc, char *argv[])
{
    int rc;
    int result;
    int peer_id;

    if (isdigit(*argv[1]))
    {
        extern pid_t tce_collector_pid;
        extern int dump_tce_collector(void);
        extern int stop_tce_collector(void);
        int rc;
        
        tce_collector_pid = atol(argv[1]);
        rc = dump_tce_collector();
        stop_tce_collector();
        if (rc != 0)
        {
            fprintf(stderr, "Error dumping TCE data from %d, code = %x", 
                    tce_collector_pid, rc);
            exit(EXIT_FAILURE);
        }
        exit(0);
    }
        
    return 0;
}
