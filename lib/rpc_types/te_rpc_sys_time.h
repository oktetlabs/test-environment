/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/time.h.
 * 
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_SYS_TIME_H__
#define __TE_RPC_SYS_TIME_H__

#include "te_rpc_defs.h"
#include "tarpc.h"
#include "te_errno.h"


#ifdef __cplusplus
extern "C" {
#endif

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

/** IDs of various system clocks */
typedef enum rpc_clock_id {
    RPC_CLOCK_REALTIME,
    RPC_CLOCK_MONOTONIC,
    RPC_CLOCK_PROCESS_CPUTIME_ID,
    RPC_CLOCK_THREAD_CPUTIME_ID,
    RPC_CLOCK_MONOTONIC_RAW,
    RPC_CLOCK_REALTIME_COARSE,
    RPC_CLOCK_MONOTONIC_COARSE,
    RPC_CLOCK_BOOTTIME,
    RPC_CLOCK_REALTIME_ALARM,
    RPC_CLOCK_BOOTTIME_ALARM,
} rpc_clock_id;

/**
 * Convert RPC clock ID to native one.
 *
 * @param id      RPC clock ID
 *
 * @return Native clock ID on success, @c -1 on failure.
 */
extern int clock_id_rpc2h(rpc_clock_id id);

/**
 * Get string representation of clock ID.
 *
 * @param id        RPC clock ID
 *
 * @return Pointer to string constant.
 */
extern const char *clock_id_rpc2str(rpc_clock_id id);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_TIME_H__ */
