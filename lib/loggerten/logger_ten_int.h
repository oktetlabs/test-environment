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

#include "te_config.h"

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

#include "te_stdint.h"
#include "te_raw_log.h"
#include "logger_int.h"


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
static inline void log_message_va(uint8_t **msg_buf, size_t *msg_buf_len,
                                  uint16_t level, const char *entity_name,
                                  const char *user_name,
                                  const char *form_str, va_list ap);


/**
 * Internal function for logging. It has the same prototype as
 * log_message().
 *
 * @attention It calls log_message_va(), therefore it may be called
 *            from log_message_va() just before exit.
 */
static inline void
log_message_int(uint8_t **msg_buf, size_t *msg_buf_len, uint16_t level,
                const char *entity_name, const char *user_name,
                const char *form_str, ...)
{
    va_list ap;

    va_start(ap, form_str);
    log_message_va(msg_buf, msg_buf_len, level, entity_name, user_name,
                   form_str, ap);
    va_end(ap);
}

/**
 * Create log message and send to Logger server.
 *
 * @param msg_buf       Location of the pointer to buffer for the message
 * @param msg_buf_len   Length of the buffer for the message
 * @param level         Log level valued to be passed to raw log file
 * @param entity_name   Entity name whose user generates this message
 * @param user_name     Arbitrary "user name"
 * @param form_str      Raw log format string. This string should contain
 *                      conversion specifiers if some arguments following
 * @param  ap           Arguments passed into the function according to
 *                      raw log format string description.
 */
static inline void
log_message_va(uint8_t **msg_buf, size_t *msg_buf_len, uint16_t level,
               const char *entity_name, const char *user_name,
               const char *form_str, va_list ap)
{
    static const char * const skip_flags = "#-+ 0";
    static const char * const skip_width = "*0123456789";

    uint8_t            *msg_ptr = *msg_buf;
    uint8_t            *msg_end = msg_ptr + *msg_buf_len;
    size_t              msg_prefix_len = LGR_UNACCOUNTED_LEN;
    uint8_t            *msg_len_ptr = NULL;
    te_log_msg_len_t    msg_len;
    te_bool             msg_trunc = FALSE;
    size_t              tmp_length;
    struct timeval      tv;
    const char         *p_fs;
    unsigned int        narg = 0;


    assert(msg_buf != NULL);
    assert(*msg_buf != NULL);

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
            uint8_t *new_buf = malloc(*msg_buf_len << 1);           \
            size_t   data_len = msg_ptr - *msg_buf;                 \
                                                                    \
            if (new_buf == NULL)                                    \
            {                                                       \
                log_message_int(msg_buf, msg_buf_len, TE_LL_ERROR,  \
                                te_lgr_entity, TE_LGR_USER,         \
                                "malloc(%u) failed",                \
                                *msg_buf_len << 1);                 \
                return;                                             \
            }                                                       \
            memcpy(new_buf, *msg_buf, data_len);                    \
            if (msg_len_ptr != NULL)                                \
                msg_len_ptr = new_buf + (msg_len_ptr - *msg_buf);   \
            free(*msg_buf);                                         \
            *msg_buf = new_buf;                                     \
            (*msg_buf_len) <<= 1;                                   \
            msg_ptr = new_buf + data_len;                           \
            msg_end = new_buf + *msg_buf_len;                       \
        }                                                           \
    } while (0)

/**
 * This macro fills in NFL(Next_Field_Length) field and corresponding
 * string field as following:
 *     NFL - actual next string length without trailing null;
 *     Argument field (1 .. 255 bytes) - logged string.
 *
 * @param _log_str      logged string location
 * @param _len          (OUT) string length
 *
 * @note If logged string pointer equal NULL, the "(NULL)" string
 *       is written in the raw log file.
 */
#define LGR_PUT_STR(_log_str, _len) \
    do {                                            \
        const char *tmp_str = "(NULL)";             \
                                                    \
        if ((_log_str) != NULL)                     \
            tmp_str = (_log_str);                   \
        (_len) = strlen(tmp_str);                   \
        if ((_len) > TE_LOG_FIELD_MAX)              \
        {                                           \
            msg_trunc = TRUE;                       \
            (_len) = TE_LOG_FIELD_MAX;              \
        }                                           \
                                                    \
        LGR_CHECK_BUF_LEN(TE_LOG_NFL_SZ + (_len));  \
        LGR_NFL_PUT((_len), msg_ptr);               \
        memcpy(msg_ptr, tmp_str, (_len));           \
        msg_ptr += (_len);                          \
     } while (0);

    /* Fill in Entity_name field and corresponding Next_filed_length one */
    LGR_PUT_STR(entity_name, tmp_length);
    msg_prefix_len += TE_LOG_NFL_SZ + tmp_length;

    
    LGR_CHECK_BUF_LEN(LGR_UNACCOUNTED_LEN);

    /* Fill in Log_ver and timestamp fields */
    *msg_ptr = TE_LOG_VERSION;
    ++msg_ptr;

    gettimeofday(&tv, NULL);
    LGR_TIMESTAMP_PUT(&tv, msg_ptr);

    /* Fill in Log level to be passed in raw log */
    LGR_LEVEL_PUT(level, msg_ptr);

    /* Fix Log_message_length field */
    msg_len_ptr = msg_ptr;
    msg_ptr += TE_LOG_MSG_LEN_SZ;


    /* Fill in User_name field and corresponding Next_filed_length one */
    LGR_PUT_STR(user_name, tmp_length);

    /*
     * Fill in Log_data_format_string field and corresponding
     * Next_filed_length one
     */
    LGR_PUT_STR(form_str, tmp_length);

    for (p_fs = form_str; *p_fs; p_fs++)
    {
        if (*p_fs != '%')
            continue;

        if (*++p_fs == '%')
            continue;

        /* skip the flags field  */
        for (; index(skip_flags, *p_fs); ++p_fs);

        /* skip to possible '.', get following precision */
        for (; index(skip_width, *p_fs); ++p_fs);
        if (*p_fs == '.')
            ++p_fs;

        /* skip to conversion char */
        for (; index(skip_width, *p_fs); ++p_fs);


        if ((++narg) > LGR_MAX_ARGS)
        {
            log_message_int(msg_buf, msg_buf_len,
                            TE_LL_ERROR, te_lgr_entity, TE_LGR_USER,
                            "Too many arguments");
            return;
        }

        switch (*p_fs)
        {
            case 'c':
            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
            case 'p':
            case 'r':   /* TE-specific specifier for error codes */
            case 'T':   /* TE-specific specified for Test ID */
            {
                int32_t val;

                LGR_CHECK_BUF_LEN(TE_LOG_NFL_SZ + sizeof(val));
                LGR_NFL_PUT(sizeof(val), msg_ptr);
                val = va_arg(ap, int);
                LGR_32_TO_NET(val, msg_ptr);
                msg_ptr += sizeof(val);
                break;
            }

            case 's':
            {
                char *tmp = va_arg(ap, char *);

                LGR_PUT_STR(tmp, tmp_length);
                break;
            }

            case 't': /* %tm, %tb, %te, %tf */
                if (*++p_fs != 0 )
                {
                    if (*p_fs == 'm')
                    {
                        /*
                         * Args order should be:
                         *  - start address of dumped memory;
                         *  - size of memory piece;
                         */
                        char *dump = (char *)va_arg(ap, char *);

                        tmp_length = (uint16_t)va_arg(ap, int);
                        if (tmp_length > TE_LOG_FIELD_MAX)
                        {
                            msg_trunc = TRUE;
                            tmp_length = TE_LOG_FIELD_MAX;
                        }

                        LGR_CHECK_BUF_LEN(TE_LOG_NFL_SZ + tmp_length);
                        LGR_NFL_PUT(tmp_length, msg_ptr);
                        memcpy(msg_ptr, dump, tmp_length);
                        break;
                    }
                    else if (*p_fs == 'f')
                    {
                        /* FIXME */
                        char new_path[256] = "";
                        char templ[LGR_FILE_MAX] = "tmp_raw_log.XXXXXX";
                        char *tmp = va_arg(ap, char *);
                        int  fd;

                        fd =  mkstemp(templ);
                        if (fd < 0)
                        {
                            log_message_int(msg_buf, msg_buf_len,
                                            TE_LL_ERROR,
                                            te_lgr_entity, TE_LGR_USER,
                                            "mkstemp() failure");
                            return;
                        }

                        if (close(fd) != 0)
                        {
                            log_message_int(msg_buf, msg_buf_len,
                                            TE_LL_ERROR,
                                            te_lgr_entity, TE_LGR_USER,
                                            "close() failure");
                            return;
                        }

                        strcat(new_path, te_log_dir);
                        strcat(new_path, "/");
                        strcat(new_path, templ);
                        if (rename(tmp, new_path) != 0)
                        {
                            log_message_int(msg_buf, msg_buf_len,
                                            TE_LL_ERROR,
                                            te_lgr_entity, TE_LGR_USER,
                                            "File %s copying error %X",
                                            tmp, errno);
                            return;
                        }

                        LGR_PUT_STR(new_path, tmp_length);
                        break;
                    }
                }

                log_message_int(msg_buf, msg_buf_len,
                                TE_LL_ERROR, te_lgr_entity, TE_LGR_USER,
                                "Illegal specifier - t%c", *p_fs);
                return;

            default:
                log_message_int(msg_buf, msg_buf_len,
                                TE_LL_ERROR, te_lgr_entity, TE_LGR_USER,
                               "Illegal specifier - %c", *p_fs);
                return;
        }
    }
#undef LGR_PUT_STR

    msg_len = (msg_ptr - *msg_buf) - msg_prefix_len;
    LGR_MSG_LEN_PUT(msg_len, msg_len_ptr);

    assert(te_log_message_tx != NULL);
    te_log_message_tx(*msg_buf, msg_prefix_len + msg_len);

    if (msg_trunc)
    {
        log_message_int(msg_buf, msg_buf_len,
                        TE_LL_WARN, te_lgr_entity, TE_LGR_USER,
                        "Previous message from %s:%s was truncated",
                        entity_name, user_name);
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TEN_INT_H__ */
