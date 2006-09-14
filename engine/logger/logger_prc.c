/** @file
 * @brief Logger subsystem
 * 
 * TEN side Logger library. 
 *
 *  
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h> 
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "logger_api.h"

#define TE_LOGGER_TEN_INTERNALS
#include "logger_ten_int.h"

#include "logger_internal.h"


/** Mutual exclusion execution lock */
static pthread_mutex_t  lgr_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Logging output interface.
 *
 * @note It should be used under lgr_lock only.
 */
static te_log_msg_raw_data lgr_out;


/** 
 * Create message and register it in the raw log file.
 *
 * This function complies with te_log_message_f prototype.
 */
static void
lgr_log_message(const char *file, unsigned int line,
                te_log_ts_sec sec, te_log_ts_usec usec,
                unsigned int level, const char *entity, 
                const char *user, const char *fmt, va_list ap)
{
    pthread_mutex_lock(&lgr_lock);

    if (te_log_message_tx == NULL)
    {
        te_log_message_tx = lgr_register_message;

        /* Initialize backend */
        lgr_out.common = te_log_msg_out_raw;
        lgr_out.buf = lgr_out.end = NULL;
        lgr_out.args_max = 0;
        lgr_out.args = NULL;
    }

    log_message_va(&lgr_out, file, line, sec, usec, level, entity, user,
                   fmt, ap);

    pthread_mutex_unlock(&lgr_lock);
}

/** Logging backend */
te_log_message_f te_log_message_va = lgr_log_message;
