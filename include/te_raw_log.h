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


#ifdef __cplusplus
extern "C" {
#endif


/** Current log version */
#define TE_LOG_VERSION          1

/* Appropriate fields length in the raw log file format */
#define TE_LOG_NFL_SZ           2   /**< Next-Field-Length size */
#define TE_LOG_VERSION_SZ       1   /**< Version size */
#define TE_LOG_TIMESTAMP_SZ     8   /**< Size of timestamp */
#define TE_LOG_LEVEL_SZ         2   /**< Size of log level */

/** Length of the End-Of-Record is equal to maximum supported by NFL */
#define TE_LOG_RAW_EOR_LEN  ((1 << (sizeof(te_log_nfl) << 3)) - 1)
/** Actual maximum field length */
#define TE_LOG_FIELD_MAX    (TE_LOG_RAW_EOR_LEN - 1)


#if (TE_LOG_NFL_SZ == 1)
typedef uint8_t te_log_nfl;
#elif (TE_LOG_NFL_SZ == 2)
typedef uint16_t te_log_nfl;
#elif (TE_LOG_NFL_SZ == 4)
typedef uint32_t te_log_nfl;
#else
#error Such TE_LOG_NFL_SZ is not supported.
#endif

#if (TE_LOG_LEVEL_SZ == 1)
typedef uint8_t te_log_level_t;
#elif (TE_LOG_LEVEL_SZ == 2)
typedef uint16_t te_log_level_t;
#elif (TE_LOG_LEVEL_SZ == 4)
typedef uint32_t te_log_level_t;
#else
#error Such TE_LOG_LEVEL_SZ is not supported.
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RAW_LOG_H__ */
