/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to control rxq_stats BPF program.
 *
 * @defgroup tapi_bpf_rxq_stats Test API to control rxq_stats BPF program
 * @ingroup tapi_bpf
 * @{
 *
 * Definition of API to control rxq_stats BPF program.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_BPF_RXQ_STATS_H__
#define __TE_TAPI_BPF_RXQ_STATS_H__

#include "te_defs.h"
#include "te_errno.h"
#include "tapi_bpf.h"
#include "te_rpc_sys_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Per-queue received packets count */
typedef struct tapi_bpf_rxq_stats {
    unsigned int rx_queue; /**< Rx queue ID */
    uint64_t pkts; /**< Number of received packets */
} tapi_bpf_rxq_stats;

/**
 * Initialize "rxq_stats" BPF object, link it to an interface.
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param prog_dir  Where rxq_stats object file is located (may be
 *                  a path relative to TA directory)
 * @param bpf_id    Where to save BPF object ID (may be @c NULL)
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_init(const char *ta, const char *if_name,
                                        const char *prog_dir,
                                        unsigned int *bpf_id);

/**
 * Unlink "rxq_stats" program from interface, destroy BPF object.
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param bpf_id    BPF object ID
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_fini(const char *ta, const char *if_name,
                                        unsigned int bpf_id);

/**
 * Get ID of BPF object of "rxq_stats" program linked to a given interface.
 *
 * @param ta        Test Agent
 * @param if_name   Interface name
 * @param bpf_id    Where to save BPF object ID
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_get_id(const char *ta,
                                          const char *if_name,
                                          unsigned int *bpf_id);

/**
 * Set parameters for "rxq_stats" program.
 *
 * @param ta            Test Agent name
 * @param bpf_id        BPF object ID
 * @param addr_family   @c AF_INET or @c AF_INET6
 * @param src_addr      Source address/port
 * @param dst_addr      Destination address/port
 * @param protocol      IP protocol (@c IPPROTO_UDP, @c IPPROTO_TCP)
 * @param enable        Whether counting of packets should be
 *                      enabled now
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_set_params(
                                      const char *ta, unsigned int bpf_id,
                                      int addr_family,
                                      const struct sockaddr *src_addr,
                                      const struct sockaddr *dst_addr,
                                      int protocol, te_bool enable);

/**
 * Enable or disable packets counting in "rxq_stats" program.
 *
 * @param ta            Test Agent name
 * @param bpf_id        BPF object ID
 * @param enable        Whether counting of packets should be
 *                      enabled now
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_enable(const char *ta,
                                          unsigned int bpf_id,
                                          te_bool enable);

/**
 * Clear statistics collected by "rxq_stats" program.
 *
 * @param ta            Test Agent name
 * @param bpf_id        BPF object ID
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_clear(const char *ta,
                                         unsigned int bpf_id);

/*
 * Reset state of "rxq_stats" program (disable it, clear statistics,
 * parameters).
 *
 * @param ta            Test Agent name
 * @param bpf_id        BPF object ID
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_reset(const char *ta,
                                         unsigned int bpf_id);

/**
 * Get statistics collected by "rxq_stats" program.
 *
 * @param ta            Test Agent name
 * @param bpf_id        BPF object ID
 * @param stats_out     Where to save statistics (number of packets
 *                      received by every Rx queue)
 * @param count_out     Where to save number of collected statistics
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_read(const char *ta, unsigned int bpf_id,
                                        tapi_bpf_rxq_stats **stats_out,
                                        unsigned int *count_out);

/**
 * Log statistics retrieved with tapi_bpf_rxq_stats_read().
 *
 * @param title       Title to print on the first line (if @c NULL
 *                    or empty, default title will be used)
 * @param stats       Array of statistics
 * @param count       Number of elements in the array
 */
extern void tapi_bpf_rxq_stats_print(const char *title,
                                     tapi_bpf_rxq_stats *stats,
                                     unsigned int count);

/**
 * Check whether expected Rx queue received all the expected packets.
 * It is assumed that statistics were cleared before sending packets.
 *
 * @note This function will print verdict in case of failure.
 *
 * @param ta          Test Agent name
 * @param bpf_id      BPF object ID
 * @param exp_queue   Expected Rx queue ID
 * @param exp_pkts    Expected number of received packets
 * @param sock_type   Socket type (@c RPC_SOCK_STREAM, @c RPC_SOCK_DGRAM)
 * @param vpref       Prefix to use in verdicts
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_rxq_stats_check_single(const char *ta,
                                                unsigned int bpf_id,
                                                unsigned int exp_queue,
                                                unsigned int exp_pkts,
                                                rpc_socket_type sock_type,
                                                const char *vpref);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_BPF_RXQ_STATS_H__ */
