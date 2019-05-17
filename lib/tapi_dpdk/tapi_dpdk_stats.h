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
 * Report packet per second statistics as a test artifact.
 *
 * @param l1_bitrate    Layer 1 bits per second
 */
void tapi_dpdk_stats_l1_bitrate_artifact(uint64_t l1_bitrate);

/**
 * Report rates corresponding to PPS and packet_size as test artifacts.
 *
 * @param pps          Packets per second
 * @param packet_size  Packet size in bytes (without l1 and FCS)
 */
void tapi_dpdk_stats_log_rates(uint64_t pps, unsigned int packet_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TAPI_DPDK_STATS_H__ */

/**@} <!-- END tapi_dpdk_stats --> */
