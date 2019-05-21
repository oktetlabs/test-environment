/** @file
 * @brief DPDK statistics helper functions TAPI
 *
 * @defgroup tapi_dpdk_stats DPDK statistics helper functions TAPI
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle DPDK-related operations with statistics
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __TAPI_DPDK_STATS_H__
#define __TAPI_DPDK_STATS_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Report packet per second statistics as a test artifact.
 *
 * @param pps  Packets per second
 */
void tapi_dpdk_stats_pps_artifact(uint64_t pps);

/**
 * Calculate layer 1 bits per second from PPS and packet size.
 *
 * @param pps          Packets per second
 * @param packet_size  Packet size in bytes (without l1 and FCS)
 *
 * @return  Layer 1 bits per second
 */
uint64_t tapi_dpdk_stats_calculate_l1_bitrate(uint64_t pps,
                                              unsigned int packet_size);

/**
 * Calculate layer 1 link usage from layer 1 bitrate and link speed.
 *
 * @param      l1_bitrate     Layer 1 bits per second
 * @param      link_speed     Link speed in Mbps
 * @param[out] l1_link_usage  Layer 1 link usage ratio
 *
 * @return  Status code
 */
te_errno tapi_dpdk_stats_calculate_l1_link_usage(uint64_t l1_bitrate,
                                                 unsigned int link_speed,
                                                 double *l1_link_usage);

/**
 * Report packet per second statistics as a test artifact.
 *
 * @param l1_bitrate    Layer 1 bits per second
 */
void tapi_dpdk_stats_l1_bitrate_artifact(uint64_t l1_bitrate);

/**
 * Report layer 1 link usage statistics as a test artifact.
 *
 * @param l1_link_usage    Layer 1 link usage ratio
 */
void tapi_dpdk_stats_l1_link_usage_artifact(double l1_link_usage);

/**
 * Report rates corresponding to PPS, packet_size and link speed
 * as test artifacts.
 *
 * @param pps          Packets per second
 * @param packet_size  Packet size in bytes (without l1 and FCS)
 * @param link_speed   Link speed in Mbps
 */
void tapi_dpdk_stats_log_rates(uint64_t pps, unsigned int packet_size,
                               unsigned int link_speed);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_DPDK_STATS_H__ */

/**@} <!-- END tapi_dpdk_stats --> */
