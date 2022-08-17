/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Logger subsystem API - TEN side
 *
 * Common function used to format raw log message from Logger TEN
 * library and Logger process internals.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef TE_LOGGER_TEN_INTERNALS
#error This file may be included from Logger TEN internals only
#endif

#ifndef __TE_LOGGER_TEN_INT_H__
#define __TE_LOGGER_TEN_INT_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif

#include "te_defs.h"
#include "logger_defs.h"
#include "te_log_fmt.h"


#ifdef _cplusplus
extern "C" {
#endif

/** See description in tapi_test.h */
unsigned int te_test_id = TE_LOG_ID_UNDEFINED;


/** Log message transport */
typedef void (* te_log_message_tx_f)(const void *msg, size_t len);

/** Transport to log messages */
static te_log_message_tx_f te_log_message_tx = NULL;


/* See description below */
static void log_message_va(te_log_msg_raw_data *out,
                           const char *file, unsigned int line,
                           te_log_ts_sec sec, te_log_ts_usec usec,
                           unsigned int level,
                           const char *entity, const char *user,
                           const char *fmt, va_list ap);

/**
 * Internal function for logging. It has the same prototype as
 * log_message().
 *
 * @attention It calls log_message_va(), therefore it may be called
 *            from log_message_va() just before exit.
 */
static void
log_message_int(te_log_msg_raw_data *out,
                const char *file, unsigned int line,
                unsigned int level,
                const char *entity, const char *user,
                const char *fmt, ...)
{
    va_list         ap;
    struct timeval  tv;

    (void)gettimeofday(&tv, NULL);

    va_start(ap, fmt);
    log_message_va(out, file, line, tv.tv_sec, tv.tv_usec,
                   level, entity, user, fmt, ap);
    va_end(ap);
}

/**
 * Create log message and send to Logger server.
 *
 * @param out       Output interface parameters
 * @param ap        Arguments passed into the function according to
 *                  raw log format string description
 *
 * The rest of parameters complies with te_log_message_f prototype.
 */
static void
log_message_va(te_log_msg_raw_data *out,
               const char *file, unsigned int line,
               te_log_ts_sec sec, te_log_ts_usec usec,
               unsigned int level,
               const char *entity, const char *user,
               const char *fmt, va_list ap)
{
    te_errno rc;

    assert(te_log_message_tx != NULL);

    rc = te_log_message_raw_va(out, sec, usec, level,
                               te_test_id, entity, user, fmt, ap);
    if (rc != 0)
    {
        log_message_int(out, __FILE__, __LINE__,
                        TE_LL_ERROR, te_lgr_entity, TE_LGR_USER,
                        "Cannot print message from %s:%s logged at %s:%u: %r",
                        entity, user, file, line, rc);
        return;
    }

    te_log_message_tx(out->buf, out->ptr - out->buf);

    if (out->trunc)
    {
        log_message_int(out, __FILE__, __LINE__,
                        TE_LL_WARN, te_lgr_entity, TE_LGR_USER,
                        "Previous message from %s:%s logged at %s:%u "
                        "was truncated", entity, user, file, line);
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TEN_INT_H__ */
