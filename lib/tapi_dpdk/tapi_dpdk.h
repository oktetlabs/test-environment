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
#include "te_meas_stats.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Name of testpmd tool */
#define TAPI_DPDK_TESTPMD_NAME "testpmd"
#define TAPI_DPDK_TESTPMD_ARG_PREFIX TAPI_DPDK_TESTPMD_NAME "_arg_"
#define TAPI_DPDK_TESTPMD_COMMAND_PREFIX TAPI_DPDK_TESTPMD_NAME "_command_"
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
    tapi_job_channel_t *link_speed_filter;
} tapi_dpdk_testpmd_job_t;


/**
 * Create a job for test-pmd binary execution.
 * The created job can be manipulated with other tapi_dpdk functions as
 * well as passed to generic tapi_job functions.
 *
 * @note @p prop is advisory
 *
 * @param rpcs              RPC server to run test-pmd on
 * @param env               Test environment
 * @param n_fwd_cpus        Number of test-pmd forwarding CPUs
 * @param prop              Advisory properties of CPUs
 * @param test_args         Test arguments
 * @param[out] testpmd_job  Handle of test-pmd job
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_create_testpmd_job(rcf_rpc_server *rpcs,
                                         tapi_env *env,
                                         size_t n_fwd_cpus,
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
 * Get link speed from running test-pmd job.
 *
 * @param testpmd_job       Handle of test-pmd job
 * @param[out] link_speed   Link speed in Mbps
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_testpmd_get_link_speed(
                                    tapi_dpdk_testpmd_job_t *testpmd_job,
                                    unsigned int *link_speed);
/**
 * Get performance statistics from running test-pmd job.
 *
 * @note The @p testpmd_job must be started.
 *
 * @param testpmd_job   Handle of running test-pmd job
 * @param tx            Evaluated Tx statistics
 * @param rx            Evaluated Rx statistics
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_testpmd_get_stats(
                                    tapi_dpdk_testpmd_job_t *testpmd_job,
                                    te_meas_stats_t *tx,
                                    te_meas_stats_t *rx);

/**
 * Destroy test-pmd job
 *
 * @param testpmd_job       Handle of test-pmd job
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_testpmd_destroy(tapi_dpdk_testpmd_job_t *testpmd_job);

/**
 * Calculate required MTU by given packet size and decide whether the MTU
 * should be specified explicitly in the parameters.
 *
 * @param packet_size   Size of a packet
 * @param mtu[out]      Required mtu (set when the function returns @c TRUE)
 *
 * @return          @c TRUE - MTU should be set, @c FALSE - should not be
 */
extern te_bool tapi_dpdk_mtu_by_pkt_size(unsigned int packet_size,
                                         unsigned int *mtu);

/**
 * Calculate required mbuf size by given packet size and decide whether the
 * mbuf size should be specified explicitly in the parameters.
 *
 * @param packet_size       Size of a packet
 * @param mbuf_size[out]    Required mbuf size (set when the function
 *                          returns @c TRUE)
 *
 * @return          @c TRUE - mbuf size should be set, @c FALSE - should not be
 */
extern te_bool tapi_dpdk_mbuf_size_by_pkt_size(unsigned int packet_size,
                                               unsigned int *mbuf_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_DPDK_H__ */

/**@} <!-- END tapi_dpdk --> */
