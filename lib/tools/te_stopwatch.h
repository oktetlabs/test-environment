/** @file
 * @brief Test API to operate the Stopwatch service
 *
 * @defgroup te_tools_te_stopwatch Stopwatch
 * @ingroup te_tools
 * @{
 *
 * Functions for time measurement
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * @section te_tools_te_stopwatch_example Example of usage
 *
 * Let's assume we need to measure some routine duration
 *
 * @code
 * #include "te_stopwatch.h"
 *
 * te_stopwatch_t stopwatch = TE_STOPWATCH_INIT;
 * struct timeval duration;
 *
 * CHECK_RC(te_stopwatch_start(&stopwatch));
 * RING("Some routine");
 * CHECK_RC(te_stopwatch_stop(&stopwatch, &duration));
 * @endcode
 */

#ifndef __TE_STOPWATCH_H__
#define __TE_STOPWATCH_H__

#include "te_defs.h"
#include "te_time.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Stopwatch context */
typedef struct te_stopwatch_t {
    te_bool is_running;         /**< Whether it is in progress or not */
    struct timeval start;       /**< Stopwatch start timestamp */
} te_stopwatch_t;

/** On-stack stopwatch context initializer */
#define TE_STOPWATCH_INIT { \
    .is_running = FALSE,    \
    .start = { 0 }          \
}

/**
 * Start stopwatch. It does not care if the stopwatch is already started,
 * it will restart it.
 *
 * @param stopwatch         Stopwatch handle
 *
 * @return Status code
 */
extern te_errno te_stopwatch_start(te_stopwatch_t *stopwatch);

/**
 * Stop stopwatch and calculate its duration
 *
 * @param[in]  stopwatch    Stopwatch handle
 * @param[out] lap          Overall time of stopwatch job
 *
 * @return Status code
 */
extern te_errno te_stopwatch_stop(te_stopwatch_t *stopwatch,
                                  struct timeval *lap);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_STOPWATCH_H__ */
/**@} <!-- END te_tools_te_stopwatch --> */