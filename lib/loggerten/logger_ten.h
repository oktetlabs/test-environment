/** @file
 * @brief Logger subsystem API - TEN side
 *
 * API provided by Logger subsystem to users on TEN side.
 * Set of macros provides compete functionality for debugging
 * and tracing and highly recommended for using in the source
 * code.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_LOGGER_TEN_H__
#define __TE_LOGGER_TEN_H__

#include <stdio.h>

#include "te_defs.h"
#include "logger_api.h"


#ifdef _cplusplus
extern "C" {
#endif


/** Compose log message and send it to TE Logger. */
extern te_log_message_f ten_log_message;


/** Discover name of the Logger IPC server */
static inline const char *
logger_server_name(void)
{
    static const char *logger_name = NULL;

    if ((logger_name == NULL) &&
        (logger_name = getenv("TE_LOGGER")) == NULL)
    {
        logger_name = "TE_LOGGER";
    }

    return logger_name;
}


/** Name of the Logger server */
#define LGR_SRV_NAME            (logger_server_name())

/** Type of IPC used for Logger TEN API <-> Logger server */
#define LOGGER_IPC              (FALSE) /* Connectionless IPC */


/** Discover name of the Logger client for TA */
static inline const char *
logger_ta_prefix(void)
{
    static char    prefix[64];
    static te_bool init = FALSE;

    if (!init)
    {
        snprintf(prefix, sizeof(prefix), "%s-ta-", LGR_SRV_NAME);
        init = TRUE;
    }

    return prefix;
}

/** Prefix of the name of the per Test Agent Logger server */
#define LGR_SRV_FOR_TA_PREFIX   logger_ta_prefix()

/** Discover name of the log flush client */
static inline const char *
logger_flush_name(void)
{
    static char    name[64];
    static te_bool init = FALSE;

    if (!init)
    {
        snprintf(name, sizeof(name), "%s-flush", LGR_SRV_NAME);
        init = TRUE;
    }

    return name;
}

/** Logger flush command */
#define LGR_FLUSH       logger_flush_name()

/**
 * Close IPC with Logger server and release resources.
 *
 * Usually user should not worry about calling of the function, since
 * it is called automatically using atexit() mechanism.
 */
extern void log_client_close(void);

/**
 * Pump out all log messages (only older than time of
 * this procedure calling) accumulated into the Test Agent local log
 * buffer/file and register it into the raw log file.
 *
 * @param ta_name   - the name of separate TA whose local log should
 *                    be flushed
 *
 * @return Status code
 */
extern int log_flush_ten(const char *ta_name);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_TEN_H__ */
