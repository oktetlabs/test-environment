/** @file
 * @brief Logger subsystem API - TA side
 *
 * TA side Logger functionality
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

#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_printf.h"
#include "logger_defs.h"
#include "logger_api.h"
#include "logger_int.h"
#include "rcf_pch_mem.h"
#include "logger_ta_internal.h"
#include "logger_ta.h"


/** Local log buffer instance */
struct lgr_rb log_buffer;

/**
 * Each message to be registered in the local log buffer
 * increases this variable by 1.
 */
uint32_t log_sequence = 0;


#if HAVE_PTHREAD_H
pthread_mutex_t ta_log_mutex;
#elif HAVE_SEMAPHORE_H
sem_t           ta_log_sem;
#endif

/** Logging backend */
te_log_message_f te_log_message = logfork_log_message;


/**
 * Register message in the raw log (slow mode).
 *
 * This function complies with te_log_message_f prototype.
 */
static void
ta_log_message(const char *file, unsigned int line,
               unsigned int level, const char *entity, 
               const char *user, const char *fmt, ...)
{
    va_list             ap;
    ta_log_lock_key     key;
    uint32_t            position;
    int                 res;
    const char         *p_str;
    md_list             cp_list = {&cp_list, &cp_list, 0, NULL, 0};
    md_list            *tmp_list = NULL;
    uint32_t            narg = 0;

    lgr_mess_header header;
    lgr_mess_header *hdr_addr = NULL;
    
    static char *null_str = "(NULL)";
    static char *skip_flags, *skip_width;
    
    UNUSED(file);
    UNUSED(line);
    UNUSED(entity);
    
    va_start(ap, fmt);
    
    skip_flags = "#-+ 0";
    skip_width = "*0123456789";

    memset(&header, 0, sizeof(struct lgr_mess_header));
    header.level = level;
    header.user = (user != NULL) ? user : null_str;
    header.fmt = (fmt != NULL) ? fmt : null_str;
    for (p_str = fmt; *p_str; p_str++)
    {
        if (*p_str != '%')
            continue;

        if (*++p_str == '%')
            continue;

        /* skip the flags field  */
        for (; index(skip_flags, *p_str); ++p_str);

        /* skip to possible '.', get following precision */
        for (; index(skip_width, *p_str); ++p_str);
        if (*p_str == '.')
            ++p_str;

        /* skip to conversion char */
        for (; index(skip_width, *p_str); ++p_str);

        switch (*p_str)
        {
            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
            case 'c':
            case 'r':   /* TE-specific specifier for error codes */
            {
                int tmp = va_arg(ap, int);

                LGR_SET_ARG(header, narg, tmp);
                break;
            }

            case 'p':
            {
                int tmp = rcf_pch_mem_alloc(va_arg(ap, void *));
                
                LGR_SET_ARG(header, narg, tmp);
                break;
            }

            case 's':
            {
                size_t  length;
                char   *addr = va_arg(ap, char *);
                
                if (addr == NULL)
                    addr = null_str;
                length = strlen(addr) + 1;

                cp_list.length += length;
                LGR_PUT_MD_LIST(cp_list, narg, addr, length);
                break;
            }

            case 't':
                if ((*++p_str != 0) && (*p_str == 'm'))
                {
                    uint8_t    *addr;
                    size_t      length;

                    addr = va_arg(ap, uint8_t *);
                    length = va_arg(ap, size_t);

                    cp_list.length += length;
                    LGR_PUT_MD_LIST(cp_list, narg, addr, length);
                    if ((++narg) > LGR_MAX_ARGS)
                        goto resume;
                    LGR_SET_ARG(header, narg, length);
                }
                break;

            default:
                break;
        }

        if ((++narg) > LGR_MAX_ARGS)
            goto resume;
    }

    if (ta_log_lock(&key) != 0)
        return;

    res = lgr_rb_allocate_head(&log_buffer, LGR_RB_FORCE, &position);
    if (res == 0)
    {
        (void)ta_log_unlock(&key);
        goto resume;
    }
    hdr_addr = (struct lgr_mess_header *)(log_buffer.rb) + position;

    header.elements = hdr_addr->elements;
    header.sequence = hdr_addr->sequence;
    header.mark = hdr_addr->mark;

    ta_log_timestamp(&header.sec, &header.usec);

    *hdr_addr = header;


    tmp_list = cp_list.next;
    while (tmp_list != &cp_list)
    {
        uint32_t  val;
        uint8_t  *arg_addr;
        uint32_t *arg_location = (&hdr_addr->arg1) + tmp_list->narg;

        res = lgr_rb_allocate_and_copy(&log_buffer, tmp_list->addr,
                                       tmp_list->length, &arg_addr);
        if (res == 0)
        {
            (void)ta_log_unlock(&key);
            goto resume;
        }

        *arg_location = rcf_pch_mem_alloc(arg_addr);
        val = LGR_GET_ELEMENTS_FIELD(&log_buffer, position);
        val += res;
        LGR_SET_ELEMENTS_FIELD(&log_buffer, position, val);
        tmp_list = tmp_list->next;
    };
    (void)ta_log_unlock(&key);

resume:
    LGR_FREE_MD_LIST(cp_list);
    va_end(ap);
}


/**
 * Convert NFL from host to net order.
 */
static inline te_log_nfl
log_nfl_hton(te_log_nfl val)
{
#if (SIZEOF_TE_LOG_NFL == 1)
    return val;
#elif (SIZEOF_TE_LOG_LEVEL == 2)
    return htons(val);
#elif (SIZEOF_TE_LOG_LEVEL == 4)
    return htonl(val);
#else
#error Such SIZEOF_TE_LOG_NFL is not supported
#endif
}

/**
 * Get message from log buffer.
 * On success the processed message will be removed from log buffer.
 *
 * @return  Length of processed message.
 */
static uint32_t
log_get_message(uint32_t length, uint8_t *buffer)
{
    uint32_t            argn = 0;
    const char         *fs;
    uint32_t            mess_length = 0;
    uint32_t            tmp_length;
    ta_log_lock_key     key;
    uint8_t            *tmp_buf;
    static char        *skip_flags, *skip_width;
    static int          n_calls = 0;
    lgr_mess_header     header;
    uint8_t            *ring_last = log_buffer.rb + LGR_TOTAL_RB_BYTES;

    n_calls++;

    skip_flags = "#-+ 0";
    skip_width = "*0123456789";

    if (length < LGR_RB_ELEMENT_LEN)
        return 0;

    if (ta_log_lock(&key) != 0)
        return 0;

    if (LGR_RB_UNUSED(&log_buffer) == LGR_TOTAL_RB_EL)
    {
        (void)ta_log_unlock(&key);
        return 0;
    }

    LGR_SET_MARK_FIELD(&log_buffer, log_buffer.head, 1);
    if (ta_log_unlock(&key) != 0)
    {
        LGR_SET_MARK_FIELD(&log_buffer, log_buffer.head, 0);
        return 0;
    }

    tmp_buf = buffer;

    lgr_rb_get_elements(&log_buffer, LGR_RB_HEAD(&log_buffer),
                        1, (uint8_t *)&header);


#define LGR_CHECK_LENGTH(_field_length) \
    do {                                                            \
        if (mess_length + (_field_length) > length)                 \
        {                                                           \
            LGR_SET_MARK_FIELD(&log_buffer, log_buffer.head, 0);    \
            return 0;                                               \
        }                                                           \
        mess_length += (_field_length);                             \
    } while (0)
    
    LGR_CHECK_LENGTH(sizeof(te_log_seqno) + TE_LOG_MSG_COMMON_HDR_SZ);
        
    /* Write message sequence number FIXME */
    *((uint32_t *)tmp_buf) = htonl(header.sequence);
    tmp_buf += sizeof(uint32_t);

    /* Write current log version */
#if (SIZEOF_TE_LOG_VERSION != 1)
#error Such SIZEOF_TE_LOG_VERSION is not supported
#endif
    *tmp_buf = TE_LOG_VERSION;
    tmp_buf++;

    /* Write timestamp */
    *((uint32_t *)tmp_buf) = htonl(header.sec);
    tmp_buf += sizeof(uint32_t);
    *((uint32_t *)tmp_buf) = htonl(header.usec);
    tmp_buf += sizeof(uint32_t);

    /* Write log level */
    *((te_log_level *)tmp_buf) = 
#if (SIZEOF_TE_LOG_LEVEL == 1)
        header.level;
#elif (SIZEOF_TE_LOG_LEVEL == 2)
        htons(header.level);
#elif (SIZEOF_TE_LOG_LEVEL == 4)
        htonl(header.level);
#else
#error Such SIZEOF_TE_LOG_LEVEL is not supported
#endif
    tmp_buf += sizeof(te_log_level);

    /* Write user name and corresponding (NFL) next field length */
    fs = header.user;
    tmp_length = strlen(fs);
    LGR_CHECK_LENGTH(sizeof(te_log_nfl) + tmp_length);
    *((te_log_nfl *)tmp_buf) = log_nfl_hton(tmp_length);
    tmp_buf += sizeof(te_log_nfl);
    memcpy(tmp_buf, fs, tmp_length);
    tmp_buf += tmp_length;

    /* Write format string and corresponding NFL */
    fs = header.fmt;
    tmp_length = strlen(fs);
    LGR_CHECK_LENGTH(sizeof(te_log_nfl) + tmp_length);
    *((te_log_nfl *)tmp_buf) = log_nfl_hton(tmp_length);
    tmp_buf += sizeof(te_log_nfl);
    memcpy(tmp_buf, fs, tmp_length);
    tmp_buf += tmp_length;
    
    /* Parse format string */
    for (; *fs; fs++)
    {
        if (*fs != '%')
            continue;

        if (*++fs == '%') /* double %*/
            continue;

        /* skip the flags field  */
        for (; index(skip_flags, *fs); ++fs);

        /* skip to possible '.', get following precision */
        for (; index(skip_width, *fs); ++fs);
        if (*fs == '.')
            ++fs;

        /* skip to conversion char */
        for (; index(skip_width, *fs); ++fs);
        
        switch (*fs)
        {
            case 'd':
            case 'i':
            case 'o':
            case 'x':
            case 'X':
            case 'u':
            case 'r':   /* TE-specific specifier for error codes */
            {
                int32_t val;

                LGR_CHECK_LENGTH(sizeof(te_log_nfl) + sizeof(uint32_t));
                *((te_log_nfl *)tmp_buf) = log_nfl_hton(sizeof(uint32_t));
                tmp_buf += sizeof(te_log_nfl);
                val = LGR_GET_ARG(header, argn++);
                LGR_32_TO_NET(val, tmp_buf);
                tmp_buf += sizeof(uint32_t);
                break;
            }

            case 'p':
            {
                int         id;
                void       *val;
                uint32_t    tmp;
                
                assert(sizeof(val) == SIZEOF_VOID_P);
                LGR_CHECK_LENGTH(sizeof(te_log_nfl) + sizeof(void *));
                *((te_log_nfl *)tmp_buf) = log_nfl_hton(sizeof(void *));
                tmp_buf += sizeof(te_log_nfl);
                id = LGR_GET_ARG(header, argn++);
                val = rcf_pch_mem_get(id);
                rcf_pch_mem_free(id);
                
#if (SIZEOF_VOID_P == 4)
                tmp = (uint32_t)val;
                LGR_32_TO_NET(tmp, tmp_buf);
                tmp_buf += sizeof(uint32_t);
#elif (SIZEOF_VOID_P == 8)
                tmp = (uint32_t)((uint64_t)val >> 32);
                LGR_32_TO_NET(tmp, tmp_buf);
                tmp_buf += sizeof(uint32_t);

                /* 
                 * At first, cast to the integer of appropriate size,
                 * then reject the most significant bits.
                 */
                tmp = (uint32_t)(uint64_t)val;
                LGR_32_TO_NET(tmp, tmp_buf);
                tmp_buf += sizeof(uint32_t);
#else
#error Such sizeof(void *) is not supported by Logger TEN library.
#endif
                break;
            }

            case 'c':
                LGR_CHECK_LENGTH(sizeof(te_log_nfl) + sizeof(char));
                *((te_log_nfl *)tmp_buf) = log_nfl_hton(sizeof(char));
                tmp_buf += sizeof(te_log_nfl);
                *tmp_buf = (char)LGR_GET_ARG(header, argn++);
                tmp_buf++;
                break;

            case 's':
            {
                te_log_nfl *arglen_location;
                int           id = LGR_GET_ARG(header, argn++);
                char         *arg_str = (char *)rcf_pch_mem_get(id);
                
                rcf_pch_mem_free(id);

                LGR_CHECK_LENGTH(sizeof(te_log_nfl));
                arglen_location = (te_log_nfl *)tmp_buf;
                tmp_buf += sizeof(te_log_nfl);
                *arglen_location = 0;

                if (arg_str == NULL)
                    break;
                tmp_length = 0;

                do {
                    if ((uint8_t *)arg_str > ring_last)
                        arg_str = (char *)log_buffer.rb;

                    LGR_CHECK_LENGTH(1);
                    *tmp_buf = *arg_str;
                    tmp_buf++; arg_str++; tmp_length++;
                } while (*arg_str != '\0');

                *arglen_location = log_nfl_hton(tmp_length);
                break;
            }

            case 'T': /* %Tm - args order: dumped memory address, length */
                if ((*++fs != 0) && (*fs == 'm'))
                {
                    uint8_t *mem_addr;
                    int      id = LGR_GET_ARG(header, argn++);

                    mem_addr = (uint8_t *)rcf_pch_mem_get(id);
                    tmp_length = LGR_GET_ARG(header, argn++);

                    LGR_CHECK_LENGTH(sizeof(te_log_nfl) + tmp_length);

                    *((te_log_nfl *)tmp_buf) = log_nfl_hton(tmp_length);
                    tmp_buf += sizeof(te_log_nfl);

                    if (tmp_length == 0)
                        break;

                    if ((mem_addr + tmp_length) > ring_last)
                    {
                        uint32_t piece1, piece2;

                        piece1 = (uint32_t)(ring_last - mem_addr);
                        piece2 = tmp_length - piece1;
                        memcpy(tmp_buf, mem_addr, piece1);
                        tmp_buf += piece1;
                        memcpy(tmp_buf, log_buffer.rb, piece2);
                        tmp_buf += piece2;
                    }
                    else
                    {
                        memcpy(tmp_buf, mem_addr, tmp_length);
                        tmp_buf += tmp_length;
                    }
                }
                break;

            default:
                break;
        }
    }
    
    LGR_CHECK_LENGTH(sizeof(te_log_nfl));
    *((te_log_nfl *)tmp_buf) = log_nfl_hton(TE_LOG_RAW_EOR_LEN);

#undef LGR_CHECK_LENGTH

    if (ta_log_lock(&key) != 0)
    {
        /* TODO: Is it safe to do it without lock? */
        LGR_SET_MARK_FIELD(&log_buffer, log_buffer.head, 0);
        return 0;
    }
    LGR_SET_MARK_FIELD(&log_buffer, log_buffer.head, 0);
    lgr_rb_remove_oldest(&log_buffer);
    (void)ta_log_unlock(&key);

    return mess_length;
}

/**
 * Function to be called in fork-child.
 */
static void
log_atfork_child(void)
{
    te_log_message = logfork_log_message;
}

/**
 * Initialize Logger resources on the Test Agent side (log buffer,
 * log file and so on).
 *
 * @return  Operation status.
 *
 * @retval  0  Success.
 * @retval -1  Failure.
 */
te_errno
ta_log_init(void)
{
    if (pthread_atfork(NULL, NULL, log_atfork_child) != 0)
        return -1;

    if (ta_log_lock_init() != 0)
        return -1;

    if (lgr_rb_init(&log_buffer) != 0)
        return -1;

    te_log_message = ta_log_message;

    return 0;
}


/**
 * Finish Logger activity on the Test Agent side (fluhes buffers
 * in the file if that means exists and so on).
 *
 * @return  Operation status.
 *
 * @retval  0  Success.
 * @retval -1  Failure.
 */
te_errno
ta_log_shutdown(void)
{
    (void)ta_log_lock_destroy();

    return lgr_rb_destroy(&log_buffer);
}


/**
 * Request the log messages accumulated in the Test Agent local log
 * buffer. Passed messages are deleted from local log.
 *
 * @param  buf_length   Length of the transfer buffer.
 * @param  transfer_buf Pointer to the transfer buffer.
 *
 * @retval  Length of the filled part of the transfer buffer in bytes
 */
uint32_t
ta_log_get(uint32_t buf_length, uint8_t *transfer_buf)
{
    uint32_t log_length = 0;
    uint32_t mess_length, rest_length;
    uint8_t *tmp_buf = transfer_buf;


    if ((buf_length <= 0) || (transfer_buf == NULL))
        goto ret;

    do {
        if (LGR_RB_UNUSED(&log_buffer) == LGR_TOTAL_RB_EL)
            goto ret;

        rest_length = buf_length - log_length;
        if (rest_length == 0)
            goto ret;

        mess_length = log_get_message(rest_length, tmp_buf);
        if (mess_length == 0)
            goto ret;

        tmp_buf += mess_length;
        log_length += mess_length;

    } while (1);

ret:
    return log_length;
}
