/** @file
 * @brief TE log format string processing
 *
 * @defgroup te_tools_te_log_fmt Log format string processing
 * @ingroup te_tools
 * @{
 *
 * Some TE-specific features, such as memory dump, file content logging,
 * and additional length modifiers are supported.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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

/**
 * Preprocess and output message to log with special features parsing.
 *
 * @param out      Output parameters
 * @param fmt      Format string
 * @param ap       Arguments for the format string
 *
 * @return Error code (see te_errno.h)
 */
extern te_errno te_log_vprintf(te_log_msg_out *out,
                               const char *fmt, va_list ap);




/* Raw log version of backend common parameters */
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
    te_bool             trunc;      /**< Is log message truncated? */
} te_log_msg_raw_data;

/** Information about truncated part of raw log argument. */
typedef struct te_log_msg_truncated {
    union {
        const void  *addr; /**< Pointer argument for MEM type. */
        uint64_t     fd;   /**< File descriptor. */
    } data;                /**< Type specific data. */
    size_t        len;     /**< Length of the truncated part. */
    const char   *fmt;     /**< Format of the message. */
    size_t        fmt_len; /**< Format length. */
} te_log_msg_truncated;


/**
 *
 *
 * @param data     Output parameters
 * @param ts_sec   Timestamp seconds
 * @param ts_usec  Timestamp microseconds
 * @param level    Log levelt
 * @param log_id   Test ID or TE_LOG_ID_UNDEFINED
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
                                      te_log_id            log_id,
                                      const char          *entity,
                                      const char          *user,
                                      const char          *fmt,
                                      va_list              ap);

/**
 * Print a given string in log, splitting it in multiple messages
 * if it is too long to fit in a single one.
 *
 * @note This function does not parse TE format specifiers like '%r',
 *       and involves some processing overhead. So use it only when
 *       it is necessary.
 *       This function will try to split the string at '\n' symbols
 *       (removing them from the beginning and the end of each message).
 *       If it is not possible, it will cut the string at arbitrary
 *       positions.
 *
 * @param file        Name of file where this function is called
 *                    (use __FILE__)
 * @param line        Number of line where this function is called
 *                    (use __LINE__)
 * @param level       Log level
 * @param entity      Logger entity
 * @param user        Logger user
 * @param fmt         Format string
 * @param ...         Format string arguments
 *
 * @return Status code
 */
extern te_errno te_log_message_split(const char *file, unsigned int line,
                                     unsigned int level, const char *entity,
                                     const char *user,
                                     const char *fmt, ...);

#endif /* !__TE_LOG_FMT_H__ */
/**@} <!-- END te_tools_te_log_fmt --> */
