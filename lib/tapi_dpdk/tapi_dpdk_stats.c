/** @file
 * @brief DPDK statistics helper functions TAPI
 *
 * TAPI to handle DPDK-related operations with statistics (implementation)
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#include "te_config.h"

#include "tapi_dpdk_stats.h"
#include "tapi_test_log.h"

void
tapi_dpdk_stats_pps_artifact(uint64_t pps)
{
    TEST_ARTIFACT("PPS: %lu", pps);
}

uint64_t
tapi_dpdk_stats_calculate_l1_bitrate(uint64_t pps, unsigned int packet_size)
{
    /*
     * Assume the overhead size: 20 (Preamble + SOF + IPG) + 4 (FCS) = 24 bytes
     * per packet. IPG could be different, but the information is not present.
     */
    unsigned int overhead_size = 24;

    return (packet_size + overhead_size) * 8U * pps;
}

void
tapi_dpdk_stats_l1_bitrate_artifact(uint64_t l1_bitrate)
{
    TEST_ARTIFACT("L1 bit rate: %lu bit/s", l1_bitrate);
}


void
tapi_dpdk_stats_log_rates(uint64_t pps, unsigned int packet_size)
{
    uint64_t l1_bitrate;

    l1_bitrate = tapi_dpdk_stats_calculate_l1_bitrate(pps, packet_size);

    tapi_dpdk_stats_pps_artifact(pps);
    tapi_dpdk_stats_l1_bitrate_artifact(l1_bitrate);
}
