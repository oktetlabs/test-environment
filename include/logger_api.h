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

/** @defgroup logger_api API: Logger
 * In order to use @ref logger_api you need to include
 * <A href="logger__api_8h.html">logger_api.h</A> file into your sources.
 * 
 * If you want to use short versions of @ref logger_api functions you need
 * to define TE_LGR_USER macro at the beginning of source file.
 *
 * For example your code might look like:
 * @code
 * #define TE_LGR_USER  "My module"
 *
 * #include "te_config.h"
 * ...
 * #include "logger_api.h"
 * ...
 * ERROR("An error condition happens: %s", result_string);
 * @endcode
 *
 * @ingroup te_engine_logger
 * @{
 */

/**
 * @name Generic functions of frontend Logger API
 * Auxiliary @ref logger_api macros that you are unlikely to use
 * directly from your tests or other TE components.
 */

/**
 * Unconditional logging.
 *
 * @param _evel    - log level of the message
 * @param _entity  - log entity name
 * @param _user    - log user name
 * @param _fs      - format string and arguments
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

/**
 * Log message from default user with specified level.
 *
 * @param _lvl  - log level of the message
 * @param _fs   - format string and arguments
 */
#define LOG_MSG(_lvl, _fs...)   LGR_MESSAGE(_lvl, TE_LGR_USER, _fs)

/**@}*/

/**
 * @name Frontend Logger API that allows to specify log user name
 * Normally you would use short versions of these macros,
 * but you can use these interface if you need to log a message
 * with user name different from the default.
 */

/**
 * Logging abnormal/unexpected situations
 *
 * @param _us - log user name
 * @param _fs - format string and arguments
 */
#define TE_LOG_ERROR(_us, _fs...)   LGR_MESSAGE(TE_LL_ERROR, _us, _fs)

/**
 * Logging situations of some failed initialization of
 * the optional feature or some unexpect events that
 * does not break operation
 *
 * @param _us - log user name
 * @param _fs - format string and arguments
 */
#define TE_LOG_WARN(_us, _fs...)    LGR_MESSAGE(TE_LL_WARN, _us, _fs)

/**
 * Logging very important events in TE and tests required to
 * undestand testing results
 *
 * @param _us - log user name
 * @param _fs - format string and arguments
 */
#define TE_LOG_RING(_us, _fs...)    LGR_MESSAGE(TE_LL_RING, _us, _fs)

/**
 * Logging important event for debugging of the test
 *
 * @param _us - log user name
 * @param _fs - format string and arguments
 */
#define TE_LOG_INFO(_us, _fs...)    LGR_MESSAGE(TE_LL_INFO, _us, _fs)

/**
 * Logging additional events for detalization of processing
 *
 * @param _us - log user name
 * @param _fs - format string and arguments
 */
#define TE_LOG_VERB(_us, _fs...)    LGR_MESSAGE(TE_LL_VERB, _us, _fs)

/**
 * Auxiliary macro to insert __FUNCTION__ just after
 * format string before arguments.
 */
#define _LOG_ENTRY(_us, _fs, _args...) \
    TE_LOG(TE_LL_ENTRY_EXIT, TE_LGR_ENTITY, _us,                \
           "ENTRY to %s(): " _fs, __FUNCTION__, _args + 0)

/**
 * Logging of entry to a function
 *
 * @param _us - log user name
 * @param _fs - format string and arguments
 */
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

/**
 * Auxiliary macro to insert __LINE__ and __FUNCTION__ just after
 * format string before arguments.
 */
#define _LOG_EXIT(_us, _fs, _args...) \
    TE_LOG(TE_LL_ENTRY_EXIT, TE_LGR_ENTITY, _us,                \
           "EXIT in line %d from %s(): " _fs,                   \
           __LINE__, __FUNCTION__, _args + 0)

/**
 * Logging of exit from a function
 *
 * @param _us - log user name
 * @param _fs - format string and arguments
 */
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

/*@}*/

/**
 * @name Frontend Logger API
 * In order to use these functions a caller should provide definition
 * of TE_LGR_USER macro that specifies entity name that generated
 * the particular log.
 * TE_LGR_USER is a character string that will be later available
 * in resulting log file to determine log origins.
 */

/**
 * Logging abnormal/unexpected situations
 * with user-defined log user name TE_LGR_USER.
 *
 * @param _fs - format string and arguments
 */
#define ERROR(_fs...)               TE_LOG_ERROR(TE_LGR_USER, _fs)

/** 
 * Logging situations some failed initialization of
 * the optional feature
 *
 * @param _fs - format string and arguments
 */
#define WARN(_fs...)                TE_LOG_WARN(TE_LGR_USER, _fs)

/**
 * Logging very important events in TE and tests required to
 * undestand testing results
 *
 * @param _fs - format string and arguments
 */
#define RING(_fs...)                TE_LOG_RING(TE_LGR_USER, _fs)

/**
 * Logging important event for debugging of the test
 *
 * @param _fs - format string and arguments
 */
#define INFO(_fs...)                TE_LOG_INFO(TE_LGR_USER, _fs)

/**
 * Logging additional events for detalization of processing
 *
 * @param _fs - format string and arguments
 */
#define VERB(_fs...)                TE_LOG_VERB(TE_LGR_USER, _fs)

/**
 * Logging of entry to a function
 *
 * @param _fs - format string and arguments
 */
#define ENTRY(_fs...)  TE_LOG_ENTRY(TE_LGR_USER, _fs)

/**
 * Logging of exit from a function
 *
 * @param _fs - format string and arguments
 */
#define EXIT(_fs...)  TE_LOG_EXIT(TE_LGR_USER, _fs)

/*@}*/

/**@} <!-- END logger_api --> */

#endif /* !__TE_LOGGER_API_H__ */
