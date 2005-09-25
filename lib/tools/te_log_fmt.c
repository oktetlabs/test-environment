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

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_printf.h"
#include "te_raw_log.h"
#include "logger_defs.h"
#include "logger_int.h"


/** Does TE have a support of formatted message logging */
#define TE_HAVE_LOG_MSG_FMT     1
/** Does TE have a support of raw message logging */
#define TE_HAVE_LOG_MSG_RAW     1


#define TE_LOG_MSG_OUT_AS_FILE(_out) \
    (((te_log_msg_fmt_to_file *)(_out))->file)


/** Types of TE log message arguments */
typedef enum te_log_msg_arg_type {
    TE_LOG_MSG_FMT_ARG_EOR,     /**< End-of-record */
    TE_LOG_MSG_FMT_ARG_INT,     /**< Integer in network bytes order */
    TE_LOG_MSG_FMT_ARG_MEM,     /**< Memory dump or string */
    TE_LOG_MSG_FMT_ARG_FILE,    /**< File content */
} te_log_msg_arg_type;

struct te_log_msg_out;
typedef struct te_log_msg_out te_log_msg_out;

typedef te_errno (*te_log_msg_fmt_f)(te_log_msg_out *out,
                                     const char     *fmt,
                                     va_list         ap);

typedef te_errno (*te_log_msg_raw_arg_f)(te_log_msg_out      *out,
                                         te_log_msg_arg_type  type,
                                         const void          *addr,
                                         size_t               len,
                                         te_bool              no_nfl);

struct te_log_msg_out {
    te_log_msg_fmt_f        fmt;        /**< FIXME */
    te_log_msg_raw_arg_f    raw_arg;    /**< Format string argument */
};

#if 0
void
dump(const uint8_t *p, const uint8_t *q)
{
    unsigned int len = q - p;
    unsigned int i;

    for (i = 0; i < len; ++i)
        fprintf(stderr, "%#02x ", p[i]);
    fprintf(stderr, "\n\n");
    fflush(stderr);
}

#define DUMP_RAW    dump(raw->buf, raw->ptr);
#endif

/**
 * Map TE log level to string.
 *
 * @param level     TE log level
 *
 * @return TE log level as string
 */
const char *
te_log_level2str(te_log_level level)
{
    switch (level)
    {
#define CASE_TE_LL_TO_STR(_level) \
        case TE_LL_##_level:    return #_level;

        CASE_TE_LL_TO_STR(ERROR);
        CASE_TE_LL_TO_STR(WARN);
        CASE_TE_LL_TO_STR(RING);
        CASE_TE_LL_TO_STR(INFO);
        CASE_TE_LL_TO_STR(VERB);

#undef CASE_TE_LL_TO_STR

        default:
            return "UNKNOWN";
    }
}

typedef struct te_log_msg_fmt_to_file {
    struct te_log_msg_out   common;
    FILE                   *file;
} te_log_msg_fmt_to_file;


static te_errno
te_log_msg_fmt_file(te_log_msg_out *out, const char *fmt, va_list ap)
{
    FILE *f = TE_LOG_MSG_OUT_AS_FILE(out);

    (void)vfprintf(f, fmt, ap);

    return 0;
}

static const struct te_log_msg_out te_log_msg_out_file = {
    te_log_msg_fmt_file,
    NULL
};


#if 0
static struct te_log_msg_to_file te_log_msg_to_stderr =
    { te_log_msg_out_file, stderr };
#endif



typedef struct te_log_msg_raw_data {
    struct te_log_msg_out   common;
    uint8_t                *buf;
    size_t                  len;
    uint8_t                *ptr;
} te_log_msg_raw_data;


/** Log argument descriptor */
typedef struct te_log_arg_descr {
    te_log_msg_arg_type     type;   /**< Type of the argument */
    size_t                  len;    /**< Data length */
    union {
        const void         *a;      /**< Pointer argument */
        uint64_t            i;      /**< Integer argument */
    } u;
} te_log_arg_descr;


/** Maximum number of arguments supported by arguments descriptor */
#define TE_LOG_ARGS_DESCR_MAX   32

/** Arguments descriptor structure */
typedef struct te_log_args_descr {
    unsigned int        n;      /**< Number of arguments */
    size_t              len;    /**< Total length required in raw log */
    te_log_arg_descr    args[TE_LOG_ARGS_DESCR_MAX];    /**< Arguments */
} te_log_args_descr;

typedef struct te_log_msg_args_data {
    struct te_log_msg_out   common;
    te_log_args_descr       descr;
} te_log_msg_args_data;

    
static te_errno
te_log_msg_args_arg(te_log_msg_out      *out,
                    te_log_msg_arg_type  type,
                    const void          *addr,
                    size_t               len,
                    te_bool              no_nfl)
{
    te_log_msg_args_data   *data = (te_log_msg_args_data *)out;
    unsigned int            n;
    int                     fd = -1;
    struct stat             stat_buf;

    assert(data != NULL);

    n = data->descr.n;
    if (n == TE_LOG_ARGS_DESCR_MAX)
        return TE_E2BIG;

    /* Validate requested length */
    switch (type)
    {
        case TE_LOG_MSG_FMT_ARG_INT:
            /* Store in network byte order */
            switch (len)
            {
                case 1:
                    *((int8_t *)&(data->descr.args[n].u.i)) =
                        *(const int8_t *)addr;
                    break;
                case 2:
                    LGR_16_TO_NET(*(const int16_t *)addr,
                                  &(data->descr.args[n].u.i));
                    break;
                case 4:
                    LGR_32_TO_NET(*(const int32_t *)addr,
                                  &(data->descr.args[n].u.i));
                    break;
                case 8:
                    LGR_32_TO_NET(((const int32_t *)addr)[1],
                                  &(data->descr.args[n].u.i));
                    LGR_32_TO_NET(((const int32_t *)addr)[0],
                        ((uint32_t *)&(data->descr.args[n].u.i)) + 1);
                    break;

                defautl:
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
                len = stat_buf.st_size;
            }
            if (fd >= 0)
                (void)close(fd);
            /*FALLTHROGH*/

        case TE_LOG_MSG_FMT_ARG_MEM:
            data->descr.args[n].u.a = addr;
            break;

        default:
            return TE_EINVAL;
    }

    data->descr.args[n].type = type;
    data->descr.args[n].len  = len;

    data->descr.len += len + (no_nfl) ? 0 : TE_LOG_NFL_SZ;

    data->descr.n++;

    return 0;
}


static te_errno
te_log_msg_raw_arg(te_log_msg_out      *out,
                   te_log_msg_arg_type  type,
                   const void          *addr,
                   size_t               len,
                   te_bool              no_nfl)
{
    te_log_msg_raw_data    *raw = (te_log_msg_raw_data *)out;
    te_errno                rc = 0;
    int                     fd;
    struct stat             stat_buf;


    /* Validate requested length */
    switch (type)
    {
        case TE_LOG_MSG_FMT_ARG_EOR:
            len = TE_LOG_RAW_EOR_LEN;
            break;

        case TE_LOG_MSG_FMT_ARG_INT:
            switch (len)
            {
                case 1:
                case 2:
                case 4:
                case 8:
                    break;

                defautl:
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
                len = stat_buf.st_size;
            }
            /*FALLTHROGH*/

        case TE_LOG_MSG_FMT_ARG_MEM:
            if (no_nfl)
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

    /* Add next field length */
    if (!no_nfl)
        LGR_NFL_PUT(len, raw->ptr);

    switch (type)
    {
        case TE_LOG_MSG_FMT_ARG_EOR:
            break;

        case TE_LOG_MSG_FMT_ARG_INT:
            /* Put in network byte order */
            switch (len)
            {
                case 1:
                    *(raw->ptr) = *(const uint8_t *)addr;
                    break;

                case 2:
                    LGR_16_TO_NET(*(const uint16_t *)addr, raw->ptr);
                    break;

                case 4:
                    LGR_32_TO_NET(*(const uint32_t *)addr, raw->ptr);
                    break;

                case 8:
                    LGR_32_TO_NET(((const uint32_t *)addr)[1],
                                  raw->ptr);
                    LGR_32_TO_NET(((const uint32_t *)addr)[0],
                                  raw->ptr + 4);
                    break;

                default:
                    assert(0);
            }
            raw->ptr += len;
            break;

        case TE_LOG_MSG_FMT_ARG_MEM:
            /* Just copy */
            memcpy(raw->ptr, addr, len);
            raw->ptr += len;
            break;

        case TE_LOG_MSG_FMT_ARG_FILE:
        {
            /* Just copy from file */
            uint8_t buf[1024];
            ssize_t r;

            /* Control length to be sure that we'll not copy more */
            while ((len > 0) && (r = read(fd, buf, sizeof(buf))) > 0)
            {
                r = MIN(len, r);
                memcpy(raw->ptr, buf, r);
                raw->ptr += r;
                len -= r;
            }
            /* If actual file size is less than expected, add zeros */
            if (len > 0)
            {
                memset(raw->ptr, 0, len);
                raw->ptr += len;
            }
            break;
        }

        default:
            assert(0);
    }

    return rc;
}

static te_errno
te_log_msg_raw_string(te_log_msg_out *out, const char *str)
{
    if (str == NULL)
        str = "(null)";

    return te_log_msg_raw_arg(out, TE_LOG_MSG_FMT_ARG_MEM,
                              str, strlen(str), FALSE); 
}


static struct te_log_msg_out te_log_msg_out_raw = {
    NULL,
    te_log_msg_raw_arg
};


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
  * Preprocess and output message to log with special features parsing
  *
  * @param out      Output parameters
  * @param fmt      Format string
  * @param ap       Arguments for the format string
  *
  * @return Error code (see te_errno.h)
  */
static te_errno
te_log_vprintf(te_log_msg_out *out, const char *fmt, va_list ap)
{
    const char * const flags = "#0+- '";

    te_errno    rc = 0;
    char        modifier;
    int         spec_size;
    char        tmp;
#if !TE_HAVE_LOG_MSG_FMT
    const char *s;
#else
    char       *s;
    char       *fmt_dup;
    char       *fmt_start;
    char       *fmt_end;
    te_bool     flushed = FALSE;
    va_list     ap_start;
#endif
    

    if (fmt == NULL)
    {
#if TE_HAVE_LOG_MSG_FMT
        if (out->fmt != NULL)
            rc = te_log_msg_fmt(out, "(null)");
#endif
        return rc;
    }

#if TE_HAVE_LOG_MSG_FMT
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
#endif

#if TE_HAVE_LOG_MSG_RAW
#define TE_LOG_VPRINTF_RAW_ARG(_type, _addr, _len) \
    do { \
        if (out->raw_arg != NULL) \
        { \
            out->raw_arg(out, _type, _addr, _len, FALSE);  \
        } \
    } while (0)
#else
#define TE_LOG_VPRINTF_RAW_ARG(_type, _addr, _len)
#endif
    

#if TE_HAVE_LOG_MSG_FMT
/*
 * We don't need to restore symbol at 'fmt_end', since:
 *  - it must be '%' or '\0';
 *  - the function is called when TE-specific is encountered.
 */
#define TE_LOG_VPRINTF_FMT_VFLUSH \
    do {                                                    \
        if (out->fmt != NULL)                               \
        {                                                   \
            *fmt_end = '\0';                                \
            out->fmt(out, fmt_start, ap_start);             \
            va_end(ap_start);                               \
            flushed = TRUE;                                 \
            fmt_start = fmt_end;                            \
        }                                                   \
    } while (0)

#define TE_LOG_VPRINTF_FMT_FLUSH(_args...) \
    do {                                                    \
        if (out->fmt != NULL)                               \
        {                                                   \
            te_log_msg_fmt(out, _args);                     \
        }                                                   \
    } while (0)
    
#else
#define TE_LOG_VPRINTF_FMT_VFLUSH
#define TE_LOG_VPRINTF_FMT_FLUSH(_args...)
#endif
            
    for (s = 
#if TE_HAVE_LOG_MSG_FMT
            fmt_start = fmt_dup != NULL ? fmt_dup : (char *)fmt;
#else
            fmt;
#endif
         *s != '\0'; s++)
    {
        if (*s != '%')
            continue;

#if TE_HAVE_LOG_MSG_FMT
        flushed = FALSE;
        fmt_end = s;
#endif
        ++s;

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
                TE_LOG_VPRINTF_FMT_VFLUSH;
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
                TE_LOG_VPRINTF_FMT_VFLUSH;
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

            default:
                modifier = '\0';
        }

        switch (*s)
        {
            case 'T':
                ++s;
                TE_LOG_VPRINTF_FMT_VFLUSH;
                if (*s == 'm')
                {
                    const uint8_t  *base;
                    unsigned int    len;
                    unsigned int    i;
                    
                    base = va_arg(ap, const uint8_t *);
                    len = va_arg(ap, unsigned int);
                    TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_MEM,
                                           base, len);
#if TE_HAVE_LOG_MSG_FMT
                    if (out->fmt != NULL)
                    {
                        TE_LOG_VPRINTF_FMT_FLUSH("\n");
                        for (i = 0; i < len; i++)
                        {
                            TE_LOG_VPRINTF_FMT_FLUSH("%02hhX", base[i]);
                            if ((i & 0xf) == 0xf)
                            {
                                TE_LOG_VPRINTF_FMT_FLUSH("\n");
                            }
                            else 
                            {
                                TE_LOG_VPRINTF_FMT_FLUSH(" ");
                            }
                        }
                        TE_LOG_VPRINTF_FMT_FLUSH("\n");
                    }
                    fmt_start = s + 1;
#endif
                }
                else if (*s == 'f')
                {
                    const char *filename = va_arg(ap, const char *);
                    
                    TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_FILE,
                                           filename, 0);
#if TE_HAVE_LOG_MSG_FMT
                    {
                        FILE   *fp;
                        char    buf[1024];

                        if ((fp = fopen(filename, "r")) == NULL)
                        {
                            TE_LOG_VPRINTF_FMT_FLUSH(
                                " CANNOT OPEN FILE %s ", filename);
                        }
                        else
                        {
                            while (fgets(buf, sizeof(buf), fp) != NULL)
                            {
                                TE_LOG_VPRINTF_FMT_FLUSH("%s", buf);
                            }
                            (void)fclose(fp);
                        }
                    }
                    fmt_start = s + 1;
#endif
                }
#if TE_HAVE_LOG_MSG_FMT
                else
                {
                    *fmt_start = '%';
                }
#endif
                break;

            case 'r':
            {
                te_errno arg;

                TE_LOG_VPRINTF_FMT_VFLUSH;
                arg = va_arg(ap, te_errno);
                TE_LOG_VPRINTF_RAW_ARG(TE_LOG_MSG_FMT_ARG_INT,
                                       &arg, sizeof(arg));
#if TE_HAVE_LOG_MSG_FMT
                if (TE_RC_GET_MODULE(arg) == 0)
                {
                     TE_LOG_VPRINTF_FMT_FLUSH("%s", te_rc_err2str(arg));
                }
                else
                {
                     TE_LOG_VPRINTF_FMT_FLUSH("%s-%s",
                                              te_rc_mod2str(arg),
                                              te_rc_err2str(arg));
                }
                fmt_start = s + 1;
#endif
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

            case 'p':
                /* See note below for integer specifiers */
                if (modifier != '\0')
                {
                    /* TODO */
                }
                va_arg(ap, void *);
                break;

            case 'c':
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
                 * When mentioned issues are coped in RGT, it may be
                 * reasonable pass through TE raw log to avoid
                 * dependency on host's snprintf() (possibly very dummy)
                 * support.
                 */
                switch (modifier)
                {
                    case 'j':
                        va_arg(ap, intmax_t);
                        break;
                    case 't':
                        va_arg(ap, ptrdiff_t);
                        break;
                    case 'L':
                        va_arg(ap, long long);
                        break;
                    case 'l':
                        va_arg(ap, long);
                        break;
                    case '8':
                        va_arg(ap, int64_t);
                        break;
                    case '4':
                    case '2':
                    case '1':
                    case 'H':
                    case 'h':
                    case '\0':
                        va_arg(ap, int);
                        break;
                    default:
                        /* TODO */
                }
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
                        va_arg(ap, long double);
                        break;
                    case '\0':
                        va_arg(ap, double);
                        break;
                    default:
                        /* TODO */
                }
                break;

            default:
                /* TODO */
        }

#if TE_HAVE_LOG_MSG_FMT
        if (flushed)
            va_copy(ap_start, ap);
#endif
    }

#if TE_HAVE_LOG_MSG_FMT
    fmt_end = s;
#endif
    TE_LOG_VPRINTF_FMT_VFLUSH;

#if TE_HAVE_LOG_MSG_FMT
    free(fmt_dup);
#endif

#undef TE_LOG_VPRINTF_FMT_FLUSH
#undef TE_LOG_VPRINTF_FMT_VFLUSH

    return 0;
}

static const uint8_t te_log_version = TE_LOG_VERSION;

/**
  * Preprocess and output message to log with special features parsing
  *
  * @param out      Output parameters
  * @param level    Log levelt
  * @param ts_sec   Timestamp seconds
  * @param ts_usec  Timestamp microseconds
  * @param entity   Entity name
  * @param user     User name
  * @param fmt      Format string
  * @param ap       Arguments for the format string
  *
  * @return Error code (see te_errno.h)
  */
te_errno
te_log_message_raw_va(te_log_msg_out *out, te_log_level level,
                      te_log_ts_sec ts_sec, te_log_ts_usec ts_usec,
                      const char *entity, const char *user,
                      const char *fmt, va_list ap)
{
    te_log_msg_raw_arg(out, TE_LOG_MSG_FMT_ARG_INT,
                       &te_log_version, sizeof(te_log_version), TRUE);
    te_log_msg_raw_arg(out, TE_LOG_MSG_FMT_ARG_INT,
                       &level, sizeof(level), TRUE);
    te_log_msg_raw_arg(out, TE_LOG_MSG_FMT_ARG_INT,
                       &ts_sec, sizeof(ts_sec), TRUE);
    te_log_msg_raw_arg(out, TE_LOG_MSG_FMT_ARG_INT,
                       &ts_usec, sizeof(ts_usec), TRUE);
    te_log_msg_raw_arg(out, TE_LOG_MSG_FMT_ARG_INT,
                       &ts_usec, sizeof(ts_usec), TRUE);

    te_log_msg_raw_string(out, entity);
    te_log_msg_raw_string(out, user);
    te_log_msg_raw_string(out, fmt);

    te_log_vprintf(out, fmt, ap);

    te_log_msg_raw_arg(out, TE_LOG_MSG_FMT_ARG_EOR, NULL,
                       TE_LOG_RAW_EOR_LEN, FALSE);
}

/**
  * Preprocess and output message to log with special features parsing
  *
  * @param out      Output parameters
  * @param level    Log levelt
  * @param ts_sec   Timestamp seconds
  * @param ts_usec  Timestamp microseconds
  * @param entity   Entity name
  * @param user     User name
  * @param fmt      Format string
  * @param ap       Arguments for the format string
  *
  * @return Error code (see te_errno.h)
  */
te_errno
te_log_message_file_va(te_log_msg_out *out, te_log_level level,
                       te_log_ts_sec ts_sec, te_log_ts_usec ts_usec,
                       const char *entity, const char *user,
                       const char *fmt, va_list ap)
{
    FILE *f;

    if (out == NULL)
        return TE_EINVAL;

    f = TE_LOG_MSG_OUT_AS_FILE(out);

    fputc('\n', f);
    fputs(te_log_level2str(level), f);
    fputs("  ", f);
    fputs(entity, f);
    fputs("  ", f);
    fputs(user, f);
    fputs("  ", f);
    fprintf(f, "%u.%u", ts_sec, ts_usec);
    fputc('\n', f);

    te_log_vprintf(out, fmt, ap);

    fputc('\n', f);
}

te_errno
te_log_message_int(te_log_msg_out *out, te_log_level level,
                   te_log_ts_sec ts_sec, te_log_ts_usec ts_usec,
                   const char *entity, const char *user,
                   const char *fmt, ...)
{
    te_errno    rc;
    va_list     ap;

    va_start(ap, fmt);
    rc = te_log_message_raw_va(out, level, ts_sec, ts_usec, entity, user,
                           fmt, ap);
    va_end(ap);

    return rc;
}
te_errno
te_log_message_int2(te_log_msg_out *out, te_log_level level,
                   te_log_ts_sec ts_sec, te_log_ts_usec ts_usec,
                   const char *entity, const char *user,
                   const char *fmt, ...)
{
    te_errno    rc;
    va_list     ap;

    va_start(ap, fmt);
    rc = te_log_message_file_va(out, level, ts_sec, ts_usec, entity, user,
                           fmt, ap);
    va_end(ap);

    return rc;
}

int
main(void)
{
    uint8_t buf[1000];
    te_log_msg_raw_data out = { te_log_msg_out_raw, NULL, 0, NULL };
    struct te_log_msg_fmt_to_file te_log_msg_to_stderr;
    uint8_t mem[] = { 1, 2, 4, 5 };

    te_log_msg_to_stderr.common = te_log_msg_out_file;
    te_log_msg_to_stderr.file   = stderr;

    out.buf = buf;
    out.len = 1000;
    out.ptr = buf;

    te_log_message_int(&out, 2, 1, 1, "Entity", "User",
                       "Log %d %Tm %10s %Tf",
                       10, mem, sizeof(mem), "kuku", "/etc/resolv.conf");
    te_log_message_int2(&te_log_msg_to_stderr, 1, 1, 1, "ENTITY", "USER",
                        "LOG=%Tm\n", out.buf, out.ptr - out.buf);
    return 0;
}
