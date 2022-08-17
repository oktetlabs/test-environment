/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Format string parsing
 *
 * Some TE-specific features, such as memory dump, file content logging,
 * and additional length modifiers are supported.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STDDEF_H
#include <stddef.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_defs.h"
#include "te_printf.h"
#include "te_errno.h"

#include "te_format.h"


/** Buffer extension step */
#define TEF_BUFFER_EXT_STEP     256
/** Buffer size limit */
#define TEF_BUFFER_MAX          0x10000

#ifdef WINDOWS
char *
index(const char *s, int c)
{
    if (s == NULL)
        return NULL;

    while (*s != c && *s != 0)
        s++;

    return *s == 0 ? NULL : s;
}
#endif

/**
 * Output the message to log - va_list version.
 *
 * @param param        Output parameters
 * @param fmt          Format string
 * @param ap           Arguments for the format string
 *
 * @return             Error code
 */
static te_errno
msg_va_output(struct te_log_out_params *param, const char *fmt, va_list ap)
{
    int ret;

    assert(param != NULL);

    /* Write to file */
    if (param->fp != NULL)
        (void)vfprintf(param->fp, fmt, ap);

    /* Write to buffer */
    if (param->buf != NULL)
    {
        if (param->offset >= param->buflen)
            return TE_EINVAL;

        ret = vsnprintf((char *)(param->buf + param->offset),
                        param->buflen - param->offset, fmt, ap);

        if (ret < 0)
        {
            return te_rc_os2te(errno);
        }
        else if (param->offset + ret >= param->buflen)
        {
            const char trunc_str[] = "<TRUNCATED!>";

            if (param->buflen > sizeof(trunc_str))
            {
                memcpy(param->buf + param->buflen - sizeof(trunc_str),
                       trunc_str, sizeof(trunc_str));
            }
            param->offset = param->buflen - 1;
            return TE_ESMALLBUF;
        }

        param->offset += ret;
    }

    return 0;
}

/**
 * Output the message to log.
 *
 * @param param         Output parameters
 * @param fmt           Format string
 * @param ...           Arguments for the format string
 *
 * @return              Error code
 */
static te_errno
msg_output(struct te_log_out_params *param, const char *fmt, ...)
{
    te_errno    result;
    va_list     ap;

    va_start(ap, fmt);
    result = msg_va_output(param, fmt, ap);
    va_end(ap);
    return result;
}

/**
 * Finish message processing
 *
 * @param param        Output parameters
 *
 * @return             Error code
 */
static te_errno
msg_end_process(struct te_log_out_params *param)
{
    if (param->fp != NULL)
    {
        fprintf(param->fp, "\n");
        fflush(param->fp);
    }
    return 0;
}

/*
 * FIXME: Read 'man va_copy'. Add missing 'va_end' calls.
 * FIXME: Resourse leak here: fmt_dup.
 * FIXME: Many switches below do not have 'default'. It is not good.
 */
te_errno
te_log_vprintf_old(struct te_log_out_params *param,
                   const char *fmt, va_list ap)
{
    const char * const flags = "#0+- '";

    char *s0;                     /* The first symbol not written */
    char *s;                      /* Current symbol */
    char *spec_start;
    int   spec_size;
    char *fmt_dup;
    char  modifier;
    char  tmp;
    int   i;

    te_errno    rc = 0;
    va_list     ap0;

    if (param == NULL)
        return TE_EINVAL;

    if (param->buflen == 0)
        return TE_EINVAL;
    param->buf[0] = '\0';

    if (fmt == NULL)
    {
        return msg_output(param, "(null)");
    }

    fmt_dup = strdup(fmt);
    if (fmt_dup == NULL)
        return TE_ENOMEM;

    va_copy(ap0, ap);

/* FIXME: Incorrect macro definition */
#define FLUSH(arg...) \
    if ((rc = msg_output(param, arg)) != 0)\
        goto finish;

/* FIXME: Incorrect macro definition */
#define VFLUSH(arg...) \
    if ((rc = msg_va_output(param, arg)) != 0)\
        goto finish;

    for (s = s0 = fmt_dup; *s != '\0'; s++)
    {
        if (*s != '%')
            continue;

        modifier = '\0';
        spec_start = s++;

/* FIXME: Incorrect macro definition
 * See strcmp_start() in te_defs.h */
#define strcmp_start(s1, s2) strncmp(s1, s2, strlen(s1))

        /* Skip flags, field width, and precision */
        while (index(flags, *s) != 0)
            s++;
        while (isdigit(*s))
            s++;
        if (*s == '.')
        {
            s++;
            while (isdigit(*s))
                s++;
        }

        /* Length modifiers parsing */
        switch (*s)
        {
            case '=':
            {
                *spec_start = '\0';
                VFLUSH(s0, ap0);
                va_copy(ap0, ap);
                s0 = spec_start;
                *spec_start = '%';
                modifier = *(s + 1);
                switch (modifier)
                {
#define CASE_SPEC_CPY(mod_, spec_) \
    case mod_:                      \
    {                               \
        spec_size = strlen(spec_);  \
        memcpy(s, spec_, spec_size);\
        s[spec_size] = s[2];        \
        break;                      \
    }
                    CASE_SPEC_CPY('1', TE_PRINTF_8);
                    CASE_SPEC_CPY('2', TE_PRINTF_16);
                    CASE_SPEC_CPY('4', TE_PRINTF_32);
                    CASE_SPEC_CPY('8', TE_PRINTF_64);
#undef CASE_SPEC_CPY
                    default:
                    {
                        FLUSH(" unsupported length modifier: =%c ",
                              modifier);
                        free(fmt_dup);
                        return TE_EFMT;
                    }
                }
                /* Save the symbol next to the specifier */
                tmp = s[3];
                /* Truncate the string before the specifier */
                s[spec_size + 1] = '\0';
                VFLUSH(s0, ap0);
                s[3] = tmp;
                /* Shift to next argument */
                if (modifier == '8')
                {
                    (void)va_arg(ap, int64_t);
                }
                else
                {
                    (void)va_arg(ap, int);
                }

                va_copy(ap0, ap);

                /* Go to the symbol after the specifier */
                s += 3;
                s0 = s;
                break;
            }
            case 'l':
            {
                modifier = 'l';
                if (strcmp_start("ll", s) == 0)
                {
                    modifier = 'L';
                    s++;
                }
                s++;
                break;
            }
            case 'h':
            {
                modifier = 'h';
                if (strcmp_start("hh", s) == 0)
                {
                    modifier = 'H';
                    s++;
                }
                s++;
                break;
            }
            case 'j':
            case 't':
            case 'z':
                modifier = *s++;
                break;
            default:
            {
                modifier = '\0';
            }
        }
        switch (*s)
        {
            case 'T':
            {
                *spec_start = '\0';
                VFLUSH(s0, ap0);
                if (*(s + 1) == 'm')
                {
                    int             len;
                    const uint8_t  *base;

                    base = va_arg(ap, const uint8_t *);
                    len = va_arg(ap, int);
                    /* Our own argument is not for printf(), skip it */
                    va_copy(ap0, ap);
                    FLUSH("\n");
                    for (i = 0; i < len; i++)
                    {
                        FLUSH("%02hhX", base[i]);
                        if (i % 16 == 15)
                        {
                            FLUSH("\n");
                        }
                        else
                        {
                            FLUSH(" ");
                        }
                    }
                    FLUSH("\n");
                }
                else if (*(s + 1) == 'f')
                {
                    const char *filename;
                    char        buf[1024];
                    FILE       *fp;

                    filename = va_arg(ap, const char *);
                    /* Our own argument is not for printf(), skip it */
                    va_copy(ap0, ap);
                    if ((fp = fopen(filename, "r")) == NULL)
                    {
                        FLUSH(" cannot open file %s ", filename);
                    }
                    while (fgets(buf, sizeof(buf), fp) != NULL)
                    {
                        FLUSH("%s", buf);
                    }
                    fclose(fp);
                }

                s++;
                s0 = s +1;       /* Set pointers next to the specifier */
                break;
            }

            case 'r':
            {
                te_errno err;

                *spec_start = '\0';
                VFLUSH(s0, ap0);
                err = va_arg(ap, te_errno);
                va_copy(ap0, ap);
                if (TE_RC_GET_MODULE(err) == 0)
                {
                     FLUSH("%s", te_rc_err2str(err));
                }
                else
                {
                     FLUSH("%s-", te_rc_mod2str(err));
                     FLUSH("%s", te_rc_err2str(err));
                }
                s0 = s + 1;
                break;
            }

            case 's':
            {
                (void)va_arg(ap, char *);
/* FIXME: What are you doing here? */
#if 0
                char *arg_str;
                char *fake_str;
                int   percent_count = 0;
                int   i;
                char *tmp;
                char *s00;
                char *fs00;

                *spec_start = '\0';
                VFLUSH(s0, ap0);
                arg_str = va_arg(ap, char *);
                va_copy(ap0, ap);

                for (tmp = arg_str; *tmp != '\0'; tmp++)
                {
                    if (*tmp == '%')
                    {
                        percent_count++;
                    }
                }
                fake_str = (char *)malloc(strlen(arg_str) +
                                          percent_count + 1);
                for (i = 1, tmp = s00 = arg_str, fs00 = fake_str;
                     *tmp != '\0'; tmp++)
                {
                    if (*tmp == '%')
                    {
                        strncpy(fs00, s00, i);
                        fs00[i] = '%';
                        s00 += i;
                        fs00 += ++i;
                        i = 1;
                    }
                    else
                    {
                        i++;
                    }
                }
                strncpy(fs00, s00, i);

                FLUSH("%s", fake_str);
                free(fake_str);
                s0 = s + 1;
#endif
                break;
            }

            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X':
            {
                switch(modifier)
                {
                    case 'L':
                    {
                        (void)va_arg(ap, long long);
                        break;
                    }
                    case 'z':
                    {
                        (void)va_arg(ap, size_t);
                        break;
                    }
                    case 't':
                    {
                        (void)va_arg(ap, ptrdiff_t);
                        break;
                    }
                    case 'j':
                    {
                        (void)va_arg(ap, intmax_t);
                        break;
                    }
                    case '8':
                    case '4':
                    case '2':
                    case '1':
                    {
                        break;
                        /* The ap was already shifted */
                    }
                    default:
                    {
                        (void)va_arg(ap, int);
                    }
                }
                modifier = '\0';
                break;
            }
            case 'c':
            {
                (void)va_arg(ap, int);
                modifier = '\0';
                break;
            }
            case 'p':
            {
                (void)va_arg(ap, void *);
                modifier = '\0';
                break;
            }
        }
    }
    VFLUSH(s0, ap0);

finish:
    msg_end_process(param);

#undef FLUSH
#undef VFLUSH

    free(fmt_dup);

    return rc;
}
