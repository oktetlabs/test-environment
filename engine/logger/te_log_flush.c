/** @file
 * @brief Logger subsystem
 *
 * Separate Logger task to flush TA local log.
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "ipc_client.h"

#include "logger_api.h"
#include "logger_ten.h"


#define LGR_TANAMES_LEN 1024


DEFINE_LGR_ENTITY("Log Flush");


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
    char *ta_names = NULL;
    int   names_len;
    int   scale = 0;
    int   res;
    int   str_len = 0;

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
        if (res != ETESMALLBUF)
        {
            ERROR("Can not interact with RCF\n");
            rcf_api_cleanup();
            free(ta_names);
            return 1;
        }
    } while (res == ETESMALLBUF);

    rcf_api_cleanup();

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
