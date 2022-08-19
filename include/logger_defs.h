/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Logger Common Definitions
 *
 * Some common definitions is used by Logger process library, TEN side
 * Logger lib, TA side Logger lib.
 *
 * DO NOT include this file directly.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LOGGER_DEFS_H__
#define __TE_LOGGER_DEFS_H__

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif

#include "te_raw_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Global variable with name of the Logger entity to be used from
 * libraries to log from this process context.
 *
 * @note It MUST be initialized to some value to avoid segmentation fault.
 */
extern const char *te_lgr_entity;

/*
 * Default value should be overriden by user in user' module before
 * inclusion of logger_api.h or like this:
 *     #undef   TE_LGR_ENTITY
 *     #define  TE_LGR_ENTITY <"SOMEENTITY">
 *
 * @attention It's highly not recommended to use/define it. Use te_log_init().
 */
#ifndef TE_LGR_ENTITY
/** Default entity name */
#define TE_LGR_ENTITY  te_lgr_entity
#endif

/*
 * Default value should be overriden by user in user' module before
 * inclusion of logger_api.h or like this:
 *     #undef   TE_LGR_USER
 *     #define  TE_LGR_USER <"SOMEUSER">
 */
#ifndef TE_LGR_USER
/** Default entity name */
#define TE_LGR_USER     "Self"
#endif

/** @group Reserved log users for logs nesting control
 * @{
 */
/** Reserved log user for #TEST_STEP() macro */
#define TE_USER_STEP         "Step"
/** Reserved log user for #TEST_SUBSTEP() macro */
#define TE_USER_SUBSTEP      "SubStep"
/** Reserved log user for #TEST_STEP_PUSH() macro */
#define TE_USER_STEP_PUSH    "StepPush"
/** Reserved log user for #TEST_STEP_POP() macro  */
#define TE_USER_STEP_POP     "StepPop"
/** Reserved log user for #TEST_STEP_NEXT() macro  */
#define TE_USER_STEP_NEXT    "StepNext"
/** Reserved log user for #TEST_STEP_RESET() macro  */
#define TE_USER_STEP_RESET    "StepReset"

/*@}*/

/*
 * 16-bit field is provided for TE_LOG_LEVEL masks
 */

/** Any abnormal/unexpected situation (ERROR macro) */
#define TE_LL_ERROR         0x0001

/** It's not error situation, but may be an error (WARN macro) */
#define TE_LL_WARN          0x0002

/** Very important event in TE and tests (RING macro) */
#define TE_LL_RING          0x0004

/** Important events required for test debugging (INFO macro) */
#define TE_LL_INFO          0x0008

/** Verbose logging of entity internals (VERB macro) */
#define TE_LL_VERB          0x0010

/*
 * Logging for function entry/exit point with function name
 * and line number (for exit point only). (ENTRY,EXIT macros)
 */
#define TE_LL_ENTRY_EXIT    0x0020

/** Events of network packet received (PACKET macro) */
#define TE_LL_PACKET        0x0040

/** Events of machine interface data received */
#define TE_LL_MI            0x0080

/**
 * Control message (this is a flag which can come together with
 * ERROR/WARN/RING).
 */
#define TE_LL_CONTROL       0x8000

/** @name A set of macros used for string representation of log level */
#define TE_LL_ERROR_STR       "ERROR"
#define TE_LL_WARN_STR        "WARN"
#define TE_LL_RING_STR        "RING"
#define TE_LL_INFO_STR        "INFO"
#define TE_LL_VERB_STR        "VERB"
#define TE_LL_ENTRY_EXIT_STR  "ENTRY/EXIT"
#define TE_LL_PACKET_STR      "PACKET"
#define TE_LL_MI_STR          "MI"
#define TE_LL_CONTROL_STR     "CONTROL"
/*@}*/

/*
 * Override default level macros into the user' module for logging
 * behaviour tunning at compilation time.
 * For example, into user' module the logging level can be modified
 * as follows:
 *
 *     #define TE_LOG_LEVEL   <level>
 *
 * Do NOT insert '#undef TE_LOG_LEVEL' before in order to see warning
 * during build and do not forget about overriden system wide
 * TE_LOG_LEVEL.
 */
#ifndef TE_LOG_LEVEL
/**
 * Default log level. Zero means printing only messages which
 * cannot be ignored: ERROR/WARN/RING/CONTROL.
 */
#define TE_LOG_LEVEL 0
#endif

/**
 * Log levels which are always enabled (whatever TE_LOG_LEVEL
 * is defined to).
 */
#define TE_LOG_LEVELS_MANDATORY \
    (TE_LL_ERROR | TE_LL_WARN | TE_LL_RING | TE_LL_CONTROL)

/**
 * Convert Log level value from integer to readable string.
 *
 * @param level   Log level value
 *
 * @return string literal pointer
 *
 * @note In case of unknown Log level function returns NULL, so
 * be careful using it.
 */
static inline const char *
te_log_level2str(te_log_level level)
{
    /*
     * Ignore TE_LL_CONTROL here unless it is
     * the only bit set.
     */
    if (level != TE_LL_CONTROL)
        level &= ~TE_LL_CONTROL;

    switch (level)
    {
#define TE_LL_CASE(lvl_) \
        case TE_LL_ ## lvl_:               \
            return TE_LL_ ## lvl_ ## _STR; \
            break

        TE_LL_CASE(ERROR);
        TE_LL_CASE(WARN);
        TE_LL_CASE(RING);
        TE_LL_CASE(INFO);
        TE_LL_CASE(VERB);
        TE_LL_CASE(ENTRY_EXIT);
        TE_LL_CASE(PACKET);
        TE_LL_CASE(MI);
        TE_LL_CASE(CONTROL);

#undef TE_LL_CASE
    }

    return NULL;
}

/**
 * Logging backend function.
 *
 * @param file      Name of the file with the log message
 * @param line      Line in the @a file with the log message
 * @param sec       Timestamp seconds
 * @param usec      Timestamp microseconds
 * @param level     Log level
 * @param entity    Entity name whose user generates this message
 * @param user      Arbitrary "user name"
 * @param fmt       Log message format string. This string should contain
 *                  conversion specifiers if some arguments follows
 * @param ap        Arguments passed into the function according
 *                  to log message format string
 */
typedef void (te_log_message_f)(const char       *file,
                                unsigned int      line,
                                te_log_ts_sec     sec,
                                te_log_ts_usec    usec,
                                unsigned int      level,
                                const char       *entity,
                                const char       *user,
                                const char       *fmt,
                                va_list           ap);

/** Logging backend */
extern te_log_message_f *te_log_message_va;


/**
 * Every TE process (engine application, test, test agent) must call
 * this function in its main to set 'te_lgr_entity' global variable
 * with specified initial value.
 *
 * @param lgr_entity    Logger entity name (e.g. RCF) or @c NULL
 * @param log_message   Log message backend function or @c NULL
 */
extern void te_log_init(const char *lgr_entity, te_log_message_f *log_message);


/**
 * Wrapper for te_log_message_va().
 *
 * All parameters are transparently forwarded to te_log_message_va()
 * which complies to te_log_message_f prototype.
 */
static inline void
te_log_message_ts(const char *file, unsigned int line,
                  te_log_ts_sec sec, te_log_ts_usec usec,
                  unsigned int level,
                  const char *entity, const char *user,
                  const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    te_log_message_va(file, line, sec, usec, level, entity, user,
                      fmt, ap);
    va_end(ap);
}

/**
 * Wrapper for te_log_message_va() to get log message timestamp.
 *
 * All parameters are transparently forwarded to te_log_message_va()
 * which complies to te_log_message_f prototype.
 */
static inline void
te_log_message(const char *file, unsigned int line, unsigned int level,
               const char *entity, const char *user, const char *fmt, ...)
{
    struct timeval  tv;
    va_list         ap;

    (void)gettimeofday(&tv, NULL);

    va_start(ap, fmt);
    te_log_message_va(file, line, tv.tv_sec, tv.tv_usec, level,
                      entity, user, fmt, ap);
    va_end(ap);
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGGER_DEFS_H__ */
