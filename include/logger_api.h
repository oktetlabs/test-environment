/** @file
 * @brief Logger API
 *
 * Definition of the C API provided by Logger subsystem to TE
 * subsystems and tests.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Igor Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_API_H__
#define __TE_LOGGER_API_H__

#include "te_stdint.h"
#include "logger_defs.h"


/**
 * Log message of the specified level from the user.
 *
 * @note This macro is intended for internal purposes and should not
 *       be used directly outside of this file.
 *
 * @param _lvl      - log level of the message
 * @param _lgruser  - log user name
 * @param _fs       - format string and arguments
 */
#define LGR_MESSAGE(_lvl, _lgruser, _fs...) \
    do {                                                   \
        if (LOG_LEVEL & (_lvl))                            \
        {                                                  \
            log_message(_lvl, LGR_ENTITY, _lgruser, _fs);  \
        }                                                  \
    } while (0)


/** @name Logging abnormal/unexpected situations */
#define LOG_ERROR(_us, _fs...)  LGR_MESSAGE(ERROR_LVL, _us, _fs)
#define ERROR(_fs...)           LOG_ERROR(LGR_USER, _fs)
/*@}*/

/** 
 * @name Logging situations same failed initialization of the optional
 *       feature 
 */
#define LOG_WARN(_us, _fs...)  LGR_MESSAGE(WARNING_LVL, _us, _fs)
#define WARN(_fs...)           LOG_WARN(LGR_USER, _fs)
/*@}*/

/**
 * @name Logging very important event in TE and tests required to
 *       undestand testing results
 */
#define LOG_RING(_us, _fs...)  LGR_MESSAGE(RING_LVL, _us, _fs)
#define RING(_fs...)           LOG_RING(LGR_USER, _fs)
/*@}*/

/** @name Logging important event for debugging of the test */
#define LOG_INFO(_us, _fs...)  LGR_MESSAGE(INFORMATION_LVL, _us, _fs)
#define INFO(_fs...)           LOG_INFO(LGR_USER, _fs)
/*@}*/

/** @name Logging additional events for detalization of processing */
#define LOG_VERB(_us, _fs...)  LGR_MESSAGE(VERBOSE_LVL, _us, _fs)
#define VERB(_fs...)           LOG_VERB(LGR_USER, _fs)
/*@}*/

/** @name Logging of entry to and exit from function */
#define _LOG_ENTRY(_us, _fs, _args...) \
    do {                            \
        log_message(ENTRY_EXIT_LVL, LGR_ENTITY, _us,                    \
                    "ENTRY to %s(): " _fs, __FUNCTION__, _args + 0);    \
    } while (0)

#define LOG_ENTRY(_us, _fs...) \
    do {                                                                \
        if (LOG_LEVEL & ENTRY_EXIT_LVL)                                 \
        {                                                               \
            if (!!(#_fs[0]))                                            \
            {                                                           \
                _LOG_ENTRY(_us, _fs);                                   \
            }                                                           \
            else                                                        \
            {                                                           \
                log_message(ENTRY_EXIT_LVL, LGR_ENTITY, _us,            \
                            "ENTRY to %s()", __FUNCTION__);       \
            }                                                           \
        }                                                               \
    } while (0)

#define ENTRY(_fs...)  LOG_ENTRY(LGR_USER, _fs)

#define _LOG_EXIT(_us, _fs, _args...) \
    do {                                                            \
        log_message(ENTRY_EXIT_LVL, LGR_ENTITY, _us,                \
                    "EXIT in line %d from %s(): " _fs,              \
                    __LINE__, __FUNCTION__, _args + 0);             \
    } while (0)
        

#define LOG_EXIT(_us, _fs...) \
    do {                                                            \
        if (LOG_LEVEL & ENTRY_EXIT_LVL)                             \
        {                                                           \
            if (!!(#_fs[0]))                                        \
            {                                                       \
                _LOG_EXIT(_us, _fs);                                \
            }                                                       \
            else                                                    \
            {                                                       \
                log_message(ENTRY_EXIT_LVL, LGR_ENTITY, _us,        \
                            "EXIT in line %d from %s()",            \
                            __LINE__, __FUNCTION__);                \
            }                                                       \
        }                                                           \
    } while (0)

#define EXIT(_fs...)  LOG_EXIT(LGR_USER, _fs)
/*@}*/


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create message and register it in the raw log file.
 *
 * @param level         Log level to be passed to the raw log
 * @param entity_name   Entity name whose user generates this message
 * @param user_name     Arbitrary "user name"
 * @param form_str      Log message format string. This string should 
 *                      contain conversion specifiers if some arguments
 *                      follows
 * @param ...           Arguments passed into the function according
 *                      to log message format string
 */
extern void log_message(uint16_t level, const char *entity_name,
                        const char *user_name, const char *form_str, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGGER_API_H__ */
