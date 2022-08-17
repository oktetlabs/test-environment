/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Logger subsystem
 *
 * Separate Logger task to flush TA local log.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "ipc_client.h"

#include "logger_api.h"
#include "logger_ten.h"


#define LGR_TANAMES_LEN 1024


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
    RING("Log flush operation interrupted");
    exit(EXIT_FAILURE);
}


/**
 * This is an entry point of te_log_flush task.
 */
int
main(void)
{
    char           *ta_names = NULL;
    size_t          names_len;
    unsigned int    scale = 0;
    size_t          str_len = 0;
    int             res;

    te_log_init("Log Flush", ten_log_message);

    signal(SIGINT, sigint_handler);

    do {
        if (ta_names != NULL)
            free(ta_names);
        ++scale;
        names_len = LGR_TANAMES_LEN * scale;
        ta_names = (char *)calloc(names_len * sizeof(char), 1);
        res = rcf_get_ta_list(ta_names, &names_len);
        if (res == 0)
            break;    /* on success list_len - used space real value */
        if (TE_RC_GET_ERROR(res) != TE_ESMALLBUF)
        {
            ERROR("Can not interact with RCF\n");
            free(ta_names);
            return 1;
        }
    } while (TE_RC_GET_ERROR(res) == TE_ESMALLBUF);

    /* Create single linked list of active TA */
    while (names_len != str_len)
    {
        char *aux_str = ta_names + str_len;

        log_flush_ten(aux_str);
        str_len += strlen(aux_str) + 1;
    }

    free(ta_names);

    return 0;
}
