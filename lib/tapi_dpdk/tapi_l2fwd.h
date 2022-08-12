/** @file
 * @brief TAPI functions to support l2fwd
 *
 * @defgroup tapi_l2fwd TAPI functions to support l2fwd
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle DPDK l2fwd operations
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 * @author Georgiy Levashov <Georgiy.Levashov@oktetlabs.ru>
 */

#ifndef __TAPI_L2FWD_H__
#define __TAPI_L2FWD_H__

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

/** Timeout for job termination */
#define TAPI_DPDK_L2FWD_TERM_TIMEOUT_MS 60000
/** Timeout to wait for job receiving */
#define TAPI_DPDK_L2FWD_RECEIVE_TIMEOUT_MS 60000

/** L2fwd job description */
typedef struct tapi_dpdk_l2fwd_job_t {
    char *ta;
    unsigned int port_number;
    tapi_job_t *job;
    tapi_job_channel_t *in_channel;
    tapi_job_channel_t *out_channels[2];
    tapi_job_channel_t *err_filter;
    tapi_job_channel_t *packets_sent;
    tapi_job_channel_t *packets_received;
} tapi_dpdk_l2fwd_job_t;

/*
 * Start l2fwd job
 *
 * @param l2fwd_job     l2fwd job
 *
 * @return  Status code
 */
extern te_errno tapi_dpdk_l2fwd_start(tapi_dpdk_l2fwd_job_t *l2fwd_job);

/*
 * Destroy l2fwd job
 *
 * @param l2fwd_job     l2fwd job
 *
 * @return  Status code
 */
extern te_errno tapi_dpdk_l2fwd_destroy(tapi_dpdk_l2fwd_job_t *l2fwd_job);

/*
 * Retrieve stats from running l2fwd job
 *
 * @param l2fwd_job     l2fwd job
 * @param tx            Evaluated Tx statistics
 * @param rx            Evaluated Rx statistics
 *
 * @note l2fwd job must be started
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_l2fwd_get_stats(tapi_dpdk_l2fwd_job_t *l2fwd_job,
                                          te_meas_stats_t *tx,
                                          te_meas_stats_t *rx);

/**
 * Create a job for l2fwd binary execution.
 * The created job can be manipulated with other tapi_dpdk functions as
 * well as passed to generic tapi_job functions.
 *
 * @note @p prop is advisory
 *
 * @param rpcs              RPC server to run l2fwd on
 * @param env               Test environment
 * @param n_fwd_cpus        Number of l2fwd forwarding CPUs
 * @param prop              Advisory properties of CPUs
 * @param test_args         Test arguments
 * @param[out] l2fwd_job    Pointer to the l2fwd job
 *
 * @return  Status code
 */
extern te_errno tapi_dpdk_create_l2fwd_job(rcf_rpc_server *rpcs, tapi_env *env,
                                           unsigned int n_fwd_cpus,
                                           const tapi_cpu_prop_t *prop,
                                           te_kvpair_h *test_args,
                                           tapi_dpdk_l2fwd_job_t *l2fwd_job);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_L2FWD_H__ */

/**@} <!-- END tapi_l2fwd --> */
