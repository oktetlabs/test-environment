/** @file
 * @brief TE Raw Log Format Definitions
 *
 * Definitions for TE raw log format.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RAW_LOG_H__
#define __TE_RAW_LOG_H__

#include "te_stdint.h"


/* 
 * Raw log message format:
 *      NFL(Entity name)
 *      Entity name
 *      Log version
 *      Timestamp seconds
 *      Timestamp microseconds
 *      Log level
 *      NFL(User name)
 *      User name
 *      NFL(Format string)
 *      Format string
 *      NFL(arg1)
 *      arg1
 *      ...
 *      NFL(argN+1) = EOR
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Current TE log version */
#define TE_LOG_VERSION  1

/**
 * Log ID value that should be used when logging is being done from
 * Engine Applications and Test Agents.
 * In case of tests Tester passes "test ID" value as a command line
 * argument, which should be used as "Log ID" in all log messages.
 */
#define TE_LOG_ID_UNDEFINED  0

/** Type to store Next-Field-Length in raw log */
typedef uint16_t te_log_nfl;
/** Type to store TE log version in raw log */
typedef uint8_t  te_log_version;
/** Type to store timestamp seconds in raw log */
typedef uint32_t te_log_ts_sec;
/** Type to store timestamp microseconds in raw log */
typedef uint32_t te_log_ts_usec;
/** Type to store log level in raw log */
typedef uint16_t te_log_level;
/**
 * Type to store log ID in raw log.
 * Currently this field is used for detecting the test-owner of the log
 * message.
 */
typedef uint32_t te_log_id;
/** Type to store TE log sequence numbers in raw log */
typedef uint32_t te_log_seqno;


/** Length of the End-Of-Record is equal to maximum supported by NFL */
#define TE_LOG_RAW_EOR_LEN  ((1 << (sizeof(te_log_nfl) << 3)) - 1)
/** Actual maximum field length */
#define TE_LOG_FIELD_MAX    (TE_LOG_RAW_EOR_LEN - 1)


/**
 * Size of TE raw log message fields which do not use NFL.
 *
 * @attention In the case of TA it is necessary to add
 *            sizeof(te_log_seqno).
 */
#define TE_LOG_MSG_COMMON_HDR_SZ    (sizeof(te_log_version) + \
                                     sizeof(te_log_ts_sec) + \
                                     sizeof(te_log_ts_usec) + \
                                     sizeof(te_log_level))


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RAW_LOG_H__ */
