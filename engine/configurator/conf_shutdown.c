/** @file
 * @brief Configurator
 *
 * Shutdown the Configurator
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
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#include "config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "ipc_client.h"
#include "logger_api.h"

#include "conf_messages.h"


DEFINE_LGR_ENTITY("Configurator Shutdown");


/**
 * SIGINT handler.
 *
 * @param sig   - signal number
 */
static void
sigint_handler(int sig)
{
    UNUSED(sig);
    /* FIXME: Error possible here, if main was interrupted during logging */
    RING("Configurator shut down operation interrupted");
    exit(EXIT_FAILURE);
}


/**
 * Send SHUTDOWN message to Configurator.
 *
 * @retval EXIT_SUCCESS - success
 * @retval EXIT_FAILURE - failure
 */
int
main(void)
{
    const char         *name = "confshutdown_client";
    struct ipc_client  *ipcc = NULL;
    cfg_shutdown_msg    msg = { CFG_SHUTDOWN, sizeof(msg), 0 };
    size_t              anslen = sizeof(msg);
    int                 result = EXIT_SUCCESS;
    int                 rc;

    signal(SIGINT, sigint_handler);

    if ((rc = ipc_init_client(name, &ipcc)) != 0)
    {
        ERROR("Failed to initialize IPC client '%s': 0x%X", name, rc);
        return EXIT_FAILURE;
    }

    rc = ipc_send_message_with_answer(ipcc, CONFIGURATOR_SERVER,
                                      (char *)&msg, sizeof(msg),
                                      (char *)&msg, &anslen);
    if (rc != 0)
    {
        ERROR("Failed to send IPC message with answer to %s: 0x%X",
              CONFIGURATOR_SERVER, rc);
        result = EXIT_FAILURE;
    }

    if ((rc = ipc_close_client(ipcc)) != 0)
    {
        ERROR("Failed to close IPC client: 0x%X", rc);
        result = EXIT_FAILURE;
    }

    return result;
}
