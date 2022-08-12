/** @file
 * @brief Log processing
 *
 * This module provides a view structure for log messages and
 * and a set of utility functions.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "Log processing"

#include "te_config.h"

#include <assert.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "log_msg_view.h"
#include "te_alloc.h"
#include "te_raw_log.h"
#include "logger_api.h"
#include "logger_int.h"

#define UINT8_NTOH(val) (val)
#define UINT16_NTOH(val) ntohs(val)
#define UINT32_NTOH(val) ntohl(val)

/* See description in raw_log_filter.h */
te_errno
te_raw_log_parse(const void *buf, size_t buf_len, log_msg_view *view)
{
    const char *buf_ptr = buf;
    size_t      buf_off = 0;

#define OFFSET(BYTES) \
    do {                  \
        buf_ptr += BYTES; \
        buf_off += BYTES; \
    } while(0)

#define READINT(BITS, TARGET) \
    do {                                                        \
        if ((buf_off + BITS / 8 - 1) >= buf_len)                \
        {                                                       \
            ERROR("te_raw_log_parse: attempt to read %d bytes " \
                  "from offset %d, buffer length is %llu",      \
                  BITS / 8, buf_off, buf_len);                  \
            return TE_EINVAL;                                   \
        }                                                       \
                                                                \
        TARGET = *(const uint ## BITS ## _t *)buf_ptr;          \
        TARGET = UINT ## BITS ## _NTOH(TARGET);                 \
        OFFSET(BITS / 8);                                       \
    } while (0)

    view->length = buf_len;
    view->start = buf;

    READINT(8, view->version);
    assert(view->version == 1);

    READINT(32, view->ts_sec);
    READINT(32, view->ts_usec);
    READINT(16, view->level);
    READINT(32, view->log_id);

    READINT(16, view->entity_len);
    view->entity = buf_ptr;
    OFFSET(view->entity_len);

    READINT(16, view->user_len);
    view->user = buf_ptr;
    OFFSET(view->user_len);

    READINT(16, view->fmt_len);
    view->fmt = buf_ptr;
    OFFSET(view->fmt_len);

    view->args = buf_ptr;

#undef OFFSET
#undef READINT
    return 0;
}

/**
 * Parse the arguments in a Tm[[n].[w]] specifier.
 *
 * @param fmt_orig      pointer to the potential start of the argument list,
 *                      will be updated to the next character after the
 *                      arguments in case of a successful match
 * @param fmt_end       pointer to the next character after the end of the
 *                      format string
 * @param size          first argument: tuple size
 * @param nmemb         second argument: number of tuples
 *
 * @returns TRUE if the arguments were extracted, FALSE otherwise
 */
static te_bool
extended_format(const char **fmt_orig, const char *fmt_end,
                int *size, int *nmemb)
{
    const char *fmt;
    int         tuple_width;
    int         n_tuples;

    if (fmt_orig == NULL)
        return FALSE;
    fmt = *fmt_orig;

    /* Match [[ */
    if (fmt_end - fmt > 2 &&
        strncmp(fmt, "[[", 2) == 0)
    {
        fmt += 2;
        /* Match number */
        n_tuples = 0;
        while (fmt < fmt_end && isdigit(*fmt))
        {
            n_tuples = n_tuples * 10 + (*fmt - '0');
            fmt++;
        }
        /* match ].[ */
        if (fmt_end - fmt > 3 &&
            strncmp(fmt, "].[", 3) == 0)
        {
            fmt += 3;
            /* Match number */
            tuple_width = 0;
            while (fmt < fmt_end && isdigit(*fmt))
            {
                tuple_width = tuple_width * 10 + (*fmt - '0');
                fmt++;
            }
            /* Match ]] */
            if (fmt_end - fmt >= 2 &&
                strncmp(fmt, "]]", 2) == 0)
            {
                fmt += 2;
                *size = tuple_width;
                *nmemb = n_tuples;
                *fmt_orig = fmt;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* See description in raw_log_filter.h */
te_errno
te_raw_log_expand(const log_msg_view *view, te_string *target)
{
    te_errno    rc = 0;
    const char *fmt;
    const char *fmt_end;
    const void *arg;
    const void *view_end;
    te_log_nfl  arg_len;

    if (view == NULL || target == NULL)
        return TE_EINVAL;

    fmt = view->fmt;
    arg = view->args;
    fmt_end = fmt + view->fmt_len;
    view_end = view->start + view->length;

    while (fmt < fmt_end)
    {
        if (*fmt == '%' && (fmt + 1) < fmt_end)
        {
            if (arg >= view_end ||
                (arg_len = ntohs(*(te_log_nfl *)arg)) == TE_LOG_RAW_EOR_LEN)
            {
                /* Too few arguments in the message */
                /* Simply write the rest of format string */
                return te_string_append(target, "%.*s", (fmt_end - fmt), fmt);
            }
            arg += sizeof(te_log_nfl);
            if (arg + arg_len >= view_end)
            {
                ERROR("Argument ends after the containing message, truncating argument");
                arg_len = view_end - arg;
            }
            fmt++;
            switch (*fmt)
            {
                case '%':
                {
                    rc = te_string_append(target, "%%");
                    if (rc != 0)
                        return rc;
                    break;
                }
                case 'c':
                case 'd':
                case 'u':
                case 'o':
                case 'x':
                case 'X':
                {
                    char format[3] = {'%', *fmt, '\0'};
                    uint32_t val;

                    val = ntohl(*(uint32_t *)arg);
                    te_string_append(target, format, val);
                    break;
                }
                case 'p':
                {
                    int      j;
                    uint32_t val;
                    assert(arg_len % 4 == 0);

                    te_string_append(target, "0x");
                    for (j = 0; j < arg_len / 4; j++)
                    {
                        val = *(((uint32_t *)arg) + j);

                        /* Skip not trailing zero words */
                        if (val == 0 && (j + 1) < arg_len / 4)
                            continue;
                        val = ntohl(val);

                        rc = te_string_append(target, "%08x", val);
                        if (rc != 0)
                            return rc;
                    }
                    break;
                }
                case 's':
                {
                    rc = te_string_append(target, "%.*s", arg_len, arg);
                    if (rc != 0)
                        return rc;
                    break;
                }
                case 'r':
                {
                    te_errno    err;
                    const char *src_str;
                    const char *err_str;

                    err = ntohl(*(uint32_t *)arg);
                    src_str = te_rc_mod2str(err);
                    err_str = te_rc_err2str(err);
                    if (strlen(src_str) > 0)
                    {
                        rc = te_string_append(target, "%s-", src_str);
                        if (rc != 0)
                            return rc;
                    }
                    rc = te_string_append(target, "%s", err_str);
                    if (rc != 0)
                        return rc;
                    break;
                }
                case 'T':
                {
                    fmt++;
                    if (fmt >= fmt_end)
                        return TE_EINVAL;

                    switch (*fmt)
                    {
                        case 'f':
                        {
                            rc = te_string_reserve(target, target->len + arg_len);
                            if (rc != 0)
                                return rc;
                            memcpy(target->ptr + target->len, arg, arg_len);
                            target->len += arg_len;
                            break;
                        }
                        case 'm':
                        {
                            int i;
                            int tuple_width;
                            int n_tuples;
                            int line_cnt = 1;
                            int tuple_cnt = 1;

/*
 *  %Tm[[n].[w]] - memory dump, n - the number of elements after
 *                 which "\n" is to be inserted , w - width (in bytes) of
 *                 the element.
 */

                            fmt++;
                            if (!extended_format(&fmt, fmt_end, &tuple_width, &n_tuples))
                            {
                                tuple_width = 1;
                                n_tuples = 16;
                            }

                            for (i = 0; i < arg_len; i++)
                            {
                                line_cnt--;
                                tuple_cnt--;
                                if (line_cnt == 0)
                                {
                                    rc = te_string_append(target, "\n  ");
                                    line_cnt = tuple_width * n_tuples;
                                    tuple_cnt = tuple_width;
                                }
                                else if (tuple_cnt == 0)
                                {
                                    rc = te_string_append(target, " ");
                                    tuple_cnt = tuple_width;
                                }
                                if (rc != 0)
                                    return rc;
                                rc = te_string_append(target, "%02X", ((const char *)arg)[i]);
                                if (rc != 0)
                                    return rc;
                            }
                            rc = te_string_append(target, "\n\n");
                            if (rc != 0)
                                return rc;
                        }
                    }
                    break;
                }
            }
            arg += arg_len;
        }
        else
        {
            te_string_append(target, "%c", *fmt);
        }
        fmt++;
    }
    return 0;
}
