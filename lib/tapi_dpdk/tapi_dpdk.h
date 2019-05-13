/** @file
 * @brief DPDK helper functions TAPI
 *
 * @defgroup tapi_dpdk DPDK helper functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle DPDK-related operations
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TAPI_DPDK_H__
#define __TAPI_DPDK_H__

#include "te_errno.h"
#include "te_string.h"
#include "tapi_job.h"
#include "tapi_env.h"
#include "tapi_cfg_cpu.h"
#include "te_kvpair.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_DPDK_TESTPMD_ARG_PREFIX "testpmd_arg_"
#define TAPI_DPDK_TESTPMD_COMMAND_PREFIX "testpmd_command_"
#define TAPI_DPDK_TESTPMD_TERM_TIMEOUT_MS 60000
#define TAPI_DPDK_TESTPMD_RECEIVE_TIMEOUT_MS 60000

typedef struct tapi_dpdk_testpmd_job_t {
    char *ta;
    char *cmdline_file;
    te_string cmdline_setup;
    te_string cmdline_start;
    unsigned int port_number;
    tapi_job_t *job;
    tapi_job_channel_t *in_channel;
    tapi_job_channel_t *out_channels[2];
    tapi_job_channel_t *err_filter;
    tapi_job_channel_t *tx_pps_filter;
    tapi_job_channel_t *rx_pps_filter;
} tapi_dpdk_testpmd_job_t;

typedef struct tapi_dpdk_testpmd_stats_t {
    uint64_t tx_pps;
    uint64_t rx_pps;
} tapi_dpdk_testpmd_stats_t;


/**
 * Create a job for test-pmd binary execution.
 * The created job can be manipulated with other tapi_dpdk functions as
 * well as passed to generic tapi_job functions.
 *
 * @note @p prop is advisory
 *
 * @param rpcs              RPC server to run test-pmd on
 * @param env               Test environment
 * @param n_cpus            Number of CPUs to grab for test-pmd to execute on
 * @param prop              Advisory properties of CPUs
 * @param test_args         Test arguments
 * @param[out] testpmd_job  Handle of test-pmd job
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_create_testpmd_job(rcf_rpc_server *rpcs,
                                         tapi_env *env,
                                         size_t n_cpus,
                                         const tapi_cpu_prop_t *prop,
                                         te_kvpair_h *test_args,
                                         tapi_dpdk_testpmd_job_t *testpmd_job);

/**
 * Start test-pmd job
 *
 * @param testpmd_job       Handle of test-pmd job
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_testpmd_start(tapi_dpdk_testpmd_job_t *testpmd_job);

/**
 * Get performance statistics from running test-pmd job.
 *
 * @note The @p testpmd_job must be started.
 *
 * @param testpmd_job   Handle of running test-pmd job
 * @param n_stats       Number of times to get statistics from test-pmd
 * @param[out] stats    Evaluated statistics
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_testpmd_get_stats(tapi_dpdk_testpmd_job_t *testpmd_job,
                                            unsigned int n_stats,
                                            tapi_dpdk_testpmd_stats_t *stats);

/**
 * Destroy test-pmd job
 *
 * @param testpmd_job       Handle of test-pmd job
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_testpmd_destroy(tapi_dpdk_testpmd_job_t *testpmd_job);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_DPDK_H__ */

/**@} <!-- END tapi_dpdk --> */
