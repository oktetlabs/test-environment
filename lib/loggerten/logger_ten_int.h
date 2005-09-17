/** @file
 * @brief Logger subsystem API - TEN side 
 *
 * Common function used to format raw log message from Logger TEN
 * library and Logger process internals.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_LOGGER_TEN_INT_H__
#define __TE_LOGGER_TEN_INT_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_stdint.h"
#include "te_printf.h"
#include "te_raw_log.h"
#include "logger_int.h"
#include "te_tools.h"

/*   Raw log file format. Max length of the raw log message
 *   is a sum of:
 *      NFL(Entity_name) - 1 byte
 *      Entity_name - 255 bytes max
 *      Log_ver - 1 byte
 *      Timestamp - 8 byte
 *      Level - 2 bytes
 *      Log_message_length - 2 bytes
 *      NFL(User_name) - 1 byte
 *      User_name - 255 bytes max
 *      NFL(of form_str) - 1 byte
 *      Log_data_format_str - 255 bytes max
 *      NFL(arg1) - 1 byte
 *      arg1 - 255 bytes max
 *         ...
 *      NFL(arg12) - 1 byte
 *      arg12 - 255 bytes max
 *
 *      TOTAL = 3853 bytes
 */
    

#ifdef _cplusplus
extern "C" {
#endif

/** Maximum logger message arguments */
#define LGR_MAX_ARGS        16

/** Template for temporary file name */
#define LGR_TMP_FILE_TMPL   "tmp_raw_log.XXXXXX"


/** Log message transport */
typedef void (* te_log_message_tx_f)(const void *msg, size_t len);

/**
 * Buffer for log message.
 *
 * @note It should be used under lgr_lock only.
 */

/** Directory with TE logs */
extern const char *te_log_dir;
/** Current log message transport */
extern te_log_message_tx_f te_log_message_tx;


/* See description below */
static inline void log_message_va(struct te_log_out_params *out,
                                  uint16_t level, const char *entity_name,
                                  const char *user_name,
                                  const char *form_str, va_list ap);

/**
 * Write contents of the file to another file descriptor.
 *
 * @param out       Destination file descriptor
 * @param filename  Name of the source file
 *
 * @retval 0        Success
 * @retval -1       Failure
 */
static inline int
write_file_to_fd(int out, const char *filename)
{
    uint8_t buf[1024];
    int     in = open(filename, O_RDONLY);
    ssize_t r;

    while ((r = read(in, buf, sizeof(buf))) > 0)
    {
        if (write(out, buf, r) != r)
        {
            errno = EIO;
            return -1;
        }
    }
    
    return r;
}

/**
 * Internal function for logging. It has the same prototype as
 * log_message().
 *
 * @attention It calls log_message_va(), therefore it may be called
 *            from log_message_va() just before exit.
 */
static inline void
log_message_int(struct te_log_out_params *out, uint16_t level,
                const char *entity_name, const char *user_name,
                const char *form_str, ...)
{
    va_list ap;

    va_start(ap, form_str);
    log_message_va(out, level, entity_name, user_name, form_str, ap);
    va_end(ap);
}

/**
 * Create log message and send to Logger server.
 *
 * @param out           Output interface parameters
 * @param level         Log level valued to be passed to raw log file
 * @param entity_name   Entity name whose user generates this message
 * @param user_name     Arbitrary "user name"
 * @param form_str      Raw log format string. This string should contain
 *                      conversion specifiers if some arguments following
 * @param  ap           Arguments passed into the function according to
 *                      raw log format string description.
 */
static inline void
log_message_va(struct te_log_out_params *out, uint16_t level,
               const char *entity_name, const char *user_name,
               const char *form_str, va_list ap)
{
    uint8_t            *msg_ptr = out->buf;
    uint8_t            *msg_end = msg_ptr + out->buflen;
    size_t              msg_prefix_len = LGR_UNACCOUNTED_LEN;
    size_t              msg_len_offset = 0;
    size_t              data_len_offset = 0;
    te_log_msg_len_t    msg_len;
    te_bool             msg_trunc = FALSE;
    size_t              tmp_length;
    struct timeval      tv;

    if (te_log_dir == NULL)
    {
         /* Get environment variable value for temporary TE location. */
         te_log_dir = getenv("TE_LOG_DIR");
         if (te_log_dir == NULL)
         {
             fprintf(stderr, "TE_LOG_DIR is not defined\n");
             return;
         }
    }
#define LGR_CHECK_BUF_LEN(_len) \
    do {                                                            \
        if (msg_ptr + (_len) > msg_end)                             \
        {                                                           \
            size_t   data_len = msg_ptr - (uint8_t *)out->buf;      \
                                                                    \
            out->buflen <<= 1;                                      \
            out->buf = realloc(out->buf, out->buflen);              \
            if (out->buf == NULL)                                   \
            {                                                       \
                fprintf(stderr, "%s(): realloc(%" TE_PRINTF_SIZE_T  \
                        "u) failed", __FUNCTION__, out->buflen);    \
                return;                                             \
            }                                                       \
            msg_ptr = out->buf + data_len;                          \
            msg_end = out->buf + out->buflen;                       \
        }                                                           \
    } while (0)

/**
 * This macro fills in NFL(Next_Field_Length) field and corresponding
 * string field as following:
 *     NFL - actual next string length without trailing null;
 *     Argument field (1 .. 255 bytes) - logged string.
 *
 * @param _log_str      logged string location
 * @param _len          string length (OUT)
 *
 * @note If logged string pointer equal NULL, the "(NULL)" string
 *       is written in the raw log file.
 */
#define LGR_PUT_STR(_log_str, _len) \
    do {                                                        \
        const char *tmp_str;                                    \
                                                                \
        tmp_str = ((_log_str) != NULL) ? (_log_str) : "(NULL)"; \
        (_len) = strlen(tmp_str);                               \
        if ((_len) > TE_LOG_FIELD_MAX)                          \
        {                                                       \
            msg_trunc = TRUE;                                   \
            (_len) = TE_LOG_FIELD_MAX;                          \
        }                                                       \
                                                                \
        LGR_CHECK_BUF_LEN(TE_LOG_NFL_SZ + (_len));              \
        LGR_NFL_PUT((_len), msg_ptr);                           \
        memcpy(msg_ptr, tmp_str, (_len));                       \
        msg_ptr += (_len);                                      \
     } while (0);


    /* Fill in Entity_name field and corresponding Next_filed_length one */
    LGR_PUT_STR(entity_name, tmp_length);
    msg_prefix_len += TE_LOG_NFL_SZ + tmp_length;

    
    LGR_CHECK_BUF_LEN(LGR_UNACCOUNTED_LEN);

    /* Fill in Log_ver and timestamp fields */
    *msg_ptr = TE_LOG_VERSION;
    msg_ptr++;

    gettimeofday(&tv, NULL);
    LGR_TIMESTAMP_PUT(&tv, msg_ptr);

    /* Fill in Log level to be passed in raw log */
    LGR_LEVEL_PUT(level, msg_ptr);

    /* Fix Log_message_length field */
    msg_len_offset = msg_ptr - (uint8_t *)out->buf;
    msg_ptr += TE_LOG_MSG_LEN_SZ;


    /* Fill in User_name field and corresponding Next_filed_length one */
    LGR_PUT_STR(user_name, tmp_length);

    /*
     * Fill in Log_data_format_string field and corresponding
     * Next_filed_length one
     */
    
    /* Fix Log_message_data/format_string field */
    data_len_offset = msg_ptr - (uint8_t *)out->buf;
    msg_ptr += TE_LOG_NFL_SZ;
   
    out->offset = msg_ptr - (uint8_t *)out->buf;
    te_log_vprintf(out, form_str, ap);
    
#undef LGR_PUT_STR    
#undef LGR_CHECK_BUF_LEN

    msg_ptr = out->buf + data_len_offset;
    LGR_NFL_PUT(out->offset - data_len_offset - TE_LOG_NFL_SZ, msg_ptr);

    msg_len = out->offset - msg_prefix_len;
    LGR_MSG_LEN_PUT(msg_len, out->buf + msg_len_offset);

    assert(te_log_message_tx != NULL);
    te_log_message_tx(out->buf, out->offset);

    if (msg_trunc)
    {
        log_message_int(out, TE_LL_WARN, te_lgr_entity, TE_LGR_USER,
                        "Previous message from %s:%s was truncated",
                        entity_name, user_name);
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TEN_INT_H__ */
