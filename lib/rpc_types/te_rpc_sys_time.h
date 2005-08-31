/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/time.h.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_SYS_TIME_H__
#define __TE_RPC_SYS_TIME_H__

#include "te_rpc_defs.h"

/**
 * Convert 'struct timeval' to 'struct tarpc_timeval'.
 * 
 * @param tv_h      Pointer to 'struct timeval'
 * @param tv_rpc    Pointer to 'struct tarpc_timeval'
 */
static inline int
timeval_h2rpc(const struct timeval *tv_h, struct tarpc_timeval *tv_rpc)
{
    tv_rpc->tv_sec  = tv_h->tv_sec;
    tv_rpc->tv_usec = tv_h->tv_usec;

    return (tv_h->tv_sec  != tv_rpc->tv_sec ||
            tv_h->tv_usec != tv_rpc->tv_usec) ?
               TE_RC(TE_TA, TE_EH2RPC) : 0;
}

/**
 * Convert 'struct tarpc_timeval' to 'struct timeval'.
 * 
 * @param tv_rpc    Pointer to 'struct tarpc_timeval'
 * @param tv_h      Pointer to 'struct timeval'
 */
static inline int
timeval_rpc2h(const struct tarpc_timeval *tv_rpc, struct timeval *tv_h)
{
    tv_h->tv_sec  = tv_rpc->tv_sec;
    tv_h->tv_usec = tv_rpc->tv_usec;

    return (tv_h->tv_sec  != tv_rpc->tv_sec ||
            tv_h->tv_usec != tv_rpc->tv_usec) ?
               TE_RC(TE_TA, TE_ERPC2H) : 0;
}

/**
 * Convert 'struct timezone' to 'struct tarpc_timezone'.
 * 
 * @param tz_h      Pointer to 'struct timezone'
 * @param tz_rpc    Pointer to 'struct tarpc_timezone'
 */
static inline int
timezone_h2rpc(const struct timezone *tz_h, struct tarpc_timezone *tz_rpc)
{
    tz_rpc->tz_minuteswest = tz_h->tz_minuteswest;
    tz_rpc->tz_dsttime     = tz_h->tz_dsttime;

    return (tz_h->tz_minuteswest != tz_rpc->tz_minuteswest ||
            tz_h->tz_dsttime     != tz_rpc->tz_dsttime) ?
               TE_RC(TE_TA, TE_EH2RPC) : 0;
}

/**
 * Convert 'struct tarpc_timezone' to 'struct timezone'.
 * 
 * @param tz_rpc    Pointer to 'struct tarpc_timezone'
 * @param tz_h      Pointer to 'struct timezone'
 */
static inline int
timezone_rpc2h(const struct tarpc_timezone *tz_rpc, struct timezone *tz_h)
{
    tz_h->tz_minuteswest = tz_rpc->tz_minuteswest;
    tz_h->tz_dsttime     = tz_rpc->tz_dsttime;

    return (tz_h->tz_minuteswest != tz_rpc->tz_minuteswest ||
            tz_h->tz_dsttime     != tz_rpc->tz_dsttime) ?
               TE_RC(TE_TA, TE_ERPC2H) : 0;
}

#endif /* !__TE_RPC_SYS_TIME_H__ */
