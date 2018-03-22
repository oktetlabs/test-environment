/** @file
 * @brief Configurator
 *
 * Shutdown the Configurator
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
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
    char                name[64];
    struct ipc_client  *ipcc = NULL;
    cfg_shutdown_msg    msg = { CFG_SHUTDOWN, sizeof(msg), 0 };
    size_t              anslen = sizeof(msg);
    int                 result = EXIT_SUCCESS;
    int                 rc;

    sprintf(name, "cs_shut_%d", getpid());

    signal(SIGINT, sigint_handler);

    if ((rc = ipc_init_client(name, CONFIGURATOR_IPC, &ipcc)) != 0)
    {
        ERROR("Failed to initialize IPC client '%s': %r", name, rc);
        return EXIT_FAILURE;
    }

    rc = ipc_send_message_with_answer(ipcc, CONFIGURATOR_SERVER,
                                      (char *)&msg, sizeof(msg),
                                      (char *)&msg, &anslen);
    if (rc != 0)
    {
        ERROR("Failed to send IPC message with answer to %s: %r",
              CONFIGURATOR_SERVER, rc);
        result = EXIT_FAILURE;
    }

    if ((rc = ipc_close_client(ipcc)) != 0)
    {
        ERROR("Failed to close IPC client: %r", rc);
        result = EXIT_FAILURE;
    }

    return result;
}
