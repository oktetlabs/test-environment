/** @file
 * @brief TE tools
 *
 * @defgroup te_tools_te_sleep Sleep
 * @ingroup te_tools
 * @{
 *
 * Functions for different delays.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_SLEEP_H__
#define __TE_SLEEP_H__

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "logger_api.h"

/**
 * Sleep specified number of seconds.
 *
 * @param to_sleep      number of seconds to sleep
 */
static inline void
te_sleep(unsigned int to_sleep)
{
    RING("Sleeping %u seconds", to_sleep);
    (void)sleep(to_sleep);
}

/**
 * Sleep specified number of microseconds.
 *
 * @param to_sleep      Number of microseconds to sleep
 * @param why           String that describes reason to sleep here.
 */
static inline void
te_motivated_usleep(unsigned int to_sleep, const char *why)
{
    RING("Sleeping %u microseconds: %s", to_sleep, why);
    (void)usleep(to_sleep);
}

/**
 * Sleep specified number of milliseconds.
 *
 * @param to_sleep      Number of milliseconds to sleep
 * @param why           String that describes reason to sleep here.
 */
static inline void
te_motivated_msleep(unsigned int to_sleep, const char *why)
{
    RING("Sleeping %u milliseconds: %s", to_sleep, why);
    (void)usleep(to_sleep * 1000);
}

/**
 * Sleep specified number of seconds.
 *
 * @param to_sleep      Number of seconds to sleep
 * @param why           String that describes reason to sleep here.
 */
static inline void
te_motivated_sleep(unsigned int to_sleep, const char *why)
{
    RING("Sleeping %u seconds: %s", to_sleep, why);
    (void)sleep(to_sleep);
}

/**
 * Sleep specified number of milliseconds.
 *
 * @param to_sleep      number of milliseconds to sleep
 */
static inline void
te_msleep(unsigned int to_sleep)
{
    RING("Sleeping %u milliseconds", to_sleep);
    (void)usleep(to_sleep * 1000);
}

/**
 * Sleep specified number of microseconds.
 *
 * @param to_sleep      number of milliseconds to sleep
 */
static inline void
te_usleep(unsigned int to_sleep)
{
    RING("Sleeping %u microseconds", to_sleep);
    (void)usleep(to_sleep);
}

/**
 * Substract tv1 to tv2 of type struct timeval.
 *
 * @param tv1
 * @param tv2
 *
 * @return Diff in microseconds
 */
#define TIMEVAL_SUB(_tv1, _tv2) \
    (((_tv1).tv_sec - (_tv2).tv_sec) * 1000000L + \
     ((_tv1).tv_usec - (_tv2).tv_usec))

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_SLEEP_H__ */
/**@} <!-- END te_tools_te_sleep --> */
