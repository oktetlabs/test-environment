/** @file
 * @brief Wrapper for Google Test
 *
 * @defgroup tapi_gtest GTest support
 * @ingroup te_ts_tapi
 * @{
 *
 * Test API for run Google Test binaries
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TAPI_GTEST_H__
#define __TAPI_GTEST_H__

#include "tapi_job.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Internal implementation struct */
typedef struct tapi_gtest_impl {
    tapi_job_t *job;                /**< GTest job handler */
    tapi_job_channel_t *out[2];     /**< GTest stdour/stderr channels */
} tapi_gtest_impl;

/** Defaults for implementation struct */
#define TAPI_GTEST_IMPL_DEFAULTS (tapi_gtest_impl)  \
{                                                   \
    .job = NULL,                                    \
    .out = {0},                                     \
}

/** GTest handler */
typedef struct tapi_gtest {
    const char *bin;        /**< Path to GTest binary */
    const char *group;      /**< Group name in GTest */
    const char *name;       /**< Test name in GTest */

    te_bool run_disabled;   /**< Force run disabled test */
    int rand_seed;          /**< Random seed */

    tapi_gtest_impl impl;   /**< Internal implementation struct */
} tapi_gtest;

/** Defaults for implementation for GTest handler */
#define TAPI_GTEST_DEFAULTS (tapi_gtest)    \
{                                           \
    .bin = NULL,                            \
    .group = NULL,                          \
    .name = NULL,                           \
    .run_disabled = FALSE,                  \
    .impl = TAPI_GTEST_IMPL_DEFAULTS,       \
}

/** A way for read gtest option from test arguments */
#define TEST_GTEST_PARAM(_gtest) (tapi_gtest)    \
{                                               \
    .bin = TEST_STRING_PARAM(_gtest##_bin),      \
    .group = TEST_STRING_PARAM(_gtest##_group),  \
    .name = TEST_STRING_PARAM(_gtest##_name),    \
    .rand_seed = TEST_INT_PARAM(te_rand_seed),  \
    .run_disabled = FALSE,                      \
    .impl = TAPI_GTEST_IMPL_DEFAULTS,           \
}

/** A way for read gtest option from test arguments */
#define TEST_GET_GTEST_PARAM(_gtest) \
    (_gtest) = TEST_GTEST_PARAM(_gtest)

/**
 * Create GTest
 *
 * @param gtest     GTest handler
 * @param factory   Job factory
 *
 * @return Status code
 */
extern te_errno tapi_gtest_init(tapi_gtest *gtest, tapi_job_factory_t *factory);

/**
 * Start GTest
 *
 * @param gtest     GTest handler
 *
 * @return Status code
 */
extern te_errno tapi_gtest_start(tapi_gtest *gtest);

/**
 * Stop GTest
 *
 * @param gtest     GTest handler
 *
 * @return Status code
 */
extern te_errno tapi_gtest_stop(tapi_gtest *gtest);

/**
 * Wait GTest and get result
 *
 * @param gtest         GTest handler
 * @param timeout_ms    Timeout for wait
 *
 * @return Status code
 */
extern te_errno tapi_gtest_wait(tapi_gtest *gtest, int timeout_ms);

/**
 * Cleanup GTest
 *
 * @param gtest         GTest handler
 *
 * @return Status code
 */
extern te_errno tapi_gtest_fini(tapi_gtest *gtest);

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
 * @page tapi-gtest-scenarios GTest TAPI usage scenarios
 *
 * Wrap GTest
 * ------------
 *
 * \code{.c}
 * int
 * main(int argc, char *argv[])
 * {
 *     tapi_gtest gtest;
 *     tapi_job_factory_t *factory;
 *
 *     TEST_START;
 *     TEST_GET_GTEST_PARAM(gtest);
 *
 *     CHECK_RC(tapi_job_factory_rpc_create(rpcs, &factory));
 *     CHECK_RC(tapi_gtest_init(&gtest, factory));
 *     CHECK_RC(tapi_gtest_start(&gtest));
 *     CHECK_RC(tapi_gtest_wait(&gtest, TE_SEC2MS(10)));
 *
 *     TEST_SUCCESS;
 *
 * cleanup:
 *     CLEANUP_CHECK_RC(tapi_gtest_fini(&gtest));
 *     tapi_job_factory_destroy(factory);
 *     TEST_END;
 * }
 * \endcode
 */

#endif /* __TAPI_GTEST_H__ */

/**@} <!-- END tapi_gtest --> */
