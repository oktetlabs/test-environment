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

te_errno
tapi_dpdk_stats_calculate_l1_link_usage(uint64_t l1_bitrate,
                                        unsigned int link_speed,
                                        double *l1_link_usage)
{
    if (link_speed == 0)
    {
        ERROR("Link usage cannot be calculated when link speed is zero");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    *l1_link_usage = (double)l1_bitrate /
                     (double)((uint64_t)link_speed * 1000000ULL);

    return 0;
}

void
tapi_dpdk_stats_l1_link_usage_artifact(double l1_link_usage)
{
    TEST_ARTIFACT("L1 rate percent: %.3f", l1_link_usage * 100.0);
}

void
tapi_dpdk_stats_log_rates(uint64_t pps, unsigned int packet_size,
                          unsigned int link_speed)
{
    uint64_t l1_bitrate;

    l1_bitrate = tapi_dpdk_stats_calculate_l1_bitrate(pps, packet_size);

    tapi_dpdk_stats_pps_artifact(pps);
    tapi_dpdk_stats_l1_bitrate_artifact(l1_bitrate);

    if (link_speed == 0)
    {
        WARN_VERDICT("Link speed is zero: link usage report is skipped");
    }
    else
    {
        double l1_link_usage;

        tapi_dpdk_stats_calculate_l1_link_usage(l1_bitrate, link_speed,
                                                &l1_link_usage);
        tapi_dpdk_stats_l1_link_usage_artifact(l1_link_usage);
    }
}
