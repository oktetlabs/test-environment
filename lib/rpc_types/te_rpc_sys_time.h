/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/time.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_SYS_TIME_H__
#define __TE_RPC_SYS_TIME_H__

#include "te_rpc_defs.h"
#include "tarpc.h"
#include "te_errno.h"
#include "te_string.h"


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

/** Mode flags in timex structure (see man clock_adjtime) */
typedef enum rpc_adj_mode {
    RPC_ADJ_OFFSET = (1 << 0),
    RPC_ADJ_FREQUENCY = (1 << 1),
    RPC_ADJ_MAXERROR = (1 << 2),
    RPC_ADJ_ESTERROR = (1 << 3),
    RPC_ADJ_STATUS = (1 << 4),
    RPC_ADJ_TIMECONST = (1 << 5),
    RPC_ADJ_TAI = (1 << 6),
    RPC_ADJ_SETOFFSET = (1 << 7),
    RPC_ADJ_MICRO = (1 << 8),
    RPC_ADJ_NANO = (1 << 9),
    RPC_ADJ_TICK = (1 << 10),

    RPC_ADJ_UNKNOWN = (1 << 31)
} rpc_adj_mode;

#define ADJ_MODE_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(ADJ_OFFSET),           \
    RPC_BIT_MAP_ENTRY(ADJ_FREQUENCY),        \
    RPC_BIT_MAP_ENTRY(ADJ_MAXERROR),         \
    RPC_BIT_MAP_ENTRY(ADJ_ESTERROR),         \
    RPC_BIT_MAP_ENTRY(ADJ_STATUS),           \
    RPC_BIT_MAP_ENTRY(ADJ_TIMECONST),        \
    RPC_BIT_MAP_ENTRY(ADJ_TAI),              \
    RPC_BIT_MAP_ENTRY(ADJ_SETOFFSET),        \
    RPC_BIT_MAP_ENTRY(ADJ_MICRO),            \
    RPC_BIT_MAP_ENTRY(ADJ_NANO),             \
    RPC_BIT_MAP_ENTRY(ADJ_TICK),             \
    RPC_BIT_MAP_ENTRY(ADJ_UNKNOWN)

/** adj_mode_flags_rpc2str() */
RPCBITMAP2STR(adj_mode_flags, ADJ_MODE_MAPPING_LIST)

/**
 * Convert RPC timex mode flags to native ones.
 *
 * @param flags       RPC flags
 *
 * @return Native flags.
 */
extern unsigned int adj_mode_flags_rpc2h(unsigned int flags);

/**
 * Convert native timex mode flags to RPC ones.
 *
 * @param flags       Native flags
 *
 * @return RPC flags.
 */
extern unsigned int adj_mode_flags_h2rpc(unsigned int flags);

/** Status flags in timex structure (see man clock_adjtime) */
typedef enum rpc_timex_status {
    RPC_STA_PLL = (1 << 0),
    RPC_STA_PPSFREQ = (1 << 1),
    RPC_STA_PPSTIME = (1 << 2),
    RPC_STA_FLL = (1 << 3),
    RPC_STA_INS = (1 << 4),
    RPC_STA_DEL = (1 << 5),
    RPC_STA_UNSYNC = (1 << 6),
    RPC_STA_FREQHOLD = (1 << 7),
    RPC_STA_PPSSIGNAL = (1 << 8),
    RPC_STA_PPSJITTER = (1 << 9),
    RPC_STA_PPSWANDER = (1 << 10),
    RPC_STA_PPSERROR = (1 << 11),
    RPC_STA_CLOCKERR = (1 << 12),
    RPC_STA_NANO = (1 << 13),
    RPC_STA_MODE = (1 << 14),
    RPC_STA_CLK = (1 << 15),

    RPC_STA_UNKNOWN = (1 << 31)
} rpc_timex_status;

#define TIMEX_STATUS_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(STA_PLL),         \
    RPC_BIT_MAP_ENTRY(STA_PPSFREQ),     \
    RPC_BIT_MAP_ENTRY(STA_PPSTIME),     \
    RPC_BIT_MAP_ENTRY(STA_FLL),         \
    RPC_BIT_MAP_ENTRY(STA_INS),         \
    RPC_BIT_MAP_ENTRY(STA_DEL),         \
    RPC_BIT_MAP_ENTRY(STA_UNSYNC),      \
    RPC_BIT_MAP_ENTRY(STA_FREQHOLD),    \
    RPC_BIT_MAP_ENTRY(STA_PPSSIGNAL),   \
    RPC_BIT_MAP_ENTRY(STA_PPSJITTER),   \
    RPC_BIT_MAP_ENTRY(STA_PPSWANDER),   \
    RPC_BIT_MAP_ENTRY(STA_PPSERROR),    \
    RPC_BIT_MAP_ENTRY(STA_CLOCKERR),    \
    RPC_BIT_MAP_ENTRY(STA_NANO),        \
    RPC_BIT_MAP_ENTRY(STA_MODE),        \
    RPC_BIT_MAP_ENTRY(STA_CLK),         \
    RPC_BIT_MAP_ENTRY(STA_UNKNOWN)

/** timex_status_flags_rpc2str() */
RPCBITMAP2STR(timex_status_flags, TIMEX_STATUS_MAPPING_LIST)

/**
 * Convert RPC timex status flags to native ones.
 *
 * @param flags       RPC flags
 *
 * @return Native flags.
 */
extern unsigned int timex_status_flags_rpc2h(unsigned int flags);

/**
 * Convert native timex status flags to RPC ones.
 *
 * @param flags       Native flags
 *
 * @return RPC flags.
 */
extern unsigned int timex_status_flags_h2rpc(unsigned int flags);

/**
 * Append string representation of tarpc_timex structure to TE string.
 *
 * @param val   Pointer to tarpc_timex structure
 * @param str   Pointer to TE string
 *
 * @return Status code.
 */
extern te_errno timex_tarpc2te_str(const tarpc_timex *val, te_string *str);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_TIME_H__ */
