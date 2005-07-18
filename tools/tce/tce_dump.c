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

#include <te_config.h>
#include <config.h>
#include <te_defs.h>

#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <tce_collector.h>

int 
main(int argc, char *argv[])
{
    int rc;
        
    if (argc < 3)
    {
        fputs("USAGE: tce_dump <collector_pid> <data_file_prefix>\n",
              stderr);
        return EXIT_FAILURE;
    }

    tce_standalone = TRUE;
    tce_init_collector(argc - 2, argv + 2);
    tce_collector_pid = atol(argv[1]);
    rc = tce_dump_collector();
    tce_stop_collector();
    if (rc != 0)
    {
        fprintf(stderr, "Error dumping TCE data from %d, code = %x\n", 
                tce_collector_pid, rc);
        return EXIT_FAILURE;
    }
    return 0;
}
