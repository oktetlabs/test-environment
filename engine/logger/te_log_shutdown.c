/** @file
 * @brief TE project. Logger subsystem.
 *
 * Separate Logger task for shutdowning main Logger process.
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
 *
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#include <stdlib.h>

#include "te_stdint.h"
#include "te_raw_log.h"
#include "logger_int.h"
#include "ipc_client.h"
#include "logger_internal.h"


/**
 * This is an entry point of te_log_shutdown task.
 */
int
main(void)
{
    int                 result = EXIT_SUCCESS;
    te_log_nfl_t        nfl = strlen(LGR_SHUTDOWN);
    uint8_t             mess[TE_LOG_NFL_SZ + nfl];
    struct ipc_client  *log_client;


    /* Prepare message with entity name LGR_SHUTDOWN */
    *(te_log_nfl_t *)mess = nfl;
    memcpy(mess + TE_LOG_NFL_SZ, LGR_SHUTDOWN, nfl);

    log_client = ipc_init_client("LOGGER_SHUTDOWN_CLIENT");
    if (log_client == NULL)
    {
        fprintf(stderr, "ipc_init_client() failed\n");
        return EXIT_FAILURE;
    }
    if (ipc_send_message(log_client, LGR_SRV_NAME, mess, sizeof(mess)) != 0)
    {
        fprintf(stderr, "ipc_send_message() failed\n");
        result = EXIT_FAILURE;
    }
    if (ipc_close_client(log_client) != 0)
    {
        fprintf(stderr, "ipc_close_client() failed\n");
        result = EXIT_FAILURE;
    }

    return result;
}
