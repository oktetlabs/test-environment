/** @file
 * @brief TE log format string processing
 *
 * Some TE-specific features, such as memory dump, file content logging,
 * and additional length modifiers are supported.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Ivan Soloducha <Ivan.Soloducha@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef  __TE_LOG_FMT_H__
#define  __TE_LOG_FMT_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_raw_log.h"


/** Types of TE log message arguments */
typedef enum te_log_msg_arg_type {
    TE_LOG_MSG_FMT_ARG_EOR,     /**< End-of-record */
    TE_LOG_MSG_FMT_ARG_INT,     /**< Integer in network bytes order */
    TE_LOG_MSG_FMT_ARG_MEM,     /**< Memory dump or string */
    TE_LOG_MSG_FMT_ARG_FILE,    /**< File content */
} te_log_msg_arg_type;


/* Forward declarations */
struct te_log_msg_out;
typedef struct te_log_msg_out te_log_msg_out;


/**
 * Callback to process possibly few format string arguments in
 * vprintf()-like mode.
 *
 * @param out       Backend parameters
 * @param fmt       Part of format string, terminated by NUL, which 
 *                  corresponds to arguments pointed by @a ap
 * @param ap        Arguments for format string
 *
 * @return FIXME
 */
typedef te_errno (*te_log_msg_fmt_args_f)(te_log_msg_out *out,
                                          const char     *fmt,
                                          va_list         ap);

/**
 * Callback to process one format string argument in raw mode.
 *
 * @param out       Backend parameters
 * @param fmt       Part of format string which corresponds to the
 *                  argument (possibly not terminated by NUL) or @c NULL
 * @param fmt_len   Length of the @a fmt (zero, if @a fmt is @c NULL)
 * @param arg_type  Type of the argument
 * @param arg_addr  Address of the argument
 * @param arg_len   Argument length
 *
 * @return FIXME
 */
typedef te_errno (*te_log_msg_raw_arg_f)(te_log_msg_out      *out,
                                         const char          *fmt,
                                         size_t               fmt_len,
                                         te_log_msg_arg_type  arg_type,
                                         const void          *arg_addr,
                                         size_t               arg_len);


/** Parameters common for all format string processing backends. */
struct te_log_msg_out {
    te_log_msg_fmt_args_f   fmt;    /**< Callback to process format string
                                         with arguments in vprintf()-like
                                         mode */
    te_log_msg_raw_arg_f    raw;    /**< Callcack to process format string
                                         with one argument in raw mode */
};

/** Raw log version of backend common parameters */
extern const struct te_log_msg_out te_log_msg_out_raw;

/** Log argument descriptor */
typedef struct te_log_arg_descr {
    te_log_msg_arg_type     type;   /**< Type of the argument */
    size_t                  len;    /**< Data length */
    union {
        const void         *a;      /**< Pointer argument */
        uint64_t            i;      /**< Integer argument */
    } u;
} te_log_arg_descr;

/** Raw logging backend parameters */
typedef struct te_log_msg_raw_data {
    struct te_log_msg_out   common; /**< Parameters common for all
                                         backends. Must be the first
                                         field in the scructure. */

    /* Parametes specific for this type of backend */

    uint8_t    *buf;    /**< Buffer allocated for the message */
    uint8_t    *end;    /**< End of the buffer @a buf */
    uint8_t    *ptr;    /**< Current pointer in the buffer @a buf */

    unsigned int        args_max;   /**< Maximum number of raw arguments */
    te_log_arg_descr   *args;       /**< Array of raw argument */
    unsigned int        args_n;     /**< Number of raw arguments */
    size_t              args_len;   /**< Total length required in raw log
                                         to store raw arguments */

    te_bool     trunc;  /**< Is log message truncated? */

} te_log_msg_raw_data;


/**
 *
 *
 * @param data     Output parameters
 * @param ts_sec   Timestamp seconds
 * @param ts_usec  Timestamp microseconds
 * @param level    Log levelt
 * @param test_id  Test ID or TE_TEST_ID_INVALID
 * @param entity   Entity name
 * @param user     User name
 * @param fmt      Format string
 * @param ap       Arguments for the format string
 *
 * @return Error code (see te_errno.h)
 */
extern te_errno te_log_message_raw_va(te_log_msg_raw_data *data,
                                      te_log_ts_sec        ts_sec,
                                      te_log_ts_usec       ts_usec,
                                      te_log_level         level,
                                      te_log_test_id       test_id,
                                      const char          *entity,
                                      const char          *user,
                                      const char          *fmt,
                                      va_list              ap);

#endif /* !__TE_LOG_FMT_H__ */
