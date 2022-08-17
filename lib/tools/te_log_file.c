/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Logger subsystem API
 *
 * Logger library to be used by standalone TE off-line applications.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Logger File"

#include "te_config.h"

#include <stdio.h>
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
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_defs.h"

#include "te_log_fmt.h"
#include "logger_file.h"


/** File stream to write log messages. */
FILE *te_log_message_file_out = NULL;

/**
 * Data of the backend to output message by format string and arguments
 * to a file.
 */
typedef struct te_log_msg_fmt_to_file {
    struct te_log_msg_out   common; /**< Common backends data */
    FILE                   *file;   /**< File stream to output */
    int                     fd;     /**< File descriptor to output */
} te_log_msg_fmt_to_file;

/** Macro to get file stream pointer for backend data */
#define TE_LOG_MSG_OUT_AS_FILE(_out) \
    (((te_log_msg_fmt_to_file *)(_out))->file)


/**
 * Process format string with its arguments in vprintf()-like mode
 * with output to a file.
 *
 * This functions complies with te_log_msg_fmt_args_f prototype.
 */
static te_errno
te_log_msg_fmt_args_to_file(te_log_msg_out *out,
                            const char *fmt, va_list ap)
{
    (void)vfprintf(TE_LOG_MSG_OUT_AS_FILE(out), fmt, ap);

    return 0;
}

/**
 * Process an argument and format string which corresponds to it
 * in raw mode.
 *
 * This functions complies with te_log_msg_raw_arg_f prototype.
 */
static te_errno
te_log_msg_raw_arg_to_file(te_log_msg_out      *out,
                           const char          *fmt,
                           size_t               fmt_len,
                           te_log_msg_arg_type  arg_type,
                           const void          *arg_addr,
                           size_t               arg_len)
{
    FILE   *f = TE_LOG_MSG_OUT_AS_FILE(out);
    int     fd = ((te_log_msg_fmt_to_file *)(out))->fd;
    int     sysrc __attribute__((unused));

    switch (arg_type)
    {
        case TE_LOG_MSG_FMT_ARG_EOR:
            fputc('\n', f);
            break;

        case TE_LOG_MSG_FMT_ARG_INT:
            assert(fmt != NULL);
            assert(fmt_len > 0);
            switch (fmt[fmt_len - 1])
            {
                case 'r':
                {
                    te_errno err = *(const te_errno *)arg_addr;

                    sysrc = write(fd, fmt, strchr(fmt, '%') - fmt);
                    if (sysrc < 0)
                    {
                        fprintf(stderr, "Write to fd=%d failed, errno=%s",
                                fd, strerror(errno));
                        break;
                    }
                    if (TE_RC_GET_MODULE(err) == 0)
                        fputs(te_rc_err2str(err), f);
                    else
                        fprintf(f, "%s-%s",
                                te_rc_mod2str(err), te_rc_err2str(err));
                    break;
                }

                case 'c':
                    fprintf(f, fmt, *(const int *)arg_addr);
                    break;

                default:
                    fprintf(f, "<Unknown conversion specifier %s>", fmt);
                    break;
            }
            break;

        case TE_LOG_MSG_FMT_ARG_FILE:
        {
            int         in = -1;
            struct stat stat_buf;

            sysrc = write(fd, fmt, strchr(fmt, '%') - fmt);
            if (sysrc < 0)
            {
                fprintf(stderr, "Write to fd=%d failed, errno=%s",
                        fd, strerror(errno));
                break;
            }
            if (arg_addr == NULL)
            {
                fputs("(NULL file name)", f);
            }
            else if (((in = open(arg_addr, O_RDONLY)) < 0) ||
                     (fstat(in, &stat_buf) != 0))
            {
                fputs(arg_addr, f);
            }
            else
            {
                char    buf[256];
                ssize_t r;

                while ((r = read(in, buf, sizeof(buf) - 1)) > 0)
                {
                    if (write(fd, buf, r) != r)
                        fprintf(f, "<write() failed>");
                }
            }
            if (in >= 0)
                (void)close(in);
            break;
        }

        case TE_LOG_MSG_FMT_ARG_MEM:
            assert(fmt != NULL);
            assert(fmt_len > 0);
            switch (fmt[fmt_len - 1])
            {
                case 's':
                    fprintf(f, fmt, (const char *)arg_addr);
                    break;

                case 'm':
                {
                    const uint8_t  *p = arg_addr;
                    size_t          i;

                    sysrc = write(fd, fmt, strchr(fmt, '%') - fmt);
                    fputc('\n', f);
                    for (i = 0; i < arg_len; i++)
                    {
                        fprintf(f, "%02hhX%c", p[i],
                                ((i & 0xf) == 0xf) ? '\n' : ' ');
                    }
                    if ((arg_len & 0xf) != 0)
                        fputc('\n', f);
                    break;
                }

                default:
                    fprintf(f, "<Unknown conversion specifier %s>", fmt);
                    break;
            }
            break;

        default:
            return TE_EINVAL;
    }

    return 0;
}


/** Common data for backend to output to a file */
static const struct te_log_msg_out te_log_msg_out_file = {
    te_log_msg_fmt_args_to_file,
    te_log_msg_raw_arg_to_file
};



/* See the description in logger_file.h */
void
te_log_message_file(const char *file, unsigned int line,
                    te_log_ts_sec sec, te_log_ts_usec usec,
                    unsigned int level,
                    const char *entity, const char *user,
                    const char *fmt, va_list ap)
{
    time_t                  sec_time = sec;
    struct tm               tm_time;
    te_log_msg_fmt_to_file  fmt_backend;

    if (te_log_message_file_out == NULL)
    {
        te_log_message_file_out = stderr;
        fmt_backend.fd = STDERR_FILENO;
    }
    else
    {
        fmt_backend.fd = fileno(te_log_message_file_out);
    }
    fmt_backend.file = te_log_message_file_out;
    fmt_backend.common = te_log_msg_out_file;

    if (localtime_r(&sec_time, &tm_time) == NULL)
    {
        fprintf(te_log_message_file_out, "localtime_r() failed\n");
        memset(&tm_time, 0, sizeof(tm_time));
    }

    fprintf(te_log_message_file_out,
            "%s  %s  %s  %02d:%02d:%02d %u us\n", te_log_level2str(level),
            entity == NULL ? "(nil)" : entity,
            user == NULL ? "(nil)" : user,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
            (unsigned int)usec);

    if (te_log_vprintf(&fmt_backend.common, fmt, ap) != 0)
        fprintf(te_log_message_file_out,
                "ERROR: Processing of format string '%s' from %s:%u "
                "failed\n", fmt, file, line);

    fputc('\n', te_log_message_file_out);
}
