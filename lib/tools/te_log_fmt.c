/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TE log format string processing
 *
 * Some TE-specific features, such as memory dump, file content logging,
 * and additional length modifiers are supported.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDDEF_H
#include <stddef.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_printf.h"
#include "te_raw_log.h"
#include "logger_defs.h"
#include "logger_int.h"
#include "te_string.h"
#include "te_alloc.h"

#include "te_log_fmt.h"

#if 0
void
dump(const uint8_t *p, const uint8_t *q)
{
    unsigned int len = q - p;
    unsigned int i;

    for (i = 0; i < len; ++i)
    {
        fprintf(stderr, "%02x ", p[i]);
        if ((i & 0xf) == 0xf)
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n\n");
    fflush(stderr);
}

#define DUMP_RAW    dump(data->buf, data->ptr);
#endif


/** Initial length of raw message buffer */
#define TE_LOG_MSG_RAW_BUF_INIT     512
/**
 * Additional length to allocate for raw message buffer, if current
 * length is not sufficient.
 */
#define TE_LOG_MSG_RAW_BUF_GROW     256

/** Initial number of raw arguments the space allocate for */
#define TE_LOG_MSG_RAW_ARGS_INIT    16
/**
 * Number of additional arguments the space allocate for, if current
 * space is not sufficient.
 */
#define TE_LOG_MSG_RAW_ARGS_GROW    4


/**
 * Helper to call out->fmt when arguments should be specified by caller.
 *
 * @param out       Backend parameters
 * @param fmt       Part of format string, terminated by NUL, which
 *                  corresponds to arguments
 * @param ...       Arguments for format string
 */
static te_errno
te_log_msg_fmt(te_log_msg_out *out, const char *fmt, ...)
{
    te_errno    rc;
    va_list     ap;

    va_start(ap, fmt);
    rc = out->fmt(out, fmt, ap);
    va_end(ap);

    return rc;
}

/**
 * Check that raw log buffer has enough space for specified amount
 * of bytes. If no enough space, reallocate to have sufficient.
 *
 * @param data      Current buffer parameters
 * @param len       Required number of bytes
 *
 * @return Status code (see te_errno.h)
 * @retval 0            Enough space
 * @retval TE_EAGAIN    Enough space after reallocation
 * @retval TE_ENOMEM    Reallocation failed
 */
static te_errno
ta_log_msg_raw_buf_check_len(te_log_msg_raw_data *data, size_t len)
{
    size_t  buflen;
    size_t  off;
    void   *tmp;

    assert(data != NULL);
    assert((data->end == NULL) == (data->buf == NULL));
    assert((data->ptr == NULL) == (data->buf == NULL));

#if 0
fprintf(stderr, "CHECK: BUF=%p END=%p PTR=%p diff=%u len=%u\n",
        data->buf, data->end, data->ptr, data->end - data->ptr, len);
#endif

    if (data->end - data->ptr >= (ptrdiff_t)len)
        return 0;

    buflen = data->end - data->buf;
    off = data->ptr - data->buf;

    if (buflen == 0)
    {
        buflen = TE_LOG_MSG_RAW_BUF_INIT;
        if (len >= buflen)
            buflen += ((len - buflen) / TE_LOG_MSG_RAW_BUF_GROW + 1) *
                      TE_LOG_MSG_RAW_BUF_GROW;
    }
    else
    {
        buflen += ((len - (data->end - data->ptr)) /
                   TE_LOG_MSG_RAW_BUF_GROW + 1) *
                  TE_LOG_MSG_RAW_BUF_GROW;
    }

    tmp = realloc(data->buf, buflen);
    if (tmp == NULL)
    {
        free(data->buf);
        data->buf = data->end = data->ptr = NULL;

        return TE_ENOMEM;
    }

    data->buf = (uint8_t *)tmp;
    data->end = data->buf + buflen;
    data->ptr = data->buf + off;

    return TE_EAGAIN;
}

/**
 * Check if there is enough memory for arguments.
 * Also copy format string to the buffer.
 *
 * @param data      Raw log message data.
 * @param fmt       Format string.
 * @param fmt_len   Length of the format string.
 *
 * @return Status code.
 *
 */
static te_errno
te_log_msg_prepare_raw_args(te_log_msg_raw_data *data, const char *fmt, size_t fmt_len)
{
    te_errno rc;
    unsigned int arg_i;

    assert((fmt != NULL) || (fmt_len == 0));
    if (fmt_len > 0)
    {
        /* We have not enough space in format string storage */
        rc = ta_log_msg_raw_buf_check_len(data, fmt_len);
        if ((rc != 0) && (rc != TE_EAGAIN))
            return rc;

        memcpy(data->ptr, fmt, fmt_len);
        data->ptr += fmt_len;
        assert(data->ptr <= data->end);
    }

    assert(data != NULL);
    assert((data->args == NULL) == (data->args_max == 0));

    /* Get the argument index and check that we have enough space */
    arg_i = data->args_n;
    if (arg_i == data->args_max)
    {
        void *tmp;

        if (data->args_max == 0)
            data->args_max = TE_LOG_MSG_RAW_ARGS_INIT;
        else
            data->args_max += TE_LOG_MSG_RAW_ARGS_GROW;

        tmp = realloc(data->args, data->args_max * sizeof(*data->args));
        if (tmp == NULL)
        {
            free(data->args);
            data->args = NULL;

            data->args_len = data->args_n = data->args_max = 0;

            return TE_ENOMEM;
        }
        data->args = (te_log_arg_descr *)tmp;
    }
    /* Now we have to have enough space */
    assert(arg_i < data->args_max);
    return 0;
}

/**
 * Process format string with its arguments in vprintf()-like mode.
 *
 * This functions complies with te_log_msg_fmt_args_f prototype.
 */
static te_errno
te_log_msg_fmt_args(te_log_msg_out *out, const char *fmt, va_list ap)
{
    te_log_msg_raw_data   *data = (te_log_msg_raw_data *)out;

    te_errno rc;
    int      ret;


    assert(data != NULL);
    assert(fmt != NULL);
    assert(data->ptr <= data->end);

    ret = vsnprintf((char *)data->ptr, data->end - data->ptr, fmt, ap);

    /*
     * If buffer is too small, expand it and write rest of
     * the message.
     */
    while ((rc = ta_log_msg_raw_buf_check_len(data, ret + 1))
               == TE_EAGAIN)
    {
        rc = 0;
        ret = vsnprintf((char *)data->ptr, data->end - data->ptr, fmt, ap);
        /*
         * Sometimes this return value does not equal to returned
         * by the previous call with the same format string and its
         * arguments. It seems it contradicts with C99, but...
         */
    }
    if (rc != 0)
        return rc;

    data->ptr += ret;

    assert(data->ptr <= data->end);

    return 0;
}

/**
 * Handle truncated log messages.
 *
 * If data size is too big to fit into a single argument, add more arguments
 * of the same type, so that the data can be split between them.
 *
 * @param data         Raw log message data.
 * @param saved_data   Holds saved data of the truncated message.
 *
 * @return Exit status.
 */
static te_errno
te_log_msg_handle_trunc(te_log_msg_raw_data  *data,
                        te_log_msg_truncated *saved_data)
{
    te_errno              rc;
    unsigned int          arg_i;
    size_t                arg_len;
    te_log_msg_arg_type   arg_type;
    size_t                fmt_len = saved_data->fmt_len;
    const char           *fmt     = saved_data->fmt;
    te_bool               trunc;

    do {
        rc = te_log_msg_prepare_raw_args(data, fmt, fmt_len);
        if (rc != 0)
            return rc;

        trunc    = FALSE;
        arg_i    = data->args_n;
        arg_len  = saved_data->len;
        arg_type = data->args[arg_i - 1].type;
        switch (arg_type)
        {
            case TE_LOG_MSG_FMT_ARG_FILE:
            {
                data->args[arg_i].u.i = saved_data->data.fd;
                break;
            }
            case TE_LOG_MSG_FMT_ARG_MEM:
            {
                data->args[arg_i].u.a = saved_data->data.addr;
                /* Save the rest part of the message. Has no affect if the
                 * message isn't truncated. */
                saved_data->data.addr += TE_LOG_FIELD_MAX;
                break;
            }
            default:
                return TE_EINVAL;
        }

        if (arg_len > TE_LOG_FIELD_MAX)
        {
            trunc = TRUE;
            saved_data->len = arg_len - TE_LOG_FIELD_MAX;
            arg_len = TE_LOG_FIELD_MAX;
        }

        data->args[arg_i].type = arg_type;
        data->args[arg_i].len  = arg_len;

        data->args_len += sizeof(te_log_nfl) + arg_len;

        data->args_n++;
    } while (trunc);
    data->trunc = FALSE;
    return 0;
}

/**
 * Process an argument and format string which corresponds to it
 * in raw mode.
 *
 * This functions complies with te_log_msg_raw_arg_f prototype.
 */
static te_errno
te_log_msg_raw_arg(te_log_msg_out      *out,
                   const char          *fmt,
                   size_t               fmt_len,
                   te_log_msg_arg_type  arg_type,
                   const void          *arg_addr,
                   size_t               arg_len)
{
    te_log_msg_raw_data   *data = (te_log_msg_raw_data *)out;
    te_errno                rc;
    unsigned int            arg_i;
    int                     fd = -1;
    struct stat             stat_buf;
    te_log_msg_truncated    saved_data = {{ 0 }, 0, NULL, 0};


    rc = te_log_msg_prepare_raw_args(data, fmt, fmt_len);
    if (rc != 0)
        return rc;

    arg_i = data->args_n;
    /* Validate requested length */
    switch (arg_type)
    {
        case TE_LOG_MSG_FMT_ARG_EOR:
            break;

        case TE_LOG_MSG_FMT_ARG_INT:
#if 0
fprintf(stderr, "%s(): INT len=%d\n", __FUNCTION__, arg_len);
#endif
            /* Store in host byte order */
            switch (arg_len)
            {
                case sizeof(int8_t):
                    *((int8_t *)&(data->args[arg_i].u.i)) =
                        *(const int8_t *)arg_addr;
                    break;

                case sizeof(int16_t):
                    *((int16_t *)&(data->args[arg_i].u.i)) =
                        *(const int16_t *)arg_addr;
                    break;

                case sizeof(int32_t):
                    *((int32_t *)&(data->args[arg_i].u.i)) =
                        *(const int32_t *)arg_addr;
                    break;

                case sizeof(int64_t):
                    data->args[arg_i].u.i =
                        *(const int64_t *)arg_addr;
                    break;

                default:
                    return TE_EINVAL;
            }
            break;

        case TE_LOG_MSG_FMT_ARG_FILE:
            if (arg_addr == NULL)
            {
                /* Log the following string */
                arg_type = TE_LOG_MSG_FMT_ARG_MEM;
                arg_addr = "(NULL file name)";
                arg_len = strlen(arg_addr);
            }
            else if (((fd = open(arg_addr, O_RDONLY)) < 0) ||
                     (fstat(fd, &stat_buf) != 0))
            {
                /* Log name of the file instead of its contents */
                arg_type = TE_LOG_MSG_FMT_ARG_MEM;
                arg_len = strlen(arg_addr);
            }
            else
            {
                /* Store file descriptor */
                data->args[arg_i].u.i = fd;
                /* Get length of the file */
                arg_len = stat_buf.st_size;
                if (arg_len > TE_LOG_FIELD_MAX)
                {
                    data->trunc = TRUE;
                    /* Save file descriptor of truncated file. */
                    saved_data.data.fd = fd;
                    /* Need to save pointer to modifier string. */
                    saved_data.fmt = rindex(fmt, '%');
                    saved_data.fmt_len = strlen(saved_data.fmt);

                    saved_data.len = arg_len - TE_LOG_FIELD_MAX;
                    arg_len = TE_LOG_FIELD_MAX;
                }
#if 0
fprintf(stderr, "%s(): FILE len=%d\n", __FUNCTION__, arg_len);
#endif
                break;
            }
            /*FALLTHROUGH*/

        case TE_LOG_MSG_FMT_ARG_MEM:
#if 0
fprintf(stderr, "%s(): MEM len=%d\n", __FUNCTION__, arg_len);
#endif
            data->args[arg_i].u.a = arg_addr;
            if (arg_len > TE_LOG_FIELD_MAX)
            {
                data->trunc = TRUE;
                /* Save the rest part of the message. */
                saved_data.data.addr = arg_addr + TE_LOG_FIELD_MAX;
                /* Need to save pointer to modifier string. */
                saved_data.fmt = rindex(fmt, '%');
                saved_data.fmt_len = strlen(saved_data.fmt);

                saved_data.len = arg_len - TE_LOG_FIELD_MAX;
                arg_len = TE_LOG_FIELD_MAX;
            }
            break;

        default:
            return TE_EINVAL;
    }

    data->args[arg_i].type = arg_type;
    data->args[arg_i].len  = arg_len;

    data->args_len += sizeof(te_log_nfl) + arg_len;

    data->args_n++;


    /* Add truncated part of the message as an argument with
     * the modifier of the message.
     */
    if (data->trunc)
    {
        rc = te_log_msg_handle_trunc(data, &saved_data);
    }

    return rc;
}

/** Raw log version of backend common parameters */
const struct te_log_msg_out te_log_msg_out_raw = {
    te_log_msg_fmt_args,
    te_log_msg_raw_arg
};


/**
 * Put specified argument in raw log format without any checks:
 * i.e. too long or inappropriate length.
 *
 * @param data      Raw log message data
 * @param type      Type of argument
 * @param addr      Address with the data
 * @param len       Data length
 * @param use_nfl   Put NFL or not
 */
static void
te_log_msg_raw_put_no_check(te_log_msg_raw_data *data,
                            te_log_msg_arg_type  type,
                            const void          *addr,
                            size_t               len,
                            te_bool              use_nfl)
{
    if (use_nfl)
    {
        if (type == TE_LOG_MSG_FMT_ARG_EOR)
            len = TE_LOG_RAW_EOR_LEN;
        LGR_NFL_PUT(len, data->ptr);
    }

    switch (type)
    {
        case TE_LOG_MSG_FMT_ARG_EOR:
            break;

        case TE_LOG_MSG_FMT_ARG_INT:
            /* Put in network byte order */
            switch (len)
            {
                case sizeof(int8_t):
                    *(data->ptr) = *(const uint8_t *)addr;
                    break;

                case sizeof(int16_t):
                    LGR_16_TO_NET(*(const uint16_t *)addr, data->ptr);
                    break;

                case sizeof(int32_t):
                    LGR_32_TO_NET(*(const uint32_t *)addr, data->ptr);
                    break;

                case sizeof(int64_t):
                    LGR_32_TO_NET(
                        (int32_t)((*(const int64_t *)addr) >>
                                  (sizeof(int32_t) * 8)),
                        data->ptr);
                    LGR_32_TO_NET(
                        (int32_t)((*(const int64_t *)addr) &
                                  ((1ull << (sizeof(int32_t) * 8)) - 1)),
                        data->ptr + sizeof(int32_t));
                    break;

                default:
                    assert(0);
            }
            data->ptr += len;
            break;

        case TE_LOG_MSG_FMT_ARG_MEM:
            memcpy(data->ptr, addr, len);
            data->ptr += len;
            break;

        case TE_LOG_MSG_FMT_ARG_FILE:
        {
            int         fd = *(int *)addr;
            char        buf[256];
            ssize_t     r;

            while ((len > 0) &&
                   (r = read(fd, buf, MIN(len, sizeof(buf)))) > 0)
            {
                memcpy(data->ptr, buf, r);
                data->ptr += r;
                /*
                 * We have to track length here, since file can have
                 * more data, if we have decided to truncate it.
                 */
                len -= r;
            }

            if (len > 0)
            {
                /* Unexpected EOF */
                memset(data->ptr, 0, len);
                data->ptr += len;
            }
            break;
        }

        default:
            assert(0);
    }
}

static te_errno
te_log_msg_raw_put(te_log_msg_raw_data *data,
                   te_log_msg_arg_type  type,
                   const void          *addr,
                   size_t               len,
                   te_bool              use_nfl)
{
    te_errno    rc = 0;
    int         fd;
    struct stat stat_buf;


    /* Validate requested length */
    switch (type)
    {
        case TE_LOG_MSG_FMT_ARG_EOR:
            break;

        case TE_LOG_MSG_FMT_ARG_INT:
            switch (len)
            {
                case sizeof(int8_t):
                case sizeof(int16_t):
                case sizeof(int32_t):
                case sizeof(int64_t):
                    break;

                default:
                    return TE_EINVAL;
            }
            break;

        case TE_LOG_MSG_FMT_ARG_FILE:
            if (addr == NULL)
            {
                /* Log the following string */
                type = TE_LOG_MSG_FMT_ARG_MEM;
                addr = "(NULL file name)";
                len = strlen(addr);
            }
            else if (((fd = open(addr, O_RDONLY)) < 0) ||
                     (fstat(fd, &stat_buf) != 0))
            {
                /* Log name of the file instead of its contents */
                type = TE_LOG_MSG_FMT_ARG_MEM;
                len = strlen(addr);
            }
            else
            {
                /* Get length of the file and validate it below */
                addr = &fd;
                len = stat_buf.st_size;
            }
            /*FALLTHROUGH*/

        case TE_LOG_MSG_FMT_ARG_MEM:
            if (!use_nfl)
                return TE_EINVAL;
            if (len > TE_LOG_FIELD_MAX)
            {
                len = TE_LOG_FIELD_MAX;
                rc = TE_E2BIG;
                /* Continue processing with truncated field */
            }
            break;

        default:
            return TE_EINVAL;
    }

    rc = ta_log_msg_raw_buf_check_len(data,
                                      len + ((use_nfl) ?
                                                 sizeof(te_log_nfl) : 0));
    if ((rc != 0) && (rc != TE_EAGAIN))
        return rc;

    te_log_msg_raw_put_no_check(data, type, addr, len, use_nfl);

    return 0;
}

static te_errno
te_log_msg_raw_put_string(te_log_msg_raw_data *data, const char *str)
{
    if (str == NULL)
        str = "(null)";

    return te_log_msg_raw_put(data, TE_LOG_MSG_FMT_ARG_MEM,
                              str, strlen(str), TRUE);
}



/* See description in te_log_fmt.h */
te_errno
te_log_vprintf(te_log_msg_out *out, const char *fmt, va_list ap)
{
    const char * const flags = "#0+- '";

    te_errno    rc = 0;
    char        modifier;
#if 0
    int         spec_size;
#endif
    const char *s;
    const char *fmt_dup;
    const char *fmt_start;
    const char *spec_start;
    te_bool     fmt_needed = FALSE;
    va_list     ap_start;


    if (fmt == NULL)
    {
        if (out->fmt != NULL)
            rc = te_log_msg_fmt(out, "(null)");
        return rc;
    }

    if (out->fmt != NULL)
    {
        fmt_dup = strdup(fmt);
        if (fmt_dup == NULL)
        {
            return TE_ENOMEM;
        }
        va_copy(ap_start, ap);
    }
    else
    {
        fmt_dup = NULL;
    }

#define TE_LOG_VPRINTF_RAW_ARG(_type, _addr, _len) \
    do {                                                    \
        if (fmt_needed && out->fmt != NULL)                 \
        {                                                   \
            char _tmp = *spec_start;                        \
                                                            \
            *(char *)spec_start = '\0';                     \
            rc = out->fmt(out, fmt_start, ap_start);        \
            if (rc != 0)                                    \
                goto cleanup;                               \
            *(char *)spec_start = _tmp;                     \
            fmt_start = spec_start;                         \
            fmt_needed = FALSE;                             \
        }                                                   \
        if (out->raw != NULL)                               \
        {                                                   \
            char _tmp = s[1];                               \
                                                            \
            ((char *)s)[1] = '\0';                          \
            rc = out->raw(out, fmt_start, s - fmt_start + 1,\
                          _type, _addr, _len);              \
            if (rc != 0)                                    \
                goto cleanup;                               \
            ((char *)s)[1] = _tmp;                          \
        }                                                   \
        fmt_start = s + 1;                                  \
        va_end(ap_start);                                   \
        va_copy(ap_start, ap);                              \
    } while (0)


    for (s = fmt_start = fmt_dup != NULL ? fmt_dup : (char *)fmt;
         *s != '\0'; s++)
    {
        if (*s != '%')
            continue;

        spec_start = s++;

        /* Skip flags */
        while (index(flags, *s) != NULL)
            s++;

        /* Skip field width */
        while (isdigit(*s))
            s++;

        if (*s == '.')
        {
            /* Skip precision */
            s++;
            while (isdigit(*s))
                s++;
        }

        /* Length modifiers parsing */
        switch (*s)
        {
#if 0
            case '=':
                modifier = *(s + 1);
                va_copy(ap_start, ap);
                *fmt_end = '%';
                switch (modifier)
                {
/* FIXME: Incorrect macro definition */
#define SPEC_CPY(mod_, spec_)\
case mod_:\
{\
    strncpy(s, spec_, 2);\
    spec_size = strlen(spec_);\
    s[spec_size] = s[2];\
    break;\
}
                    SPEC_CPY('1', TE_PRINTF_8);
                    SPEC_CPY('2', TE_PRINTF_16);
                    SPEC_CPY('4', TE_PRINTF_32);
                    SPEC_CPY('8', TE_PRINTF_64);
#undef SPEC_CPY
                    default:
                    {
                        TE_LOG_VPRINTF_FMT_FLUSH(
                            " unsupported length modifier: =%c ",
                            modifier);
                        free(fmt_dup);
                        return TE_EFMT;
                    }
                }
                /* Save the symbol next to the specifier */
                tmp = s[3];
                /* Truncate the string before the specifier */
                s[spec_size + 1] = '\0';
                s[3] = tmp;

                va_copy(ap_start, ap);

                /* Go to the symbol after the specifier */
                s += 3;
                break;
#endif

            case 'l':
                modifier = (*++s == 'l') ? (++s, 'L') : 'l';
                break;

            case 'h':
                modifier = (*++s == 'h') ? (++s, 'H') : 'h';
                break;

            case 'L':
            case 'j':
            case 't':
            case 'z':
                modifier = *s++;
                break;

            default:
                modifier = '\0';
        }

        switch (*s)
        {
            case 'T':
                ++s;
                if (*s == 'm')
                {
                    const uint8_t  *base;
                    unsigned int    len;

                    base = va_arg(ap, const uint8_t *);
                    len = va_arg(ap, unsigned int);
                    TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_MEM,
                                           base, len);
                }
                else if (*s == 'f')
                {
                    const char *filename = va_arg(ap, const char *);

                    TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_FILE,
                                           filename, 0);
                }
                else
                {
                    /*FIXME*/
                }
                break;

            case 'r':
            {
                te_errno arg;

                arg = va_arg(ap, te_errno);
                TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_INT,
                                       &arg, sizeof(arg));
                break;
            }

            case 's':
            {
                /*
                 * String MUST be passed through TE raw log arguments
                 * to avoid any issues with conversion specifiers and
                 * any special symbols in it.
                 */
                const char *arg;

                if (modifier != '\0')
                {
                    /* TODO */
                }
                arg = va_arg(ap, const char *);
                if (arg == NULL)
                    arg = "(null)";
                TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_MEM,
                                       arg, strlen(arg));
                break;
            }

            case 'c':
            {
                /*
                 * String MUST be passed through TE raw log arguments
                 * to avoid any issues with conversion specifiers and
                 * any special symbols which may be created using %c.
                 */
                int arg;

                if (modifier != '\0')
                {
                    /* TODO */
                }
                arg = va_arg(ap, int);
                TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_INT,
                                       &arg, sizeof(arg));
                break;
            }

            case 'p':
                /* See note below for integer specifiers */
                if (modifier != '\0')
                {
                    /* TODO */
                }
                (void)va_arg(ap, void *);
                fmt_needed = TRUE;
                break;

            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X':
                /*
                 * Integer conversion specifiers are supported in TE raw
                 * log format, however, it is easier to process locally
                 * to avoid issues with difference of length modifiers
                 * meaning on different architectures.
                 *
                 * When mentioned issues are copied in RGT, it may be
                 * reasonable pass through TE raw log to avoid
                 * dependency on host's snprintf() (possibly very dummy)
                 * support.
                 */
                switch (modifier)
                {
                    case 'j':
                        (void)va_arg(ap, intmax_t);
                        break;
                    case 't':
                        (void)va_arg(ap, ptrdiff_t);
                        break;
                    case 'z':
                        (void)va_arg(ap, size_t);
                        break;
                    case 'L':
                        (void)va_arg(ap, long long);
                        break;
                    case 'l':
                        (void)va_arg(ap, long);
                        break;
                    case '8':
                        (void)va_arg(ap, int64_t);
                        break;
                    case '4':
                    case '2':
                    case '1':
                    case 'H':
                    case 'h':
                    case '\0':
                        (void)va_arg(ap, int);
                        break;
                    default:
                        /* TODO */
                        break;
                }
                fmt_needed = TRUE;
                break;

            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
                /*
                 * Float point conversion specifiers are not supported
                 * in TE raw log format, so process locally later, but
                 * move 'ap' in accordance with length modifier.
                 */
                switch (modifier)
                {
                    case 'L':
                        (void)va_arg(ap, long double);
                        break;
                    case '\0':
                        (void)va_arg(ap, double);
                        break;
                    default:
                        /* TODO */
                        break;
                }
                fmt_needed = TRUE;
                break;

            default:
                /* TODO */
                break;
        }
        /* Skip conversion specifier in for loop step */
    }

    /* Force output */
    fmt_needed = (*fmt_start != '\0');
    /* Move 's' 1 symbol back to have correct calculations in macro */
    spec_start = s--;
    TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_EOR, NULL, 0);

cleanup:

    free((char *)fmt_dup);

    if (out->fmt != NULL)
        va_end(ap_start);

#undef TE_LOG_VPRINTF_RAW_ARG

    return rc;
}


static const te_log_version log_version = TE_LOG_VERSION;

/* See description in te_log_fmt.h */
te_errno
te_log_message_raw_va(te_log_msg_raw_data *data,
                      te_log_ts_sec ts_sec, te_log_ts_usec ts_usec,
                      te_log_level level, te_log_id log_id,
                      const char *entity, const char *user,
                      const char *fmt, va_list ap)
{
    te_errno    rc;
    size_t      fmt_nfl_off;    /**< Offset of format string NFL */
    uint8_t    *fmt_nfl_ptr;    /**< Format string NFL pointer */
    size_t      fmt_start_off;  /**< Offset of the format string start */
    size_t      fmt_nfl;        /**< Format string NFL value */

    unsigned int i;
    int          fd_prev;


    data->ptr = data->buf;
    data->args_n = data->args_len = 0;
    data->trunc = FALSE;

    te_log_msg_raw_put(data, TE_LOG_MSG_FMT_ARG_INT,
                       &log_version, sizeof(log_version), FALSE);
    te_log_msg_raw_put(data, TE_LOG_MSG_FMT_ARG_INT,
                       &ts_sec, sizeof(ts_sec), FALSE);
    te_log_msg_raw_put(data, TE_LOG_MSG_FMT_ARG_INT,
                       &ts_usec, sizeof(ts_usec), FALSE);
    te_log_msg_raw_put(data, TE_LOG_MSG_FMT_ARG_INT,
                       &level, sizeof(level), FALSE);
    te_log_msg_raw_put(data, TE_LOG_MSG_FMT_ARG_INT,
                       &log_id, sizeof(log_id), FALSE);

    te_log_msg_raw_put_string(data, entity);
    te_log_msg_raw_put_string(data, user);

    /* Put fake empty string to allocate space for 'format string' NFL */
    fmt_nfl_off = data->ptr - data->buf;
    te_log_msg_raw_put_string(data, "");
    fmt_start_off = data->ptr - data->buf;

    rc = te_log_vprintf((te_log_msg_out *)data, fmt, ap);
    if (rc != 0)
        return rc;

    /* Calculate format string length */
    fmt_nfl = (data->ptr - data->buf) - fmt_start_off;

    /* Calculate current pointer to format string NFL and put value */
    fmt_nfl_ptr = data->buf + fmt_nfl_off;
    LGR_NFL_PUT(fmt_nfl, fmt_nfl_ptr);

    /* We have not enough space for all arguments */
    rc = ta_log_msg_raw_buf_check_len(data, data->args_len);
    if ((rc != 0) && (rc != TE_EAGAIN))
        return rc;

    /* Now we have enough space to put all arguments */
    for (i = 0; i < data->args_n; ++i)
    {
        te_log_msg_raw_put_no_check(data, data->args[i].type,
            (data->args[i].type == TE_LOG_MSG_FMT_ARG_MEM) ?
                data->args[i].u.a : &data->args[i].u.i,
            data->args[i].len, TRUE);
    }
    /* Find and close open descriptors if there aren't close already.*/
    for (fd_prev = -1, i = 0; i < data->args_n; ++i)
    {
        if (data->args[i].type == TE_LOG_MSG_FMT_ARG_FILE)
        {
            int fd = (int)data->args[i].u.i;
            if (fd_prev != fd)
            {
                close(fd);
                fd_prev = fd;
            }
        }
    }

    assert(data->ptr <= data->end);

    return 0;
}

/* See description in te_log_fmt.h */
te_errno
te_log_message_split(const char *file, unsigned int line,
                     unsigned int level, const char *entity,
                     const char *user, const char *fmt, ...)
{
    te_string   str = TE_STRING_INIT;
    char       *inter_str = "[continuation]\n";
    size_t      inter_len = strlen(inter_str);
    size_t      add_len = 0;
    size_t      len;
    size_t      step_len;
    va_list     ap;
    long int    i;
    int         rc;

    char       *p;
    char       *p_end;
    char       *p_begin;
    char       *str_end;
    char        buf[TE_LOG_FIELD_MAX + 1];
    char       *q;

    va_start(ap, fmt);
    rc = te_string_append_va(&str, fmt, ap);
    va_end(ap);

    if (rc != 0)
        return rc;

    str_end = str.ptr + str.len;
    p = str.ptr;
    do {
        while (*p == '\n')
            p++;

        if (*p == '\0')
            break;

        p_begin = p;
        step_len = TE_LOG_FIELD_MAX - 1 - add_len;
        if ((size_t)(str_end - p) < step_len)
        {
            p = str_end;
            p_end = str_end;
        }
        else
        {
            p += step_len;
            p_end = p;

            while (*p_end != '\n' &&
                   p_end > p_begin)
            {
                p_end--;
            }
            if (p_end == p_begin)
                p_end = p;
        }

        len = p_end - p_begin + 1;

        if (p_end < str_end)
        {
            memcpy(buf, p_begin, len);
            q = buf;
        }
        else
        {
            q = p_begin;
            len--;
        }

        q[len] = '\0';
        for (i = len - 1; i >= 0 && q[i] == '\n'; i--)
            q[i] = '\0';

        te_log_message(file, line, level, entity, user,
                       "%s%s", (add_len > 0 ? inter_str : ""),
                       q);
        add_len = inter_len;

        p = p_end + 1;
    } while (p < str_end);

    te_string_free(&str);
    return 0;
}
