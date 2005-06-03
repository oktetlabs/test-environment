/** @file
 * @brief Logger subsystem API - TEN side 
 *
 * API provided by Logger subsystem to users on TEN side.
 * Set of macros provides compete functionality for debugging
 * and tracing and highly recommended for using in the source
 * code. 
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

#ifndef __TE_LOGGER_TEN_H__
#define __TE_LOGGER_TEN_H__
    
#ifdef _cplusplus
extern "C" {
#endif

#include "logger_api.h"

/** Discover name of the Logger IPC server */
static inline const char *
logger_server_name()
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
#define LGR_SRV_NAME            logger_server_name()

/** Discover name of the Logger client for TA */
static inline const char *
logger_ta_prefix()
{
    static char    prefix[64];
    static te_bool init = FALSE;
    
    if (!init)
    {
        sprintf(prefix, "%s-ta-", LGR_SRV_NAME);
        init = TRUE;
    }
    
    return prefix;
}

/** Prefix of the name of the per Test Agent Logger server */
#define LGR_SRV_FOR_TA_PREFIX   logger_ta_prefix()

/** Discover name of the log flush client */
static inline const char *
logger_flush_name()
{
    static char    name[64];
    static te_bool init = FALSE;
    
    if (!init)
    {
        sprintf(name, "%s-flush", LGR_SRV_NAME);
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
