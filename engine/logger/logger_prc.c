/** @file
 * @brief Logger subsystem
 * 
 * TEN side Logger library. 
 *
 *  
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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


/** Initial size of the log message buffer */
#define LGR_PRC_MSG_BUF_INIT    0x100

#ifdef HAVE_PTHREAD_H
/** Mutual exclusion execution lock */
static pthread_mutex_t  lgr_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/**
 * Logging output interface.
 *
 * @note It should be used under lgr_lock only.
 */
static struct te_log_out_params lgr_out;


/** 
 * Create message and register it in the raw log file.
 *
 * This function complies with te_log_message_f prototype.
 */
static void
lgr_log_message(const char *file, unsigned int line,
                unsigned int level, const char *entity, 
                const char *user, const char *fmt, ...)
{
    va_list ap;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&lgr_lock);
#endif
    
    if (lgr_out.buf == NULL)
    {
        lgr_out.buf = malloc(LGR_PRC_MSG_BUF_INIT);
        if (lgr_out.buf == NULL)
        {
            perror("malloc() failed");
            return;
        }
        lgr_out.buflen = LGR_PRC_MSG_BUF_INIT;
        te_log_message_tx = lgr_register_message;
    }

    va_start(ap, fmt);
    log_message_va(&lgr_out, file, line, level, entity, user, fmt, ap);
    va_end(ap);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&lgr_lock);
#endif
}

/** Logging backend */
te_log_message_f te_log_message = lgr_log_message;
