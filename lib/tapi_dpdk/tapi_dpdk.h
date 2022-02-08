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
#include "te_mi_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Name of l2fwd tool */
#define TAPI_DPDK_L2FWD_NAME "l2fwd"
/** Name of testpmd tool */
#define TAPI_DPDK_TESTPMD_NAME "testpmd"
#define TAPI_DPDK_TESTPMD_ARG_PREFIX TAPI_DPDK_TESTPMD_NAME "_arg_"
#define TAPI_DPDK_TESTPMD_COMMAND_PREFIX TAPI_DPDK_TESTPMD_NAME "_command_"
#define TAPI_DPDK_TESTPMD_TERM_TIMEOUT_MS 60000
#define TAPI_DPDK_TESTPMD_RECEIVE_TIMEOUT_MS 60000
#define TAPI_DPDK_TESTPMD_TSO_MSS_HDRS_LEN \
            (ETHER_HDR_LEN + TAD_IP4_HDR_LEN + TAD_TCP_HDR_LEN)

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
    tapi_job_channel_t *tx_dbells_filter;
    tapi_job_channel_t *tx_dbells_skip_filter;
    tapi_job_channel_t *rx_dbells_filter;
    tapi_job_channel_t *rx_dbells_skip_filter;
    tapi_job_channel_t *rx_pkts_filter;
    tapi_job_channel_t *rx_bytes_filter;
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
                                         unsigned int n_fwd_cpus,
                                         const tapi_cpu_prop_t *prop,
                                         te_kvpair_h *test_args,
                                         tapi_dpdk_testpmd_job_t *testpmd_job);

/**
 * Check if option specified in @p opt is supported by test-pmd.
 *
 * @param rpcs                  RPC server to run test-pmd on
 * @param env                   Test environment
 * @param opt                   Option to check support for
 * @param[out] opt_supported    Result
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_testpmd_is_opt_supported(rcf_rpc_server *rpcs,
                                                   tapi_env *env,
                                                   te_kvpair_h *opt,
                                                   te_bool *opt_supported);

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
 * @param[out] mtu      Location for required MTU
 *
 * @return If required MTU is greater than @c ETHER_DATA_LEN
 * @retval TRUE         Required MTU is greater than @c ETHER_DATA_LEN
 * @retval FALSE        Required MTU is less or equal to @c ETHER_DATA_LEN
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

/**
 * Append argument to the arguments storage
 * Jumps out via TEST_FAIL in case of failure
 *
 * @param argument      Argument to add
 * @param argc_p        Pointer to the number of arguments
 * @param argv_out      Pointer to the arguments
 */
extern void tapi_dpdk_append_argument(const char *argument,
                                      int *argc_p, char ***argv_out);

/**
 * Build EAL arguments for TAPI jobs
 * and store it to the arguments storage
 *
 * @param rpcs              RPC server to run job on
 * @param env               Test environment
 * @param n_cpus            Number of job forwarding CPUs
 * @param cpu_ids[in]       Indices of job forwarding CPUs
 * @param program_name      Full path to the binary job
 * @param argc_p            Pointer to the number of arguments
 * @param argv_out          Pointer to the arguments
 *
 * @return          Status code
 */
extern te_errno tapi_dpdk_build_eal_arguments(rcf_rpc_server *rpcs,
                                              tapi_env *env,
                                              unsigned int n_cpus,
                                              tapi_cpu_index_t *cpu_ids,
                                              const char *program_name,
                                              int *argc_c, char ***argv_out);

/*
 * Grab CPUs with required properties
 *
 * @param ta                    Test Agent
 * @param n_cpus_preferred      Prefered number of CPUs to grab
 * @param n_cpus_required       Required number of CPUs to grab
 * @param numa_node             NUMA node at which cores should be grabbed or
 *                              @c -1 for any node
 * @param prop                  Required properties for CPUs
 * @param n_cpus_grabbed        Number of grabbed CPUs
 * @param cpu_ids               Indices of grabbed CPUs
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_grab_cpus(const char *ta,
                                    unsigned int n_cpus_preferred,
                                    unsigned int n_cpus_required,
                                    int numa_node,
                                    const tapi_cpu_prop_t *prop,
                                    unsigned int *n_cpus_grabbed,
                                    tapi_cpu_index_t *cpu_ids);

/*
 * Try to grab CPUs with required properties.
 * If fails, grab CPUs without required properties.
 *
 * @param ta                    Test Agent
 * @param n_cpus_preferred      Prefered number of CPUs to grab
 * @param n_cpus_required       Required number of CPUs to grab
 * @param numa_node             NUMA node at which cores should be grabbed or
 *                              @c -1 for any node
 * @param prop                  Required properties for CPUs
 * @param n_cpus_grabbed        Number of grabbed CPUs
 * @param cpu_ids               Indices of grabbed CPUs
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_grab_cpus_nonstrict_prop(const char *ta,
                                                   unsigned int n_cpus_preferred,
                                                   unsigned int n_cpus_required,
                                                   int numa_node,
                                                   const tapi_cpu_prop_t *prop,
                                                   unsigned int *n_cpus_grabbed,
                                                   tapi_cpu_index_t *cpu_ids);
/*
 * Get vdev argument value from EAL arguments if exists.
 * Otherwise @c NULL.
 *
 * @param eal_argc      Number of EAL arguments
 * @param eal_argv      EAL arguments
 *
 * @return vdev argument value or @c NULL
 */
extern const char *tapi_dpdk_get_vdev_eal_argument(int eal_argc,
                                                   char **eal_argv);

/*
 * Get vdev port number
 *
 * @param vdev          Vdev name
 * @param port_number   Pointer to store port number
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_get_vdev_port_number(const char *vdev,
                                               unsigned int *port_number);

/**
 * Attach filters to catch Rx doorbells rate and if they are supported.
 *
 * @param testpmd_job           Handle of running test-pmd job
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_attach_dbells_filter_rx(
                                        tapi_dpdk_testpmd_job_t *testpmd_job);

/**
 * Attach filters to catch Tx doorbells rate and if they are supported.
 *
 * @param testpmd_job           Handle of running test-pmd job
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_attach_dbells_filter_tx(
                                        tapi_dpdk_testpmd_job_t *testpmd_job);

/**
 * Add display Rx doorbells xstats to dpdk-testpmd parameters.
 *
 * @param[in] test_params       kvpair storing dpdk-testpmd parameters
 * @param[in] q_num             Number of Rx queues.
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_add_rx_dbells_display(te_kvpair_h *test_params,
                                                const char *q_num);

/**
 * Add display Tx doorbells xstats to dpdk-testpmd parameters.
 *
 * @param[in] test_params       kvpair storing dpdk-testpmd parameters
 * @param[in] q_num             Number of Tx queues.
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_add_tx_dbells_display(te_kvpair_h *test_params,
                                                const char *q_num);


/**
 * Report Rx doorbells rate and packets per doorbell as test artifacts.
 *
 * @param[in] testpmd_job       Handle of running test-pmd job
 * @param[in] meas_stats_pps    Statistics of packets per second
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_stats_log_rx_dbells(
                                    const tapi_dpdk_testpmd_job_t *testpmd_job,
                                    const te_meas_stats_t *meas_stats_pps);

/**
 * Report Tx doorbells rate and packets per doorbell as test artifacts.
 *
 * @param[in] testpmd_job       Handle of running test-pmd job
 * @param[in] meas_stats_pps    Statistics of packets per second
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_stats_log_tx_dbells(
                                    const tapi_dpdk_testpmd_job_t *testpmd_job,
                                    const te_meas_stats_t *meas_stats_pps);

/**
 * Attach Rx packets/bytes filters.
 *
 * @param[in] testpmd_job       Handle of test-pmd job
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_attach_rx_pkts_bytes_filters(
                                          tapi_dpdk_testpmd_job_t *testpmd_job);

/**
 * Get latest values of Rx packets and bytes from filters attached by
 * tapi_dpdk_attach_rx_pkts_bytes_filters().
 *
 * @param[in]  testpmd_job          Rx job channel handle
 * @param[out] pkts                 Latest Rx packets
 * @param[out] bytes                Latest Rx bytes
 *
 * @return Status code
 */
extern te_errno tapi_dpdk_get_last_rx_pkts_bytes(
                                           tapi_dpdk_testpmd_job_t *testpmd_job,
                                           unsigned long *pkts,
                                           unsigned long *bytes);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_DPDK_H__ */

/**@} <!-- END tapi_dpdk --> */
