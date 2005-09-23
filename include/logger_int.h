/** @file
 * @brief Logging Subsystem Internal Definitions
 *
 * Definitions common for parts of logging subsystem but not required
 * for other TE entities.
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

#ifndef __TE_LOGGER_INT_H__
#define __TE_LOGGER_INT_H__

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/** Convert value to the network order */
#define LGR_16_TO_NET(_val, _res) \
    (((int8_t *)(_res))[0] = (int8_t)(((_val)>>8) & 0xff), \
     ((int8_t *)(_res))[1] = (int8_t)((_val) & 0xff))

#define LGR_32_TO_NET(_val, _res) \
    (((int8_t *)(_res))[0] = (int8_t)(((_val)>>24) & 0xff), \
     ((int8_t *)(_res))[1] = (int8_t)(((_val)>>16) & 0xff), \
     ((int8_t *)(_res))[2] = (int8_t)(((_val)>>8) & 0xff),  \
     ((int8_t *)(_res))[3] = (int8_t)((_val) & 0xff))


#define LGR_16_PUT(val_, buf_) \
    do {                            \
        uint16_t    nfl_ = (val_);  \
                                    \
        LGR_16_TO_NET(nfl_, buf_);  \
        (buf_) += 2;                \
    } while (0)

#if (SIZEOF_TE_LOG_NFL == 2)
#define LGR_NFL_PUT(val_, buf_)     LGR_16_PUT(val_, buf_)
#else
#error LGR_NFL_PUT is not defined
#endif

#if (SIZEOF_TE_LOG_LEVEL == 2)
#define LGR_LEVEL_PUT(val_, buf_)   LGR_16_PUT(val_, buf_)
#else
#error LGR_LEVEL_PUT is not defined
#endif

#if (SIZEOF_TE_LOG_TS_SEC + SIZEOF_TE_LOG_TS_USEC == 8)
#define LGR_TIMESTAMP_PUT(ts_, buf_) \
    do {                                        \
        C_ASSERT(sizeof((ts_)->tv_sec) == 4);   \
        LGR_32_TO_NET((ts_)->tv_sec, buf_);     \
        (buf_) += 4;                            \
        C_ASSERT(sizeof((ts_)->tv_usec) == 4);  \
        LGR_32_TO_NET((ts_)->tv_usec, buf_);    \
        (buf_) += 4;                            \
    } while (0)
#else
#error LGR_TIMESTAMP_PUT is not defined
#endif


/* ==== Test Agent Logger lib definitions */

/*
 * If it's needed override default value to release oldest message
 * and register new one when unused ring buffer resources is absent
 * as following:
 *  #undef LGR_RB_FORCE
 *  #define LGR_RB_FORCE 1
 */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGGER_INT_H__ */
