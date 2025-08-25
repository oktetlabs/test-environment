/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TE project. Logger subsystem.
 *
 * Separate Logger task for shutdowning main Logger process.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "te_stdint.h"
#include "te_raw_log.h"
#include "te_str.h"
#include "logger_int.h"
#include "ipc_client.h"
#include "logger_internal.h"
#include "logger_ten.h"

/* How long to wait for Logger shutdown, in seconds */
#define SHUTDOWN_TIMEOUT 120

static bool logger_finished = false;
static bool signal_received = false;

/** Signal handler function */
static void sig_handler(int sig)
{
    if (sig == SIGUSR1 || sig == SIGALRM)
        signal_received = true;
    if (sig == SIGUSR1)
        logger_finished = true;
}

/**
 * This is an entry point of te_log_shutdown task.
 */
int
main(int argc, const char *argv[])
{
    int                 result = EXIT_SUCCESS;
    int                 len = strlen(LGR_SHUTDOWN);
    te_log_nfl          nfl = len + sizeof(uint32_t);
    te_log_nfl          nfl_net = htons(nfl);
    uint8_t             mess[sizeof(nfl) + nfl];
    struct ipc_client  *log_client = NULL;
    struct sigaction    sigact;
    te_errno            rc;
    long int            log_shut_timeout = SHUTDOWN_TIMEOUT;

    /* Process command line arguments */
    if (argc > 2)
    {
        fprintf(stderr, "%s: too many arguments\n", __FILE__);
        return EXIT_FAILURE;
    }
    else if (argc == 2)
    {
        rc = te_strtol_silent(argv[1], 10, &log_shut_timeout);
        if (rc != 0)
        {
            fprintf(stderr, "%s: invalid timeout value: '%s'\n", __FILE__,
                    argv[1]);
            return EXIT_FAILURE;
        }
    }

    /* Set up the signal handler */
    sigact.sa_handler = sig_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGUSR1, &sigact, NULL);
    sigaction(SIGALRM, &sigact, NULL);

    /* Prepare message with entity name LGR_SHUTDOWN */
    memcpy(mess, &nfl_net, sizeof(nfl_net));
    memcpy(mess + sizeof(nfl_net), LGR_SHUTDOWN, len);
    *(uint32_t *)(mess + sizeof(nfl_net) + len) = htonl(getpid());

    rc = ipc_init_client("LOGGER_SHUTDOWN_CLIENT", LOGGER_IPC, &log_client);
    if (rc != 0)
    {
        fprintf(stderr, "ipc_init_client() failed: 0x%X\n", rc);
        return EXIT_FAILURE;
    }
    assert(log_client != NULL);
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

    /* Do not wait for signal if failed to send the shutdown request */
    if (result != EXIT_FAILURE)
    {
        alarm(log_shut_timeout);
        while (!signal_received)
            pause();

        if (!logger_finished)
        {
            fprintf(stderr, "Logger didn't exit in time");
            result = EXIT_FAILURE;
        }
    }

    return result;
}
