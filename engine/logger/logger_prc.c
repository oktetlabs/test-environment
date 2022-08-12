/** @file
 * @brief Logger subsystem
 *
 * TEN side Logger library.
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
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


void
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
