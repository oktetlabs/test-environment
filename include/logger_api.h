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
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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

#include "logger_defs.h"


/**
 * Unconditional logging.
 */
#define TE_LOG(_level, _entity, _user, _fs...) \
    te_log_message(__FILE__, __LINE__, _level, _entity, _user, _fs)
        

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
    do {                                                \
        if (TE_LOG_LEVEL & (_lvl))                      \
        {                                               \
            TE_LOG(_lvl, TE_LGR_ENTITY, _lgruser, _fs); \
        }                                               \
    } while (0)

/** Log message from default user with specified level. */
#define LOG_MSG(_lvl, _fs...)   LGR_MESSAGE(_lvl, TE_LGR_USER, _fs)

/** @name Logging abnormal/unexpected situations */
#define TE_LOG_ERROR(_us, _fs...)   LGR_MESSAGE(TE_LL_ERROR, _us, _fs)
#define ERROR(_fs...)               TE_LOG_ERROR(TE_LGR_USER, _fs)
/*@}*/

/** 
 * @name Logging situations same failed initialization of the optional
 *       feature 
 */
#define TE_LOG_WARN(_us, _fs...)    LGR_MESSAGE(TE_LL_WARN, _us, _fs)
#define WARN(_fs...)                TE_LOG_WARN(TE_LGR_USER, _fs)
/*@}*/

/**
 * @name Logging very important event in TE and tests required to
 *       undestand testing results
 */
#define TE_LOG_RING(_us, _fs...)    LGR_MESSAGE(TE_LL_RING, _us, _fs)
#define RING(_fs...)                TE_LOG_RING(TE_LGR_USER, _fs)
/*@}*/

/** @name Logging important event for debugging of the test */
#define TE_LOG_INFO(_us, _fs...)    LGR_MESSAGE(TE_LL_INFO, _us, _fs)
#define INFO(_fs...)                TE_LOG_INFO(TE_LGR_USER, _fs)
/*@}*/

/** @name Logging additional events for detalization of processing */
#define TE_LOG_VERB(_us, _fs...)    LGR_MESSAGE(TE_LL_VERB, _us, _fs)
#define VERB(_fs...)                TE_LOG_VERB(TE_LGR_USER, _fs)
/*@}*/

/** @name Logging of entry to and exit from function */
/**
 * Auxiliary macro to insert __FUNCTION__ just after
 * format string before arguments.
 */
#define _LOG_ENTRY(_us, _fs, _args...) \
    TE_LOG(TE_LL_ENTRY_EXIT, TE_LGR_ENTITY, _us,                \
           "ENTRY to %s(): " _fs, __FUNCTION__, _args + 0)

#define TE_LOG_ENTRY(_us, _fs...) \
    do {                                                        \
        if (TE_LOG_LEVEL & TE_LL_ENTRY_EXIT)                    \
        {                                                       \
            if (!!(#_fs[0]))                                    \
            {                                                   \
                _LOG_ENTRY(_us, _fs);                           \
            }                                                   \
            else                                                \
            {                                                   \
                TE_LOG(TE_LL_ENTRY_EXIT, TE_LGR_ENTITY, _us,    \
                       "ENTRY to %s()", __FUNCTION__);          \
            }                                                   \
        }                                                       \
    } while (0)

#define ENTRY(_fs...)  TE_LOG_ENTRY(TE_LGR_USER, _fs)

/**
 * Auxiliary macro to insert __LINE__ and __FUNCTION__ just after
 * format string before arguments.
 */
#define _LOG_EXIT(_us, _fs, _args...) \
    TE_LOG(TE_LL_ENTRY_EXIT, TE_LGR_ENTITY, _us,                \
           "EXIT in line %d from %s(): " _fs,                   \
           __LINE__, __FUNCTION__, _args + 0)

#define TE_LOG_EXIT(_us, _fs...) \
    do {                                                        \
        if (TE_LOG_LEVEL & TE_LL_ENTRY_EXIT)                    \
        {                                                       \
            if (!!(#_fs[0]))                                    \
            {                                                   \
                _LOG_EXIT(_us, _fs);                            \
            }                                                   \
            else                                                \
            {                                                   \
                TE_LOG(TE_LL_ENTRY_EXIT, TE_LGR_ENTITY, _us,    \
                       "EXIT in line %d from %s()",             \
                       __LINE__, __FUNCTION__);                 \
            }                                                   \
        }                                                       \
    } while (0)

#define EXIT(_fs...)  TE_LOG_EXIT(TE_LGR_USER, _fs)
/*@}*/


#endif /* !__TE_LOGGER_API_H__ */
